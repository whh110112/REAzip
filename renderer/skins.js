'use strict';

/**
 * ZENzip 皮肤接口
 * ------------------------------------------------------------
 * 内置皮肤 + 自定义皮肤（用户目录 skins/*.json）。
 *
 * 自定义皮肤 JSON 格式：
 * {
 *   "id": "my-skin",            // 唯一 id（字母数字-_）
 *   "name": "我的皮肤",          // 显示名
 *   "base": "light",            // light | dark（决定基础主题与标题栏配色）
 *   "author": "your-name",      // 可选
 *   "vars": {                   // 覆盖 CSS 变量，可只写需要的
 *     "--bg": "#ffffff",
 *     "--surface": "#f2f2f7",
 *     "--text": "#1d1d1f",
 *     "--accent": "#0a84ff"
 *   },
 *   "css": "…"                  // 可选：追加 CSS（受信本地文件，谨慎使用）
 * }
 *
 * 可用变量见 README「皮肤接口」。
 */

const SKIN_VARS = [
  '--bg', '--surface', '--surface-2', '--card', '--border', '--border-strong',
  '--text', '--muted', '--hint', '--accent', '--accent-strong', '--accent-soft',
  '--accent-border', '--ok', '--danger', '--overlay', '--shadow'
];

const BUILTIN_SKINS = [
  {
    id: 'light',
    name: '亮色轻氧',
    nameEn: 'Light Airy',
    base: 'light',
    vars: {
      '--bg': '#FFFFFF', '--surface': '#F1EFE8', '--surface-2': '#E8E6DE', '--card': '#FFFFFF',
      '--border': '#D3D1C7', '--border-strong': '#B4B2A9',
      '--text': '#2C2C2A', '--muted': '#5F5E5A', '--hint': '#888780',
      '--accent': '#378ADD', '--accent-strong': '#185FA5', '--accent-soft': '#E6F1FB', '--accent-border': '#85B7EB',
      '--ok': '#1D9E75', '--danger': '#E24B4A',
      '--overlay': 'rgba(30,30,28,.38)', '--shadow': '0 18px 40px rgba(20,20,18,.16)'
    }
  },
  {
    id: 'sleep',
    name: '睡眠模式',
    nameEn: 'Sleep',
    base: 'dark',
    vars: {
      '--bg': '#232220', '--surface': '#2C2C2A', '--surface-2': '#383835', '--card': '#2C2C2A',
      '--border': '#444441', '--border-strong': '#5F5E5A',
      '--text': '#F1EFE8', '--muted': '#B4B2A9', '--hint': '#888780',
      '--accent': '#85B7EB', '--accent-strong': '#B5D4F4', '--accent-soft': '#0C447C', '--accent-border': '#185FA5',
      '--ok': '#5DCAA5', '--danger': '#F09595',
      '--overlay': 'rgba(0,0,0,.55)', '--shadow': '0 18px 40px rgba(0,0,0,.45)'
    }
  },
  {
    id: 'mac',
    name: 'macOS 风格',
    nameEn: 'macOS',
    base: 'light',
    vars: {
      '--bg': '#FFFFFF', '--surface': '#F5F5F7', '--surface-2': '#E8E8ED', '--card': '#FFFFFF',
      '--border': '#D9D9DE', '--border-strong': '#BDBDC4',
      '--text': '#1D1D1F', '--muted': '#6E6E73', '--hint': '#86868B',
      '--accent': '#007AFF', '--accent-strong': '#0051D5', '--accent-soft': '#E8F2FF', '--accent-border': '#7DB8FF',
      '--ok': '#34C759', '--danger': '#FF3B30',
      '--overlay': 'rgba(0,0,0,.30)', '--shadow': '0 20px 50px rgba(0,0,0,.12)'
    }
  },
  {
    id: 'mac-dark',
    name: 'macOS 深色',
    nameEn: 'macOS Dark',
    base: 'dark',
    vars: {
      '--bg': '#1E1E1E', '--surface': '#2A2A2C', '--surface-2': '#3A3A3C', '--card': '#2A2A2C',
      '--border': '#3F3F42', '--border-strong': '#5A5A5E',
      '--text': '#F5F5F7', '--muted': '#A1A1A6', '--hint': '#7C7C82',
      '--accent': '#0A84FF', '--accent-strong': '#64B5FF', '--accent-soft': '#0A2A4A', '--accent-border': '#0A84FF',
      '--ok': '#30D158', '--danger': '#FF453A',
      '--overlay': 'rgba(0,0,0,.55)', '--shadow': '0 20px 50px rgba(0,0,0,.5)'
    },
    extraCss: 'body[data-skin="mac-dark"] .btn.primary{color:#fff}'
  },
  {
    id: 'glass',
    name: '毛玻璃',
    nameEn: 'Frosted Glass',
    base: 'light',
    vars: {
      '--bg': 'rgba(244,247,252,0.66)', '--surface': 'rgba(255,255,255,0.52)', '--surface-2': 'rgba(255,255,255,0.34)',
      '--card': 'rgba(255,255,255,0.72)',
      '--border': 'rgba(255,255,255,0.65)', '--border-strong': 'rgba(150,162,180,0.45)',
      '--text': '#1C2430', '--muted': '#4A5568', '--hint': '#7A8699',
      '--accent': '#3B82F6', '--accent-strong': '#1D4ED8', '--accent-soft': 'rgba(59,130,246,.16)', '--accent-border': 'rgba(59,130,246,.45)',
      '--ok': '#10B981', '--danger': '#EF4444',
      '--overlay': 'rgba(15,23,42,.35)', '--shadow': '0 24px 60px rgba(15,23,42,.18)'
    }
  },
  {
    id: 'material',
    name: 'Material Design',
    nameEn: 'Material Design',
    base: 'light',
    vars: {
      '--bg': '#FEFBFF', '--surface': '#F3EDF7', '--surface-2': '#E8DEF8', '--card': '#FFFFFF',
      '--border': '#CAC4D0', '--border-strong': '#938F99',
      '--text': '#1D1B20', '--muted': '#49454F', '--hint': '#79747E',
      '--accent': '#6750A4', '--accent-strong': '#4F378B', '--accent-soft': '#EADDFF', '--accent-border': '#CAC4D0',
      '--ok': '#146C2E', '--danger': '#B3261E',
      '--overlay': 'rgba(0,0,0,.32)', '--shadow': '0 6px 18px rgba(0,0,0,.12)'
    }
  }
];

