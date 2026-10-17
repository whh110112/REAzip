# REAzip 项目审查（2026-10-17）

审查对象：`D:\Workbuddy\2026-10-13-14-25-42\reazip`，提交 `163e17a`，版本 0.2.1。另阅读上级目录的 Bandizip 逆向分析报告。此次为审查，没有修改项目源码、标签或 Release。

## 总体结论

目前的准确定位是：参考 Bandizip 界面与部分行为的 Electron 归档工具，以原生 7-Zip 命令行程序处理压缩包。它已有可用的基础功能，但还不能视为 Bandizip 的完整功能复刻或经过三平台运行验收的稳定版本。

之前发布过程中确认的“三平台构建成功”只证明打包、上传成功，不能推导所有平台和架构的压缩、解压与文件关联都能工作。

## 为什么是 JavaScript 和 HTML

调用链：`renderer/*.html + *.css + *.js → preload.js IPC → main.js → lib/archive.js → spawn(7z.exe/7zzs/7zz) → 原生压缩引擎`。

- HTML/CSS 管界面，JavaScript 管操作编排，Electron 提供桌面窗口和文件系统桥接。
- 压缩计算并非 JavaScript 重写：实测 Windows 使用官方 7-Zip 26.04，`7z.exe` 加载随附 `7z.dll`。
- 当前仓库没有 Bandizip `ark.dll` 的重建代码，没有应用自身的 C/C++ 引擎源码或原生界面工程。
- `bandizip-re/RE-analysis.md` 第 4–5 行明确记录：本次是 PE 资源、语言文案和界面捕获，未做函数级反编译。第 134 行主动建议选择 Electron。技术栈来自这个选型，并非漏上传了 C++ 源码。
- 行为分析后的独立实现可以采用不同语言；但这与还原原软件内部实现是不同深度的工作。当前证据支持前者的早期阶段。

## 已确认问题

### 1. Windows 文件关联脚本生成错误（高优先级）

`lib/assoc.js:76`、`:78` 把 `exePath` 和 `cmd.replace(...)` 原样写进 PowerShell 文本，未在 JavaScript 中正确插值。

在模拟子进程和文件系统的环境里生成脚本，再交给 PowerShell Parser 做只读语法检查，得到字符串未终止、缺少括号错误。没有实际修改注册表或执行提权。

同时，`main.js` 没有处理 `process.argv`、macOS `open-file` 或 `second-instance`。即使修好关联注册，系统双击归档后也没有完整的“把路径交给界面打开”链路。

### 2. macOS/Linux 路径和 TGZ 创建不是跨平台实现（高优先级）

`renderer/app.js:38` 的 `join()` 固定使用反斜杠。实测 `/home/user/Downloads` 与 `sample` 拼接得到 `/home/user/Downloads\sample`，在 POSIX 上不能表示预期的子目录。

`lib/archive.js:236` 固定执行 `tar.exe`，普通 macOS/Linux 环境没有此命令。TGZ 创建分支也未传递压缩对话框中的密码、分卷、删源等选项，应按格式禁用不支持的选项并明确行为。

### 3. Linux ARM64 包选用 x64 引擎（高优先级）

`lib/archive.js:18` 只按 `process.platform` 选目录，没有按 `process.arch` 选择；Linux 始终使用 `linux-x64`。

实读随附 `7zzs` 的 ELF 头，machine=62，即 x86-64。ARM64 Linux 发布包因此没有获得匹配架构的完整引擎。现有回退仅在文件不存在时发生，不会在架构不匹配时切换。

Windows ARM64 包同样选择 x64 引擎，需要依赖系统 x64 仿真，并非原生 ARM64 压缩内核。

另已从 GitHub 下载实际 v0.2.1 Linux x64 tar.gz 检查：`resources/engine/linux-x64/7zz`、`7zzs` 的权限均为 `-rw-r--r--`，缺少执行位，Git 中也记录为 100644。代码在运行时尝试 chmod，解压到用户可写目录时可能补上权限；但只读安装位置无法依赖这种补救。AppImage 的实际运行尚未验证，应在打包前设置执行位并单独验收。该 Linux 包还同时捆绑了 macOS、Windows 引擎，存在可精简的跨平台冗余。

### 4. 加密文件头归档打开时等待输入（高优先级）

`lib/archive.js:79` 创建带 stdin 管道的 7-Zip 子进程；无密码时既不提供非交互密码，也不处理密码提示。

使用临时文件创建 `-mhe=on` 的加密 7z 后，调用现有 `lib.list()`：进程输出 `Enter password` 并等待。两秒后由检查脚本主动取消。界面的密码弹窗依赖 Promise 拒绝，因此此路径下不会及时触发。

### 5. 选中解压的“询问覆盖”会变成直接覆盖（高优先级）

`renderer/app.js:318–326` 只在整包解压时检查冲突；有选中条目且用户选择“询问”时，直接设置 `overwrite = 'overwrite'`。最终使用 `-y -aoa`，目标同名文件可能在未询问的情况下被覆盖。

### 6. ZIP 加密算法显示错误（高优先级）

`lib/archive.js:220` 仅添加 `-p`，没有 ZIP AES 配置；`renderer/app.js:226` 只要条目为 encrypted 就显示 AES 勾选。

使用现有 create() 新建带密码 ZIP，再读取原生引擎的技术列表，实测 `Method = ZipCrypto Store`。因此界面给出了不正确的加密算法提示。7-Zip 本身支持 ZIP AES，缺的是接入参数和准确展示。

