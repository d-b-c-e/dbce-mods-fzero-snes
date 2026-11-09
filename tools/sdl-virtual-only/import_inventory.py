"""Read pinned x64 PE import tables as bytes. Never loads a target module."""
import argparse
import json
import struct
from pathlib import Path

from prepare import HERE, digest


def imports(data):
    def unpack(fmt, offset):
        return struct.unpack_from('<' + fmt, data, offset)

    def string(offset):
        return data[offset:data.index(b'\0', offset)].decode('ascii')

    if data[:2] != b'MZ':
        raise ValueError('Not PE')
    pe, = unpack('I', 0x3c)
    if data[pe:pe + 4] != b'PE\0\0':
        raise ValueError('Not PE')
    machine, count = unpack('HH', pe + 4)
    size, = unpack('H', pe + 20)
    opt = pe + 24
    if machine != 0x8664 or unpack('H', opt)[0] != 0x20b:
        raise ValueError('Expected AMD64 PE32+')
    if size < 112 + 14 * 8 or unpack('I', opt + 108)[0] < 14:
        raise ValueError('Missing data directories')
    sections = [unpack('IIII', opt + size + i * 40 + 8) for i in range(count)]

    def offset(rva):
        for virtual_size, address, raw_size, raw in sections:
            if address <= rva < address + max(virtual_size, raw_size):
                delta = rva - address
                if delta >= raw_size or raw + delta >= len(data):
                    raise ValueError('RVA outside file-backed section')
                return raw + delta
        raise ValueError('Unmapped RVA')

    delay = unpack('II', opt + 112 + 13 * 8)
    if delay != (0, 0):
        raise ValueError('Delay imports need separate inventory')
    rva, length = unpack('II', opt + 120)
    modules = []
    for i in range(length // 20):
        lookup, stamp, chain, name, iat = unpack('IIIII', offset(rva + i * 20))
        if (lookup, stamp, chain, name, iat) == (0, 0, 0, 0, 0):
            return modules
        if not lookup or stamp:
            raise ValueError('Missing original thunk or bound imports')
        symbols = []
        for j in range(len(data) // 8):
            thunk, = unpack('Q', offset(lookup + j * 8))
            if not thunk:
                break
            if thunk & (1 << 63):
                symbols.append({'name': None, 'ordinal': thunk & 0xffff, 'hint': None})
            else:
                pos = offset(thunk)
                symbols.append({'name': string(pos + 2), 'ordinal': None,
                                'hint': unpack('H', pos)[0]})
        else:
            raise ValueError('Unterminated thunk table')
        modules.append({'module': string(offset(name)), 'symbols': symbols})
    raise ValueError('Unterminated import descriptors')


def inventory(consumer, runtime_dir):
    prior_path = HERE.parents[1] / 'docs/2026-10-26-consumer-loader-imports.json'
    prior_bytes = prior_path.read_bytes()
    prior = json.loads(prior_bytes)
    candidates = json.loads((HERE / 'runtime-candidates.json').read_text())
    preparation = json.loads((HERE / 'approved-preparation.json').read_text())
    if (candidates['runtimeAllowed'] is not False
            or candidates['windowsBaseline'] is not None
            or preparation['runtimeAllowed'] is not False):
        raise ValueError('Qualification state drift')
    pins = {x['name']: x['sha256'] for x in candidates['files']}
    pins['fzero_virtual_consumer.exe'] = preparation['consumerSHA256']
    result = []
    for entry in prior['artifacts']:
        name = entry['name']
        if pins[name] != entry['sha256']:
            raise ValueError('Conflicting identity pins: ' + name)
        path = Path(consumer) if name.endswith('.exe') else Path(runtime_dir) / name
        data = path.read_bytes()
        if digest(data) != pins[name]:
            raise ValueError('Artifact identity mismatch: ' + name)
        modules = imports(data)
        if [x['module'] for x in modules] != entry['imports']:
            raise ValueError('DLL inventory mismatch: ' + name)
        result.append({'name': name, 'sha256': digest(data), 'size': len(data),
                       'machine': 'AMD64', 'delayImports': [], 'imports': modules})
    if len(result) != 4 or len(pins) != 4:
        raise ValueError('Expected four pinned artifacts')
    return {'schema': 1, 'runtimeAllowed': False, 'windowsBaseline': None,
            'method': 'Read-only PE32+ original import-thunk decoding; no target loading',
            'readerSHA256': digest(Path(__file__).read_bytes()),
            'priorInventorySHA256': digest(prior_bytes), 'artifacts': result}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--consumer', required=True)
    parser.add_argument('--runtime-dir', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    Path(args.output).write_text(json.dumps(inventory(args.consumer, args.runtime_dir),
                                          indent=2) + '\n', encoding='utf-8')
