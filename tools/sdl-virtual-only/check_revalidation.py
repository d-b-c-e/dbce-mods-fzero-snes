"""Changed-route checks only; no consumer execution."""
import json
from pathlib import Path
import subprocess
import tempfile
from prepare import prepare, revalidate

ROOT = Path(__file__).resolve().parents[2]
SDL = ROOT / '.local/sdl-reviewed/build'
CONSUMER = ROOT / '.local/sdl-admission-consumer'
CMAKE = Path('C:/Program Files/CMake/bin/cmake.exe')

with tempfile.TemporaryDirectory(prefix='fzero-revalidation-') as scratch:
    parent = Path(scratch)
    copied = parent / 'consumer'
    copied.mkdir()
    for name in ('fzero_virtual_consumer.exe', 'consumer-link-provenance.txt'):
        (copied / name).write_bytes((CONSUMER / name).read_bytes())
    root = prepare(SDL, copied, parent, CMAKE)
    def validate():
        return revalidate(root, parent, SDL, copied, CMAKE)
    def refused(label):
        try:
            validate()
        except (ValueError, OSError, subprocess.CalledProcessError):
            print('PASS refused:', label)
        else:
            raise AssertionError('Drift admitted: ' + label)
    assert validate() is None
    print('PASS unchanged preparation; no execution capability returned')
    manifest = root / 'preparation.json'
    original = manifest.read_bytes()
    changes = {
        'environment injection': lambda s: s['environment'].update({'sDl_DYNAMIC_API': 'owner.dll'}),
        'environment path escape': lambda s: s['environment'].update({'TEMP': str(parent)}),
        'working directory escape': lambda s: s.update(workingDirectory=str(parent)),
        'runtime flag': lambda s: s.update(runtimeAllowed=True),
        'inherit environment': lambda s: s.update(inheritEnvironment=True),
        'arguments': lambda s: s.update(arguments=['--run']),
        'payload path': lambda s: s.update(payload='../consumer.exe'),
        'identity claim': lambda s: s.update(consumerSHA256='0' * 64),
        'extra manifest key': lambda s: s.update(command='run'),
    }
    for label, change in changes.items():
        spec = json.loads(original)
        change(spec)
        manifest.write_text(json.dumps(spec))
        refused(label)
        manifest.write_bytes(original)
    for path in (root / 'consumer.exe.disabled', copied / 'fzero_virtual_consumer.exe',
                 copied / 'consumer-link-provenance.txt'):
        data = path.read_bytes()
        path.write_bytes(data + b'DRIFT')
        refused(str(path.relative_to(parent)))
        path.write_bytes(data)
    extra = root / 'cwd' / 'config.ini'
    extra.write_text('unapproved')
    refused('nonempty cwd')
    extra.unlink()
    extra = root / 'consumer.exe'
    extra.write_bytes(b'inert')
    refused('extra executable member')
    extra.unlink()
    # Windows junctions do not require symlink privileges.
    target = parent / 'redirect-target'
    target.mkdir()
    (root / 'temp').rmdir()
    result = subprocess.run(['cmd', '/c', 'mklink', '/J', str(root / 'temp'), str(target)],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    try:
        refused('junction redirect')
    finally:
        (root / 'temp').rmdir()
        (root / 'temp').mkdir()
    try:
        revalidate(root, parent / 'wrong-parent', SDL, copied, CMAKE)
    except (ValueError, OSError):
        print('PASS refused: wrong scratch parent')
    else:
        raise AssertionError('Wrong parent admitted')
    assert validate() is None
    assert not list(root.rglob('*.exe'))
    print('PASS restored preparation; payload remains disabled')
print('PASS disposable cleanup; no runtime, input, window or owner-state access')
