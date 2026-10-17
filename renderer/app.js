'use strict';

const $ = (s) => document.querySelector(s);
const $$ = (s) => [...document.querySelectorAll(s)];
const T = (k, f) => (window.REAzipI18n ? REAzipI18n.t(k, f) : (f || k));

const ASSOC_ALL = ['zip', '7z', 'rar', 'tar', 'gz', 'tgz', 'bz2', 'xz', 'cab', 'iso', 'lzh', 'arj', 'jar', 'apk', 'deb', 'rpm', 'wim', 'msi', '001'];

const S = {
  tabs: [],
  activeId: null,
  settings: {},
  search: '',
  selected: [],
  cSources: [],
  cFormat: 'zip',
  cVolume: '',
  pwCallback: null
};
let tabSeq = 0;

/* ---------- 工具 ---------- */
const fmtSize = (n) => {
  if (n === undefined || n === null) return '—';
  if (n < 1024) return n + ' B';
  const units = ['KB', 'MB', 'GB', 'TB'];
  let v = n, i = -1;
  while (v >= 1024 && i < units.length - 1) { v /= 1024; i++; }
  return (v >= 100 ? v.toFixed(0) : v.toFixed(1)) + ' ' + units[i];
};
const baseName = (p) => p.replace(/[\\/]+$/, '').split(/[\\/]/).pop() || p;
const dirName = (p) => {
  const s = p.replace(/[\\/]+$/, '');
  const i = Math.max(s.lastIndexOf('\\'), s.lastIndexOf('/'));
  return i > 0 ? s.slice(0, i) : '';
};
const stripExt = (name) => name.replace(/\.(tar\.gz|tgz|zip|7z|rar|tar|gz)$/i, '');
const join = (a, b) => (a || '').replace(/[\\/]+$/, '') + '\\' + (b || '').replace(/^[\\/]+/, '');
const fmtDate = (d) => {
  if (!d) return '—';
  const m = d.match(/(\d{4})-(\d{2})-(\d{2})[ T](\d{2}):(\d{2})/);
  return m ? `${m[1]}-${m[2]}-${m[3]} ${m[4]}:${m[5]}` : d;
};
const extOf = (name) => {
  const m = name.match(/\.([a-z0-9]{1,6})$/i);
  return m ? m[1].toUpperCase() : 'FILE';
};
const activeTab = () => S.tabs.find((t) => t.id === S.activeId) || null;
const escapeHtml = (s) => String(s).replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
const escapeAttr = (s) => escapeHtml(s);

/* ---------- Toast ---------- */
let toastTimer = null;
function toast(msg, isErr = false) {
  const el = $('#toast');
  el.textContent = msg;
  el.style.background = isErr ? 'var(--danger)' : 'var(--text)';
  el.style.color = isErr ? '#fff' : 'var(--bg)';
  el.classList.add('show');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => el.classList.remove('show'), 2800);
}

/* ---------- 弹窗 ---------- */
const openModal = (id) => $('#' + id).classList.add('open');
const closeModal = (id) => $('#' + id).classList.remove('open');
function closeTopModal() {
  const open = $$('.overlay.open');
  if (open.length) open[open.length - 1].classList.remove('open');
}

/* ---------- 打开归档 ---------- */
async function openDialogs() {
  const paths = await api.openArchives();
  for (const p of paths) await openArchive(p);
}

async function openArchive(p, password = '') {
  try {
    const { meta, entries } = await api.list(p, password, S.settings.codepage || '');
    addTab(p, meta, entries, password);
  } catch (e) {
    const msg = String((e && e.message) || e);
    if (/password|encrypt/i.test(msg)) {
      askPassword(baseName(p), async (pw) => { await openArchive(p, pw); });
    } else {
      toast(T('openFailed') + '：' + msg.split('\n')[0], true);
    }
  }
}

function addTab(p, meta, entries, password = '') {
  const exist = S.tabs.find((t) => t.path === p);
  if (exist) { S.activeId = exist.id; S.selected = []; render(); return; }
  const tab = { id: ++tabSeq, path: p, name: baseName(p), meta: meta || {}, entries: entries || [], folder: '', password };
  S.tabs.push(tab);
  S.activeId = tab.id;
  S.selected = [];
  render();
}