### 7. TGZ 解压只完成第一层（中优先级）

实测 `createTgz()` 生成的 `.tar.gz` 经当前 extract() 解压，输出只有 `sample.tar`，没有直接展开内层文件。现有 TGZ 测试只检查能够列出 gzip，未检查最终解包结果。

### 8. 并发任务和取消缺少任务隔离（中优先级）

`lib/archive.js` 使用单一全局 `currentProc`；所有进度共用一个 IPC 事件，没有任务 ID。并行列目录、测试、解压或通过快捷键启动另一操作时，新进程会覆盖引用，某个旧进程退出还会把引用清空，取消对象可能不正确。需要统一任务队列或按任务管理进程。

### 9. 便携版并非完整的数据随身模式（中优先级）

Windows ZIP 是免安装的应用目录，但配置和自定义皮肤仍写到 `app.getPath('userData')`。没有切换到程序旁的数据目录，不能把免安装包直接等同于设置也随 U 盘迁移的完全便携模式。

### 10. 发布验证、安全设置和文档不完整

- 工作流执行 npm ci、打包和发布，没有启动应用、运行归档测试或按架构验证引擎。
- 两个现有测试在 Windows 都跑完，但多处只 console.log 布尔结果，不用断言判失败；没有覆盖用户界面、ARM64、文件关联、覆盖询问、缺密码、嵌套归档等。
- `main.js:13` 全局关闭 sandbox；IPC 参数缺少系统性的校验。已有 contextIsolation、禁用 nodeIntegration 和 CSP 是正面措施，但仍需独立安全审查，不能据此宣称存在已验证的任意代码执行漏洞。
- `FEATURE-GAP.md` 仍是 v0.1 差距表，部分已实现能力仍写成没有；“体积与启动开销更小”等结论缺少基准，现有约百 MB 的安装包也不支持“小于 8.5 MB 原版便携包”的表述。
- 自动更新客户端、资源管理器右键菜单、包内文件双击临时解压打开、拖出解压、重命名、注释、全包搜索、可排序列、大目录虚拟列表、SFX、修复/恢复等仍有缺口。
- 逆向工作缺少逐功能行为规格、边界样例与回归用例之间的可追踪关系。仅从语言包找到按钮名称，不能证明按钮背后的处理规则已被还原。

## NanaZip 与 7-Zip 对比

| 方面 | 当前 REAzip | NanaZip | 7-Zip |
|---|---|---|---|
| 项目基础 | 自写 Electron 界面与 JS 编排 | 直接基于 7-Zip 源码派生 | 原始归档引擎及工具 |
| 界面 | HTML/CSS/JS | Windows 原生技术，现代版 XAML，Classic 为 Win32 | 原生 Windows GUI 与命令行工具 |
| 引擎接入 | 启动外部命令，解析文本输出 | 源码级集成并扩展 | 提供原始实现、CLI/相关库 |
| 平台定位 | 目标 Win/macOS/Linux；仍有运行缺陷 | 现代版以 Windows 为主 | 引擎/命令行覆盖多个平台 |
| 系统集成 | 不完整，文件关联有错误 | 官方声明支持 Explorer 菜单、文件关联、MSIX | 各发行形式各自提供能力 |
| 核心区别 | 需要自己补齐任务、交互和平台适配 | 可继承并改造较完整的归档应用实现 | 是引擎基础，不等于完整 Bandizip 体验 |

来源：

- https://github.com/M2Team/NanaZip （本次读取的主分支说明；不把其中所有特性等同于某个稳定版已实测）
- https://github.com/M2Team/NanaZip/blob/main/License.md
- https://www.7-zip.org/faq.html
- https://www.7-zip.org/sdk.html
- https://github.com/rikyoz/bit7z （C++ 动态库封装方案，支持归档元数据、进度与相关回调；具体能力取决于所用引擎）

## 建议顺序

1. 先修复会覆盖数据、卡住操作和让平台不可用的问题，并用实际归档样本验证。
2. 给每个发布平台/CPU 架构建立打开、列表、压缩、解压、密码、冲突、取消的验收矩阵；构建通过和运行通过分别展示。
3. 若目标仍是跨平台 Bandizip 体验复刻，可继续复用界面，但将引擎、任务和系统集成分层。CLI 接入可以做成产品，必须补齐协议与错误处理。
4. 若目标更强调接近 Bandizip/NanaZip 的原生体积和系统集成，应评估原生 UI 与原生引擎服务。Windows 优先可参考 NanaZip；多平台应单独评估 UI 框架与跨平台 7-Zip 库封装，不能把 NanaZip 直接视为三平台产品模板。
5. 不需要为了“逆向”重写 LZMA/Deflate 算法。应把 REA 用在 Bandizip 的交互流程、路径规则、覆盖策略、编码策略、分卷和错误恢复行为上，再用成熟 7-Zip 实现基础归档能力。

## 验证范围

已执行原有两个 Windows 冒烟脚本，并在临时目录中复现加密 ZIP 算法、7z 缺密码等待和 TGZ 单层解包；对文件关联只生成脚本并做语法分析，没有执行注册表操作。Linux ARM64 的引擎架构通过 ELF 头确认。没有进行 macOS/Linux 的真实 GUI 运行验收，也没有做完整漏洞扫描或性能基准。