let customSkins = [];
let currentSkin = null;

function skinById(id) {
  return BUILTIN_SKINS.concat(customSkins).find((s) => s.id === id) || BUILTIN_SKINS[0];
}

function applySkin(id) {
  const skin = skinById(id);
  currentSkin = skin;

  // 清除上一次的内联变量
  for (const v of SKIN_VARS) document.documentElement.style.removeProperty(v);
  for (const [k, v] of Object.entries(skin.vars || {})) {
    if (k.startsWith('--')) document.documentElement.style.setProperty(k, v);
  }

  document.body.dataset.skin = skin.id;
  document.body.dataset.theme = skin.base === 'dark' ? 'sleep' : 'light';

  // 额外 CSS
  let el = document.getElementById('skin-extra');
  if (!el) {
    el = document.createElement('style');
    el.id = 'skin-extra';
    document.head.appendChild(el);
  }
  el.textContent = skin.extraCss || '';

  // 原生标题栏跟随
  if (window.api && api.setTheme) api.setTheme(skin.base === 'dark' ? 'sleep' : 'light');
  // 毛玻璃使用 Windows 11 亚克力背景
  if (window.api && api.setMaterial) api.setMaterial(skin.id === 'glass' ? 'acrylic' : 'none');
  return skin;
}

function setCustomSkins(list) {
  customSkins = (list || []).filter((s) => s && s.id && s.vars && typeof s.vars === 'object');
}

function allSkins() {
  return BUILTIN_SKINS.concat(customSkins);
}

window.ZENzipSkins = { BUILTIN_SKINS, SKIN_VARS, applySkin, setCustomSkins, allSkins, skinById, get current() { return currentSkin; } };
