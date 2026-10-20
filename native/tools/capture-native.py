"""Capture real ZENzip windows with paired languages and traceable build evidence."""
import argparse, hashlib, json, os, pathlib, subprocess
p = argparse.ArgumentParser()
p.add_argument('application'); p.add_argument('output')
p.add_argument('--font-file'); p.add_argument('--scale', default='1'); p.add_argument('--full', action='store_true')
a = p.parse_args()
root = pathlib.Path(__file__).resolve().parents[1]
out = pathlib.Path(a.output).resolve(); out.mkdir(parents=True, exist_ok=True)
scenes = [(theme, language, scene, a.scale)
          for theme in (['light', 'sleep', 'mac', 'mac-dark', 'glass', 'material'] if a.full else ['light'])
          for language in ['en', 'zh-CN']
          for scene in (['archive', 'settings', 'appearance', 'create', 'extract', 'conflict', 'vault', 'associations', 'language', 'about'] if a.full else ['archive'])]
if a.full:
    scenes += [('light', language, scene, scale) for language in ['en', 'zh-CN', 'de', 'ar']
               for scene in ['settings', 'vault', 'appearance', 'associations', 'language', 'about']
               for scale in ['1', '1.25', '1.5', '2']]
scenes = list(dict.fromkeys(scenes))
records = []
for theme, language, scene, scale in scenes:
    name = f'{theme}-{language}-{scene}' + (f'-scale-{scale}' if scale != a.scale else '')
    path = out / (name + '.png')
    command = [str(pathlib.Path(a.application).resolve()), '--smoke', '--theme', theme,
               '--language', language, '--screenshot', str(path)]
    if a.font_file: command += ['--font-file', str(pathlib.Path(a.font_file).resolve())]
    if scene == 'archive': command.append(str(root / 'tests/fixtures/sample.iso'))
    elif scene != 'empty': command += ['--design-scene', scene]
    env = dict(os.environ, QT_SCALE_FACTOR=scale)
    result = subprocess.run(command, env=env, capture_output=True, timeout=30)
    (out / (name + '.stderr.txt')).write_bytes(result.stderr)
    if result.returncode or result.stderr:
        raise SystemExit(f'{name}: capture failed: {result.stderr.decode(errors="replace")}')
    records.append(dict(theme=theme, language=language, scene=scene, scale=scale,
                        file=path.name, sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
commit = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip()
(out / 'manifest.json').write_text(json.dumps(dict(version=(root/'VERSION.txt').read_text().strip(),
    sourceCommit=commit, captureFontSha256=hashlib.sha256(pathlib.Path(a.font_file).read_bytes()).hexdigest() if a.font_file else None, captures=records), indent=2) + '\n', encoding='utf-8')
print(f'{len(records)} native window captures, no QML diagnostics')
