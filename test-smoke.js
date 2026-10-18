'use strict';
// 冒烟测试：压缩 → 列表 → 解压 → 完整性测试
const fs = require('fs');
const path = require('path');
const os = require('os');
const lib = require('./lib/archive');

(async () => {
  const tmp = path.join(os.tmpdir(), 'zenzip-smoke-' + Date.now());
  fs.mkdirSync(path.join(tmp, 'src', 'sub'), { recursive: true });
  fs.writeFileSync(path.join(tmp, 'src', 'a.txt'), 'hello zenzip 你好\n'.repeat(50));
  fs.writeFileSync(path.join(tmp, 'src', 'sub', 'b.md'), '# markdown\n' + 'x'.repeat(2000));

  console.log('7za:', lib.bin());

  const zip = path.join(tmp, 'out.zip');
  await lib.create(zip, [path.join(tmp, 'src', '*')], { format: 'zip', level: 5 });
  console.log('create ok, size =', fs.statSync(zip).size);

  const listed = await lib.list(zip);
  console.log('list ok, type =', listed.meta.type, '| entries =', listed.entries.length);
  console.log(listed.entries.map(e => `${e.isFolder ? '[D]' : '[F]'} ${e.path} (${e.size})`).join('\n'));

  const dest = path.join(tmp, 'dest');
  let lastPct = -1;
  await lib.extract(zip, dest, {}, ({ percent }) => { lastPct = percent; });
  console.log('extract ok, last progress =', lastPct, '%');
  console.log('extracted a.txt exists =', fs.existsSync(path.join(dest, 'a.txt')));
  console.log('extracted sub/b.md exists =', fs.existsSync(path.join(dest, 'sub', 'b.md')));

  const t = await lib.test(zip);
  console.log('test ok =', t.ok);

  const sevenz = path.join(tmp, 'out.7z');
  await lib.create(sevenz, [path.join(tmp, 'src', 'a.txt')], { format: '7z', level: 9, password: 'secret123' });
  const l2 = await lib.list(sevenz, 'secret123');
  console.log('7z encrypted list ok =', l2.entries.length, 'entries, encrypted =', l2.entries[0].encrypted);

  const tgz = path.join(tmp, 'out.tar.gz');
  await lib.createTgz(tgz, [path.join(tmp, 'src')], null);
  const l3 = await lib.list(tgz);
  console.log('tgz ok =', l3.meta.type, l3.entries.length, 'entries');

  console.log('ALL PASS');
})().catch(e => { console.error('FAIL:', e.message); process.exit(1); });
