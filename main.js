'use strict';

const { app, BrowserWindow, ipcMain, dialog, shell, nativeTheme, Menu } = require('electron');
const path = require('path');
const fs = require('fs');
const archive = require('./lib/archive');
const assoc = require('./lib/assoc');

// 工具类应用优先稳定：部分环境（远程桌面 / 受限沙箱）GPU 进程不可用
app.disableHardwareAcceleration();
app.commandLine.appendSwitch('disable-gpu');
app.commandLine.appendSwitch('disable-gpu-compositing');
app.commandLine.appendSwitch('no-sandbox');
app.commandLine.appendSwitch('disable-gpu-sandbox');

let win = null;
let settings = {};

const ARCHIVE_EXT = [
  'zip', '7z', 'rar', 'tar', 'gz', 'tgz', 'bz2', 'tbz', 'xz', 'txz', 'zst', 'lz4', 'z',
  'iso', 'cab', 'arj', 'lzh', 'lha', 'jar', 'apk', 'xpi', 'deb', 'rpm', 'img', 'wim', 'msi', '001'
];

function settingsFile() {
  return path.join(app.getPath('userData'), 'settings.json');
}

function loadSettings() {
  try {
    settings = JSON.parse(fs.readFileSync(settingsFile(), 'utf8')) || {};
  } catch (e) {
    settings = {};
  }
  if (!settings.theme) settings.theme = 'light';
  if (!settings.skin) settings.skin = 'light';
  if (!settings.language) settings.language = 'zh-CN';
  if (!settings.codepage) settings.codepage = '';
  if (!Array.isArray(settings.assocExts)) settings.assocExts = assoc.DEFAULT_EXTS.slice();
  if (!settings.extractDir) settings.extractDir = app.getPath('downloads');
  if (!settings.archiveDir) settings.archiveDir = app.getPath('downloads');
}

function persistSettings() {
  try {
    fs.mkdirSync(path.dirname(settingsFile()), { recursive: true });
    fs.writeFileSync(settingsFile(), JSON.stringify(settings, null, 2));
  } catch (e) { /* 写入失败不影响使用 */ }
}

function isDarkTheme() {
  if (settings.theme === 'sleep') return true;
  if (settings.theme === 'system') return nativeTheme.shouldUseDarkColors;
  return false;
}

function applyTitlebar() {
  if (!win || win.isDestroyed()) return;
  const dark = isDarkTheme();
  win.setBackgroundColor(dark ? '#232220' : '#FFFFFF');
  try {
    win.setTitleBarOverlay({
      color: dark ? '#2C2C2A' : '#F1EFE8',
      symbolColor: dark ? '#F1EFE8' : '#2C2C2A',
      height: 36
    });
  } catch (e) { /* 非 Windows 或旧版本忽略 */ }
}

function createWindow() {
  const dark = isDarkTheme();
  win = new BrowserWindow({
    width: 1120,
    height: 740,
    minWidth: 880,
    minHeight: 560,
    backgroundColor: dark ? '#232220' : '#FFFFFF',
    titleBarStyle: 'hidden',
    titleBarOverlay: {
      color: dark ? '#2C2C2A' : '#F1EFE8',
      symbolColor: dark ? '#F1EFE8' : '#2C2C2A',
      height: 36
    },
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false,
      spellcheck: false
    }
  });

  win.loadFile(path.join(__dirname, 'renderer', 'index.html'));

  const logFile = path.join(__dirname, 'logs.txt');
  const log = (m) => { try { fs.appendFileSync(logFile, new Date().toISOString() + ' ' + m + '\n'); } catch (e) {} };
  win.webContents.on('did-fail-load', (e, code, desc, url) => log(`did-fail-load ${code} ${desc} ${url}`));
  win.webContents.on('console-message', (e, level, message, line, source) => log(`console[${level}] ${message} (${source}:${line})`));
  win.webContents.on('render-process-gone', (e, details) => log('render-process-gone ' + JSON.stringify(details)));
  win.webContents.on('did-finish-load', () => log('did-finish-load'));

  win.on('closed', () => { win = null; });
}

