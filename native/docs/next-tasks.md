# 下一阶段任务与接手顺序（2026-10-18 更新）

从 native-migration 继续，代码检查点 `caa96528`。T00–T09 已有实现且六目标全绿；T10 的自动化门禁已通过，人工验收项仍未运行。先读[当前报告](acceptance-report.md)、[限制](known-limitations.md)、[原任务书](../migration-plan/00-START-HERE.md)。旧标签 v0.2.0/v0.2.1 保持不动，main 未合并。

## N01 / P0：磁盘满解压崩溃 —— **已完成并实测通过**

bit7z 的 `CStdOutStream::Write` 把回写给 7-Zip 的 `processedSize` 算成 `tellp() - oldPos`。`std::ostream::tellp()` 一旦流上置了 `badbit` 就**不调用 `seekoff`、直接返回 -1**，而短写正是产生 `badbit` 的条件。于是 tmpfs 写满时 7-Zip 收到约 4.29e9 的 `processedSize`，`size -= curSize` 无符号下溢，下一次 `Write` 用荒谬长度读入 → SIGSEGV。创建归档路径不受影响，因为那条路径由 7-Zip 自己的文件流落盘，`ENOSPC` 正常映射成 `E_FAIL`。

修复（`native/engine/write/secure-output.cpp`）：`FileBuffer::xsputn` 不再报告短写，首次真实失败记录 errno / GetLastError，其后数据被吸收、逻辑位置继续累加，`badbit` 从未置起；`seekoff` 不再返回 `pos_type(-1)`；异常改在 `SecureOutput::file` 中、7-Zip 正常返回后用自己的栈抛出 `IoError`。原压缩包仍由临时文件 + 原子提交保护。

实测（`caa96528`，2MiB tmpfs 真实 ENOSPC）：Linux x64 与 arm64 均输出 `disk-test: extraction refused: Destination write or commit failed (OS 28); existing file retained` 后以非零码退出，无 SIGSEGV，原归档与原目标完好。`disk.py` 在 gdb 重放前重挂干净 tmpfs 并加 `-nx` / `set auto-load off`；用例未删除、未跳过。

## N02 / P1：恢复界面设计 —— 已改，**仍缺目视验收**

对照 `native/design/reference/settings.png`，常规页已重写为"标题 + 说明 + 右置控件 + 分隔线"的行式布局，接回 `restoreTabsDesc`、`legacyCodepage`、`legacyCodepageDesc`、`cpAuto`；设置/解压/任务页 footer 改为左弹性占位 + 右下动作按钮，设置页"完成"加 `primary` 且保留 `objectName: "settingsDone"`；`Sheet.qml` 恢复标准 `DialogButtonBox` 语义；`Pill.qml` 增加 `primary` 变体。全部 `objectName` 保留，六目标 `native-gui-flow` 通过。

**仍缺**：本机无 Qt，无法出并排截图；100/125/150/200% 缩放、六皮肤、长译文与 RTL 的目视检查需要在有 Qt 的环境执行。这是 R25 仍记为未通过的唯一原因。

## N03 / P1：桌面与状态边界验收（未运行）

专用测试用户/可恢复环境验收 Windows 关联/经典菜单及卸载所有权；Finder 冷/热 open-file；Linux 默认应用/desktop；真实拖出只导出所选。补 macOS quarantine 传播、恢复标签重启闭环、密码不落设置/argv/日志检查、同归档并发及跨标签取消、十万条实际归档端到端耗时/内存。无环境的项目保持 NOT RUN，不伪造 PASS。Win11 现代菜单/Finder/Nautilus 扩展不纳入已实现范围。

## N04 / P1：单 SHA 六目标 —— **已完成**

`caa96528` 上 Native candidate 与 Native foundation 六目标全部成功，各 14/14 CTest 通过，aggregate 校验产物格式齐全且 `sourceCommit` 唯一。实际包已下载核对 manifest（`sourceDirty: false`、架构、逐文件 SHA-256）。

剩余：干净物理机/VM 上的真实安装运行验收（当前只在 runner 上做过安装→启动→卸载）。

## N05 / P1：密码库与系统集成 —— **已完成（0.3.1-dev）**

- 密码库 `native/vault`：Windows 用 DPAPI 加密整库（附加固定应用熵），macOS/Linux 以 0600 保存并在界面标注"未加密"；损坏文件保留为 `.corrupt-<ms>`；便携格式在 Windows 上首次保存升级为 DPAPI；无法解密的库在其他平台置只读、拒绝覆盖。界面入口：设置 → 密码、创建归档密码区（显示/隐藏、选用、保存）、解压密码弹窗（选用、保存）。
- Windows 经典右键菜单：22 个扩展名的"解压到此处/解压到…"带图标与多选，`*`/`Directory` 的"添加到压缩包"同样；新增 `Applications\reazip.exe` 使"打开方式"可见。
- 三平台关联：Windows ProgID+Capabilities+RegisteredApplications+OpenWithProgids；macOS `CFBundleTypeExtensions`/`LSHandlerRank`/`LSSupportsOpeningDocumentsInPlace` 与 9 个 UTI；Linux `.desktop` 15 MIME + Actions + `update-desktop-database` + `xdg-mime default`。设置页"文件关联"显示真实状态。

**仍未完成**：Win11 现代菜单（IExplorerCommand + 包标识 + 签名）、Finder Quick Action、Nautilus/Dolphin 扩展、macOS Keychain / Linux Secret Service 接入。这些需要各自平台的扩展工程与签名，不在"补个注册表键"的范围里。

## N06 / 发布准备 —— 已执行

`v0.3.0-dev` 已作为 pre-release 发布；`v0.3.1-dev` 为本次密码库与系统集成版本，附件为六目标真实包 + 各平台 manifest + 校验和。R01–R35/T00–T10 状态按实测更新；未签名/未公证状态如实标注。旧公开 v0.2.1 保留。**没有**因任务书齐全就标记 T10 通过。

## 关于"和 NanaZip / Bandizip 功能类似"

能对齐的部分已在 0.3.0-dev/0.3.1-dev 实现：多标签、任务队列与取消、智能解压、逐条目冲突处理、分卷、ZIP64、AES-256、旧代码页、密码库、经典右键菜单、三平台关联声明、六主题、18 语言 RTL。**无法在本版本对齐**：Win11 现代右键菜单、Finder/Nautilus shell 扩展、SFX 创建、密码恢复、复杂损坏修复、图库预览、自动更新。这些需要 COM shell 扩展、签名证书与长期维护的独立工程，不是本次迁移能"补完"的；界面上不会出现假入口。见[功能对照](feature-parity.md)与[已知限制](known-limitations.md)。

## 体积（可选后续优化，均未实施）

当前 CI 产物：Windows x64 Setup 24.9 MiB / Portable 34.1 MiB。可再降但各有代价：`windeployqt --no-translations` 只保留 18 种语言（几 MB，风险低）；MSVC 开 LTO `/GL` + `/OPT:REF,ICF`、`/Gy`（EXE 段几 MB，构建变慢）；Linux/macOS `--strip`（解包体积十几 MB，无副作用）。**不建议**自建去 ICU 的 Qt（放弃 `deps.lock.json` 可复现性）或 UPX 加壳（杀软误报 + 首启变慢）。

接手提示词：请在 whh110112/REAzip 的 native-migration 分支上按 native/docs/next-tasks.md 顺序继续，从 N02 目视验收与 N03 桌面边界开始。先检查最新 CI，不回退现有原生实现，不删除失败测试。每个修复提交说明根因、行为变化、实测结果和仍缺的环境验收。
