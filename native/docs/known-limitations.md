# 当前差距与发布阻塞（2026-10-18，0.3.1-dev）

状态基于 native-migration 的 0.3.1-dev 提交。磁盘满解压崩溃已在 `caa96528` 修复并实测，0.3.1-dev 在其上补齐密码库、Windows 经典右键菜单与三平台关联声明；下列人工验收项仍未运行，不能据此声明 T10 正式验收通过。

## 必须先修复

1. ~~**Linux磁盘满解压崩溃**~~ —— **已修复并实测通过**（`caa96528`）。根因是 bit7z `CStdOutStream::Write` 用 `tellp() - oldPos` 回写 `processedSize`，而 `tellp()` 在 `badbit` 置起时直接返回 -1，令 7-Zip 剩余长度无符号下溢。修复后 Linux x64/ARM64 在 2MiB tmpfs 真实 ENOSPC 下均以 `IoError (OS 28)` 正常失败，无 SIGSEGV，原归档与原目标完好。
2. **视觉保真未收口**：设置侧栏/常规页和弹窗底部按钮排列已按旧设计重写（行式设置项 + 右下动作按钮），但**尚未出并排截图**，100/125/150/200% 缩放、六皮肤、长译文与 RTL 的目视检查也未执行。透明/模糊/阴影不能声称完全等价。
3. 六目标复验：`caa96528` 一轮已通过；0.3.1-dev 的新代码需要同一 SHA 的六目标证据，见 Release 说明。

## 尚未完成的验收

- Explorer/Finder/Linux桌面的真实默认应用选择、冷/热启动、菜单与拖出。现有自动测试涵盖隔离注册表、同用户进程通信、QFileOpenEvent入口、QDrag实现及安装卸载，不能代替完整桌面验收。
- 18语言166个key齐全，但全部语言长文案、键盘焦点、屏幕阅读器和各系统缩放未全面人工检查。
- 十万条目录模型性能有测试，十万条真实归档经worker到界面的端到端耗时/内存还需补测。
- PATH隔离的打包应用运行已通过旧候选；全新系统的依赖、文件关联和升级覆盖仍需验证。
- macOS quarantine传播仍需实现/验证；目前Windows来源流有自动行为测试。
- 密码库的按键统一由各平台自身能力决定：Windows DPAPI 受账户与机器绑定；macOS/Linux 目前只靠 0600 文件权限，**未接入 Keychain/Secret Service**，界面会显示"未加密"而不是假装安全。密码敏感内存只减少引用，不承诺内存取证级清零。

## 明确未实现/限制

- Win11 现代右键菜单（IExplorerCommand + 包标识 + 签名）、Finder Quick Action、Nautilus/Dolphin 扩展；Windows 只实现经典静态菜单注册（Win11 在「显示更多选项」中可见）。
- macOS 不允许第三方程序写入默认处理程序数据库，只能在 Finder「显示简介 → 打开方式 → 全部更改」由用户确认；Linux 通过 `xdg-mime` 写入用户级 `mimeapps.list`，但不会覆盖桌面环境已有的显式选择。
- SFX创建、密码恢复、复杂损坏修复、病毒扫描、图库预览、自动更新客户端。
- RAR写入、TAR/TGZ加密、TGZ分卷；已有分卷集覆盖要求换新文件名；不会以成功提示静默忽略参数。
- 自定义JSON仅迁移已支持的颜色token；任意CSS保留备份但不执行。Qt界面不是原HTML的逐像素复刻。
- POSIX写入拒绝其他用户可写且非sticky的祖先目录；链接输入/归档链接提取拒绝。安全边界不包含已控制同一用户全部文件与进程的攻击者。恢复日志FNV用于意外变更识别，不是密码学签名。
- Windows未代码签名，macOS仅ad-hoc签名、未公证。没有证书/公证凭据时不能宣称已完成。
- Linux候选在Ubuntu24.04构建，deb依赖glibc>=2.39；macOS构建部署目标13.0；更旧系统未验证。Windows旧系统兼容性不以Win11/Server2022 runner结果作保证。
- 包体积明显下降，首帧速度尚未证明优于Electron，详见[size-and-performance.md](size-and-performance.md)。