app.whenReady().then(() => {
  loadSettings();
  Menu.setApplicationMenu(null);
  createWindow();

  nativeTheme.on('updated', () => {
    if (settings.theme === 'system') applyTitlebar();
  });

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});

/* ---------------- IPC ---------------- */

ipcMain.handle('dialog:open-archive', async () => {
  const r = await dialog.showOpenDialog(win, {
    title: '打开压缩包',
    properties: ['openFile', 'multiSelections'],
    filters: [
      { name: '压缩包', extensions: ARCHIVE_EXT },
      { name: '所有文件', extensions: ['*'] }
    ]
  });
  return r.canceled ? [] : r.filePaths;
});

ipcMain.handle('dialog:select-files', async () => {
  const r = await dialog.showOpenDialog(win, {
    title: '选择要压缩的文件',
    properties: ['openFile', 'multiSelections'],
    filters: [{ name: '所有文件', extensions: ['*'] }]
  });
  return r.canceled ? [] : r.filePaths;
});

ipcMain.handle('dialog:select-folder', async () => {
  const r = await dialog.showOpenDialog(win, {
    title: '选择要压缩的文件夹',
    properties: ['openDirectory']
  });
  return r.canceled ? '' : r.filePaths[0];
});

ipcMain.handle('dialog:select-dest', async (e, defaultDir) => {
  const r = await dialog.showOpenDialog(win, {
    title: '选择解压目标文件夹',
    properties: ['openDirectory', 'createDirectory'],
    defaultPath: defaultDir || settings.extractDir || app.getPath('downloads')
  });
  return r.canceled ? '' : r.filePaths[0];
});

ipcMain.handle('dialog:save-archive', async (e, defaultName, format) => {
  const filters = format === '7z'
    ? [{ name: '7Z 压缩包', extensions: ['7z'] }]
    : format === 'tgz'
      ? [{ name: 'TAR.GZ 压缩包', extensions: ['tar.gz', 'tgz'] }]
      : format === 'tar'
        ? [{ name: 'TAR 归档', extensions: ['tar'] }]
        : [{ name: 'ZIP 压缩包', extensions: ['zip'] }];
  const r = await dialog.showSaveDialog(win, {
    title: '保存压缩包',
    defaultPath: path.join(settings.archiveDir || app.getPath('downloads'), defaultName || '未命名.zip'),
    filters
  });
  return r.canceled ? '' : r.filePath;
});

ipcMain.handle('archive:list', async (e, archivePath, password, codepage) => {
  return archive.list(archivePath, password || '', codepage || settings.codepage || '');
});

ipcMain.handle('archive:extract', async (e, job) => {
  return archive.extract(job.archive, job.dest, {
    password: job.password || '',
    overwrite: job.overwrite || 'overwrite',
    entries: job.entries || [],
    codepage: job.codepage || settings.codepage || ''
  }, (p) => {
    if (win && !win.isDestroyed()) win.webContents.send('job:progress', p);
  });
});

ipcMain.handle('archive:delete', async (e, archivePath, entries, password) => {
  await archive.deleteEntries(archivePath, entries, password || '');
  return true;
});

ipcMain.handle('archive:create', async (e, job) => {
  const onProgress = (p) => {
    if (win && !win.isDestroyed()) win.webContents.send('job:progress', p);
  };
  if (job.format === 'tgz') {
    return archive.createTgz(job.archive, job.files, onProgress);
  }
  return archive.create(job.archive, job.files, {
    format: job.format,
    level: job.level,
    volume: job.volume,
    password: job.password || '',
    deleteAfter: !!job.deleteAfter
  }, onProgress);
});

ipcMain.handle('archive:test', async (e, archivePath, password) => {
  return archive.test(archivePath, password || '');
});

ipcMain.handle('archive:cancel', async () => archive.cancel());

ipcMain.handle('archive:conflicts', async (e, archivePath, dest, password) => {
  try {
    const { entries } = await archive.list(archivePath, password || '');
    let n = 0;
    for (const ent of entries) {
      if (ent.isFolder) continue;
      if (fs.existsSync(path.join(dest, ent.path))) n++;
    }
    return n;
  } catch (err) {
    return 0;
  }
});

