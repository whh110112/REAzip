"""Use a disposable dpkg root to check the real old-to-new package transition."""
import argparse, json, pathlib, subprocess, tempfile
p=argparse.ArgumentParser();p.add_argument('package');p.add_argument('output',nargs='?');a=p.parse_args()
version=(pathlib.Path(__file__).resolve().parents[1]/'VERSION.txt').read_text().strip()
package=pathlib.Path(a.package).resolve()
if package.is_dir():package=package/f'ZENzip-{version}-linux-x64.deb'
output=pathlib.Path(a.output) if a.output else package.with_name(package.stem+'-upgrade.json')
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
    output.write_text(json.dumps({'old':'previous native package','new':'zenzip '+version,
        'realDpkgTransition':True,'obsoletePayloadRemoved':True,'installedSmoke':True,
        'uninstall':True,'scope':'isolated dpkg root; dependency checks intentionally disabled'},indent=2)+'\n')
    # dpkg created root-owned metadata; clean only the explicitly allocated disposable root.
    run('sudo','chown','-R',str(__import__('os').getuid())+':'+str(__import__('os').getgid()),root)
print('Debian package rename, installed payload and removal PASS')
