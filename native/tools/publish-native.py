"""Promote a verified six-target main build to a stable GitHub release."""
import argparse
import hashlib
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
TARGETS = {(system, arch) for system in ('windows', 'macos', 'linux') for arch in ('x64', 'arm64')}
FORMATS = {'windows': {'-Setup.exe', '-Portable.zip'}, 'macos': {'.dmg', '.zip'},
           'linux': {'.AppImage', '.deb', '.tar.gz'}}


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def candidates(directory, version, source):
    assert re.fullmatch(r'\d+\.\d+\.\d+', version), 'A stable numeric version is required'
    assert re.fullmatch(r'[a-f0-9]{40}', source), 'Invalid source SHA'
    found, files = set(), {}
    for path in directory.rglob('ZENzip-*-manifest.json'):
        data = json.loads(path.read_text(encoding='utf-8'))
        target = data['os'], data['architecture']
        assert target in TARGETS and target not in found, 'Unexpected or duplicate target'
        found.add(target)
        assert data['version'] == version, 'Version mismatch'
        assert data['sourceCommit'] == source and data['sourceDirty'] is False, 'Unverified or dirty source'
        prefix = f'ZENzip-{version}-{target[0]}-{target[1]}'
        assert path.name == prefix + '-manifest.json', 'Manifest filename mismatch'
        expected = {prefix + suffix for suffix in FORMATS[target[0]]}
        assert {item['file'] for item in data['artifacts']} == expected, 'Missing/unexpected package format'
        assert len(data['artifacts']) == len(expected), 'Duplicate package entry'
        for item in data['artifacts']:
            name = item['file']
            assert pathlib.PurePosixPath(name).name == name and '\\' not in name, 'Unsafe artifact path'
            package = path.parent / name
            assert package.is_file() and not package.is_symlink(), 'Package missing or linked'
            assert package.stat().st_size == item['size'] and sha(package) == item['sha256'], 'Package hash/size mismatch'
            assert name not in files, 'Duplicate asset filename'
            files[name] = package
        files[path.name] = path
    assert found == TARGETS, 'All six platform/architecture targets are required'
    return files


def gh(*arguments, json_result=False):
    result = subprocess.check_output(['gh', *map(str, arguments)], text=True, encoding='utf-8')
    return json.loads(result) if json_result else result


def find_release(repo, tag):
    # The tag endpoint can return 404 for a newly created draft. The authenticated
    # release list includes drafts and is also the resumable lookup path.
    releases = gh('api', f'repos/{repo}/releases?per_page=100', json_result=True)
    return next((release for release in releases if release['tag_name'] == tag), None)