ipcMain.handle('settings:get', async () => settings);

ipcMain.handle('settings:set', async (e, patch) => {
  settings = { ...settings, ...patch };
  persistSettings();
  if (patch.theme) applyTitlebar();
  return settings;
});

ipcMain.handle('theme:set', async (e, theme) => {
  settings.theme = theme;
  persistSettings();
  applyTitlebar();
  return theme;
});

ipcMain.handle('window:material', async (e, m) => {
  if (!win || win.isDestroyed()) return false;
  try {
    win.setBackgroundMaterial(m === 'acrylic' ? 'acrylic' : (m === 'mica' ? 'mica' : 'none'));
    return true;
  } catch (err) {
    return false;
  }
});

ipcMain.handle('window:focus', async () => {
  if (win && !win.isDestroyed()) {
    win.show();
    win.setPosition(40, 40);
    win.focus();
    win.moveTop();
  }
  return true;
});

ipcMain.handle('shell:show-item', async (e, p) => {
  shell.showItemInFolder(p);
});

ipcMain.handle('shell:open-path', async (e, p) => {
  return shell.openPath(p);
});

ipcMain.handle('app:info', async () => ({
  version: app.getVersion(),
  electron: process.versions.electron,
  node: process.versions.node,
  sevenZip: (() => {
    try { return require('7zip-bin').path7za; } catch (err) { return '未找到'; }
  })(),
  engine: (() => {
    try { return archive.bin(); } catch (err) { return '未找到'; }
  })(),
  platform: process.platform
}));

/* ---------- 皮肤 ---------- */
function skinsDir() {
  return path.join(app.getPath('userData'), 'skins');
}

function readCustomSkins() {
  const dir = skinsDir();
  const out = [];
  try {
    if (!fs.existsSync(dir)) return out;
    for (const name of fs.readdirSync(dir)) {
      if (!/\.json$/i.test(name)) continue;
      try {
        const raw = fs.readFileSync(path.join(dir, name), 'utf8');
        const data = JSON.parse(raw);
        if (!data || typeof data !== 'object') continue;
        const id = String(data.id || '').trim();
        if (!/^[a-zA-Z0-9_-]{1,40}$/.test(id)) continue;
        const vars = {};
        for (const [k, v] of Object.entries(data.vars || {})) {
          if (/^--[a-z0-9-]+$/i.test(k) && (typeof v === 'string' || typeof v === 'number')) {
            vars[k] = String(v);
          }
        }
        out.push({
          id,
          name: String(data.name || id),
          base: data.base === 'dark' ? 'dark' : 'light',
          author: data.author ? String(data.author) : '',
          vars,
          extraCss: typeof data.css === 'string' ? data.css.slice(0, 20000) : ''
        });
      } catch (e) { /* 跳过无效文件 */ }
    }
  } catch (e) { /* ignore */ }
  return out;
}

ipcMain.handle('skins:list', async () => readCustomSkins());

ipcMain.handle('skins:open-dir', async () => {
  const dir = skinsDir();
  fs.mkdirSync(dir, { recursive: true });
  const sample = path.join(dir, 'example.json');
  if (!fs.existsSync(sample)) {
    fs.writeFileSync(sample, JSON.stringify({
      id: 'example',
      name: '示例皮肤',
      base: 'light',
      author: 'you',
      vars: {
        '--bg': '#ffffff',
        '--surface': '#f2f2f7',
        '--text': '#1d1d1f',
        '--accent': '#0a84ff'
      }
    }, null, 2), 'utf8');
  }
  return shell.openPath(dir);
});

/* ---------- 文件关联 / 默认应用 ---------- */
ipcMain.handle('assoc:apply', async (e, payload) => {
  const exts = Array.isArray(payload.exts) ? payload.exts.slice(0, 40) : assoc.DEFAULT_EXTS;
  const result = await assoc.applyAssociations({
    exts,
    setDefault: !!payload.setDefault,
    exePath: process.execPath,
    appPath: app.getAppPath(),
    isPackaged: app.isPackaged
  });
  if (Array.isArray(payload.exts)) settings.assocExts = payload.exts;
  persistSettings();
  return result;
});
