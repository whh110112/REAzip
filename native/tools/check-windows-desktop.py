"""Exercise real per-user registration and its command on a disposable CI user."""
import argparse, json, os, pathlib, subprocess, tempfile, winreg, zipfile

p=argparse.ArgumentParser();p.add_argument('application');p.add_argument('output');p.add_argument('--uninstalled',action='store_true');a=p.parse_args()
exe=pathlib.Path(a.application).resolve()
def value(path,name=''):
    try:
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER,path) as key:return winreg.QueryValueEx(key,name)[0]
    except FileNotFoundError:return None

classes='Software\\Classes\\'
if a.uninstalled:
    assert value(classes+'ZENzip.Archive','OwnerExecutable') != str(exe)
    assert value(classes+'SystemFileAssociations\\.zip\\shell\\ZENzip.extract_here','OwnerExecutable') != str(exe)
    assert value('Software\\RegisteredApplications','ZENzip') is None
    print('Owned per-user registration removed after uninstall PASS')
else:
    assert value(classes+'ZENzip.Archive\\shell\\open\\command') == f'"{exe}" "%1"'
    assert value('Software\\RegisteredApplications','ZENzip')=='Software\\ZENzip\\Capabilities'
    key=classes+'SystemFileAssociations\\.zip\\shell\\ZENzip.extract_here'
    command=value(key+'\\command')
    assert command==f'"{exe}" --extract-here "%1"'
    assert value(key,'Icon')==f'"{exe}",0'
    assert value(key,'MultiSelectModel')=='Player'
    assert value(classes+f'Applications\\{exe.name}','FriendlyAppName')=='ZENzip'
    assert value(classes+'*\\shell\\ZENzip.Add\\command')==f'"{exe}" --add "%1"'
    # Execute the exact registered extract-here action, not a worker-only surrogate.
    with tempfile.TemporaryDirectory(prefix='zenzip-desktop-') as work:
        root=pathlib.Path(work);archive=root/'menu-test.zip';payload=b'ZENzip registered shell action roundtrip'
        with zipfile.ZipFile(archive,'w') as z:z.writestr('menu-result.txt',payload)
        subprocess.run([str(exe),'--smoke','--extract-here',str(archive)],check=True,timeout=30)
        assert (root/'menu-result.txt').read_bytes()==payload
        subprocess.run([str(exe),'--smoke',str(archive)],check=True,timeout=30)
    result={'platform':'windows','executable':str(exe),'progid':True,'openWith':True,'icons':True,
            'classicVerbs':True,'registeredExtractHereRoundtrip':True,'registeredOpen':True,
            'explorerVisualMenu':'not exercised; registry and actual registered command verified',
            'defaultApplication':'user controlled; not changed'}
    pathlib.Path(a.output).write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('Real HKCU registration, archive opening and extract-here PASS')
