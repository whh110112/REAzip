'use strict';

const { contextBridge, ipcRenderer, webUtils } = require('electron');

contextBridge.exposeInMainWorld('api', {
  openArchives: () => ipcRenderer.invoke('dialog:open-archive'),
  selectFiles: () => ipcRenderer.invoke('dialog:select-files'),
  selectFolder: () => ipcRenderer.invoke('dialog:select-folder'),
  selectDest: (defaultDir) => ipcRenderer.invoke('dialog:select-dest', defaultDir),
  saveArchive: (name, format) => ipcRenderer.invoke('dialog:save-archive', name, format),

  list: (archivePath, password, codepage) => ipcRenderer.invoke('archive:list', archivePath, password, codepage),
  extract: (job) => ipcRenderer.invoke('archive:extract', job),
  create: (job) => ipcRenderer.invoke('archive:create', job),
  deleteEntries: (archivePath, entries, password) => ipcRenderer.invoke('archive:delete', archivePath, entries, password),
  testArchive: (archivePath, password) => ipcRenderer.invoke('archive:test', archivePath, password),
  cancel: () => ipcRenderer.invoke('archive:cancel'),
  conflicts: (archivePath, dest, password) => ipcRenderer.invoke('archive:conflicts', archivePath, dest, password),
  openPath: (p) => ipcRenderer.invoke('shell:open-path', p),

  getSettings: () => ipcRenderer.invoke('settings:get'),
  setSettings: (patch) => ipcRenderer.invoke('settings:set', patch),
  setTheme: (theme) => ipcRenderer.invoke('theme:set', theme),
  showItem: (p) => ipcRenderer.invoke('shell:show-item', p),
  focusWindow: () => ipcRenderer.invoke('window:focus'),
  setMaterial: (m) => ipcRenderer.invoke('window:material', m),
  info: () => ipcRenderer.invoke('app:info'),

  listSkins: () => ipcRenderer.invoke('skins:list'),
  openSkinDir: () => ipcRenderer.invoke('skins:open-dir'),
  applyAssociations: (payload) => ipcRenderer.invoke('assoc:apply', payload),

  pathForFile: (file) => {
    try { return webUtils.getPathForFile(file); } catch (e) { return ''; }
  },

  onProgress: (cb) => {
    ipcRenderer.on('job:progress', (e, data) => cb(data));
  }
});
