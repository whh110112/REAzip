'use strict';

/**
 * 文件关联与默认应用
 * Windows: 生成 PowerShell 脚本并以管理员权限运行（HKLM 注册）
 * macOS:   duti（LaunchServices）
 * Linux:   xdg-mime + desktop 文件
 */

const { execFile, spawn } = require('child_process');
const fs = require('fs');
const os = require('os');
const path = require('path');

const PROGID = 'REAzip.Archive';
const APP_NAME = 'REAzip';

const MIME = {
  zip: 'application/zip',
  '7z': 'application/x-7z-compressed',
  rar: 'application/vnd.rar',
  tar: 'application/x-tar',
  gz: 'application/gzip',
  tgz: 'application/gzip',
  bz2: 'application/x-bzip2',
  xz: 'application/x-xz',
  cab: 'application/vnd.ms-cab-compressed',
  iso: 'application/x-iso9660-image',
  lzh: 'application/x-lzh-compressed',
  arj: 'application/x-arj',
  jar: 'application/java-archive',
  apk: 'application/vnd.android.package-archive',
  deb: 'application/vnd.debian.binary-package',
  rpm: 'application/x-rpm',
  wim: 'application/x-ms-wim',
  msi: 'application/x-msi',
  '001': 'application/x-split-archive'
};

const DEFAULT_EXTS = ['zip', '7z', 'rar', 'tar', 'gz', 'tgz', 'bz2', 'xz', 'cab', 'iso', 'lzh', 'arj'];

function shellCommandFor(exePath, appPath, isPackaged) {
  const exe = `"${exePath}"`;
  if (isPackaged) return `${exe} "%1"`;
  return `${exe} "${appPath}" "%1"`;
}

function runPowerShellElevated(scriptPath) {
  return new Promise((resolve) => {
    const args = [
      '-NoProfile',
      '-Command',
      `Start-Process -FilePath powershell.exe -Verb RunAs -Wait -ArgumentList '-NoProfile','-ExecutionPolicy','Bypass','-File','${scriptPath.replace(/'/g, "''")}'`
    ];
    const p = spawn('powershell.exe', args, { windowsHide: true });
    let err = '';
    p.stderr.on('data', (d) => { err += d; });
    p.on('close', (code) => resolve({ code, err }));
    p.on('error', (e) => resolve({ code: -1, err: e.message }));
  });
}

async function applyWindows({ exts, setDefault, exePath, appPath, isPackaged }) {
  const regLines = [];
  regLines.push('$ErrorActionPreference = "Stop"');
  regLines.push('$base = "Registry::HKEY_LOCAL_MACHINE\\Software\\Classes"');

  for (const ext of exts) {
    regLines.push(`New-Item -Path "$base\\.${ext}" -Force | Out-Null`);
    regLines.push(`Set-ItemProperty -Path "$base\\.${ext}" -Name "(Default)" -Value "${PROGID}" -Force`);
    regLines.push(`New-Item -Path "$base\\${PROGID}" -Force | Out-Null`);
  }

  const cmd = shellCommandFor(exePath, appPath, isPackaged);
  regLines.push(`New-Item -Path "$base\\${PROGID}\\DefaultIcon" -Force | Out-Null`);
  regLines.push(`Set-ItemProperty -Path "$base\\${PROGID}\\DefaultIcon" -Name "(Default)" -Value '"' + exePath + ',0"' -Force`);
  regLines.push(`New-Item -Path "$base\\${PROGID}\\shell\\open\\command" -Force | Out-Null`);
  regLines.push(`Set-ItemProperty -Path "$base\\${PROGID}\\shell\\open\\command" -Name "(Default)" -Value '"' + cmd.replace(/"/g, '""') + '"' -Force`);

  // 默认程序注册（设置 → 默认应用 中的候选）
  regLines.push('$cap = "Registry::HKEY_LOCAL_MACHINE\\Software\\REAzip\\Capabilities"');
  regLines.push('New-Item -Path $cap -Force | Out-Null');
  regLines.push('Set-ItemProperty -Path $cap -Name "ApplicationName" -Value "REAzip" -Force');
  regLines.push(`Set-ItemProperty -Path $cap -Name "ApplicationDescription" -Value "Modern open-source archive manager" -Force`);
  regLines.push('New-Item -Path "$cap\\FileAssociations" -Force | Out-Null');
  for (const ext of exts) {
    regLines.push(`Set-ItemProperty -Path "$cap\\FileAssociations" -Name ".${ext}" -Value "${PROGID}" -Force`);
  }
  regLines.push('New-Item -Path "Registry::HKEY_LOCAL_MACHINE\\Software\\RegisteredApplications" -Force | Out-Null');
  regLines.push('Set-ItemProperty -Path "Registry::HKEY_LOCAL_MACHINE\\Software\\RegisteredApplications" -Name "REAzip" -Value "Software\\REAzip\\Capabilities" -Force');

  if (setDefault) {
    regLines.push('Start-Process "ms-settings:defaultapps"');
  }
  regLines.push('Write-Output "OK"');

  const scriptPath = path.join(os.tmpdir(), `reazip-assoc-${Date.now()}.ps1`);
  fs.writeFileSync(scriptPath, '\uFEFF' + regLines.join('\r\n'), 'utf8');

  const r = await runPowerShellElevated(scriptPath);
  try { fs.unlinkSync(scriptPath); } catch (e) { /* ignore */ }

  if (r.code !== 0) {
    return {
      ok: false,
      message: r.err && r.err.trim()
        ? r.err.trim()
        : '需要管理员权限（UAC 被取消或脚本执行失败）'
    };
  }
  return {
    ok: true,
    message: setDefault
      ? '已在系统中注册，并已打开「默认应用」设置页，请选择 REAzip'
      : '已注册所选格式（右键“打开方式”可见）'
  };
}

