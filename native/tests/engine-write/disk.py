import os,pathlib,platform,shutil,subprocess,sys,tempfile
helper,library,root=sys.argv[1:];root=pathlib.Path(root);root.mkdir(parents=True,exist_ok=True)
def mountTmpfs(target):subprocess.run(['sudo','-n','mount','-t','tmpfs','-o',f'size=2m,uid={os.getuid()},gid={os.getgid()},mode=0700','tmpfs',str(target)],check=True)
def unmountTmpfs(target):subprocess.run(['sudo','-n','umount',str(target)],check=False)
with tempfile.TemporaryDirectory(prefix='disk-',dir=root) as name:
    folder=pathlib.Path(name);output=folder/'output';source=folder/'source'
    if platform.system()=='Linux' and os.environ.get('GITHUB_ACTIONS')=='true':
        output.mkdir()
        mountTmpfs(output)
        try:
            result=subprocess.run([helper,library,str(source),str(output),'full'],timeout=40)
            if result.returncode:
                if shutil.which('gdb'):
                    # Remount a fresh tmpfs before the replay: residue from the failed run already
                    # exhausted the first one, so the debug pass would otherwise fail before reaching
                    # the extraction stage. -nx skips .gdbinit and avoids the runner Python conflict.
                    unmountTmpfs(output);mountTmpfs(output)
                    subprocess.run(['gdb','-nx','-batch','-ex','set pagination off','-ex','set auto-load off','-ex','run','-ex','thread apply all bt','--args',helper,library,str(folder/'debug-source'),str(output),'full'],timeout=120)
                result.check_returncode()
        finally:unmountTmpfs(output)
        print('Linux 2MiB tmpfs: real ENOSPC during archive write and extraction PASS')
    else:
        subprocess.run([helper,library,str(source),str(output),'locked' if platform.system()=='Windows' else 'limit'],check=True,timeout=40)
        print('Windows denied replacement / POSIX file-size-limit refusal PASS; real ENOSPC covered by Linux CI')
