"""Build-only native candidate packager; no publishing capability or credentials."""
import argparse,hashlib,json,os,pathlib,platform,shutil,struct,subprocess,sys,tarfile,time,urllib.request,uuid,zipfile,re
ROOT=pathlib.Path(__file__).resolve().parents[1];REPO=ROOT.parent
LOCK=json.loads((ROOT/'packaging/tools.lock.json').read_text());DEPS=json.loads((ROOT/'deps.lock.json').read_text())
def run(*args,**kw):subprocess.run([str(a) for a in args],check=True,**kw)
def sha(path):
    h=hashlib.sha256()
    with open(path,'rb') as f:
        for part in iter(lambda:f.read(1024*1024),b''):h.update(part)
    return h.hexdigest()
def download(name,directory):
    spec=LOCK[name];path=directory/(name+pathlib.PurePosixPath(spec['url']).suffix)
    if not path.exists() or sha(path)!=spec['sha256']:
        for attempt in range(3):
            try:
                with urllib.request.urlopen(spec['url'],timeout=120) as response:path.write_bytes(response.read())
                if sha(path)!=spec['sha256']:raise RuntimeError('Build tool SHA256 mismatch: '+name)
                break
            except Exception:
                if attempt==2:raise
                time.sleep(2)
    return path
def zip_tree(source,path):
    with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
        for f in sorted(source.rglob('*')):
            if f.is_file():archive.write(f,f.relative_to(source))
def architecture(path):
    with path.open('rb') as f:
        header=f.read(64)
        if header[:2]==b'MZ':
            offset=struct.unpack_from('<I',header,60)[0];f.seek(offset);pe=f.read(6)
            if pe[:4]!=b'PE\0\0':return None
            return {0x8664:'x64',0xaa64:'arm64',0xa64e:'arm64',0xa641:'arm64ec',0x14c:'x86'}.get(struct.unpack_from('<H',pe,4)[0],'unknown')
        if header[:4]==b'\x7fELF':return {62:'x64',183:'arm64'}.get(struct.unpack_from('<H',header,18)[0],'unknown')
        if header[:4] in [b'\xcf\xfa\xed\xfe',b'\xce\xfa\xed\xfe']:
            return {0x1000007:'x64',0x100000c:'arm64'}.get(struct.unpack_from('<I',header,4)[0],'unknown')
        if header[:4] in [b'\xca\xfe\xba\xbe',b'\xca\xfe\xba\xbf']:return 'universal'
    return None
