# 原生候选安装指南

0.3.0-dev用于开发验收，当前有[已知阻塞](known-limitations.md)。[Native candidate](https://github.com/whh110112/REAzip/actions/workflows/native-candidate.yml)成功运行页面底部Artifacts提供各架构包，下载需要登录GitHub。最新完整通过的旧候选见[39046385881](https://github.com/whh110112/REAzip/actions/runs/39046385881)，对应d396815；不要将其当成最新安全代码的验证结果。

按操作系统和CPU选择x64或ARM64。解开GitHub artifact外层ZIP后，先核对包旁SHA256SUMS和manifest中的版本、sourceCommit、architecture、signing；这些是候选记录，不是第三方代码签名。

| 系统 | 使用方式 |
|---|---|
| Windows安装EXE | 运行Setup，默认当前用户安装；安装器可选关联注册。默认程序最终由系统设置选择。卸载从系统应用列表执行。 |
| Windows便携ZIP | 完整解压到可写目录，运行reazip.exe。保留portable.flag，设置和主题存到旁边data；不要只复制主EXE。 |
| macOS DMG/ZIP | DMG内把REAzip.app拖到Applications，或解开ZIP。应用只有ad-hoc签名且未公证，系统可能阻止首次打开；开发验收应使用受控机器并按系统提示检查来源。 |
| Linux AppImage | 对所下载的单个文件添加执行权限后运行；运行环境仍需桌面基础库，挂载可能需要FUSE。请放到永久路径后再注册desktop入口。 |
| Linux tar.gz | 完整解压到用户目录，通过AppRun启动，不单独移动bin/reazip。 |
| Linux deb | 使用发行版包管理器安装对应架构，Ubuntu24.04/glibc>=2.39为当前构建基线。 |

无需另外安装Node、Qt SDK或7-Zip。Windows未签名；macOS未公证。不要全局关闭系统安全设置。

安装版配置使用Qt标准用户数据路径下native目录；Windows便携配置固定旁边data。首次发现旧Electron的REAzip/reazip配置时，只迁移白名单设置、颜色主题与有效标签，原目录保留；不复制密码。自定义CSS不会运行，转换告警保存在迁移记录中。损坏JSON会先备份；更高版本schema只读，避免降级破坏。

正式发布前仍需完成默认应用、真实拖出、干净系统和升级/卸载验收。旧v0.2.1公开发行保持原样。
