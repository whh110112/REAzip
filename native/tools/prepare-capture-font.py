"""Pinned, build-only CJK font for reproducible headless acceptance captures."""
import argparse, hashlib, pathlib, time, urllib.request
p=argparse.ArgumentParser();p.add_argument('output');a=p.parse_args()
root=pathlib.Path(a.output);root.mkdir(parents=True,exist_ok=True)
path=root/'NotoSansCJKsc-Regular.otf'
expected='2c76254f6fc379fddfce0a7e84fb5385bb135d3e399294f6eeb6680d0365b74b'
url='https://raw.githubusercontent.com/notofonts/noto-cjk/f8d157532fbfaeda587e826d4cd5b21a49186f7c/Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf'
if not path.exists() or hashlib.sha256(path.read_bytes()).hexdigest()!=expected:
    for attempt in range(3):
        try:
            with urllib.request.urlopen(url,timeout=120) as response:data=response.read()
            if hashlib.sha256(data).hexdigest()!=expected:raise RuntimeError('Capture font hash mismatch')
            path.write_bytes(data);break
        except Exception:
            if attempt==2:raise
            time.sleep(2*(attempt+1))
print(str(path))
