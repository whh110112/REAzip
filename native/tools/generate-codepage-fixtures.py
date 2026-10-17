"""Original, deterministic ZIP filename-byte fixtures; no third-party payload."""
import pathlib,zipfile
root=pathlib.Path(__file__).resolve().parents[1]/'tests/fixtures'
for file,name,encoding in [('gbk.zip','中文.txt','gbk'),('big5.zip','繁體中文.txt','big5'),('shiftjis.zip','日本語.txt','shift_jis')]:
    raw=name.encode(encoding);placeholder='x'*len(raw);path=root/file
    info=zipfile.ZipInfo(placeholder,date_time=(2026,1,1,0,0,0));info.compress_type=zipfile.ZIP_STORED
    with zipfile.ZipFile(path,'w') as archive:archive.writestr(info,b'REAzip original legacy filename fixture\n')
    data=path.read_bytes();assert data.count(placeholder.encode())==2
    path.write_bytes(data.replace(placeholder.encode(),raw))
print('Generated GBK/Big5/Shift-JIS filename byte fixtures')