function closeTab(id) {
  const i = S.tabs.findIndex((t) => t.id === id);
  if (i === -1) return;
  S.tabs.splice(i, 1);
  if (S.activeId === id) S.activeId = S.tabs.length ? S.tabs[Math.max(0, i - 1)].id : null;
  S.selected = [];
  render();
}

/* ---------- 渲染 ---------- */
function render() {
  const t = activeTab();
  document.title = t ? `${t.name} — REAzip` : 'REAzip';
  $('#win-title').textContent = t ? t.name : 'REAzip';
  renderTabs();
  renderCrumbs(t);
  renderTable(t);
  renderSide(t);
  renderStatus(t);
  updateToolbar(t);
}

function renderTabs() {
  $('#tabs').innerHTML = S.tabs.map((t) => `
    <div class="tab ${t.id === S.activeId ? 'active' : ''}" data-id="${t.id}" title="${escapeAttr(t.path)}">
      <span class="tab-name">${escapeHtml(t.name)}</span>
      <button class="tab-x" data-close-id="${t.id}">×</button>
    </div>`).join('') + `<button class="tab-add" id="tab-add" title="Ctrl+O">+</button>`;
}

function renderCrumbs(t) {
  const el = $('#crumbs');
  if (!t) { el.textContent = ''; return; }
  const parts = [`<button class="crumb ${t.folder ? '' : 'current'}" data-folder="">${escapeHtml(t.name)}</button>`];
  if (t.folder) {
    const segs = t.folder.split('/').filter(Boolean);
    segs.forEach((seg, i) => {
      const p = segs.slice(0, i + 1).join('/');
      const isLast = i === segs.length - 1;
      parts.push('<span class="crumb-sep">/</span>');
      parts.push(`<button class="crumb ${isLast ? 'current' : ''}" data-folder="${escapeAttr(p)}">${escapeHtml(seg)}</button>`);
    });
  }
  el.innerHTML = parts.join('');
}

function childrenOf(entries, folder) {
  const prefix = folder ? folder.replace(/\/+$/, '') + '/' : '';
  const map = new Map();
  for (const e of entries) {
    if (!e.path) continue;
    if (prefix && !e.path.startsWith(prefix)) continue;
    const rest = e.path.slice(prefix.length);
    if (!rest) continue;
    const seg = rest.split('/')[0];
    if (!seg) continue;
    const isDir = rest.includes('/') || e.isFolder;
    if (!map.has(seg)) map.set(seg, { name: seg, isFolder: isDir, size: 0, packed: 0, modified: '', count: 0 });
    const it = map.get(seg);
    if (isDir) it.count++;
    else { it.size = e.size; it.packed = e.packed; it.modified = e.modified; it.count = 1; }
  }
  let list = [...map.values()];
  if (S.search) {
    const q = S.search.toLowerCase();
    list = list.filter((x) => x.name.toLowerCase().includes(q));
  }
  return list.sort((a, b) => (b.isFolder - a.isFolder) || a.name.localeCompare(b.name, 'zh-CN'));
}

const fullPathOf = (t, name) => (t.folder ? `${t.folder}/${name}` : name);

function renderTable(t) {
  const table = $('#table');
  const empty = $('#empty');
  if (!t) {
    table.classList.remove('visible');
    table.innerHTML = '';
    empty.classList.remove('hidden');
    return;
  }
  empty.classList.add('hidden');
  table.classList.add('visible');

  const rows = childrenOf(t.entries, t.folder);
  const head = `<div class="thead">
      <span>${T('name')}</span><span class="c-r">${T('size')}</span><span class="c-r">${T('packed')}</span><span>${T('type')}</span><span>${T('modified')}</span>
    </div>`;
  const body = rows.length ? rows.map((r) => {
    const fp = fullPathOf(t, r.name);
    const sel = S.selected.includes(fp);
    return `<div class="trow ${r.isFolder ? 'folder' : ''} ${sel ? 'selected' : ''}" data-path="${escapeAttr(fp)}" data-dir="${r.isFolder ? '1' : '0'}">
      <span class="c-name">
        <span class="file-badge ${r.isFolder ? 'dir' : ''}">${r.isFolder ? 'DIR' : escapeHtml(extOf(r.name))}</span>
        <span class="nm">${escapeHtml(r.name)}</span>
      </span>
      <span class="c-r">${r.isFolder ? '—' : fmtSize(r.size)}</span>
      <span class="c-r ${r.isFolder ? '' : 'hl'}">${r.isFolder ? '—' : fmtSize(r.packed)}</span>
      <span>${r.isFolder ? r.count + ' ' + T('folders') : T('name') === 'Name' ? 'File' : '文件'}</span>
      <span>${r.isFolder ? '—' : fmtDate(r.modified)}</span>
    </div>`;
  }).join('')
    : `<div class="trow"><span style="grid-column:1/-1;color:var(--hint)">—</span></div>`;

  table.innerHTML = head + body;
  updateToolbar(t);
}

