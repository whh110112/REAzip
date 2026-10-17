"""Collect exact-version Qt notices at build time, alongside SDK SPDX inventories."""
import argparse,concurrent.futures,json,pathlib,shutil,urllib.request,subprocess,time,os
p=argparse.ArgumentParser();p.add_argument('--qt-prefix',required=True);p.add_argument('--output',required=True);a=p.parse_args()
output=pathlib.Path(a.output);output.mkdir(parents=True,exist_ok=True);jobs=[]
cache=pathlib.Path(__file__).resolve().parents[2]/'.native-build/qt-license-cache';cache.mkdir(parents=True,exist_ok=True)
gh=shutil.which('gh') or str(pathlib.Path(__file__).resolve().parents[2]/'tools/gh/bin/gh.exe')
for module in ['qtbase','qtdeclarative','qtwayland']:
    endpoint=f'repos/qt/{module}/git/trees/v6.8.3?recursive=1'
    metadata=cache/(module+'-tree.json')
    if not metadata.exists():metadata.write_bytes(subprocess.check_output([gh,'api',endpoint]))
    response=json.loads(metadata.read_bytes())
    if response.get('truncated'):raise RuntimeError('Incomplete Qt license inventory')
    tree=response['tree']
    for item in tree:
        path=item['path'];name=pathlib.PurePosixPath(path).name.lower()
        if item['type']!='blob':continue
        if path.startswith('LICENSES/') or (path.startswith('src/3rdparty/') and any(word in name for word in ['license','copying','copyright','notice','ftl.txt'])):
            if item.get('size',0)<1024*1024:jobs.append((f'https://raw.githubusercontent.com/qt/{module}/v6.8.3/{path}',output/module/path))
def fetch(job):
    url,path=job;path.parent.mkdir(parents=True,exist_ok=True)
    if path.exists():return
    cached=cache/path.relative_to(output)
    if not cached.exists():
        cached.parent.mkdir(parents=True,exist_ok=True)
        for attempt in range(3):
            try:
                with urllib.request.urlopen(url,timeout=60) as response:cached.write_bytes(response.read())
                break
            except Exception:
                if attempt==2:raise
                time.sleep(2)
    shutil.copy2(cached,path)
with concurrent.futures.ThreadPoolExecutor(max_workers=6) as executor:list(executor.map(fetch,jobs))
sbom=pathlib.Path(a.qt_prefix)/'sbom'
if sbom.exists():shutil.copytree(sbom,output/'sdk-spdx',dirs_exist_ok=True)
print(f'Collected {len(jobs)} exact-version Qt license/notice files and SDK SPDX metadata')
