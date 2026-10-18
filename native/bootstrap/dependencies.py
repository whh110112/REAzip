"""Build-time only bootstrap. No dependency downloads in the installed app."""
import argparse, hashlib, json, os, pathlib, platform, shutil, subprocess, tarfile, time, urllib.request

ROOT = pathlib.Path(__file__).resolve().parents[1]
LOCK = json.loads((ROOT / 'deps.lock.json').read_text())
DEPS = ROOT / '.deps'

def run(*args, cwd=None):
    subprocess.run([str(a) for a in args], cwd=cwd, check=True)

def download(spec):
    dest = DEPS / spec['name']
    if not dest.exists() or hashlib.sha256(dest.read_bytes()).hexdigest() != spec['sha256']:
        temporary = dest.with_suffix(dest.suffix + '.download')
        for attempt in range(3):
            try:
                with urllib.request.urlopen(LOCK['sevenZip']['baseUrl'] + spec['name'], timeout=120) as src, temporary.open('wb') as dst:
                    shutil.copyfileobj(src, dst)
                if hashlib.sha256(temporary.read_bytes()).hexdigest() != spec['sha256']:
                    raise RuntimeError('SHA256 mismatch: ' + dest.name)
                temporary.replace(dest)
                break
            except Exception:
                temporary.unlink(missing_ok=True)
                if attempt == 2:
                    raise
                time.sleep(2 * (attempt + 1))
    if hashlib.sha256(dest.read_bytes()).hexdigest() != spec['sha256']:
        raise RuntimeError('SHA256 mismatch: ' + dest.name)
    return dest

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--arch', choices=['x64','arm64'], default='arm64' if platform.machine().lower() in ['arm64','aarch64'] else 'x64')
    parser.add_argument('--extractor', default=shutil.which('7z') or '')
    args = parser.parse_args()
    DEPS.mkdir(exist_ok=True)
    bit = DEPS / 'bit7z'
    if not (bit / '.git').exists():
        run('git','clone','--no-checkout',LOCK['bit7z']['repository'],bit)
    actual = subprocess.check_output(['git','rev-parse','HEAD'],cwd=bit,text=True).strip() if (bit/'.git/HEAD').exists() else ''
    if actual != LOCK['bit7z']['commit'] or not (bit/'CMakeLists.txt').exists():
        run('git','fetch','--depth','1','origin',LOCK['bit7z']['commit'],cwd=bit)
        run('git','checkout','--detach',LOCK['bit7z']['commit'],cwd=bit)
    source = download(LOCK['sevenZip']['artifacts']['source'])
    zsrc = DEPS / '7zip'
    if not (zsrc / 'CPP/7zip/Bundles/Format7zF/makefile.gcc').exists():
        zsrc.mkdir(exist_ok=True)
        with tarfile.open(source) as archive:
            archive.extractall(zsrc, filter='data')
    engine = DEPS / 'engine' / args.arch
    engine.mkdir(parents=True, exist_ok=True)
    if platform.system() == 'Windows':
        installer = download(LOCK['sevenZip']['artifacts'][args.arch])
        extractor = args.extractor or str(ROOT.parent / 'resources/engine/win-x64/7z.exe')
        run(extractor,'x','-y',f'-o{engine}',installer,'7z.dll','License.txt')
    else:
        conversion = zsrc / 'CPP/Common/StringConvert.cpp'
        original = conversion.read_text(encoding='utf-8-sig')
        if 'ZENZIP_POSIX_CODEPAGE_V1' not in original:
            patch = (ROOT / 'bootstrap/posix-codepage.inc').read_text(encoding='utf-8')
            needle = '  if (codePage == CP_UTF8 || g_ForceToUTF8)'
            if needle not in original:
                raise RuntimeError('Codepage patch context mismatch')
            original = '#ifndef _WIN32\n#include <iconv.h>\n#endif\n' + original.replace(needle, patch + needle, 1)
            conversion.write_text(original, encoding='utf-8')
        bundle = zsrc / 'CPP/7zip/Bundles/Format7zF'
        command = ['make','-f','makefile.gcc','-j2']
        if platform.system() == 'Darwin':
            command += ['CC=clang','CXX=clang++','LIB2=-liconv']
        run(*command,cwd=bundle)
        shutil.copy2(bundle / '_o/7z.so',engine / '7z.so')
        shutil.copy2(zsrc / 'DOC/License.txt',engine / 'License.txt')
    print('ENGINE_DIR=' + str(engine.resolve()), flush=True)

if __name__ == '__main__':
    main()
