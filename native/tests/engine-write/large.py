import pathlib,subprocess,sys,tempfile,zipfile
helper,library,root=sys.argv[1:];root=pathlib.Path(root);root.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='zip64-',dir=root) as directory:
    subprocess.run([helper,library,directory],check=True,timeout=200)
    size=0x100000000+4096
    with zipfile.ZipFile(pathlib.Path(directory)/'large.zip') as archive:
        info=archive.getinfo('over-4GiB.bin');assert info.file_size==size
        count=0
        with archive.open(info) as stream:
            for block in iter(lambda:stream.read(1024*1024),b''):
                count+=len(block)
                if count==size:assert block[-1:]==b'Z' and block[:-1].count(0)==len(block)-1
                else:assert block.count(0)==len(block)
        assert count==size
print('Independent Python ZIP64 decoder verified all >4GiB payload bytes and CRC, bounded sparse storage PASS')
