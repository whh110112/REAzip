# 下一阶段任务与接手顺序（2026-10-18 更新）

从native-migration继续，代码检查点f4853ac。T00–T09已有实现，T10未通过；优先修复而非重建架构。先读[当前报告](acceptance-report.md)、[限制](known-limitations.md)、[原任务书](../migration-plan/00-START-HERE.md)。不公开原生Release、不移动旧标签。

## N01 / P0：磁盘满解压崩溃 —— 代码已修，等待六目标复验

**根因已定位（静态分析，附上游源码依据）。** bit7z 的 `CStdOutStream::Write` 把回写给 7-Zip 的 `processedSize` 算法写成 `tellp() - oldPos`。`std::ostream::tellp()` 一旦流上置了 `badbit` 就**不调用 `seekoff`、直接返回 -1**，而短写正是产生 `badbit` 的条件。于是 tmpfs 写满时 7-Zip 收到约 4.29e9 的 `processedSize`，`size -= curSize` 无符号下溢，下一次 `Write` 用荒谬长度读入 → SIGSEGV。创建归档路径不受影响，因为那条路径由 7-Zip 自己的文件流落盘，`ENOSPC` 正常映射成 `E_FAIL`。这条链解释了"压缩阶段能正常失败、只有提取阶段崩"的观测。

**已改**（`native/engine/write/secure-output.cpp`）：

- `FileBuffer::xsputn` 不再报告短写。首次真实失败记录 errno / GetLastError，其后数据被吸收，逻辑位置继续累加，因此 `badbit` 从未置起，`tellp()` 永不返回 -1。
- `seekoff` 同样不再返回 `pos_type(-1)`（那会置 `failbit`，`tellp()` 又返回 -1，触发同一个下溢）。失败后返回最后的逻辑位置。
- 异常不再跨越 7-Zip 边界：`SecureOutput::file` 在 7-Zip 调用正常返回后、在自己的栈上检查 `writeFailed()` 并抛出映射好的 `IoError`。原压缩包仍由临时文件 + 原子提交保护。

**测试改动**（`native/tests/engine-write/disk.py`）：失败时先卸载再重新挂载一块干净的 2 MiB tmpfs 再进 gdb 重放（首轮失败已把旧挂载写满，否则调试轮还没走到解压就失败），并加 `-nx -ex "set auto-load off"` 避免 runner 上的 `.gdbinit` Python pretty-printer 掩盖崩溃。未删除、未跳过任何用例。

**仍未完成**：六目标 CI 实跑。本机没有 Qt/CMake 工具链，无法在此复现 Linux 真实 ENOSPC；`PASS` 只能在 x64 与 ARM64 两个 Linux runner 都实际返回可解释失败之后写。

验收：两个Linux架构真实ENOSPC均返回可解释失败、无崩溃，既有目标、原包、源文件保持；worker只有一个终态、GUI保持可用；磁盘/安全/日志/普通读写测试复验。

## N02 / P1：恢复界面设计 —— 已改，等待截图比对

对照 `native/design/reference/settings.png` 与 `native/design/native-reference/light-de-settings.png`，旧设计是左侧窄侧栏 + 右侧行式设置项（标题 + 说明 + 右置控件，行间分隔线），底部只有一个右对齐的"完成"按钮。

**已改**：

- `native/ui/Main.qml` 常规页重写为行式布局，把 i18n 里早已存在却没用上的 `restoreTabsDesc`、`legacyCodepage`、`legacyCodepageDesc`、`cpAuto` 接回界面。
- 设置页、解压页、任务页的 `footer` 从撑满底部改为 `RowLayout { Item { Layout.fillWidth:true } ... }`，动作按钮回到右下角；设置页"完成"按钮加 `primary`，`objectName: "settingsDone"` 保留。
- `native/ui/Sheet.qml` 的 `DialogButtonBox` 恢复标准的接受/拒绝对话按钮语义，不再自绘撑满的按钮。
- `native/ui/Pill.qml` 增加 `primary` 变体（强调色填充），供底部主按钮使用。

保留全部 `objectName`（`extractDestination`、`extractStart`、`settingsDone`、`conflict-*` 等），GUI 测试选择器未被破坏。**仍未完成**：本机无 Qt，无法出并排截图；100/125/150/200% 缩放、六皮肤、长译文与 RTL 的实际目视检查仍待有 Qt 的环境执行。

## N03 / P1：桌面与状态边界验收

专用测试用户/可恢复环境验收Windows关联/经典菜单及卸载所有权；Finder冷/热open-file；Linux默认应用/desktop；真实拖出只导出所选。补macOS quarantine传播、恢复标签重启闭环、密码不落设置/argv/日志检查、同归档并发及跨标签取消、十万条实际归档端到端耗时/内存。无环境的项目保持NOT RUN，不伪造PASS。Win11现代菜单/Finder/Nautilus扩展不纳入已实现范围。

## N04 / P1：单SHA六目标与候选核对

N01/N02修复后用**同一 SHA** 重跑 Native foundation 和 Native candidate。六个原生runner全部成功；下载实际包核对SHA256与manifest、架构、版本、对应源码/许可；隔离PATH运行，Windows安装→启动→卸载，Mac签名/DMG，Linux三种形式。补真实干净系统验收。任何目标失败阻止全部通过声明。N01 若仍在任一 Linux runner 上失败，**不发布**。

## N05 / 发布准备

更新R01–R35和T00–T10状态；补最终附件地址、体积/性能、安装说明、明确未支持项目。证书/公证资源不足时如实保留未签名状态。六目标全绿后再公开原生Release，旧公开v0.2.1保留。不要因任务书齐全就标记验收完成。

## 关于"和 NanaZip / Bandizip 功能类似"

能对齐的部分已在 0.3.0-dev 实现：多标签、任务队列与取消、智能解压、逐条目冲突处理、分卷、ZIP64、AES-256、旧代码页、六主题、18 语言 RTL。**无法在本版本对齐的部分**：Win11 现代右键菜单、Finder/Nautilus shell 扩展、SFX 创建、密码恢复、复杂损坏修复、图库预览、自动更新。这些需要 COM shell 扩展、签名证书与长期维护的独立工程，不是本次迁移范围内能"补完"的；界面上不会出现假入口。见[功能对照](feature-parity.md)与[已知限制](known-limitations.md)。

接手提示词：请在whh110112/REAzip的native-migration分支上按native/docs/next-tasks.md顺序继续，从N04单SHA六目标复验开始。先检查最新CI，不回退现有原生实现，不删除失败测试，不公开发布。每个修复提交说明根因、行为变化、实测结果和仍缺的环境验收。