function renderSide(t) {
  const side = $('#side');
  if (!t) { side.innerHTML = ''; return; }
  const files = t.entries.filter((e) => !e.isFolder);
  const folders = t.entries.filter((e) => e.isFolder);
  const raw = files.reduce((a, e) => a + (e.size || 0), 0);
  const packed = files.reduce((a, e) => a + (e.packed || 0), 0);
  const ratio = raw > 0 ? Math.max(0, Math.round((1 - packed / raw) * 100)) : 0;
  const enc = t.entries.some((e) => e.encrypted);
  side.innerHTML = `
    <h4>${t.meta.type ? String(t.meta.type).toUpperCase() : '—'}</h4>
    <div class="kv"><span>${T('size')}</span><b>${fmtSize(raw)}</b></div>
    <div class="kv"><span>${T('packed')}</span><b>${fmtSize(t.meta.physicalSize)}</b></div>
    <div class="kv"><span>Ratio</span><b>${ratio}%</b></div>
    <div class="kv"><span>${T('files')}</span><b>${files.length}</b></div>
    <div class="kv"><span>${T('folders')}</span><b>${folders.length}</b></div>
    <div class="kv"><span>AES</span><b>${enc ? '✓' : '—'}</b></div>
    <button class="btn" id="side-test">${T('test')}</button>`;
}

function renderStatus(t) {
  if (!t) {
    $('#st-files').textContent = '';
    $('#st-folders').textContent = '';
    $('#st-size').textContent = '';
    setStatus(T('ready'));
    return;
  }
  const files = t.entries.filter((e) => !e.isFolder).length;
  const folders = t.entries.filter((e) => e.isFolder).length;
  $('#st-files').textContent = `${files} ${T('files')}`;
  $('#st-folders').textContent = `${folders} ${T('folders')}`;
  $('#st-size').textContent = fmtSize(t.meta.physicalSize);
  setStatus(T('ready'));
}

function setStatus(msg, isErr = false) {
  const el = $('#st-msg');
  el.textContent = msg;
  el.className = isErr ? 'err' : 'ok';
}

function updateToolbar(t) {
  $('#btn-extract').disabled = !t;
  $('#btn-test').disabled = !t;
  $('#btn-add').disabled = !t;
  $('#btn-del').disabled = !t || !S.selected.length;
}

/* ---------- 选中 ---------- */
function onRowClick(e, row) {
  const path = row.dataset.path;
  if (!path) return;
  if (e.ctrlKey || e.metaKey) {
    const i = S.selected.indexOf(path);
    if (i === -1) S.selected.push(path); else S.selected.splice(i, 1);
  } else {
    S.selected = (S.selected.length === 1 && S.selected[0] === path) ? [] : [path];
  }
  $$('.trow').forEach((r) => r.classList.toggle('selected', S.selected.includes(r.dataset.path)));
  updateToolbar(activeTab());
}

function selectionEntries() {
  const t = activeTab();
  return S.selected.map((p) => {
    const entry = t ? t.entries.find((e) => e.path === p) : null;
    const isFolder = entry ? entry.isFolder : false;
    return isFolder ? `${p}/*` : p;
  });
}

/* ---------- 解压 ---------- */
function openExtractModal() {
  const t = activeTab();
  if (!t) return;
  $('#x-dest').value = S.settings.extractDir || dirName(t.path);
  $('#x-smart').checked = S.settings.smartExtract !== false;
  $('#x-password').value = t.password || '';
  $('#x-title').textContent = `${T('extract')} ${t.name}`;
  updateExtractHint();
  openModal('modal-extract');
}

function updateExtractHint() {
  const t = activeTab();
  if (!t) return;
  const selected = S.selected.length;
  const count = selected || t.entries.filter((e) => !e.isFolder).length;
  const smart = $('#x-smart').checked;
  const dest = smart ? join($('#x-dest').value, stripExt(t.name)) : $('#x-dest').value;
  $('#x-hint').textContent = `${count} ${T('files')} · ${fmtSize(t.meta.physicalSize)} → ${dest}`;
}

