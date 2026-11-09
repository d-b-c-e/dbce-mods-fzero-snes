"""Compare retained imports with pinned app-local exports without loading code."""
import argparse
from collections import Counter
import json
from pathlib import Path
import struct

from import_inventory import inventory
from prepare import HERE, digest


def exports(data):
    def read(fmt, pos):
        return struct.unpack_from('<' + fmt, data, pos)

    pe, = read('I', 0x3c)
    if data[:2] != b'MZ' or data[pe:pe + 4] != b'PE\0\0':
        raise ValueError('Not PE')
    machine, count = read('HH', pe + 4)
    size, = read('H', pe + 20)
    opt = pe + 24
    if machine != 0x8664 or read('H', opt)[0] != 0x20b:
        raise ValueError('Expected AMD64 PE32+')
    sections = [read('IIII', opt + size + i * 40 + 8) for i in range(count)]

    def offset(rva, length=1):
        for _, address, raw_size, raw in sections:
            delta = rva - address
            if 0 <= delta and delta + length <= raw_size and raw + delta + length <= len(data):
                return raw + delta
        raise ValueError('RVA outside file-backed section')

    def string(rva):
        pos = offset(rva)
        end = data.index(b'\0', pos)
        offset(rva, end - pos + 1)
        return data[pos:end].decode('ascii')

    rva, length = read('II', opt + 112)
    if not rva or length < 40:
        raise ValueError('Missing export directory')
    fields = read('IIHHIIIIIII', offset(rva, 40))
    base, functions, names, eat, npt, ordinals = fields[5:]
    entries = {}
    for i in range(functions):
        target, = read('I', offset(eat + i * 4, 4))
        if target:
            forwarder = string(target) if rva <= target < rva + length else None
            if forwarder and target + len(forwarder) + 1 > rva + length:
                raise ValueError('Forwarder outside export directory')
            if not forwarder and not any(address <= target < address + max(virtual_size, raw_size)
                                         for virtual_size, address, raw_size, _ in sections):
                raise ValueError('Export target outside image sections')
            entries[base + i] = {'ordinal': base + i, 'rva': target,
                                 'forwarder': forwarder, 'names': []}
    by_name = {}
    for i in range(names):
        name = string(read('I', offset(npt + i * 4, 4))[0])
        index, = read('H', offset(ordinals + i * 2, 2))
        if index >= functions or base + index not in entries or name in by_name:
            raise ValueError('Invalid export name/ordinal')
        entry = entries[base + index]
        entry['names'].append(name)
        by_name[name] = entry
    return entries, by_name


def resolve(module, name, ordinal, tables, pins, seen=()):
    key = (module.lower(), name, ordinal)
    if key in seen:
        return 'unresolved-evidence', [], 'Forwarder cycle'
    if key[0] not in tables:
        return 'unresolved-evidence', [], 'Module outside pinned candidate dataset'
    by_ordinal, by_name = tables[key[0]]
    entry = by_name.get(name) if name is not None else by_ordinal.get(ordinal)
    if entry is None:
        return 'missing-export', [], 'Absent from complete pinned export table'
    step = {'module': key[0], 'sha256': pins[key[0]], 'requestedName': name,
            'requestedOrdinal': ordinal, **entry}
    if not entry['forwarder']:
        return 'resolved', [step], None
    target_module, symbol = entry['forwarder'].rsplit('.', 1)
    if not target_module.lower().endswith('.dll'):
        target_module += '.dll'
    target_ordinal = int(symbol[1:]) if symbol.startswith('#') else None
    status, chain, reason = resolve(target_module, None if target_ordinal is not None else symbol,
                                    target_ordinal, tables, pins, seen + (key,))
    return status, [step] + chain, reason


def compare(consumer, runtime_dir):
    source = HERE.parents[1] / 'docs/2026-10-26-consumer-import-symbols.json'
    source_bytes = source.read_bytes()
    retained = json.loads(source_bytes)
    if inventory(consumer, runtime_dir) != retained:
        raise ValueError('Retained import inventory drift')
    tables, pins, summaries = {}, {}, []
    for artifact in retained['artifacts'][1:]:
        name = artifact['name']
        data = (Path(runtime_dir) / name).read_bytes()
        if digest(data) != artifact['sha256']:
            raise ValueError('Runtime identity mismatch: ' + name)
        table = exports(data)
        tables[name.lower()] = table
        pins[name.lower()] = digest(data)
        summaries.append({'name': name, 'sha256': digest(data),
                          'exportSlots': len(table[0]), 'namedExports': len(table[1]),
                          'forwarders': [e for e in table[0].values() if e['forwarder']]})
    results, external = [], []
    for artifact in retained['artifacts']:
        for module in artifact['imports']:
            for symbol in module['symbols']:
                row = {'artifact': artifact['name'], 'artifactSHA256': artifact['sha256'],
                       'module': module['module'], **symbol}
                if module['module'].lower() not in pins:
                    external.append({**row, 'status': 'unresolved-evidence',
                                     'reason': 'External Windows baseline not selected'})
                    continue
                status, chain, reason = resolve(module['module'], symbol['name'], symbol['ordinal'], tables, pins)
                results.append({**row, 'status': status, 'chain': chain, 'reason': reason})
    return {'schema': 1, 'runtimeAllowed': False, 'windowsBaseline': None,
            'scope': 'Pinned app-local static imports only; no dynamic closure or loader admission',
            'readerSHA256': digest(Path(__file__).read_bytes()),
            'importInventorySHA256': digest(source_bytes),
            'runtimeManifestSHA256': digest((HERE / 'runtime-candidates.json').read_bytes()),
            'exports': summaries, 'counts': dict(Counter(r['status'] for r in results)),
            'results': results, 'externalRequirements': external}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--consumer', required=True)
    parser.add_argument('--runtime-dir', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    Path(args.output).write_text(json.dumps(compare(args.consumer, args.runtime_dir), indent=2) + '\n', encoding='utf-8')
