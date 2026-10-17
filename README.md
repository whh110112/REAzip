# REAzip

现代化开源归档管理器 —— 基于 Bandizip 7.47 逆向分析重构的界面与交互（MIT 许可，独立实现，不含任何 Bandizip 代码或资源）。

**技术栈**：Electron 44 + 官方 7-Zip 26.04 全功能引擎 ｜ **平台**：Windows / macOS / Linux

---

## 功能

| 模块 | 能力 |
|---|---|
| 打开 / 浏览 | 拖放或对话框打开，多标签、面包屑、目录双击进入、包内搜索 |
| 解压 | 整包或**选中条目**解压、智能解压、冲突策略（询问/覆盖/跳过/重命名）、密码、完成后打开目录 |
| 压缩 | ZIP / 7Z / TAR / TAR.GZ，级别 0-9、分卷、密码、压缩后删源 |
| 包内编辑 | 添加文件、**删除选中条目** |
| 完整性 | 7z 测试 |
| 旧包兼容 | **文件名编码切换**（自动 / GBK / Big5 / Shift-JIS / UTF-8） |
| 外观 | 6 套内置皮肤 + 自定义皮肤接口 |
| 语言 | 18 种界面语言 |
| 系统集成 | 关联格式选择、一键设为默认压缩软件（Win 提权 / mac duti / Linux xdg-mime） |
| 其他 | 进度与取消、密码错误重试、快捷键（Ctrl+O/N/E/K/,、Delete、Esc） |

**可读格式**（7-Zip 26.04）：7Z / ZIP / ZIPX / RAR / RAR5 / ISO / UDF / WIM / MSI / NSIS / CAB / TAR / GZ / BZ2 / XZ / LZMA / 001 等 20+。
**可写格式**：ZIP / 7Z / TAR / TAR.GZ。

## 皮肤

内置：亮色轻氧、睡眠模式、macOS 风格、macOS 深色、毛玻璃（Windows 11 亚克力）、Material Design。

### 自定义皮肤接口

设置 → 外观皮肤 → 打开目录，放入任意 `*.json`：

```json
{
  "id": "my-skin",
  "name": "我的皮肤",
  "base": "light",
  "author": "your-name",
  "vars": {
    "--bg": "#ffffff",
    "--surface": "#f2f2f7",
    "--card": "#ffffff",
    "--border": "#d9d9de",
    "--text": "#1d1d1f",
    "--muted": "#6e6e73",
    "--accent": "#0a84ff",
    "--accent-strong": "#0051d5",
    "--accent-soft": "#e8f2ff"
  },
  "css": "/* 可选：追加 CSS */"
}
```

可用变量：`--bg --surface --surface-2 --card --border --border-strong --text --muted --hint --accent --accent-strong --accent-soft --accent-border --ok --danger --overlay --shadow`。
`base: dark` 时标题栏与深色基础样式会自动跟随。皮肤保存后点「重新扫描」即可出现在列表。

## 语言

简体中文、繁體中文、English、日本語、한국어、Français、Deutsch、Español、Português (Brasil)、Русский、Italiano、Polski、Türkçe、Tiếng Việt、ไทย、Bahasa Indonesia、العربية、हिन्दी。

语言包位于 `renderer/locales-*.js`，欢迎在 GitHub 提交修正。

## 运行

```bash
npm install
npm start
```

> 若 shell 设置了 `ELECTRON_RUN_AS_NODE=1`（部分沙箱/CI 注入），先执行 `$env:ELECTRON_RUN_AS_NODE=$null`。

## 构建

```bash
npm run dist:win     # NSIS 安装包 (x64/arm64) + 便携版 (x64)
npm run dist:mac     # DMG + ZIP (x64/arm64)
npm run dist:linux   # AppImage (x64/arm64) + deb + tar.gz
```

推送 `v*` 标签会触发 GitHub Actions 三平台矩阵构建，并自动发布 Release（资产命名：`REAzip-Setup-<ver>-<arch>.exe`、`REAzip-<ver>-mac-<arch>.dmg`、`REAzip-<ver>-linux-<arch>.AppImage` 等）。

```bash
git tag v0.2.0 && git push origin v0.2.0
```

## 目录

```
reazip/
├── main.js               # 主进程：窗口、IPC、设置
├── preload.js            # 安全桥接
├── lib/archive.js        # 7-Zip 引擎封装（列表/解压/压缩/删除/测试）
├── lib/assoc.js          # 文件关联与默认应用（Win/mac/Linux）
├── renderer/
│   ├── index.html        # 主窗口 + 弹窗
│   ├── styles.css        # 设计令牌 + 皮肤增强
│   ├── skins.js          # 皮肤接口
│   ├── i18n.js           # 国际化
│   ├── locales-*.js      # 18 种语言包
│   └── app.js            # 界面逻辑
├── resources/engine/     # 官方 7-Zip 26.04 二进制（win-x64 / linux-x64 / mac）
└── .github/workflows/    # 多平台构建发布
```

## 已知限制

- SFX 自解压创建未实现（设置中标注 v0.3）
- 右键菜单尚未注册（当前通过「设为默认」+ 文件关联生效）
- 密码管理器、压缩包修复、图片预览未实现
- Windows 10/11 限制程序自动修改默认应用，会打开「默认应用」页由用户确认
- macOS 需安装 `duti` 才能静默设置默认应用

## 许可

MIT。第三方组件声明见 `LICENSE`。Bandizip 为其权利人商标，本项目与其无关联。
