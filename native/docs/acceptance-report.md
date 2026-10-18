# 原生迁移阶段验收报告（2026-10-18，交接快照）

**结论：已有可构建、可操作的原生候选；自动化门禁已通过，T10 仍因人工验收缺口未完全通过。** 当前检查点 `caa96528`（N01/N02 修复提交），六目标 CI 全绿，见下方证据。

## 本轮复验结果（caa96528 → candidate 39092715383）

同 SHA 六个原生 runner 全部成功，每个目标 CTest **14/14 通过**（Windows x64/ARM64、Linux x64/ARM64、macOS x64/ARM64），aggregate 校验六目标均产出 `.exe`+`.zip` / `.dmg`+`.zip` / `.AppImage`+`.deb`+`.gz` 且 `sourceCommit` 唯一。Foundation 39092715464 同为六目标成功。

### R34 / N01：磁盘满解压 SIGSEGV —— 已修复并实测通过

根因（静态分析 + 上游源码）：bit7z `CStdOutStream::Write` 用 `tellp() - oldPos` 生成回写给 7-Zip 的 `processedSize`；`std::ostream::tellp()` 在流置了 `badbit` 时直接返回 -1，而短写正是 `badbit` 的成因。tmpfs 写满 → 短写 → `processedSize ≈ 4.29e9` → 7-Zip 的剩余长度无符号下溢 → 以荒谬长度重入 `Write` → SIGSEGV。创建归档不受影响，因为该路径由 7-Zip 自己的文件流写入，`ENOSPC` 正常映射为 `E_FAIL`。

修复：`native/engine/write/secure-output.cpp` 的 `FileBuffer` 不再报告短写，也不从 `seekoff` 返回 `pos_type(-1)`；首次失败被记录，其余字节被吸收且逻辑位置保持连续，`badbit` 不会置起。异常不再跨 7-Zip 边界——`SecureOutput::file` 在 7-Zip 调用正常返回后再抛 `IoError`。`native/tests/engine-write/disk.py` 在 gdb 重放前重新挂载干净 tmpfs 并关闭 auto-load。失败用例保留，未跳过。

实测证据（两者均为 2MiB tmpfs 真实 ENOSPC，非限流模拟）：

- Linux x64 `LastTest.log`：`disk-test: extraction refused: Destination write or commit failed (OS 28); existing file retained` → `Linux 2MiB tmpfs: real ENOSPC during archive write and extraction PASS`
- Linux arm64：同上，`OS 28`（ENOSPC）经 `IoError` 返回，进程正常非零退出，无 SIGSEGV；崩溃栈不再出现。

### R25 / N02：设置页与弹窗底部布局 —— 已改，仍缺目视验收

按 `native/design/reference/settings.png`（左侧窄侧栏 + 右侧"标题/说明 + 右置控件"行 + 分隔线，底部单个右对齐"完成"）重写常规页，接回 `restoreTabsDesc` / `legacyCodepage` / `legacyCodepageDesc` / `cpAuto`；设置、解压、任务三个弹窗的 `footer` 改为左弹性占位、按钮右下；`Sheet.qml` 恢复标准接受/拒绝按钮语义；`Pill.qml` 增加 `primary` 变体。全部 `objectName` 保留，GUI 流程测试未受影响（六目标 `native-gui-flow` 通过）。

待验：同场景并排截图与差异说明、100/125/150/200% 缩放、六皮肤、长译文与 RTL。本轮环境无 Qt，无法出图，因此 R25 仍记为未通过。

## 历史已执行证据

