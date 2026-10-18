'use strict';

/**
 * zenzip / lib/archive.js
 * 7-Zip 引擎封装：列表 / 解压 / 压缩 / 测试
 * 直接 spawn 7za，输出用 -sccUTF-8 统一为 UTF-8，便于解析中文路径。
 */

const { spawn } = require('child_process');
const path = require('path');
const fs = require('fs');
const os = require('os');

/**
 * 引擎解析：优先使用随包分发的完整版 7-Zip（支持 RAR/ISO/WIM），
 * 缺失时回退到 7zip-bin 的 7za 精简版。
 */
const ENGINE_FOLDER = {
  win32: 'win-x64',
  linux: 'linux-x64',
  darwin: 'mac'
};

let cachedBin = null;

function engineCandidates() {
  const folder = ENGINE_FOLDER[process.platform];
  const list = [];
  if (folder) {
    const exe = process.platform === 'win32' ? '7z.exe' : (process.platform === 'linux' ? '7zzs' : '7zz');
    const exeAlt = process.platform === 'linux' ? '7zz' : exe;
    list.push(path.join(__dirname, '..', 'resources', 'engine', folder, exe));
    if (exeAlt !== exe) list.push(path.join(__dirname, '..', 'resources', 'engine', folder, exeAlt));
    if (process.resourcesPath) {
      list.push(path.join(process.resourcesPath, 'engine', folder, exe));
      if (exeAlt !== exe) list.push(path.join(process.resourcesPath, 'engine', folder, exeAlt));
    }
  }
  try {
    list.push(require('7zip-bin').path7za);
  } catch (e) { /* ignore */ }
  return list;
}

function bin() {
  if (cachedBin) return cachedBin;
  for (const candidate of engineCandidates()) {
    if (candidate && fs.existsSync(candidate)) {
      if (process.platform !== 'win32') {
        try { fs.chmodSync(candidate, 0o755); } catch (e) { /* ignore */ }
      }
      cachedBin = candidate;
      return cachedBin;
    }
  }
  throw new Error('找不到可用的 7-Zip 引擎（resources/engine 或 7zip-bin）');
}

function isZip(name) {
  return /\.zip$/i.test(name) || /\.zipx$/i.test(name);
}

let currentProc = null;

function cancel() {
  if (currentProc) {
    try { currentProc.kill(); } catch (e) { /* ignore */ }
    return true;
  }
  return false;
}

/**
 * 运行 7za 并收集输出；onLine 用于逐行进度解析
 */
function runOnce(args, opts = {}) {
  const { onLine } = opts;
  return new Promise((resolve, reject) => {
    const p = spawn(bin(), args, { windowsHide: true });
    currentProc = p;
    let out = '';
    let err = '';
    let pending = '';

    p.stdout.setEncoding('utf8');
    p.stderr.setEncoding('utf8');

    p.stdout.on('data', (chunk) => {
      out += chunk;
      if (onLine) {
        pending += chunk;
        const lines = pending.split(/[\r\n\x08]+/);
        pending = lines.pop();
        lines.forEach((line) => { if (line.trim()) onLine(line); });
      }
    });
    p.stderr.on('data', (chunk) => { err += chunk; });

    p.on('error', (e) => { currentProc = null; reject(e); });
    p.on('close', (code) => {
      currentProc = null;
      if (code === 0) {
        resolve({ stdout: out, stderr: err });
      } else {
        const e = new Error(err.trim() || out.trim() || `7za 退出码 ${code}`);
        e.code = code;
        e.stdout = out;
        e.stderr = err;
        reject(e);
      }
    });
  });
}

/**
 * 解析 `7za l -slt` 输出
 * 返回 { meta: { type, physicalSize, path }, entries: [...] }
 */
function parseSlt(text) {
  const lines = text.split(/\r?\n/);
  const entries = [];
  let cur = null;
  let inEntries = false;
  const meta = {};

  for (const line of lines) {
    if (/^-{5,}/.test(line)) { inEntries = true; cur = null; continue; }

    if (!inEntries) {
      const i = line.indexOf(' = ');
      if (i > 0) {
        const k = line.slice(0, i).trim();
        const v = line.slice(i + 3).trim();
        if (k === 'Type') meta.type = v;
        if (k === 'Physical Size') meta.physicalSize = Number(v) || 0;
        if (k === 'Path') meta.path = v;
      }
      continue;
    }

    if (!line.trim()) {
      if (cur && cur.Path) entries.push(cur);
      cur = null;
      continue;
    }
    if (!cur) cur = {};
    const i = line.indexOf(' = ');
    if (i === -1) continue;
    cur[line.slice(0, i).trim()] = line.slice(i + 3);
  }
  if (cur && cur.Path) entries.push(cur);

  const mapped = [];
  for (const e of entries) {
    const isFolder = e.Folder === '+' || /[\\/]$/.test(e.Path);
    mapped.push({
      path: e.Path.replace(/\\/g, '/'),
      isFolder,
      size: Number(e.Size) || 0,
      packed: Number(e['Packed Size']) || 0,
      modified: e.Modified || '',
      encrypted: e.Encrypted === '+',
      crc: e.CRC || ''
    });
  }
  return { meta, entries: mapped };
}

