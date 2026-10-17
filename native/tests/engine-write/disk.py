import os,pathlib,platform,shutil,subprocess,sys,tempfile
helper,library,root=sys.argv[1:];root=pathlib.Path(root);root.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='disk-',dir=root) as name:
    folder=pathlib.Path(name);output=folder/'output';source=folder/'source'
    if platform.system()=='Linux' and os.environ.get('GITHUB_ACTIONS')=='true':
        output.mkdir()
        subprocess.run(['sudo','-n','mount','-t','tmpfs','-o',f'size=2m,uid={os.getuid()},gid={os.getgid()},mode=0700','tmpfs',str(output)],check=True)
        try:
            result=subprocess.run([helper,library,str(source),str(output),'full'],timeout=40)
            if result.returncode:
                if shutil.which('gdb'):
                    subprocess.run(['gdb','-batch','-ex','set pagination off','-ex','run','-ex','thread apply all bt','--args',helper,library,str(folder/'debug-source'),str(output/'debug'),'full'],timeout=60)
                result.check_returncode()
        finally:subprocess.run(['sudo','-n','umount',str(output)],check=True)
        print('Linux 2MiB tmpfs: real ENOSPC during archive write and extraction PASS')
    else:
        subprocess.run([helper,library,str(source),str(output),'locked' if platform.system()=='Windows' else 'limit'],check=True,timeout=40)
        print('Windows denied replacement / POSIX file-size-limit refusal PASS; real ENOSPC covered by Linux CI')