async function startExtract() {
  const t = activeTab();
  if (!t) return;
  const base = $('#x-dest').value.trim();
  if (!base) return toast(T('extractFailed'), true);
  const smart = $('#x-smart').checked;
  const dest = smart ? join(base, stripExt(t.name)) : base;
  const ow = $('#x-overwrite button.on')?.dataset.ow || 'ask';
  const password = $('#x-password').value;
  const openAfter = $('#x-open').checked;
  const entries = selectionEntries();
  closeModal('modal-extract');

  let overwrite = ow;
  if (ow === 'ask' && !entries.length) {
    const n = await api.conflicts(t.path, dest, t.password);
    if (n > 0) {
      const choice = await askConflict(n);
      if (!choice) return;
      overwrite = choice;
    } else overwrite = 'overwrite';
  } else if (ow === 'ask') {
    overwrite = 'overwrite';
  }

  await runExtract(t, dest, overwrite, password, openAfter, entries);
}

async function runExtract(t, dest, overwrite, password, openAfter, entries) {
  showProgress(T('extracting'));
  setStatus(T('extracting'));
  try {
    await api.extract({ archive: t.path, dest, overwrite, password, entries, codepage: S.settings.codepage || '' });
    setProgress(100);
    closeModal('modal-progress');
    setStatus(T('extractDone'));
    toast(T('extractDone'));
    if (openAfter) api.openPath(dest);
  } catch (e) {
    closeModal('modal-progress');
    const msg = String((e && e.message) || e);
    if (/password|encrypt/i.test(msg)) {
      setStatus(T('needPassword'), true);
      askPassword(t.name, (pw) => runExtract(t, dest, overwrite, pw, openAfter, entries));
    } else {
      setStatus(T('extractFailed'), true);
      toast(T('extractFailed') + '：' + msg.split('\n')[0], true);
    }
  }
}

/* ---------- 删除 ---------- */
async function deleteSelected() {
  const t = activeTab();
  if (!t || !S.selected.length) return toast(T('noSelection'), true);
  const ok = await confirmDialog(T('delete'), T('deleteConfirm'));
  if (!ok) return;
  const entries = S.selected.slice();
  showProgress(T('loading'));
  try {
    await api.deleteEntries(t.path, entries, t.password);
    closeModal('modal-progress');
    const { meta, entries: list } = await api.list(t.path, t.password, S.settings.codepage || '');
    t.meta = meta;
    t.entries = list;
    S.selected = [];
    render();
    toast(T('deleted'));
  } catch (e) {
    closeModal('modal-progress');
    toast(T('deleteFailed') + '：' + String(e.message).split('\n')[0], true);
  }
}

/* ---------- 压缩 ---------- */
function openCompressModal() {
  S.cSources = [];
  S.cFormat = 'zip';
  S.cVolume = '';
  $('#c-name').value = 'archive.zip';
  $('#c-dest').value = S.settings.archiveDir || '';
  $('#c-level').value = 5;
  $('#c-level-label').textContent = '5';
  $('#c-password').value = '';
  $('#c-del').checked = false;
  $('#c-volume-custom').style.display = 'none';
  $$('#c-format button').forEach((b) => b.classList.toggle('on', b.dataset.fmt === 'zip'));
  $$('#c-volume button').forEach((b) => b.classList.toggle('on', b.dataset.vol === ''));
  renderChips();
  openModal('modal-compress');
}

function renderChips() {
  const list = $('#c-files');
  if (!S.cSources.length) {
    list.innerHTML = `<div class="file-empty">${T('noFiles')}</div>`;
    return;
  }
  list.innerHTML = S.cSources.map((p, i) => `
    <div class="file-chip"><span class="p" title="${escapeAttr(p)}">${escapeHtml(p)}</span><button class="rm" data-i="${i}">×</button></div>`).join('');
}

function ensureExt(name, fmt) {
  const ext = { zip: '.zip', '7z': '.7z', tar: '.tar', tgz: '.tar.gz' }[fmt] || '.zip';
  const lower = name.toLowerCase();
  if (fmt === 'tgz' && (lower.endsWith('.tar.gz') || lower.endsWith('.tgz'))) return name;
  if (lower.endsWith(ext)) return name;
  return stripExt(name) + ext;
}

