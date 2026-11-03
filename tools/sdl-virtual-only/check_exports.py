"""Focused read-only export comparison checks; mutations use disposable copies."""
import copy
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

from compare_exports import compare, exports, resolve
from prepare import HERE, digest


def main(consumer, runtime):
    consumer, runtime = Path(consumer), Path(runtime)
    report = compare(consumer, runtime)
    assert report == compare(consumer, runtime), 'Nondeterministic comparison'
    assert report['counts'] == {'resolved': 108}
    assert len(report['externalRequirements']) == 491
    prior = json.loads((HERE.parents[1] / 'docs/2026-10-26-consumer-loader-imports.json').read_text())
    objdump = runtime / 'objdump.exe'
    assert digest(objdump.read_bytes()) == prior['toolSHA256']
    for entry in report['exports']:
        data = (runtime / entry['name']).read_bytes()
        by_ordinal, by_name = exports(data)
        output = subprocess.check_output([str(objdump), '-p', str(runtime / entry['name'])], text=True)
        name_section = output.split('[Ordinal/Name Pointer] Table -- Ordinal Base ', 1)[1]
        names = {name: int(ordinal) for ordinal, name in re.findall(
            r'^\s*\[\s*\d+\]\s+\+base\[\s*(\d+)\]\s+[0-9a-fA-F]+\s+(\S+)\s*$', name_section, re.M)}
        addresses = {int(ordinal): int(rva, 16) for ordinal, rva in re.findall(
            r'^\s*\[\s*\d+\]\s+\+base\[\s*(\d+)\]\s+([0-9a-fA-F]+) Export RVA\s*$', output, re.M)}
        assert names == {name: e['ordinal'] for name, e in by_name.items()}
        assert addresses == {ordinal: e['rva'] for ordinal, e in by_ordinal.items()}
        assert not entry['forwarders']
    print('PASS: deterministic output; all 7344 export names/ordinals/RVAs match pinned objdump')
    with tempfile.TemporaryDirectory(prefix='fzero-export-check-') as scratch:
        scratch = Path(scratch)
        staged_consumer = scratch / consumer.name
        shutil.copyfile(consumer, staged_consumer)
        for entry in report['exports']:
            shutil.copyfile(runtime / entry['name'], scratch / entry['name'])
        for path in [staged_consumer] + [scratch / e['name'] for e in report['exports']]:
            original = path.read_bytes()
            path.write_bytes(original[:-1] + bytes([original[-1] ^ 1]))
            try:
                compare(staged_consumer, scratch)
            except ValueError as error:
                assert 'identity mismatch' in str(error).lower()
            else:
                raise AssertionError('Drift accepted: ' + path.name)
            path.write_bytes(original)
        assert compare(staged_consumer, scratch) == report
    print('PASS: four artifact byte-drift refusals; restored copies; disposable cleanup')
    terminal = {'ordinal': 7, 'rva': 4096, 'forwarder': None, 'names': ['End']}
    forwarded = {'ordinal': 1, 'rva': 100, 'forwarder': 'b.#7', 'names': ['Start']}
    tables = {'a.dll': ({1: forwarded}, {'Start': forwarded}),
              'b.dll': ({7: terminal}, {'End': terminal})}
    pins = {'a.dll': 'a-pin', 'b.dll': 'b-pin'}
    status, chain, _ = resolve('A.DLL', 'Start', None, tables, pins)
    assert status == 'resolved' and len(chain) == 2 and chain[-1]['ordinal'] == 7
    assert resolve('a.dll', 'start', None, tables, pins)[0] == 'missing-export'
    assert resolve('b.dll', None, 8, tables, pins)[0] == 'missing-export'
    assert resolve('absent.dll', 'End', None, tables, pins)[0] == 'unresolved-evidence'
    changed = copy.deepcopy(tables)
    changed['a.dll'][0][1]['forwarder'] = 'external.End'
    assert resolve('a.dll', 'Start', None, changed, pins)[0] == 'unresolved-evidence'
    changed['a.dll'][0][1]['forwarder'] = 'b.End'
    assert resolve('a.dll', 'Start', None, changed, pins)[0] == 'resolved'
    changed['b.dll'][0][7]['forwarder'] = 'a.Start'
    assert resolve('a.dll', 'Start', None, changed, pins)[0] == 'unresolved-evidence'
    print('PASS: synthetic named/ordinal forwarders, exact-case missing export, absent ordinal, external evidence and cycle')


if __name__ == '__main__':
    main(*sys.argv[1:])
