"""Pinned build-only Qt SDK setup on each native GitHub runner."""
import argparse,os,pathlib,platform,subprocess,time
p=argparse.ArgumentParser();p.add_argument('--arch',choices=['x64','arm64'],required=True);p.add_argument('--output',required=True);a=p.parse_args()
root=pathlib.Path(a.output).resolve();system=platform.system();host_path=None
if system=='Windows':
    host='windows';arch='win64_msvc2022_64' if a.arch=='x64' else 'win64_msvc2022_arm64_cross_compiled';folder='msvc2022_64' if a.arch=='x64' else 'msvc2022_arm64'
    if a.arch=='arm64':host_path=root/'6.8.3/msvc2022_64'
elif system=='Darwin':host='mac';arch='clang_64';folder='macos'
else:host='linux' if a.arch=='x64' else 'linux_arm64';arch='linux_gcc_64' if a.arch=='x64' else 'linux_gcc_arm64';folder='gcc_64' if a.arch=='x64' else 'gcc_arm64'
prefix=root/'6.8.3'/folder
if not (prefix/'lib/cmake/Qt6/Qt6Config.cmake').exists():
    archives=['qtbase','qtdeclarative','qttools','qttranslations']
    if system=='Linux':archives+=['icu','qtwayland']
    command=['aqt','install-qt',host,'desktop','6.8.3',arch,'--outputdir',str(root),'--archives',*archives,'--autodesktop']
    for attempt in range(3):
        if subprocess.run(command).returncode==0:break
        if attempt==2:raise SystemExit('Qt SDK download failed')
        time.sleep(3)
if not (prefix/'lib/cmake/Qt6/Qt6Config.cmake').exists():raise SystemExit('Qt prefix not found: '+str(prefix))
if os.environ.get('GITHUB_ENV'):
    with open(os.environ['GITHUB_ENV'],'a',encoding='utf-8') as f:
        f.write(f'QT_PREFIX={prefix}\nCMAKE_PREFIX_PATH={prefix}\n')
        if system=='Windows':f.write('QT_QPA_FONTDIR='+os.environ.get('SystemRoot','C:/Windows')+'/Fonts\n')
        elif system=='Darwin':f.write('QT_QPA_FONTDIR=/System/Library/Fonts\n')
        if host_path:f.write(f'QT_HOST_PATH={host_path}\n')
    with open(os.environ['GITHUB_PATH'],'a',encoding='utf-8') as f:f.write(str(prefix/'bin')+'\n')
print('QT_PREFIX='+str(prefix))
