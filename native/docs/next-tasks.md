# 下一阶段任务与接手顺序（2026-10-18）

从native-migration继续，代码检查点f4853ac。T00–T09已有实现，T10未通过；优先修复而非重建架构。先读[当前报告](acceptance-report.md)、[限制](known-limitations.md)、[原任务书](../migration-plan/00-START-HERE.md)。不公开原生Release、不移动旧标签。

## N01 / P0：磁盘满解压崩溃

入口native/tests/engine-write/disk.py、disk.cpp与native/engine/write/secure-output.cpp。Linux x64/ARM64在2MiB tmpfs提取4MiB文件SIGSEGV；创建归档ENOSPC能返回并保留旧包。先用独立新挂载/释放上一轮残留重现，清理gdb的Python环境冲突，拿到有效native栈。检查bit7z/7-Zip输出流错误处理及RAII对象销毁顺序；不删除/跳过失败用例。

验收：两个Linux架构真实ENOSPC均返回可解释失败、无崩溃，既有目标、原包、源文件保持；worker只有一个终态、GUI保持可用；磁盘/安全/日志/普通读写测试复验。修复和日志独立提交。

## N02 / P1：恢复界面设计

对比native/design/reference和native/design/native-reference。重点Sheet.qml、Main.qml中的设置侧栏/常规行、弹窗底部动作、间距/字体/深色系统按钮；旧设置页完成按钮靠右，而当前撑满底部。沿用旧设计，不新增视觉风格。保留安全错误文本及功能绑定。

验收：同场景/语言/数据截图并排与差异说明；100/125/150/200%缩放，六皮肤；长译文和RTL不遮挡关键动作；真实GUI流程通过。透明/模糊/系统控件差异明确记录。

## N03 / P1：桌面与状态边界验收

专用测试用户/可恢复环境验收Windows关联/经典菜单及卸载所有权；Finder冷/热open-file；Linux默认应用/desktop；真实拖出只导出所选。补macOS quarantine传播、恢复标签重启闭环、密码不落设置/argv/日志检查、同归档并发及跨标签取消、十万条实际归档端到端耗时/内存。无环境的项目保持NOT RUN，不伪造PASS。Win11现代菜单/Finder/Nautilus扩展不纳入已实现范围。

## N04 / P1：单SHA六目标与候选核对

N01/N02相关修复后再跑Native foundation和Native candidate。六个原生runner全部成功；下载实际包核对SHA256与manifest、架构、版本、对应源码/许可；隔离PATH运行，Windows安装→启动→卸载，Mac签名/DMG，Linux三种形式。补真实干净系统验收。任何目标失败阻止全部通过声明。

## N05 / 发布准备

更新R01–R35和T00–T10状态；补最终附件地址、体积/性能、安装说明、明确未支持项目。证书/公证资源不足时如实保留未签名状态。完成具体候选后再决定公开发布，旧公开v0.2.1保留。不要因任务书齐全就标记验收完成。

接手提示词：请在whh110112/REAzip的native-migration分支上按native/docs/next-tasks.md顺序继续，从N01磁盘满解压崩溃开始。先检查git状态和最新CI，不回退现有原生实现，不删除失败测试，不公开发布。每个修复提交说明根因、行为变化、实测结果和仍缺的环境验收。