def create_draft(repo, tag, version, notes):
    # Use the creation response; a new draft can briefly be absent from list APIs.
    release = gh('api', '--method', 'POST', f'repos/{repo}/releases',
                 '-f', 'tag_name=' + tag, '-f', 'name=ZENzip ' + version,
                 '-F', 'draft=true', '-F', 'prerelease=false',
                 '-f', 'body=' + notes.read_text(encoding='utf-8'), json_result=True)
    assert release['draft'] is True and release['tag_name'] == tag, 'Unexpected draft creation response'
    return release


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate-dir', required=True, type=pathlib.Path)
    parser.add_argument('--run-id', required=True, type=int)
    parser.add_argument('--output', required=True, type=pathlib.Path)
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args()
    repo = os.environ.get('GITHUB_REPOSITORY', 'whh110112/ZENzip')
    assert repo == 'whh110112/ZENzip', 'Unexpected publishing repository'
    source = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    version = (ROOT / 'native/VERSION.txt').read_text().strip()
    run = gh('api', f'repos/{repo}/actions/runs/{args.run_id}', json_result=True)
    assert run['status'] == 'completed' and run['conclusion'] == 'success', 'Candidate run has not passed'
    assert run['head_branch'] == 'main' and run['head_sha'] == source, 'Candidate must be this main source'
    assert run['head_repository']['full_name'] == repo, 'Candidate repository mismatch'
    assert run['path'] == '.github/workflows/native-candidate.yml', 'Unexpected candidate workflow'
    assert gh('api', f'repos/{repo}/git/ref/heads/main', json_result=True)['object']['sha'] == source, 'Main advanced; revalidate before publishing'
    subprocess.run([sys.executable, str(ROOT / 'native/tools/check-branding.py')], check=True)
    verified = candidates(args.candidate_dir.resolve(), version, source)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    for name, path in verified.items():
        shutil.copy2(path, output / name)
    checksums = output / 'SHA256SUMS.txt'
    checksums.write_text(''.join(f'{sha(output / name)}  {name}\n' for name in sorted(verified)), encoding='utf-8')
    verified[checksums.name] = checksums
    expected = {name: dict(size=(output / name).stat().st_size, digest='sha256:' + sha(output / name)) for name in verified}
    notes = output / 'RELEASE-NOTES.md'
    notes.write_text(f'''# ZENzip {version}

正式版本 / Stable release

- 同名文件冲突窗口支持「全部覆盖」，或勾选「本次任务后续同名文件不再询问」后选择跳过、重命名、覆盖；新任务会重新询问。
- 「设置 → 关于」新增手动检查更新，显示最新正式版并提供下载页面入口；不自动安装。
- Conflict dialogs offer Overwrite all and task-scoped remembered skip/rename/overwrite decisions. New tasks ask again.
- Settings → About can manually check the latest stable release and open its download page. Updates are not installed automatically.

- 采用 A「留白圆」标志；窗口内的配色、底色和底纹自动读取当前皮肤，包括符合新约定的自定义皮肤，无需单独设置 logo。
- 打开压缩包后自动按「压缩包体积 ÷ 原始文件总体积」绘制圆环：保留比例 60% 对应开口 40%。没有完整数据时使用默认开口；系统和文件关联图标保持固定比例。
- 支持六套皮肤、18 种界面语言；保留已有设置、密码库和安装升级兼容。固实压缩与分卷比例纳入验证，Linux AppImage 运行时固定到官方版本并校验摘要。

- The A open-circle identity automatically follows the active theme's colors and background artwork, including custom themes.
- Opening an archive sets the filled annulus to its retained-size ratio: 60% retained means a 40% opening. Missing metadata uses the default mark; OS and file-association icons remain fixed.
- Six palettes and 18 interface languages, with solid/split-archive coverage and version-pinned Linux AppImage runtimes.

Windows, macOS and Linux packages are available for x64 and ARM64. All six targets passed 16/16 CTest, 648 settings layouts, 108 conflict layouts and 108 archive-tab layouts per target, plus package validation. Download the matching package and verify it with `SHA256SUMS.txt`.

[Candidate run](https://github.com/{repo}/actions/runs/{args.run_id}) · [Source commit](https://github.com/{repo}/commit/{source}) · [Verification scope](https://github.com/{repo}/blob/v{version}/docs/verification-{version}.md)

Windows packages are unsigned. macOS packages are ad-hoc signed and not notarized. macOS/Linux password vaults use file permissions and are not encrypted. Modern Win11 menus and drag-out storage/compatibility improvements remain follow-up work.
''', encoding='utf-8')
    evidence = dict(version=version, sourceCommit=source, candidateRun=args.run_id, assets=expected)
    if args.dry_run:
        (output / 'promotion-verification.json').write_text(json.dumps(dict(evidence, published=False), indent=2) + '\n')
        print('Dry-run validation PASS; no tag or release was created')
        return
    tag = 'v' + version
    # A partial draft can be resumed, but an already published release is immutable here.
    tags = gh('api', f'repos/{repo}/git/matching-refs/tags/{tag}', json_result=True)
    exact = [item for item in tags if item['ref'] == 'refs/tags/' + tag]
    if exact:
        assert exact[0]['object']['type'] == 'commit' and exact[0]['object']['sha'] == source, 'Existing tag does not identify this tested source'
    else:
        gh('api', '--method', 'POST', f'repos/{repo}/git/refs', '-f', 'ref=refs/tags/' + tag, '-f', 'sha=' + source)
    release = find_release(repo, tag)
    if release:
        assert release['draft'] is True, 'Refusing to replace a published release'
    else:
        release = create_draft(repo, tag, version, notes)
    release_id = release['id']
    for name in sorted(expected):
        current = gh('api', f'repos/{repo}/releases/{release_id}', json_result=True)
        assert current['draft'] is True, 'Release published before upload verification'
        asset = next((item for item in current['assets'] if item['name'] == name), None)
        if asset and asset.get('digest') == expected[name]['digest'] and asset['size'] == expected[name]['size']:
            continue
        gh('release', 'upload', tag, output / name, '--repo', repo, '--clobber')
    release = gh('api', f'repos/{repo}/releases/{release_id}', json_result=True)
    assert release['draft'] is True and release['prerelease'] is False
    remote = {item['name']: item for item in release['assets']}
    assert set(remote) == set(expected), 'Remote asset set mismatch'
    for name, item in expected.items():
        assert remote[name]['state'] == 'uploaded' and remote[name]['size'] == item['size'] and remote[name].get('digest') == item['digest'], 'Uploaded asset verification failed: ' + name
    gh('release', 'edit', tag, '--repo', repo, '--draft=false', '--prerelease=false', '--latest', '--notes-file', notes)
    published = gh('api', f'repos/{repo}/releases/{release_id}', json_result=True)
    assert published['draft'] is False and published['prerelease'] is False
    assert gh('api', f'repos/{repo}/releases/latest', json_result=True)['tag_name'] == tag
    (output / 'promotion-verification.json').write_text(json.dumps(dict(evidence, published=True, url=published['html_url']), indent=2) + '\n')
    print(published['html_url'])


if __name__ == '__main__':
    main()