def main():
    p=argparse.ArgumentParser();p.add_argument('--build',required=True);p.add_argument('--arch',choices=['x64','arm64'],required=True);p.add_argument('--output',required=True);p.add_argument('--qt-prefix',required=True);p.add_argument('--skip-installer',action='store_true');a=p.parse_args()
    build=pathlib.Path(a.build).resolve();output=pathlib.Path(a.output).resolve();output.mkdir(parents=True,exist_ok=True);qt=pathlib.Path(a.qt_prefix).resolve()
    system=platform.system();osname={'Windows':'windows','Darwin':'macos','Linux':'linux'}[system];version=(ROOT/'VERSION.txt').read_text().strip();name=f'REAzip-{version}-{osname}-{a.arch}'
    stage=build/('package-'+uuid.uuid4().hex);stage.mkdir();tools=build/'packaging-tools';tools.mkdir(exist_ok=True)
    run('cmake','--install',build,'--config','Release','--component','Runtime','--prefix',stage)
    if system=='Windows' and a.arch=='arm64':
        host=pathlib.Path(os.environ.get('QT_HOST_PATH',str(qt.parent/'msvc2022_64')))
        qtpaths=next((qt/'bin'/name for name in ['qtpaths.bat','qtpaths6.bat','qtpaths6.exe','qtpaths.exe'] if (qt/'bin'/name).exists()),None)
        if qtpaths is None:raise RuntimeError('ARM64 target qtpaths wrapper missing')
        run(host/'bin/windeployqt.exe','--qtpaths',qtpaths,'--release','--no-compiler-runtime','--qmldir',ROOT/'ui','--dir',stage,stage/'reazip.exe')
        (stage/'qt.conf').write_text('[Paths]\nPrefix=.\nPlugins=plugins\nQmlImports=qml\n')
    bundle=stage/'reazip.app' if system=='Darwin' else stage
    platform_plugins=bundle/'Contents/PlugIns/platforms' if system=='Darwin' else stage/'plugins/platforms'
    platform_plugins.mkdir(parents=True,exist_ok=True)
    for plugin in (qt/'plugins/platforms').glob('*qoffscreen.*'):
        if plugin.suffix in ['.dll','.so','.dylib']:shutil.copy2(plugin,platform_plugins/plugin.name)
    resources=bundle/'Contents/Resources' if system=='Darwin' else stage
    resources.mkdir(parents=True,exist_ok=True);licenses=resources/'LICENSES';licenses.mkdir(exist_ok=True)
    shutil.copy2(REPO/'LICENSE',licenses/'REAzip-MIT.txt');shutil.copy2(ROOT/'.deps/bit7z/LICENSE',licenses/'bit7z-MPL-2.0.txt')
    ghc=build/'_deps/ghc_filesystem-src'
    if (ghc/'LICENSE').exists():shutil.copy2(ghc/'LICENSE',licenses/'ghc-filesystem-MIT.txt')
    if system=='Linux':shutil.copy2(ROOT/'packaging/licenses/AppImage-runtime-MIT.txt',licenses/'AppImage-runtime-MIT.txt')
    if system=='Windows' and (stage/'libstdc++-6.dll').exists():
        cache=(build/'CMakeCache.txt').read_text();match=re.search(r'^CMAKE_CXX_COMPILER:FILEPATH=(.+)$',cache,re.M)
        if not match:raise RuntimeError('Cannot locate MinGW runtime license inventory')
        compiler_licenses=pathlib.Path(match.group(1).strip()).parent.parent/'licenses'
        if not compiler_licenses.is_dir():raise RuntimeError('MinGW runtime license inventory missing')
        shutil.copytree(compiler_licenses,licenses/'MinGW-runtime',dirs_exist_ok=True)
    shutil.copytree(ROOT/'.deps/7zip/DOC',licenses/'7-Zip',ignore=shutil.ignore_patterns('*.hhp','*.wxs'),dirs_exist_ok=True)
    run(sys.executable,ROOT/'packaging/collect-licenses.py','--qt-prefix',qt,'--output',licenses/'Qt')
    shutil.copy2(ROOT/'packaging/NOTICE.md',resources/'NOTICE.md')
    sources=resources/'sources';sources.mkdir(exist_ok=True);shutil.copy2(ROOT/'.deps'/DEPS['sevenZip']['artifacts']['source']['name'],sources/'7zip-26.04-original.tar.xz')
    if (ghc/'LICENSE').exists():
        with tarfile.open(sources/'ghc-filesystem-source.tar.gz','w:gz') as archive:archive.add(ghc,arcname='ghc-filesystem',filter=lambda info:None if '/.git' in info.name else info)
    shutil.copy2(ROOT/'bootstrap/posix-codepage.inc',sources/'posix-codepage.inc');shutil.copy2(ROOT/'bootstrap/dependencies.py',sources/'dependencies.py')
    if system!='Windows':shutil.copy2(ROOT/'.deps/7zip/CPP/Common/StringConvert.cpp',sources/'StringConvert.cpp')
    with tarfile.open(sources/'bit7z-source.tar.gz','w:gz') as archive:
        archive.add(ROOT/'.deps/bit7z',arcname='bit7z',filter=lambda info:None if '/.git' in info.name else info)
    with zipfile.ZipFile(sources/'REAzip-native-source.zip','w',zipfile.ZIP_DEFLATED) as archive:
        for file in ROOT.rglob('*'):
            relative=file.relative_to(ROOT)
            if file.is_file() and not any(part in ['.deps','build','__pycache__'] for part in relative.parts):archive.write(file,pathlib.PurePosixPath('native')/relative.as_posix())
        archive.write(REPO/'LICENSE','LICENSE')
    if system=='Windows':
        # Explicit app-local CRT deployment; portable packages cannot depend on developer-machine PATH.
        if not (stage/'libstdc++-6.dll').exists():
            candidates=[]
            for base in [os.environ.get('ProgramFiles','C:/Program Files'),os.environ.get('ProgramFiles(x86)','C:/Program Files (x86)')]:
                candidates+=list(pathlib.Path(base).glob(f'Microsoft Visual Studio/*/*/VC/Redist/MSVC/*/{a.arch}/Microsoft.VC*.CRT'))
            if not candidates:raise RuntimeError('Matching app-local MSVC CRT not found')
            for file in sorted(candidates)[-1].glob('*.dll'):
                if architecture(file)==a.arch:shutil.copy2(file,stage/file.name)
        for file in stage.glob('vc_redist*.exe'):file.unlink()
    elif system=='Darwin':
        # Rebase the additional worker, then reduce Qt's universal SDK binaries to this package architecture.
        worker=bundle/'Contents/MacOS/reazip-worker'
        run(qt/'bin/macdeployqt',bundle,'-executable='+str(worker),'-always-overwrite')
        for file in bundle.rglob('*'):
            if file.is_file() and not file.is_symlink() and architecture(file)=='universal':
                thin=file.with_name(file.name+'.thin');run('lipo',file,'-thin','arm64' if a.arch=='arm64' else 'x86_64','-output',thin);thin.chmod(file.stat().st_mode);os.replace(thin,file)
    # Validate every shipped binary; a wrong-architecture Qt plugin blocks the candidate.
    binaries=[]
    for file in stage.rglob('*'):
        if not file.is_file() or file.is_symlink():continue
        relative=file.relative_to(stage).as_posix();detected=architecture(file)
        if detected and detected!=a.arch:raise RuntimeError(f'Wrong architecture {relative}: {detected}, expected {a.arch}')
        if detected:binaries.append({'path':relative,'architecture':detected})
        if detected and (any(word in file.name.lower() for word in ['webengine','electron','chromium']) or file.name.lower() in ['7z.exe','7zz','node','node.exe']):raise RuntimeError('Forbidden runtime payload: '+relative)
    commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=REPO,text=True).strip()
    manifest={'schemaVersion':1,'version':version,'sourceCommit':commit,'sourceDirty':subprocess.run(['git','diff','--quiet','HEAD','--','native'],cwd=REPO).returncode!=0,'os':osname,'architecture':a.arch,'qt':'6.8.3','bit7z':'4.1.0','sevenZip':'26.04','binaries':binaries,'signing':'ad-hoc, not notarized' if system=='Darwin' else 'unsigned','publicReleaseAuthorized':False}
    (resources/'artifact-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    (resources/'acceptance.json').write_text(json.dumps({'coreTests':'see CI LastTest.log','guiSmoke':'must pass isolated stage check','systemIntegration':'manual desktop acceptance pending','publicRelease':'blocked pending T10'},indent=2)+'\n')
    (resources/'README.txt').write_text('REAzip native development candidate '+version+'\n\nOpen reazip to use the application. No Node.js, Qt SDK, or separately installed 7-Zip is required.\nWindows portable ZIP: portable.flag keeps settings/themes under adjacent data. Installer: standard user data directory.\nRead NOTICE.md and acceptance.json for licenses, signing and acceptance status.\n')
    if system=='Darwin':
        run('codesign','--force','--deep','--sign','-','--timestamp=none',bundle)
        run('codesign','--verify','--deep','--strict',bundle)
    for binary in binaries:binary['sha256']=sha(stage/binary['path'])
    executable=bundle/'Contents/MacOS/reazip' if system=='Darwin' else stage/('reazip.exe' if system=='Windows' else 'bin/reazip')
    env=dict(os.environ)
    for key in ['QT_PLUGIN_PATH','QML_IMPORT_PATH','QML2_IMPORT_PATH','LD_LIBRARY_PATH','DYLD_LIBRARY_PATH','QT_QPA_PLATFORM_PLUGIN_PATH']:env.pop(key,None)
    env['PATH']=os.path.join(os.environ.get('SystemRoot','C:/Windows'),'System32') if system=='Windows' else '/usr/bin:/bin'
    env['QT_QPA_PLATFORM']='offscreen';env['QT_QUICK_BACKEND']='software';env['QT_FORCE_STDERR_LOGGING']='1'
    if system=='Windows':env['QT_QPA_FONTDIR']=os.environ.get('SystemRoot','C:/Windows')+'/Fonts'
    elif system=='Darwin':env['QT_QPA_FONTDIR']='/System/Library/Fonts'
    result=subprocess.run([str(executable),'--smoke',str(ROOT/'tests/fixtures/sample.iso'),'--screenshot',str(output/(name+'-smoke.png')),'--performance',str(output/(name+'-performance.json'))],cwd=stage,env=env,capture_output=True,timeout=30)
    (output/(name+'-smoke.log')).write_bytes(result.stdout+result.stderr)
    if result.returncode:raise RuntimeError(f'Isolated packaged application/worker smoke failed ({result.returncode}): '+result.stderr.decode(errors='replace'))
    artifacts=[]
    if system=='Windows':
        (stage/'portable.flag').write_text('REAzip portable data marker\n');archive=output/(name+'-Portable.zip');zip_tree(stage,archive);artifacts.append(archive)
        if not a.skip_installer:
            installer=download('inno',tools);inno=tools/'inno-6.4.3'
            if not (inno/'ISCC.exe').exists():run(installer,'/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-','/CURRENTUSER','/DIR='+str(inno))
            run(inno/'ISCC.exe','/DStage='+str(stage),'/DOutput='+str(output),'/DArch='+a.arch,'/DVersion='+version,ROOT/'packaging/REAzip.iss')
            artifacts.append(output/(name+'-Setup.exe'))
            if os.environ.get('GITHUB_ACTIONS')=='true':
                installed=build/('installed-'+uuid.uuid4().hex)
                run(artifacts[-1],'/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-','/NOICONS','/TASKS=','/DIR='+str(installed))
                if (installed/'portable.flag').exists():raise RuntimeError('Installer incorrectly enabled portable mode')
                run(installed/'reazip.exe','--smoke',ROOT/'tests/fixtures/sample.iso',env=env)
                run(installed/'unins000.exe','/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART')
                deadline=time.monotonic()+15
                while (installed/'reazip.exe').exists() and time.monotonic()<deadline:time.sleep(.2)
                if (installed/'reazip.exe').exists():raise RuntimeError('Uninstall did not remove application payload')
    elif system=='Darwin':
        archive=output/(name+'.zip');run('ditto','-c','-k','--sequesterRsrc','--keepParent',bundle,archive);artifacts.append(archive)
        dmgroot=build/('dmg-'+uuid.uuid4().hex);dmgroot.mkdir();shutil.copytree(bundle,dmgroot/'REAzip.app',symlinks=True);(dmgroot/'Applications').symlink_to('/Applications')
        (dmgroot/'.metadata_never_index').touch();dmg=output/(name+'.dmg')
        for attempt in range(4):
            try:run('hdiutil','create','-volname','REAzip','-fs','HFS+','-srcfolder',dmgroot,'-ov','-format','UDZO',dmg);break
            except subprocess.CalledProcessError:
                if attempt==3:raise
                time.sleep(3*(attempt+1))
        artifacts.append(dmg)
    else:
        # CMake deployed Qt and its transitive libraries; AppRun sets paths relative to its readonly package.
        launcher='#!/bin/sh\nAPPDIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)\nexport LD_LIBRARY_PATH="$APPDIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\nexec "$APPDIR/bin/reazip" "$@"\n'
        (stage/'AppRun').write_text(launcher);(stage/'AppRun').chmod(0o755)
        desktop=('[Desktop Entry]\nType=Application\nName=REAzip\nComment=Open, create and extract archives\n'
                 'Exec=AppRun %F\nIcon=reazip\nTerminal=false\nCategories=Utility;Archiving;\n'
                 'MimeType=application/zip;application/x-7z-compressed;application/vnd.rar;application/x-tar;application/gzip;application/x-bzip2;application/x-xz;application/zstd;application/x-lzma;application/x-cd-image;application/x-iso9660-image;application/x-ms-wim;application/x-archive;application/x-arj;application/x-lzh-compressed;\n'
                 'Actions=ExtractHere;ExtractTo;\n'
                 '\n[Desktop Action ExtractHere]\nName=Extract here\nExec=AppRun --extract-here %f\n'
                 '\n[Desktop Action ExtractTo]\nName=Extract to folder...\nExec=AppRun --extract-to %f\n')
        (stage/'reazip.desktop').write_text(desktop)
        # Code-native SVG icon, matching the existing two-square app mark.
        (stage/'reazip.svg').write_text('<svg xmlns="http://www.w3.org/2000/svg" width="256" height="256"><rect x="20" y="20" width="170" height="170" rx="42" fill="#378add"/><rect x="134" y="134" width="98" height="98" rx="24" fill="white" stroke="#378add" stroke-width="12"/></svg>')
        archive=output/(name+'.tar.gz')
        with tarfile.open(archive,'w:gz') as tar:tar.add(stage,arcname=name)
        artifacts.append(archive)
        appimage=output/(name+'.AppImage');tool=download('appimage-'+a.arch,tools);runtime=download('runtime-'+a.arch,tools);tool.chmod(0o755)
        run(tool,'--appimage-extract-and-run','--runtime-file',runtime,stage,appimage,env=dict(os.environ,ARCH='aarch64' if a.arch=='arm64' else 'x86_64',VERSION=version,NO_APPSTREAM='1'));artifacts.append(appimage)
        debroot=build/('deb-'+uuid.uuid4().hex);application=debroot/'opt/reazip';shutil.copytree(stage,application,symlinks=True);(debroot/'DEBIAN').mkdir();debarch='arm64' if a.arch=='arm64' else 'amd64'
        (debroot/'DEBIAN/control').write_text(f'Package: reazip\nVersion: {version.replace("-dev","~dev")}\nArchitecture: {debarch}\nMaintainer: whh110112\nSection: utils\nPriority: optional\nDepends: libc6 (>= 2.39), libgl1, libxkbcommon0, libxcb-cursor0\nDescription: Native archive manager using Qt Quick and 7-Zip\n')
        applications=debroot/'usr/share/applications';applications.mkdir(parents=True);(applications/'com.reazip.app.desktop').write_text(desktop.replace('Exec=AppRun','Exec=/opt/reazip/AppRun').replace('Icon=reazip','Icon=/opt/reazip/reazip.svg'))
        bindir=debroot/'usr/bin';bindir.mkdir(parents=True);(bindir/'reazip').write_text('#!/bin/sh\nexec /opt/reazip/AppRun "$@"\n');(bindir/'reazip').chmod(0o755)
        deb=output/(name+'.deb');run('dpkg-deb','--root-owner-group','--build',debroot,deb);artifacts.append(deb)
    summaries=[{'file':file.name,'size':file.stat().st_size,'sha256':sha(file)} for file in artifacts]
    (output/(name+'-manifest.json')).write_text(json.dumps(dict(manifest,artifacts=summaries,unpackedBytes=sum(f.stat().st_size for f in stage.rglob('*') if f.is_file())),indent=2)+'\n')
    (output/(name+'-SHA256SUMS.txt')).write_text(''.join(item['sha256']+'  '+item['file']+'\n' for item in summaries))
    print(json.dumps({'stage':str(stage),'artifacts':summaries},indent=2))
if __name__=='__main__':main()