async function startCompress() {
  if (!S.cSources.length) return toast(T('noFilesSelected'), true);
  const dest = $('#c-dest').value.trim();
  if (!dest) return toast(T('compressFailed'), true);
  const name = ensureExt($('#c-name').value.trim() || 'archive', S.cFormat);
  const out = join(dest, name);
  const level = Number($('#c-level').value);
  let volume = S.cVolume;
  if (volume === 'custom') {
    volume = $('#c-volume-custom').value.trim();
    if (!volume) return toast(T('compressFailed'), true);
  }
  const password = $('#c-password').value;
  const deleteAfter = $('#c-del').checked;

  closeModal('modal-compress');
  showProgress(T('compressing'));
  setStatus(T('compressing'));
  try {
    await api.create({ archive: out, files: S.cSources, format: S.cFormat, level, volume, password, deleteAfter });
    setProgress(100);
    closeModal('modal-progress');
    setStatus(T('compressDone'));
    toast(T('compressDone'));
    await openArchive(out);
  } catch (e) {
    closeModal('modal-progress');
    setStatus(T('compressFailed'), true);
    toast(T('compressFailed') + '：' + String(e.message).split('\n')[0], true);
  }
}

async function addToArchive() {
  const t = activeTab();
  if (!t) return;
  const files = await api.selectFiles();
  if (!files.length) return;
  showProgress(T('loading'));
  try {
    await api.create({ archive: t.path, files, format: 'auto' });
    closeModal('modal-progress');
    const { meta, entries } = await api.list(t.path, t.password, S.settings.codepage || '');
    t.entries = entries;
    t.meta = meta;
    render();
    toast(T('compressDone'));
  } catch (e) {
    closeModal('modal-progress');
    toast(T('addFailed') + '：' + String(e.message).split('\n')[0], true);
  }
}

/* ---------- 测试 ---------- */
async function testArchive() {
  const t = activeTab();
  if (!t) return;
  setStatus(T('loading'));
  try {
    await api.testArchive(t.path, t.password);
    setStatus(T('testPassed'));
    toast(T('testPassed'));
  } catch (e) {
    setStatus(T('testFailed'), true);
    toast(T('testFailed') + '：' + String(e.message).split('\n')[0], true);
  }
}

/* ---------- 进度 ---------- */
function showProgress(title) {
  $('#p-title').textContent = title;
  setProgress(0);
  $('#p-file').textContent = T('loading');
  openModal('modal-progress');
}
function setProgress(pct) {
  const p = Math.max(0, Math.min(100, Math.round(pct || 0)));
  $('#p-pct').textContent = p + '%';
  $('#p-fill').style.width = p + '%';
}
function bindProgress() {
  api.onProgress(({ percent, line }) => {
    if (!Number.isNaN(percent)) setProgress(percent);
    if (line) $('#p-file').textContent = line.replace(/^\s*\d{1,3}%\s*/, '').trim() || T('loading');
  });
}

/* ---------- 确认 / 冲突 / 密码 ---------- */
function confirmDialog(title, msg) {
  return new Promise((resolve) => {
    $('#cf-title').textContent = title;
    $('#cf-msg').textContent = msg;
    openModal('modal-confirm');
    const done = (v) => { closeModal('modal-confirm'); resolve(v); };
    $('#cf-yes').onclick = () => done(true);
    $('#cf-no').onclick = () => done(false);
    $('#modal-confirm').querySelectorAll('[data-close]').forEach((b) => { b.onclick = () => done(false); });
  });
}

function askConflict(count) {
  return new Promise((resolve) => {
    $('#cf-text').textContent = `${count} × ${T('conflictTitle')}`;
    openModal('modal-conflict');
    const done = (v) => { closeModal('modal-conflict'); resolve(v); };
    $('#cf-overwrite').onclick = () => done('overwrite');
    $('#cf-skip').onclick = () => done('skip');
    $('#cf-rename').onclick = () => done('rename');
    $('#modal-conflict').querySelectorAll('[data-close]').forEach((b) => { b.onclick = () => done(null); });
  });
}

function askPassword(text, cb) {
  $('#pw-text').textContent = text;
  $('#pw-input').value = '';
  S.pwCallback = cb;
  openModal('modal-password');
  setTimeout(() => $('#pw-input').focus(), 50);
}

