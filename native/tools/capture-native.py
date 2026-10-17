"""Capture only REAzip's own Qt window through its explicit development smoke hook."""
import argparse,hashlib,json,os,pathlib,subprocess
p=argparse.ArgumentParser();p.add_argument('application');p.add_argument('output');p.add_argument('--scale',default='1');a=p.parse_args()
root=pathlib.Path(__file__).resolve().parents[1];out=pathlib.Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
env=dict(os.environ,QT_SCALE_FACTOR=a.scale);records=[]
scenes=[(theme,'en','archive') for theme in ['light','sleep','mac','mac-dark','glass','material']]
scenes += [('light',language,scene) for language,scene in [('zh-CN','empty'),('en','create'),('en','extract'),('en','password'),('en','conflict'),('en','appearance'),('de','settings'),('ar','archive')]]
for theme,language,scene in scenes:
    name=f'{theme}-{language}-{scene}';path=out/(name+'.png')
    command=[str(pathlib.Path(a.application).resolve()),'--smoke','--theme',theme,'--language',language,'--screenshot',str(path)]
    if scene=='archive':command.append(str(root/'tests/fixtures/sample.iso'))
    elif scene!='empty':command+=['--design-scene',scene]
    result=subprocess.run(command,env=env,capture_output=True,timeout=30)
    (out/(name+'.stderr.txt')).write_bytes(result.stderr)
    if result.returncode or result.stderr:raise SystemExit(f'{name}: smoke failed: {result.stderr.decode(errors="replace")}')
    records.append(dict(theme=theme,language=language,scene=scene,file=path.name,sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
(out/'manifest.json').write_text(json.dumps(dict(scaleEnvironment=a.scale,captures=records),indent=2)+'\n')
print(f'{len(records)} native window captures, no QML diagnostics')
