# REAzip 0.3.0-dev 原生候选发布说明（草稿）

**尚不可正式发布。** Linux磁盘满解压崩溃已定位根因（bit7z 输出流在短写后用 `tellp()` 的 -1 回写 processedSize，令 7-Zip 剩余长度无符号下溢）并在 `secure-output.cpp` 修复，设置页/弹窗底部布局已按旧设计重写；两者都还缺六目标与截图证据，桌面验收亦未收口，见[已知限制](known-limitations.md)。不移动/覆盖v0.2.x标签，不把草稿当成公开Release。

本轮把归档处理与应用运行时迁移为C++17、Qt Quick6.8.3、bit7z4.1.0、7-Zip26.04完整原生动态库。保留本项目界面结构、六主题和18语言，发行包不带Electron或7z命令行运行依赖。

实现多标签浏览、目录搜索、ZIP/7Z/TAR/TGZ压缩与解压、选中冲突策略、密码挑战与算法元数据、添加/删除/重命名、编码覆盖、原生worker隔离和任务取消。补充归档路径与链接防护、私有暂存、提交日志和异常恢复、Windows长路径及MOTW传播。设置支持白名单迁移和自定义颜色token；不执行旧CSS。

候选矩阵：Windows x64/ARM64安装EXE+便携ZIP；macOS x64/ARM64 DMG+ZIP；Linux x64/ARM64 AppImage+tar.gz+deb。Windows未签名，macOS ad-hoc签名、未公证。候选包含版本/SHA/架构/摘要、第三方许可和对应源码；Linux基线Ubuntu24.04，其他兼容范围见安装指南。

本机首轮Windows x64原生安装包约26.7MB、便携ZIP约36.6MB，对比旧版约121.1MB、176.0MB；不是最终发行哈希/体积。内存测量下降，但启动时间没有证明更快，原始数据与测量边界见[性能记录](size-and-performance.md)。

正式发布前：同SHA六目标复验（含两个Linux架构的真实ENOSPC）→ 设置页并排截图与缩放/皮肤/长译文签收 → 桌面关联/菜单/拖出与完整压力验收 → 验收报告逐项签收 → 核对附件/许可证/签名状态 → 明确公开发布决定。当前workflow只上传候选，不公开发布。
