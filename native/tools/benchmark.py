import argparse,json,pathlib,subprocess,statistics,os
p=argparse.ArgumentParser();p.add_argument('--native',required=True);p.add_argument('--output',required=True);p.add_argument('--samples',type=int,default=3);a=p.parse_args()
root=pathlib.Path(__file__).resolve().parents[2];out=pathlib.Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
records=[];env=dict(os.environ);env.pop('QT_QPA_PLATFORM',None)
for scene in ['empty','archive']:
 for kind in ['legacy','native']:
  for i in range(a.samples):
   output=out/f'{kind}-{scene}-{i}.json'
   if kind=='legacy':command=[str(root/'node_modules/electron/dist/electron.exe'),str(root/'native/tools/benchmark-legacy.cjs'),'--output',str(output)]
   else:command=[str(pathlib.Path(a.native).resolve()),'--smoke','--performance',str(output)]
   if scene=='archive':
    if kind=='legacy':command+=['--archive']
    command+=[str(root/'native/tests/fixtures/sample.iso')]
   subprocess.run(command,cwd=root,env=env,check=True,timeout=30,capture_output=True)
   records.append(dict(json.loads(output.read_text()),implementation=kind,sample=i))
summary={}
for scene in ['empty','archive']:
 for kind in ['legacy','native']:
  subset=[r for r in records if r['scene']==scene and r['implementation']==kind]
  summary[kind+'-'+scene]={key:statistics.median(float(r[key]) for r in subset) for key in ['firstFrameMs','archiveReadyMs','residentBytes']}
(out/'summary.json').write_text(json.dumps(dict(samples=records,medians=summary,limitations=['Same Windows machine, normal window platform, warm filesystem caches.','Internal application-entry to first-frame timing excludes different runtime loading before the entry point.','Electron memory sums all app processes; native idle worker has exited.']),indent=2)+'\n')
print(json.dumps(summary,indent=2))
