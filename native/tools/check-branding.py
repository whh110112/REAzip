"""Reject obsolete presentation text while preserving reviewed data identifiers."""
import argparse
import json
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[2]
LEGACY = b'reazip'
INVENTORY = ROOT / 'native/branding-compatibility.json'


def check_sources():
    allowed = json.loads(INVENTORY.read_text(encoding='utf-8'))['identifiers']
    names = subprocess.check_output(
        ['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard'], cwd=ROOT
    ).decode('utf-8').split('\0')
    errors, found = [], {}
    for name in sorted(set(names) - {''}):
        path = ROOT / name
        if not path.is_file():
            continue
        if LEGACY.decode() in name.casefold():
            errors.append('obsolete filename: ' + name)
        data = path.read_bytes()
        if b'\0' in data[:8192]:
            continue
        count = data.lower().count(LEGACY)
        if count:
            found[name] = count
            entry = allowed.get(name)
            if entry is None or count != entry['occurrences']:
                errors.append(f'unreviewed identifier in {name}: {count} occurrence(s)')
    for name in allowed:
        if name not in found:
            errors.append('compatibility inventory needs review: ' + name)
    if errors:
        raise SystemExit('\n'.join(errors))
    print(f'Branding PASS: no obsolete presentation text; {len(found)} reviewed compatibility/audit files')


def check_stage(stage):
    errors = []
    metadata = {'README.txt', 'NOTICE.md', 'Info.plist', 'acceptance.json', 'artifact-manifest.json'}
    for path in stage.rglob('*'):
        relative = path.relative_to(stage).as_posix()
        if LEGACY.decode() in relative.casefold():
            errors.append('obsolete packaged filename: ' + relative)
        if path.is_file() and (path.name in metadata or path.suffix == '.desktop'):
            if LEGACY in path.read_bytes().lower():
                errors.append('obsolete packaged presentation: ' + relative)
    if errors:
        raise SystemExit('\n'.join(errors))
    print('Packaged branding PASS: filenames and user-facing metadata use ZENzip')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage', type=pathlib.Path)
    arguments = parser.parse_args()
    check_sources()
    if arguments.stage:
        check_stage(arguments.stage.resolve())