- caa96528：[Native candidate 39092715383](https://github.com/whh110112/REAzip/actions/runs/39092715383)，六目标各 14/14 CTest 通过 + aggregate 成功；Linux x64/ARM64 的 2MiB tmpfs 真实 ENOSPC 均以 `IoError (OS 28)` 正常失败、无崩溃。[Native foundation 39092715464](https://github.com/whh110112/REAzip/actions/runs/39092715464) 六目标成功。证据摘自各 job 的 `LastTest.log`：`disk-test: extraction refused: Destination write or commit failed (OS 28)` 与 `Linux 2MiB tmpfs: real ENOSPC during archive write and extraction PASS`。
- d39681507ee7b037d616c36e7bb338e1194af7b8：[Native candidate 39046385881](https://github.com/whh110112/REAzip/actions/runs/39046385881)，Windows/macOS/Linux各x64和ARM64六个原生runner及aggregate全部成功。包含当时10项CTest、真实QtTest鼠标/键盘、架构检查、PATH隔离包运行；Windows安装运行卸载、Mac两种包、Linux三种包。运行页Artifacts可下载，需登录GitHub。
- 833407f：本机Windows11 x64 / Qt6.8.3 / MinGW13.1.0，14/14 CTest通过，51.87秒。含ZIP64 4GiB+4096B完整独立解码CRC、提交中断恢复、路径攻击、拒绝替换、RAR/RAR5/WIM、三种代码页、协议与真实GUI。日志见[evidence/windows-x64-833407f-LastTest.log](evidence/windows-x64-833407f-LastTest.log)。系统LongPathsEnabled仍为0，长路径用例通过。
- 833407f：[foundation 39050946397](https://github.com/whh110112/REAzip/actions/runs/39050946397)失败，Linux两架构实际磁盘满用例SIGSEGV；[candidate 39050946387](https://github.com/whh110112/REAzip/actions/runs/39050946387)同样记录Linux失败，随后被新提交的并发规则取消，不能称成功。
- f4853ac：[诊断39051268735](https://github.com/whh110112/REAzip/actions/runs/39051268735)确认崩溃发生于磁盘满解压阶段，压缩阶段能正常失败并保留旧归档；有效崩溃栈尚未获得。[foundation 39051268563](https://github.com/whh110112/REAzip/actions/runs/39051268563)、[candidate 39051268644](https://github.com/whh110112/REAzip/actions/runs/39051268644)为该检查点自动重跑，结果以页面为准，不能以进行中替代通过。
- 视觉：旧Electron44张场景/倍率捕获清单及11张参考图已建立。最新Windows原生14场景分别在物理1120×740和2240×1480捕获，无QML诊断；本机系统DPR2，对应QT_SCALE_FACTOR=0.5/1。当前100%图及SHA清单见[design/native-reference](../design/native-reference/)。检查了主窗口、深色、RTL、新建、设置等图，发现设置页与按钮布局差异，未作全部视觉通过声明。

本轮复验命令：设置本机Qt/MinGW路径后，cmake --build .native-build/local -j 4；ctest --test-dir .native-build/local --output-on-failure -j 1。六目标完整复现见.github/workflows/native-candidate.yml。R编号定义见[原矩阵](regression-matrix.md)。

## R01–R35 当前结论

PASS仅指以下明确实测范围；PARTIAL包含未运行/证据不足，不等于正式验收。复合条件按缺口保守标注。

| ID | 状态 | 证据与缺口 |
|---|---|---|
| R01 | PASS（caa96528六目标） | 完整原生库装载与机器类型匹配；六目标 14/14 通过，aggregate 校验架构与产物格式 |
| R02 | PARTIAL | Unicode/非BMP及本机>400字符路径往返；六目标已通过，跨机器路径边界仍待补 |
| R03 | PARTIAL | 四格式native往返、基础CI通过，ZIP64有独立Python解码；全部格式独立工具覆盖待补 |
| R04 | PARTIAL | 新RAR/RAR5/WIM样本本机及Linux读取/提取通过，ISO旧六目标通过；最新完整矩阵待收口 |
| R05 | PASS（自动范围） | 加密头无密码、错密码重试、取消后新任务、GUI点击通过 |
| R06 | PARTIAL | AES/ZipCrypto真实元数据与AES创建有断言；独立AES工具兼容性仍需确认 |
| R07 | PARTIAL | core冲突策略实现/部分断言；完整整包四策略组合待补齐 |
| R08 | PARTIAL | GUI/AppController选中ask→skip/rename/overwrite；多选/目录完整组合待补齐 |
| R09 | PARTIAL | 虚拟目录、稳定ID、旧ID拒绝有断言；重复条目/通配符完整组合待补齐 |
| R10 | PASS（已测范围） | TAR.GZ展开最终文件内容一致，旧六目标测试通过 |
| R11 | PARTIAL | GBK/Big5/Shift-JIS真实字节样本通过本机及Linux；自动策略说明/GUI选择待复核 |
| R12 | PARTIAL | 首卷测试、ZIP64真实4GiB+4096B独立解码通过；缺卷组合待补齐 |
| R13 | PARTIAL | native/AppController增删改与7z头加密保留通过；独立工具全覆盖待补 |
| R14 | PASS（自动范围） | 取消、析构、提交中断回滚及唯一终态有测试；磁盘满解压崩溃已在 caa96528 修复并在两个 Linux 架构实测无崩溃 |
| R15 | PARTIAL | 队列/取消与归档锁已实现；跨进程同包并发/跨标签完整压力待补 |
| R16 | PASS（安全测试范围） | 路径逃逸/链接/Windows junction、父目录替换和晚到目标不覆盖测试；不是任意同UID攻击保证 |
| R17 | PARTIAL | 取消不删源、校验后可恢复trash回调有断言；真实桌面回收站/源变更完整组合待补 |
| R18 | PARTIAL | 密码走私有pipe、不持久设置、不放argv；完整日志/诊断包泄漏审计仍待验收 |
| R19 | PARTIAL | Unicode同用户二次启动转交通过；Finder及各桌面冷/热双击待人工验收 |
| R20 | PARTIAL | 隔离HKCU所有权/卸载断言与Windows安装卸载通过；专用用户实际默认应用未验收 |
| R21 | PARTIAL | Windows经典静态菜单注册；现代菜单、Finder/Nautilus扩展未实现，真实桌面菜单未验收 |
| R22 | PARTIAL | 数据目录/设置服务测试；只读目录提示已实现但完整UI场景待复验 |
| R23 | PARTIAL | 18语言147key无缺项、德语/RTL截图与快捷键；全部长译文/焦点/辅助功能待验收 |
| R24 | PARTIAL | 六皮肤截图、颜色迁移/备份与CSS拒绝；高级模糊阴影保真未完成 |
| R25 | 未通过（已改待验） | 设置侧栏/常规页/弹窗底部布局已按旧设计重写，缺并排截图与缩放/皮肤/长译文目视验收 |
| R26 | PARTIAL | 10万条目录模型搜索/64bit大小/选择刷新测试；真实大包端到端与内存记录待补 |
| R27 | PARTIAL | 旧六目标包清PATH启动GUI/worker成功；真正全新物理系统/VM未验收 |
| R28 | PARTIAL | 六目标二进制架构检查、Mac ad-hoc验证、Windows未签名如实记录；新SHA还须包验证 |
| R29 | PARTIAL | 同机体积/首帧/工作集原始记录已保存；体积下降、启动未证明更快，最终包数据待补 |
| R30 | PASS（门禁范围） | 六目标aggregate、无发布权限；新增失败确实阻止通过，旧公开版未改 |
| R31 | PARTIAL | 安全临时打开、执行类确认、选中QDrag已实现；真实跨桌面拖出未验收 |
| R32 | PARTIAL | 未知元数据不伪造、恢复标签/路径过滤/无密码；完整重启丢失路径场景待补 |
| R33 | PASS（自动范围） | 半包、无效JSON、超长帧、旧job/challenge、EOF取消、唯一终态测试 |
| R34 | PASS（自动范围） | Windows拒绝替换、提交日志/分卷回滚通过；Linux x64/ARM64 在 2MiB tmpfs 真实 ENOSPC 下经 `IoError` 正常失败、原包与原目标未损坏、无 SIGSEGV（39092715383） |
| R35 | PARTIAL | 只读格式/TGZ密码等明确拒绝/禁用；所有UI选项对照尚未完整签收 |

## 阶段决定

同 SHA 六目标复验已完成（caa96528，全绿）。据此 `v0.3.0-dev` 以 **pre-release** 形式发布候选包，供试用与回归；**不宣布 T10 正式验收通过**，因为以下人工缺口仍未运行：

1. 设置页/弹窗并排截图、缩放倍率、六皮肤与长译文/RTL 目视签收（R25）；
2. 真实桌面文件关联、右键菜单、跨应用拖出（R19/R20/R21/R31）；
3. 大包压力与真实物理机/VM 冷启动（R26/R27）。

其余 PARTIAL 项属"实现已有、证据范围不足"，不是已知缺陷。后续优先顺序见[next-tasks.md](next-tasks.md)。原任务书和新增验收材料均提交GitHub；SDK、node_modules、下载缓存和构建中间文件不进入源码仓库，候选包通过Actions artifacts与Release附件提供。
