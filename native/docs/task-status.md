# 原生迁移任务进度（2026-10-18，0.3.1-dev）

执行分支：native-migration。0.3.0-dev（caa96528）六目标全绿并有 pre-release；0.3.1-dev 在其上新增密码库、Windows 经典右键菜单与三平台关联声明，T10 自动化门禁需在本提交上重跑，人工验收缺口未收口。公开 main/v0.2.1 保持原样。

| 任务 | 当前状态 | 实现证据 | 仍需完成 |
|---|---|---|---|
| T00 基线 | 已建立 | 原Electron44张缩放/场景捕获清单、11张参考图、功能映射 | 同场景视觉差异收口 |
| T01 工程与六目标 | 已实现并有六目标成功记录 | C++17/Qt6.8.3/bit7z4.1.0/完整7-Zip26.04；caa96528 candidate 39092715383 六目标全绿 | 0.3.1-dev 同SHA重跑 |
| T02 读取/密码/编码 | 已实现 | Unicode、加密头、AES/ZipCrypto、GBK/Big5/Shift-JIS、RAR/RAR5/ISO/WIM样本 | 独立工具与GUI完整组合复核 |
| T03 写入/解压/修改 | 已实现，P0已修复并实测 | ZIP/7Z/TAR/TGZ、分卷、修改、安全句柄写入、日志恢复、真实ZIP64；磁盘满解压 SIGSEGV 已修，Linux x64/ARM64 2MiB tmpfs 实测经 IoError 失败无崩溃 | 独立工具覆盖 |
| T04 worker/调度 | 已实现并补协议边界 | 队列、密码/冲突挑战、取消、半包/超帧/旧ID/断流测试 | 同包跨进程与跨标签完整压力 |
| T05 Qt Quick界面 | 已实现，已改待目视验收 | 真实窗口/虚拟列表/弹窗，六皮肤与RTL截图；设置页行式布局、底部按钮右下；新增设置"密码"页 | 并排截图、缩放/长译文/可访问性 |
| T06 操作串联 | 已实现 | AppController闭环、QtTest真实鼠标/键盘、10万条模型；创建/解压弹窗接入密码库 | 真实拖出/状态恢复/大包端到端 |
| T07 系统集成 | Windows 已完成经典菜单；macOS/Linux 为声明级 | 同用户通信、隔离HKCU注册/卸载、MOTW；22 个扩展名的经典动词（图标+多选）、Applications/OpenWith 列表、Capabilities 默认应用项；macOS 9 个 UTI + CFBundleTypeExtensions；Linux .desktop 15 MIME + xdg-mime default | Win11 现代菜单/COM、Finder Quick Action、真实桌面默认应用与拖出 |
| T08 设置/主题/语言 | 已实现 | 原子JSON、损坏备份、旧设置颜色迁移、18语言166key齐全；密码库按平台如实标注加密状态 | 长译文、完整主题效果与桌面重启边界 |
| T09 打包/门禁 | 已建立，0.3.0-dev 六目标通过 | caa96528 run39092715383：Win EXE+ZIP、Mac DMG+ZIP、Linux AppImage+tar.gz+deb，aggregate校验sourceCommit唯一 | 0.3.1-dev 同SHA重跑与附件核对 |
| T10 验收/收口 | 自动化待重跑，人工未收口 | caa96528 六目标各14/14 CTest + aggregate成功；逐条R01–R35报告；0.3.1-dev 增 V01–V03 | 并排截图与桌面/大包人工验收 |

当前检查点：0.3.1-dev 新增 V01 密码库、V02 Windows 右键菜单、V03 三平台关联声明，均含自动化测试（`native-vault`、`native-platform` 扩展断言）。R34 已由 FAIL 转 PASS（自动范围）；R25 仍为未通过，因为环境无 Qt 无法出并排截图。详见[验收报告](acceptance-report.md)、[下一步任务](next-tasks.md)、[T00–T10交接](handoffs/)。

所有实现/文档上传开发分支；旧标签 v0.2.0/v0.2.1 未移动，main 未合并。候选以 pre-release 形式发布（v0.3.0-dev 已发；v0.3.1-dev 待本次六目标通过），附件为六目标真实包。
