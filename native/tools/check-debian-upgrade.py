"""Use a disposable dpkg root to check the real old-to-new package transition."""
import argparse, json, pathlib, subprocess, tempfile
p=argparse.ArgumentParser();p.add_argument('package');p.add_argument('output');a=p.parse_args()
package=pathlib.Path(a.package).resolve()
def run(*args,**kwargs):return subprocess.run([str(x) for x in args],check=True,**kwargs)
with tempfile.TemporaryDirectory(prefix='zenzip-dpkg-') as folder:
    work=pathlib.Path(folder);root=work/'root';root.mkdir()
    run('gh','release','download','v0.3.1-dev','--repo','whh110112/ZENzip',
        '--pattern','REAzip-0.3.1-dev-linux-x64.deb','--dir',work)
    old=work/'REAzip-0.3.1-dev-linux-x64.deb'
    run('sudo','dpkg','--root='+str(root),'--force-depends','--install',old)
    assert (root/'opt/reazip').is_dir()
    run('sudo','dpkg','--root='+str(root),'--force-depends','--install',package)
    assert (root/'opt/zenzip/bin/zenzip').is_file()
    assert not (root/'usr/bin/reazip').exists()
    assert not (root/'usr/share/applications/com.reazip.app.desktop').exists()
    assert not (root/'opt/reazip').exists()
    run(root/'opt/zenzip/AppRun','--smoke')
    run('sudo','dpkg','--root='+str(root),'--remove','zenzip')
    assert not (root/'usr/bin/zenzip').exists()
    pathlib.Path(a.output).write_text(json.dumps({'old':'reazip 0.3.1-dev','new':'zenzip 0.4.0',
        'realDpkgTransition':True,'obsoletePayloadRemoved':True,'installedSmoke':True,
        'uninstall':True,'scope':'isolated dpkg root; dependency checks intentionally disabled'},indent=2)+'\n')
    # dpkg created root-owned metadata; clean only the explicitly allocated disposable root.
    run('sudo','chown','-R',str(__import__('os').getuid())+':'+str(__import__('os').getgid()),root)
print('Debian package rename, installed payload and removal PASS')