/* ---------- 设置 ---------- */
async function openSettings() {
  const info = await api.info();
  $('#s-about').textContent = `v${info.version} · Electron ${info.electron} · Node ${info.node}`;
  $('#s-engine').textContent = info.engine || '—';
  $('#s-extract-desc').textContent = S.settings.extractDir || '—';
  $('#s-archive-desc').textContent = S.settings.archiveDir || '—';
  $('#s-restore').checked = !!S.settings.restoreTabs;
  $('#s-codepage').value = S.settings.codepage || '';
  $('#s-default-desc').textContent = platformHint(info.platform);
  renderSkinGrid();
  renderAssocGrid();
  renderLanguageSelect();
  showSettingsTab('general');
  openModal('modal-settings');
}

function platformHint(platform) {
  if (platform === 'win32') return 'Windows：将以管理员权限写入注册表，并打开「默认应用」页';
  if (platform === 'darwin') return 'macOS：通过 LaunchServices（duti）注册';
  return 'Linux：通过 xdg-mime 注册';
}

function showSettingsTab(tab) {
  $$('#s-nav button').forEach((b) => b.classList.toggle('on', b.dataset.tab === tab));
  $$('.stab').forEach((el) => el.classList.toggle('hidden', el.dataset.tab !== tab));
}

function renderSkinGrid() {
  const grid = $('#skin-grid');
  const skins = window.REAzipSkins.allSkins();
  const current = S.settings.skin || 'light';
  grid.innerHTML = skins.map((s) => {
    const v = s.vars || {};
    const bg = v['--bg'] || '#fff';
    const surf = v['--surface'] || '#eee';
    const accent = v['--accent'] || '#378ADD';
    const isCustom = !window.REAzipSkins.BUILTIN_SKINS.some((b) => b.id === s.id);
    return `<button class="skin-card ${s.id === current ? 'on' : ''}" data-skin="${escapeAttr(s.id)}">
      <span class="skin-swatch">
        <i style="background:${escapeAttr(bg)}"></i>
        <i style="background:${escapeAttr(surf)}"></i>
        <i style="background:${escapeAttr(accent)}"></i>
      </span>
      <span class="skin-name">${escapeHtml(s.name)}</span>
      <span class="skin-meta">${isCustom ? 'custom' : (s.base === 'dark' ? 'dark' : 'light')}</span>
    </button>`;
  }).join('');
}

function renderLanguageSelect() {
  const sel = $('#s-language');
  sel.innerHTML = REAzipI18n.languageList().map(([code, label]) => `<option value="${code}">${label}</option>`).join('');
  sel.value = S.settings.language || 'zh-CN';
}

function renderAssocGrid() {
  const checked = new Set(S.settings.assocExts || []);
  $('#assoc-grid').innerHTML = ASSOC_ALL.map((ext) => `
    <label class="assoc-item"><input type="checkbox" data-ext="${ext}" ${checked.has(ext) ? 'checked' : ''}> .${ext}</label>`).join('');
}

async function applyDefaultApp() {
  const exts = [...document.querySelectorAll('#assoc-grid input:checked')].map((i) => i.dataset.ext);
  if (!exts.length) return toast(T('noSelection'), true);
  $('#s-assoc-hint').textContent = T('settingDefault');
  const r = await api.applyAssociations({ exts, setDefault: true });
  $('#s-assoc-hint').textContent = r.message;
  toast(r.ok ? T('defaultApplied') : T('defaultFailed'), !r.ok);
}

