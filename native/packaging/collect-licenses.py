"""Collect exact-version Qt notices at build time, alongside SDK SPDX inventories."""
import argparse,concurrent.futures,json,pathlib,shutil,urllib.request
p=argparse.ArgumentParser();p.add_argument('--qt-prefix',required=True);p.add_argument('--output',required=True);a=p.parse_args()
output=pathlib.Path(a.output);output.mkdir(parents=True,exist_ok=True);jobs=[]
for module in ['qtbase','qtdeclarative','qtwayland']:
    url=f'https://api.github.com/repos/qt/{module}/git/trees/v6.8.3?recursive=1'
    with urllib.request.urlopen(urllib.request.Request(url,headers={'User-Agent':'REAzip-build'}),timeout=90) as response:tree=json.load(response)['tree']
    for item in tree:
        path=item['path'];name=pathlib.PurePosixPath(path).name.lower()
        if item['type']!='blob':continue
        if path.startswith('LICENSES/') or (path.startswith('src/3rdparty/') and any(word in name for word in ['license','copying','copyright','notice','ftl.txt'])):
            if item.get('size',0)<1024*1024:jobs.append((f'https://raw.githubusercontent.com/qt/{module}/v6.8.3/{path}',output/module/path))
def fetch(job):
    url,path=job;path.parent.mkdir(parents=True,exist_ok=True)
    if path.exists():return
    with urllib.request.urlopen(url,timeout=60) as response:path.write_bytes(response.read())
with concurrent.futures.ThreadPoolExecutor(max_workers=6) as executor:list(executor.map(fetch,jobs))
sbom=pathlib.Path(a.qt_prefix)/'sbom'
if sbom.exists():shutil.copytree(sbom,output/'sdk-spdx',dirs_exist_ok=True)
print(f'Collected {len(jobs)} exact-version Qt license/notice files and SDK SPDX metadata')