async function applyMac({ exts }) {
  const bundleId = 'com.reazip.app';
  let dutiOk = true;
  for (const ext of exts) {
    await new Promise((resolve) => {
      execFile('duti', ['-s', bundleId, ext, 'all'], (err) => {
        if (err) dutiOk = false;
        resolve();
      });
    });
  }
  if (!dutiOk) {
    spawn('open', ['x-apple.systempreferences:com.apple.preference.extensions'], { detached: true });
    return { ok: false, message: '未找到 duti。请在「系统设置 → 扩展 → 文件类型」中手动指定 REAzip。' };
  }
  return { ok: true, message: '已通过 LaunchServices 设为默认（可能需要注销后生效）' };
}

async function applyLinux({ exts, exePath }) {
  const appsDir = path.join(os.homedir(), '.local', 'share', 'applications');
  fs.mkdirSync(appsDir, { recursive: true });
  const desktop = path.join(appsDir, 'reazip.desktop');
  const mimes = exts.map((e) => MIME[e]).filter(Boolean).join(';');
  const content = [
    '[Desktop Entry]',
    'Type=Application',
    'Name=REAzip',
    'Comment=Modern open-source archive manager',
    `Exec="${exePath}" %f`,
    'Terminal=false',
    'Categories=Utility;Archiving;',
    `MimeType=${mimes};`,
    ''
  ].join('\n');
  fs.writeFileSync(desktop, content, 'utf8');
  try { fs.chmodSync(desktop, 0o755); } catch (e) { /* ignore */ }

  let ok = true;
  for (const ext of exts) {
    const mime = MIME[ext];
    if (!mime) continue;
    await new Promise((resolve) => {
      execFile('xdg-mime', ['default', 'reazip.desktop', mime], (err) => {
        if (err) ok = false;
        resolve();
      });
    });
  }
  return {
    ok,
    message: ok ? '已通过 xdg-mime 设为默认' : 'xdg-mime 执行失败，请检查桌面环境是否提供 xdg-utils'
  };
}

async function applyAssociations(opts) {
  const { exts = DEFAULT_EXTS, setDefault = true, exePath, appPath, isPackaged } = opts;
  const list = exts.filter((e) => MIME[e] || e);
  if (process.platform === 'win32') return applyWindows({ exts: list, setDefault, exePath, appPath, isPackaged });
  if (process.platform === 'darwin') return applyMac({ exts: list, setDefault });
  return applyLinux({ exts: list, exePath });
}

module.exports = { applyAssociations, DEFAULT_EXTS, MIME };
