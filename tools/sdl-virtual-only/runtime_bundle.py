"""Pin retained runtime bytes for source review; never load or execute a DLL."""
import argparse
import json
from pathlib import Path
import tempfile

from prepare import HERE, digest, reject_redirects


def manifest():
    spec = json.loads((HERE / 'runtime-candidates.json').read_text())
    if (spec['runtimeAllowed'] is not False
            or spec['dynamicClosureEstablished'] is not False
            or spec['windowsBaseline'] is not None):
        raise ValueError('Runtime candidates must remain unqualified')
    return spec


def candidate_bytes(directory, spec):
    result = {}
    for entry in spec['files']:
        path = Path(directory).absolute() / entry['name']
        reject_redirects(path)
        if not path.is_file() or path.stat().st_nlink != 1:
            raise ValueError('Runtime candidate file identity: ' + entry['name'])
        data = path.read_bytes()
        if digest(data) != entry['sha256']:
            raise ValueError('Runtime candidate hash: ' + entry['name'])
        result[entry['stagedName']] = data
    return result


def stage(directory, parent):
    spec = manifest()
    payloads = candidate_bytes(directory, spec)
    parent = Path(parent).absolute()
    reject_redirects(parent)
    root = Path(tempfile.mkdtemp(prefix='fzero-runtime-review-', dir=parent))
    for name, data in payloads.items():
        (root / name).write_bytes(data)
    (root / 'runtime-candidates.json').write_text(json.dumps(spec, indent=2) + '\n')
    return root


def revalidate(root, parent):
    root, parent = Path(root).absolute(), Path(parent).absolute()
    reject_redirects(root)
    reject_redirects(parent)
    if root.parent != parent or not root.name.startswith('fzero-runtime-review-'):
        raise ValueError('Runtime bundle outside selected scratch parent')
    spec = manifest()
    expected = {entry['stagedName'] for entry in spec['files']} | {'runtime-candidates.json'}
    if {p.name for p in root.iterdir()} != expected:
        raise ValueError('Runtime bundle membership drift')
    for name in expected:
        path = root / name
        reject_redirects(path)
        if not path.is_file() or path.stat().st_nlink != 1:
            raise ValueError('Runtime bundle file identity: ' + name)
    actual = json.loads((root / 'runtime-candidates.json').read_text())
    if json.dumps(actual, sort_keys=True) != json.dumps(spec, sort_keys=True):
        raise ValueError('Runtime manifest drift')
    for entry in spec['files']:
        if digest((root / entry['stagedName']).read_bytes()) != entry['sha256']:
            raise ValueError('Runtime bundle hash: ' + entry['name'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-parent', required=True, type=Path)
    route = parser.add_mutually_exclusive_group(required=True)
    route.add_argument('--runtime-dir', type=Path)
    route.add_argument('--revalidate', type=Path)
    args = parser.parse_args()
    if args.revalidate:
        revalidate(args.revalidate, args.output_parent)
        print('Runtime bundle revalidated; runtimeAllowed=false')
    else:
        print(stage(args.runtime_dir, args.output_parent))
