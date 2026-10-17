const fs = require('fs');
const path = require('path');
const vm = require('vm');
const root = path.resolve(__dirname, '../..');
const read = name => fs.readFileSync(path.join(root, 'renderer', name), 'utf8');
const out = (name, value) => {
  const dest = path.join(root, 'native', name);
  fs.mkdirSync(path.dirname(dest), { recursive: true });
  fs.writeFileSync(dest, JSON.stringify(value, null, 2) + '\n');
};
const skins = { window: {} };
vm.runInNewContext(read('skins.js'), skins);
out('themes/builtin.json', skins.window.REAzipSkins.BUILTIN_SKINS);
out('design/tokens.json', { window: {width:1120,height:740,minWidth:880,minHeight:560},
  titlebar:36,tabs:36,toolbar:56,fontSize:13,themes:skins.window.REAzipSkins.BUILTIN_SKINS });
const loc = { window: {} };
for (const name of ['locales-1.js', 'locales-2.js', 'locales-3.js']) vm.runInNewContext(read(name), loc);
for (const [key, value] of Object.entries(loc.window.LOCALES)) out('i18n/' + key + '.json', value);
const html = read('index.html');
out('design/controls.json', [...html.matchAll(/<(button|input|select|div)[^>]*\bid="([^"]+)"[^>]*>/g)].map(m=>({element:m[1],id:m[2]})));