async function list(archive, password = '', codepage = '') {
  const args = ['l', '-slt', '-sccUTF-8'];
  if (codepage && isZip(archive)) args.push(`-mcp=${codepage}`);
  if (password) args.push(`-p${password}`);
  args.push(archive);
  const { stdout } = await runOnce(args);
  return parseSlt(stdout);
}

/**
 * 解压整包或指定条目
 * @param {string} archive
 * @param {string} destDir
 * @param {{password?: string, entries?: string[], overwrite?: 'overwrite'|'skip'|'rename'}} opts
 */
async function extract(archive, destDir, opts = {}, onProgress) {
  const { password = '', entries = [], overwrite = 'overwrite', codepage = '' } = opts;
  fs.mkdirSync(destDir, { recursive: true });
  const args = ['x', '-y', '-sccUTF-8', '-bsp1', `-o${destDir}`];
  if (codepage && isZip(archive)) args.push(`-mcp=${codepage}`);
  if (overwrite === 'overwrite') args.push('-aoa');
  else if (overwrite === 'skip') args.push('-aos');
  else if (overwrite === 'rename') args.push('-aou');
  if (password) args.push(`-p${password}`);
  args.push(archive);
  if (entries.length) args.push(...entries);

  return runOnce(args, {
    onLine: (line) => {
      const m = line.match(/(\d{1,3})%/);
      if (m && onProgress) {
        onProgress({ percent: Math.min(100, Number(m[1])), line });
      }
    }
  });
}

/**
 * 压缩；format 为 'auto' 时交给 7za 按扩展名推断
 * @param {string} outPath
 * @param {string[]} files
 * @param {{format?: string, level?: number, volume?: string, password?: string, deleteAfter?: boolean}} opts
 */
async function create(outPath, files, opts = {}, onProgress) {
  const { format = 'zip', level = 5, volume = '', password = '', deleteAfter = false } = opts;
  if (!files || !files.length) throw new Error('没有选择要压缩的文件');

  const args = ['a', '-y', '-sccUTF-8', '-bsp1'];
  if (format && format !== 'auto') args.push(`-t${format}`);
  if (level !== undefined && level !== null) args.push(`-mx${level}`);
  if (volume) args.push(`-v${volume}`);
  if (password) args.push(`-p${password}`);
  if (deleteAfter) args.push('-sdel');
  args.push(outPath, ...files);

  return runOnce(args, {
    onLine: (line) => {
      const m = line.match(/(\d{1,3})%/);
      if (m && onProgress) onProgress({ percent: Math.min(100, Number(m[1])), line });
    }
  });
}

/** tar.gz 需要两段：系统自带 tar.exe 一步到位（Win10+） */
async function createTgz(outPath, files, onProgress) {
  const args = ['-czf', outPath, ...files];
  return new Promise((resolve, reject) => {
    const p = spawn('tar.exe', args, { windowsHide: true });
    currentProc = p;
    let err = '';
    p.stderr.setEncoding('utf8');
    p.stderr.on('data', (d) => { err += d; });
    p.on('error', reject);
    p.on('close', (code) => {
      currentProc = null;
      if (code === 0) { if (onProgress) onProgress({ percent: 100 }); resolve({}); }
      else reject(new Error(err || `tar 退出码 ${code}`));
    });
  });
}

/** 删除压缩包内条目（7z d） */
async function deleteEntries(archive, entries, password = '') {
  if (!entries || !entries.length) throw new Error('没有选择要删除的条目');
  const args = ['d', '-y', '-sccUTF-8'];
  if (password) args.push(`-p${password}`);
  args.push(archive, ...entries);
  return runOnce(args);
}

async function test(archive, password = '') {
  const args = ['t', '-sccUTF-8'];
  if (password) args.push(`-p${password}`);
  args.push(archive);
  const { stdout } = await runOnce(args);
  return { ok: true, detail: stdout };
}

module.exports = { list, extract, create, createTgz, test, cancel, parseSlt, bin, deleteEntries };
