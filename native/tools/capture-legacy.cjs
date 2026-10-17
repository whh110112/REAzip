// Test harness: private profile and app-only screenshots, never desktop capture.
const { app, BrowserWindow } = require('electron');
const fs = require('fs');
const path = require('path');
const os = require('os');
const root = path.resolve(__dirname, '../..');
app.setPath('userData', fs.mkdtempSync(path.join(os.tmpdir(), 'reazip-baseline-')));
require('../../main.js');
app.whenReady().then(async () => {
  const win = BrowserWindow.getAllWindows()[0];
  if (win.webContents.isLoading()) await new Promise(resolve => win.webContents.once('did-finish-load', resolve));
  const js = code => win.webContents.executeJavaScript(code);
  const out = path.join(root, '.native-build', 'baseline', String(process.env.BASELINE_DPI || '1'));
  fs.mkdirSync(out, {recursive:true});
  const capture = async name => {
    await js('new Promise(r => requestAnimationFrame(() => requestAnimationFrame(r)))');
    fs.writeFileSync(path.join(out, name+'.png'), (await win.webContents.capturePage()).toPNG());
  };
  await js('new Promise(r => setTimeout(r, 300))');
  await capture('empty');
  await js(`openArchive(${JSON.stringify(path.join(root,'demo','demo.zip'))})`);
  await capture('archive');
  for (const skin of ['light','sleep','mac','mac-dark','glass','material']) {
    await js(`REAzipSkins.applySkin(${JSON.stringify(skin)})`);
    await capture('theme-'+skin);
  }
  await js("REAzipSkins.applySkin('light');openCompressModal()"); await capture('compress');
  await js("closeModal('modal-compress');openExtractModal()"); await capture('extract');
  await js("closeModal('modal-extract');openSettings()"); await capture('settings');
  fs.writeFileSync(path.join(out,'manifest.json'), JSON.stringify({scale:process.env.BASELINE_DPI||'1',electron:process.versions.electron,os:process.platform,arch:process.arch,capturedAt:new Date().toISOString()},null,2));
  app.exit(0);
}).catch(error => { console.error(error); app.exit(1); });
