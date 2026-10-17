'use strict';
// v0.2 冒烟测试：完整引擎 / 删除条目 / 代码页 / 加密 / tgz
const fs = require('fs');
const path = require('path');
const os = require('os');
const lib = require('./lib/archive');

(async () => {
  const tmp = path.join(os.tmpdir(), 'reazip-v02-' + Date.now());
  fs.mkdirSync(path.join(tmp, 'src', 'sub'), { recursive: true });
  fs.writeFileSync(path.join(tmp, 'src', 'a.txt'), 'hello reazip 你好\n'.repeat(50));
  fs.writeFileSync(path.join(tmp, 'src', 'sub', 'b.md'), '# md\n' + 'x'.repeat(2000));
  fs.writeFileSync(path.join(tmp, 'src', 'gbk-name-test.txt'), 'gbk');

  console.log('engine:', lib.bin());

  const zip = path.join(tmp, 'out.zip');
  await lib.create(zip, [path.join(tmp, 'src', '*')], { format: 'zip', level: 5 });
  let l = await lib.list(zip);
  console.log('create/list ok, entries =', l.entries.length, 'type =', l.meta.type);

  // 代码页参数（zip）
  const lcp = await lib.list(zip, '', '936');
  console.log('list with cp936 ok, entries =', lcp.entries.length);

  // 删除一个条目
  await lib.deleteEntries(zip, ['a.txt']);
  l = await lib.list(zip);
  console.log('after delete, entries =', l.entries.length, '| a.txt gone =', !l.entries.some((e) => e.path === 'a.txt'));

  // 解压
  const dest = path.join(tmp, 'dest');
  await lib.extract(zip, dest, { codepage: '936' });
  console.log('extract ok, sub/b.md =', fs.existsSync(path.join(dest, 'sub', 'b.md')));

  // 加密 7z
  const seven = path.join(tmp, 'enc.7z');
  await lib.create(seven, [path.join(tmp, 'src', 'a.txt')], { format: '7z', level: 9, password: 'pw123' });
  const le = await lib.list(seven, 'pw123');
  console.log('7z encrypted ok, entries =', le.entries.length, 'encrypted =', le.entries[0].encrypted);

  // tar.gz
  const tgz = path.join(tmp, 'out.tar.gz');
  await lib.createTgz(tgz, [path.join(tmp, 'src')]);
  const lt = await lib.list(tgz);
  console.log('tgz ok, type =', lt.meta.type);

  // 测试
  console.log('test ok =', (await lib.test(zip)).ok);
  console.log('ALL PASS');
})().catch((e) => { console.error('FAIL:', e.message); process.exit(1); });
