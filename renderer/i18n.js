'use strict';

/**
 * ZENzip i18n
 * 语言包位于 locales-*.js，通过 window.LOCALES 注册。
 * 用法：data-i18n="key" / data-i18n-ph="key"，或 t('key')。
 */

const LOCALES = window.LOCALES || {};
const LANG_LABELS = [
  ['zh-CN', '简体中文'], ['zh-TW', '繁體中文'], ['en', 'English'], ['ja', '日本語'],
  ['ko', '한국어'], ['fr', 'Français'], ['de', 'Deutsch'], ['es', 'Español'],
  ['pt-BR', 'Português (Brasil)'], ['ru', 'Русский'], ['it', 'Italiano'], ['pl', 'Polski'],
  ['tr', 'Türkçe'], ['vi', 'Tiếng Việt'], ['th', 'ไทย'], ['id', 'Bahasa Indonesia'],
  ['ar', 'العربية'], ['hi', 'हिन्दी']
];

let current = 'zh-CN';

function t(key, fallback) {
  const dict = LOCALES[current] || {};
  if (dict[key] !== undefined) return dict[key];
  if (LOCALES.en && LOCALES.en[key] !== undefined) return LOCALES.en[key];
  return fallback || key;
}

function applyLanguage(lang) {
  if (!LOCALES[lang]) lang = LOCALES['zh-CN'] ? 'zh-CN' : 'en';
  current = lang;
  document.documentElement.lang = lang;
  document.documentElement.dir = lang === 'ar' ? 'rtl' : 'ltr';

  document.querySelectorAll('[data-i18n]').forEach((el) => {
    el.textContent = t(el.dataset.i18n);
  });
  document.querySelectorAll('[data-i18n-ph]').forEach((el) => {
    el.placeholder = t(el.dataset.i18nPh);
  });

  const sel = document.getElementById('s-language');
  if (sel) sel.value = lang;
  return lang;
}

function languageList() {
  return LANG_LABELS.filter(([code]) => LOCALES[code]);
}

window.ZENzipI18n = { t, applyLanguage, languageList, has: (l) => !!LOCALES[l], get current() { return current; } };
