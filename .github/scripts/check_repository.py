"""Fast, engine-free checks. Run from any directory with Python 3.11+."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)


def strict_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f'duplicate JSON key: {key}')
        result[key] = value
    return result


def load_json(path):
    return json.loads(path.read_text(encoding='utf-8-sig'), object_pairs_hook=strict_object)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--require-lfs', action='store_true',
                        help='Fail if an Unreal asset is still a Git LFS pointer')
    args = parser.parse_args()
    paths = [Path(p.decode('utf-8')) for p in git('ls-files', '-z').split(b'\0') if p]
    tracked = {p.as_posix() for p in paths}
    binary_paths = [p for p in paths if p.suffix.lower() in ('.uasset', '.umap', '.otf', '.ttf')]
    attributes = subprocess.check_output(
        ['git', 'check-attr', '-z', '--stdin', 'filter'], cwd=ROOT,
        input=b''.join(p.as_posix().encode('utf-8') + b'\0' for p in binary_paths),
    ).split(b'\0')
    filters = {attributes[i].decode('utf-8'): attributes[i + 2]
               for i in range(0, len(attributes) - 1, 3)}
    errors = []
    scripts = documents = assets = 0
    for relative in paths:
        path = ROOT / relative
        try:
            if path.suffix == '.py':
                # Compile in memory; Unreal Python scripts cannot be imported outside UE.
                compile(path.read_bytes(), str(relative), 'exec')
                scripts += 1
            elif path.suffix in ('.json', '.uproject'):
                load_json(path)
                documents += 1
            if path.suffix.lower() in ('.uasset', '.umap', '.otf', '.ttf'):
                if filters.get(relative.as_posix()) != b'lfs':
                    raise ValueError('binary asset must be tracked with Git LFS')
                if args.require_lfs:
                    with path.open('rb') as stream:
                        if stream.read(42).startswith(b'version https://git-lfs.github.com/spec/v1'):
                            raise ValueError('Git LFS asset has not been downloaded')
                assets += 1
        except (OSError, ValueError, SyntaxError, subprocess.CalledProcessError) as exc:
            errors.append(f'{relative.as_posix()}: {exc}')

    try:
        project = load_json(ROOT / 'ShiftStore.uproject')
        assert any(m['Name'] == 'ShiftStore' for m in project['Modules']), 'Missing ShiftStore module'
        catalog = load_json(ROOT / 'SourceArt/asset_catalog.json')
        states = load_json(ROOT / 'SourceArt/scene_states.json')
        products = load_json(ROOT / 'SourceArt/products.json')
        references = [(name, entry['asset'], '.uasset') for name, entry in catalog.items()]
        references += [(key, states[key], '.umap') for key in ('normal_map', 'shift_map')]
        for name, asset, suffix in references:
            assert asset.startswith('/Game/'), f'{name}: invalid project asset path {asset}'
            file = 'Content/' + asset.removeprefix('/Game/') + suffix
            assert file in tracked, f'{name}: referenced asset is not tracked: {file}'
        ids = [p['id'] for p in products]
        assert len(ids) == len(set(ids)), 'Duplicate product IDs'
        for product in products:
            assert product['cost'] > 0, f"Invalid cost: {product['id']}"
            label = f"SourceArt/Labels/{product['id']}.png"
            assert label in tracked, f'Missing product label: {label}'
    except (OSError, ValueError, KeyError, TypeError, AssertionError) as exc:
        errors.append(f'Project metadata: {exc}')

    for error in errors:
        print(f'ERROR: {error}', file=sys.stderr)
    print(f'Checked {scripts} Python scripts, {documents} JSON documents, {assets} LFS assets.')
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