/* ---------- 初始化 ---------- */
async function init() {
  const settings = await api.getSettings();
  S.settings = settings || {};

  try {
    const custom = await api.listSkins();
    window.REAzipSkins.setCustomSkins(custom);
  } catch (e) { /* ignore */ }

  REAzipI18n.applyLanguage(S.settings.language || 'zh-CN');
  window.REAzipSkins.applySkin(S.settings.skin || 'light');

  bindProgress();

  $('#tabs').addEventListener('click', (e) => {
    const x = e.target.closest('.tab-x');
    if (x) { closeTab(Number(x.dataset.closeId)); return; }
    const tab = e.target.closest('.tab');
    if (tab) {
      S.activeId = Number(tab.dataset.id);
      S.search = ''; $('#search').value = '';
      S.selected = [];
      render();
      return;
    }
    if (e.target.closest('#tab-add')) openDialogs();
  });

  $('#crumbs').addEventListener('click', (e) => {
    const b = e.target.closest('.crumb');
    const t = activeTab();
    if (!b || !t) return;
    t.folder = b.dataset.folder || '';
    S.selected = [];
    render();
  });

  $('#table').addEventListener('click', (e) => {
    const row = e.target.closest('.trow');
    if (row && row.dataset.path) onRowClick(e, row);
  });
  $('#table').addEventListener('dblclick', (e) => {
    const row = e.target.closest('.trow');
    if (!row || row.dataset.dir !== '1') return;
    const t = activeTab();
    if (!t) return;
    const name = String(row.dataset.path).split('/').pop();
    t.folder = t.folder ? `${t.folder}/${name}` : name;
    S.selected = [];
    render();
  });

  $('#side').addEventListener('click', (e) => {
    if (e.target.id === 'side-test') testArchive();
  });

  $('#btn-new').onclick = openCompressModal;
  $('#btn-extract').onclick = openExtractModal;
  $('#btn-test').onclick = testArchive;
  $('#btn-add').onclick = addToArchive;
  $('#btn-del').onclick = deleteSelected;
  $('#btn-settings').onclick = openSettings;
  $('#btn-open-empty').onclick = openDialogs;
  $('#search').addEventListener('input', (e) => { S.search = e.target.value.trim(); renderTable(activeTab()); });

  $('#c-browse').onclick = async () => {
    const d = await api.selectDest($('#c-dest').value);
    if (d) $('#c-dest').value = d;
  };
  $('#c-add-files').onclick = async () => {
    const files = await api.selectFiles();
    if (files.length) { S.cSources.push(...files); renderChips(); }
  };
  $('#c-add-folder').onclick = async () => {
    const d = await api.selectFolder();
    if (d) { S.cSources.push(d); renderChips(); }
  };
  $('#c-clear').onclick = () => { S.cSources = []; renderChips(); };
  $('#c-files').addEventListener('click', (e) => {
    const rm = e.target.closest('.rm');
    if (!rm) return;
    S.cSources.splice(Number(rm.dataset.i), 1);
    renderChips();
  });
  $('#c-format').addEventListener('click', (e) => {
    const b = e.target.closest('button');
    if (!b) return;
    S.cFormat = b.dataset.fmt;
    $$('#c-format button').forEach((x) => x.classList.toggle('on', x === b));
    $('#c-name').value = ensureExt($('#c-name').value, S.cFormat);
  });
  $('#c-volume').addEventListener('click', (e) => {
    const b = e.target.closest('button');
    if (!b) return;
    S.cVolume = b.dataset.vol;
    $$('#c-volume button').forEach((x) => x.classList.toggle('on', x === b));
    $('#c-volume-custom').style.display = S.cVolume === 'custom' ? 'block' : 'none';
  });
  $('#c-level').addEventListener('input', (e) => { $('#c-level-label').textContent = e.target.value; });
  $('#c-start').onclick = startCompress;

  $('#x-browse').onclick = async () => {
    const d = await api.selectDest($('#x-dest').value);
    if (d) { $('#x-dest').value = d; updateExtractHint(); }
  };
  $('#x-smart').addEventListener('change', updateExtractHint);
  $('#x-dest').addEventListener('input', updateExtractHint);
  $('#x-overwrite').addEventListener('click', (e) => {
    const b = e.target.closest('button');
    if (!b) return;
    $$('#x-overwrite button').forEach((x) => x.classList.toggle('on', x === b));
  });
  $('#x-start').onclick = startExtract;

  $('#p-cancel').onclick = () => {
    api.cancel();
    closeModal('modal-progress');
    setStatus(T('cancelled'));
  };

  $('#s-nav').addEventListener('click', (e) => {
    const b = e.target.closest('button');
    if (b) showSettingsTab(b.dataset.tab);
  });

  $('#skin-grid').addEventListener('click', async (e) => {
    const card = e.target.closest('.skin-card');
    if (!card) return;
    const id = card.dataset.skin;
    const skin = window.REAzipSkins.applySkin(id);
    S.settings.skin = id;
    await api.setSettings({ skin: id, theme: skin.base === 'dark' ? 'sleep' : 'light' });
    renderSkinGrid();
  });

  $('#s-open-skins').onclick = async () => { await api.openSkinDir(); };
  $('#s-rescan-skins').onclick = async () => {
    const custom = await api.listSkins();
    window.REAzipSkins.setCustomSkins(custom);
    renderSkinGrid();
    toast('↻');
  };

  $('#s-language').addEventListener('change', async (e) => {
    const lang = e.target.value;
    S.settings.language = lang;
    REAzipI18n.applyLanguage(lang);
    await api.setSettings({ language: lang });
    render();
    renderSkinGrid();
    renderAssocGrid();
    toast(lang);
  });

  $('#assoc-grid').addEventListener('change', async () => {
    const exts = [...document.querySelectorAll('#assoc-grid input:checked')].map((i) => i.dataset.ext);
    S.settings.assocExts = exts;
    await api.setSettings({ assocExts: exts });
  });
  $('#s-default-btn').onclick = applyDefaultApp;

  $('#s-extract-btn').onclick = async () => {
    const d = await api.selectDest(S.settings.extractDir);
    if (!d) return;
    S.settings.extractDir = d;
    $('#s-extract-desc').textContent = d;
    S.settings = await api.setSettings({ extractDir: d });
  };
  $('#s-archive-btn').onclick = async () => {
    const d = await api.selectDest(S.settings.archiveDir);
    if (!d) return;
    S.settings.archiveDir = d;
    $('#s-archive-desc').textContent = d;
    S.settings = await api.setSettings({ archiveDir: d });
  };
  $('#s-restore').addEventListener('change', async (e) => {
    S.settings = await api.setSettings({ restoreTabs: e.target.checked });
  });
  $('#s-codepage').addEventListener('change', async (e) => {
    S.settings.codepage = e.target.value;
    await api.setSettings({ codepage: e.target.value });
    const t = activeTab();
    if (!t) return;
    try {
      const { meta, entries } = await api.list(t.path, t.password, S.settings.codepage);
      t.meta = meta;
      t.entries = entries;
      render();
    } catch (err) { /* ignore */ }
  });

  $('#pw-ok').onclick = () => {
    const pw = $('#pw-input').value;
    const cb = S.pwCallback;
    closeModal('modal-password');
    if (cb) cb(pw);
  };
  $('#pw-input').addEventListener('keydown', (e) => { if (e.key === 'Enter') $('#pw-ok').click(); });

  $$('.overlay').forEach((ov) => {
    ov.addEventListener('click', (e) => { if (e.target === ov) ov.classList.remove('open'); });
    ov.querySelectorAll('[data-close]').forEach((b) => b.addEventListener('click', () => ov.classList.remove('open')));
  });

  document.addEventListener('dragover', (e) => {
    e.preventDefault();
    $('#empty')?.classList.add('dragging');
  });
  document.addEventListener('dragleave', (e) => {
    if (e.relatedTarget === null) $('#empty')?.classList.remove('dragging');
  });
  document.addEventListener('drop', async (e) => {
    e.preventDefault();
    $('#empty')?.classList.remove('dragging');
    for (const f of [...(e.dataTransfer?.files || [])]) {
      const p = api.pathForFile(f);
      if (p) await openArchive(p);
    }
  });

  document.addEventListener('keydown', (e) => {
    if (e.ctrlKey && e.key.toLowerCase() === 'o') { e.preventDefault(); openDialogs(); }
    if (e.ctrlKey && e.key.toLowerCase() === 'n') { e.preventDefault(); openCompressModal(); }
    if (e.ctrlKey && e.key.toLowerCase() === 'e') { e.preventDefault(); if (activeTab()) openExtractModal(); }
    if (e.ctrlKey && e.key.toLowerCase() === 'k') { e.preventDefault(); $('#search').focus(); }
    if (e.ctrlKey && e.key === ',') { e.preventDefault(); openSettings(); }
    if (e.key === 'Delete' && S.selected.length) { e.preventDefault(); deleteSelected(); }
    if (e.key === 'Escape') {
      if ($$('.overlay.open').length) closeTopModal();
      else if (S.search) { S.search = ''; $('#search').value = ''; renderTable(activeTab()); }
    }
  });

  if (S.settings.restoreTabs && Array.isArray(S.settings.openTabs)) {
    for (const p of S.settings.openTabs) {
      try { await openArchive(p); } catch (err) { /* 跳过 */ }
    }
  }

  render();
}

setInterval(() => {
  const paths = S.tabs.map((t) => t.path);
  if (S.settings.restoreTabs && JSON.stringify(paths) !== JSON.stringify(S.settings.openTabs || [])) {
    S.settings.openTabs = paths;
    api.setSettings({ openTabs: paths });
  }
}, 4000);

window.addEventListener('DOMContentLoaded', init);
