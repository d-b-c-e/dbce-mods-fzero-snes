"""Prepare data for source review only. There is deliberately no launch operation."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent


def digest(data):
    return hashlib.sha256(data).hexdigest().upper()


def admitted_image(sdl_dir, consumer_dir, cmake):
    sdl_dir, consumer_dir = Path(sdl_dir).resolve(), Path(consumer_dir).resolve()
    pins = json.loads((HERE / 'approved-preparation.json').read_text())
    approved = json.loads((HERE / 'approved-admission.json').read_text())
    if pins['runtimeAllowed'] is not False:
        raise ValueError('Preparation must remain runtimeAllowed=false')
    # Reuse the production compile-only dependency gate, without compiling or
    # executing the consumer. No parent environment is passed to a future child.
    subprocess.run([str(cmake), f'-DSDL3_DIR={sdl_dir}',
                    f'-DRESOLVED_ARCHIVE={sdl_dir / "libSDL3.a"}',
                    '-P', str(HERE / 'Admit.cmake')], check=True)
    provenance = (consumer_dir / 'consumer-link-provenance.txt').read_bytes()
    fields = dict(line.split('=', 1) for line in provenance.decode().splitlines())
    if (fields.get('runtimeAllowed') != 'false'
            or Path(fields['archive']).resolve() != sdl_dir / 'libSDL3.a'
            or fields.get('sha256') != approved['archiveSHA256']
            or fields.get('configSHA256') != approved['configSHA256']
            or digest(provenance) != pins['linkProvenanceSHA256']):
        raise ValueError('Consumer link provenance mismatch')
    image = (consumer_dir / 'fzero_virtual_consumer.exe').read_bytes()
    if digest(image) != pins['consumerSHA256']:
        raise ValueError('Consumer identity mismatch')
    return image, pins, approved


def specification(root, pins, approved):
    environment = {
        'LOCALAPPDATA': str(root / 'localappdata'),
        'APPDATA': str(root / 'appdata'),
        'TEMP': str(root / 'temp'), 'TMP': str(root / 'temp'),
        'USERPROFILE': str(root / 'profile'), 'DBCE_FFB_MUTE': '1',
    }
    return {
        'schema': 1, 'runtimeAllowed': False, 'operation': 'prepare-only',
        'consumerSHA256': pins['consumerSHA256'],
        'archiveSHA256': approved['archiveSHA256'],
        'configSHA256': approved['configSHA256'],
        'linkProvenanceSHA256': pins['linkProvenanceSHA256'],
        'workingDirectory': str(root / 'cwd'),
        'environment': environment, 'inheritEnvironment': False,
        'arguments': [], 'payload': 'consumer.exe.disabled',
    }


def prepare(sdl_dir, consumer_dir, output_parent, cmake):
    image, pins, approved = admitted_image(sdl_dir, consumer_dir, cmake)
    root = Path(tempfile.mkdtemp(prefix='fzero-prepared-', dir=output_parent)).resolve()
    for name in ('cwd', 'localappdata', 'appdata', 'temp', 'profile'):
        (root / name).mkdir()
    (root / 'consumer.exe.disabled').write_bytes(image)
    (root / 'preparation.json').write_text(
        json.dumps(specification(root, pins, approved), indent=2) + '\n')
    return root


def reject_redirects(path):
    # Check all existing components, including junctions on Windows. Resolve
    # alone would silently accept a redirected staging tree.
    for component in (path, *path.parents):
        info = component.lstat()
        if component.is_symlink() or getattr(info, 'st_file_attributes', 0) & 0x400:
            raise ValueError('Redirected preparation path: ' + str(component))


def revalidate(root, output_parent, sdl_dir, consumer_dir, cmake):
    """Read-only source gate. A pass is NOT runtime admission or a launch token.

    A future separately admitted launcher must repeat validation at use and
    address concurrent mutation; this function cannot close that interval.
    """
    root = Path(root).absolute()
    parent = Path(output_parent).absolute()
    reject_redirects(root)
    reject_redirects(parent)
    if root.parent != parent or not root.name.startswith('fzero-prepared-'):
        raise ValueError('Preparation outside selected scratch parent')
    image, pins, approved = admitted_image(sdl_dir, consumer_dir, cmake)
    directories = {'cwd', 'localappdata', 'appdata', 'temp', 'profile'}
    files = {'consumer.exe.disabled', 'preparation.json'}
    if {item.name for item in root.iterdir()} != directories | files:
        raise ValueError('Preparation membership drift')
    for name in directories | files:
        path = root / name
        reject_redirects(path)
        if name in directories:
            if not path.is_dir() or any(path.iterdir()):
                raise ValueError('Preparation directory drift: ' + name)
        elif not path.is_file() or path.stat().st_nlink != 1:
            raise ValueError('Preparation file identity drift: ' + name)
    if (root / 'consumer.exe.disabled').read_bytes() != image:
        raise ValueError('Prepared payload identity mismatch')
    actual = json.loads((root / 'preparation.json').read_text())
    expected = specification(root, pins, approved)
    # Canonical JSON comparison also refuses bool/integer substitutions.
    if json.dumps(actual, sort_keys=True) != json.dumps(expected, sort_keys=True):
        raise ValueError('Preparation specification drift')
    return None


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdl-dir', required=True, type=Path)
    parser.add_argument('--consumer-dir', required=True, type=Path)
    parser.add_argument('--output-parent', required=True, type=Path)
    parser.add_argument('--cmake', required=True, type=Path)
    parser.add_argument('--revalidate', type=Path, metavar='PREPARED_ROOT',
                        help='Read-only revalidation; never launches the payload')
    args = parser.parse_args()
    if args.revalidate is not None:
        revalidate(args.revalidate, args.output_parent, args.sdl_dir, args.consumer_dir, args.cmake)
        print('Revalidated preparation only; runtimeAllowed=false')
    else:
        print(prepare(args.sdl_dir, args.consumer_dir, args.output_parent, args.cmake))
