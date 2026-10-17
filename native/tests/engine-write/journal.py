import pathlib,subprocess,sys,tempfile
helper,library,root=sys.argv[1:];root=pathlib.Path(root);root.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='journal-',dir=root) as folder:
    base=pathlib.Path(folder)
    def run(mode,work,expected=0):
        result=subprocess.run([helper,library,str(work),mode],capture_output=True,text=True,timeout=40)
        assert result.returncode==expected,(mode,result.returncode,result.stdout,result.stderr)
    for mode,code in [('crash',86),('rollback',0),('complete',88)]:
        work=base/mode;run('init',work);run(mode,work,code);run('verify-complete' if mode=='complete' else 'verify',work)
    run('split',base/'split',87);run('verify-split',base/'split')
    run('init',base/'external');run('crash',base/'external',86);run('verify-external',base/'external')
print('crash before/after commit, partial volumes, ordinary rollback, external modification protection PASS')
