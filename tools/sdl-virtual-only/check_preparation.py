"""Focused checks of the real preparation route; never executes its payload."""
import json
import os
from pathlib import Path
import tempfile
from prepare import prepare

ROOT = Path(__file__).resolve().parents[2]
SDL = ROOT / '.local/sdl-reviewed/build'
CONSUMER = ROOT / '.local/sdl-admission-consumer'
CMAKE = Path('C:/Program Files/CMake/bin/cmake.exe')

with tempfile.TemporaryDirectory(prefix='fzero-preparation-check-') as scratch:
    scratch = Path(scratch)
    copied = scratch / 'consumer'
    copied.mkdir()
    for name in ('fzero_virtual_consumer.exe', 'consumer-link-provenance.txt'):
        (copied / name).write_bytes((CONSUMER / name).read_bytes())
    # Mixed-case hostile hints and unrelated credentials must never be copied.
    poison = {'sDl_GAMECONTROLLERCONFIG_FILE': '@owner-file',
              'fZeRo_INJECT': 'owner-file', 'sTeAmVirtualGamepadInfo': 'owner-file',
              'SDL3_DYNAMIC_API': 'owner.dll', 'UNRELATED_SECRET': 'must-not-copy'}
    saved = {key: os.environ.get(key) for key in poison}
    try:
        os.environ.update(poison)
        result = prepare(SDL, copied, scratch, CMAKE)
        spec = json.loads((result / 'preparation.json').read_text())
        assert spec['runtimeAllowed'] is False and spec['inheritEnvironment'] is False
        assert set(spec['environment']) == {
            'LOCALAPPDATA', 'APPDATA', 'TEMP', 'TMP', 'USERPROFILE', 'DBCE_FFB_MUTE'}
        assert not any(key.upper().startswith(('SDL', 'FZERO', 'STEAM'))
                       for key in spec['environment'])
        assert spec['environment']['DBCE_FFB_MUTE'] == '1'
        for key in ('LOCALAPPDATA', 'APPDATA', 'TEMP', 'TMP', 'USERPROFILE'):
            assert Path(spec['environment'][key]).is_relative_to(result)
        assert list((result / 'cwd').iterdir()) == []
        assert (result / spec['payload']).read_bytes() == (copied / 'fzero_virtual_consumer.exe').read_bytes()
        assert not list(result.rglob('*.exe'))
        print('PASS preparation: allowlist, disposable paths, exact inert payload, runtime disabled')
        for name in ('fzero_virtual_consumer.exe', 'consumer-link-provenance.txt'):
            path = copied / name
            original = path.read_bytes()
            path.write_bytes(original + b'\nDRIFT')
            before = set(scratch.iterdir())
            try:
                prepare(SDL, copied, scratch, CMAKE)
            except (ValueError, UnicodeError):
                assert set(scratch.iterdir()) == before
                print('PASS refused before staging:', name)
            else:
                raise AssertionError('Mismatch admitted: ' + name)
            finally:
                path.write_bytes(original)
    finally:
        for key, value in saved.items():
            if value is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = value
print('PASS scratch cleanup; no consumer execution')
