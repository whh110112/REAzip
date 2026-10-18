# REAzip 0.3.0-dev 原生候选发布说明

**这是 pre-release（候选预发布），不是 T10 正式验收通过版本。** 六目标自动化门禁已在同一提交 `caa96528` 上全绿，但以下人工项仍未运行：设置页并排截图与缩放/长译文目视签收、真实桌面文件关联与右键菜单、跨应用拖出、大包压力与干净物理机冷启动。详见[验收报告](acceptance-report.md)与[已知限制](known-limitations.md)。

## 本轮修复

**磁盘满解压崩溃（Linux，此前 SIGSEGV）。** 根因在 bit7z 适配层而非 7-Zip：`CStdOutStream::Write` 用 `tellp() - oldPos` 回写 `processedSize`，而 `std::ostream::tellp()` 在流置了 `badbit` 时直接返回 -1（短写正是其成因），7-Zip 因此收到约 4.29e9 的已处理字节数，剩余长度无符号下溢，下一次以荒谬长度重入 `Write` 而崩溃。`SecureOutput` 的 `FileBuffer` 改为不报告短写、不从 `seekoff` 返回 `-1`，首次失败记录 errno/GetLastError、后续字节吸收并保持逻辑位置连续，异常改在 7-Zip 返回后用自己的栈抛出。实测：Linux x64 与 arm64 在 2MiB tmpfs 上均得到 `Destination write or commit failed (OS 28)`（ENOSPC）后正常非零退出，原归档与原目标未被损坏。

**设置页与弹窗布局。** 设置常规页改为"标题/说明 + 右置控件 + 分隔线"行式布局，接回 `restoreTabsDesc`、`legacyCodepage(Desc)`、`cpAuto`；设置/解压/任务弹窗的按钮回到右下角而非撑满底部。`objectName` 全部保留。目视验收仍未完成。

## 验证范围

同一提交 `caa965283fc7c6c5987952ee0c9be768d232d2fa` 上：

- [Native candidate 39092715383](https://github.com/whh110112/REAzip/actions/runs/39092715383)：Windows x64/ARM64、Linux x64/ARM64、macOS x64/ARM64 六个目标各 **14/14 CTest 通过**，aggregate 校验六目标产物格式齐全且 `sourceCommit` 唯一。
- [Native foundation 39092715464](https://github.com/whh110112/REAzip/actions/runs/39092715464)：六目标成功。
- 每个包的清单（`*-manifest.json`）记录 `version`、`sourceCommit`、`sourceDirty: false`、架构、每个二进制的 SHA-256、签名状态与 `unpackedBytes`。

## 下载

| 目标 | 文件 | 体积 |
|---|---|---:|
| Windows x64 | `REAzip-0.3.0-dev-windows-x64-Setup.exe` | 24.9 MiB |
| Windows x64 | `REAzip-0.3.0-dev-windows-x64-Portable.zip` | 34.1 MiB |
| Windows ARM64 | `REAzip-0.3.0-dev-windows-arm64-Setup.exe` | 21.1 MiB |
| Windows ARM64 | `REAzip-0.3.0-dev-windows-arm64-Portable.zip` | 29.7 MiB |
| macOS x64 | `REAzip-0.3.0-dev-macos-x64.dmg` | 35.7 MiB |
| macOS x64 | `REAzip-0.3.0-dev-macos-x64.zip` | 31.5 MiB |
| macOS ARM64 | `REAzip-0.3.0-dev-macos-arm64.dmg` | 32.9 MiB |
| macOS ARM64 | `REAzip-0.3.0-dev-macos-arm64.zip` | 29.1 MiB |
| Linux x64 | `REAzip-0.3.0-dev-linux-x64.AppImage` | 43.0 MiB |
| Linux x64 | `REAzip-0.3.0-dev-linux-x64.deb` | 34.3 MiB |
| Linux x64 | `REAzip-0.3.0-dev-linux-x64.tar.gz` | 46.2 MiB |
| Linux ARM64 | `REAzip-0.3.0-dev-linux-arm64.AppImage` | 42.4 MiB |
| Linux ARM64 | `REAzip-0.3.0-dev-linux-arm64.deb` | 33.9 MiB |
| Linux ARM64 | `REAzip-0.3.0-dev-linux-arm64.tar.gz` | 45.8 MiB |

每个目标另附该平台的 `*-manifest.json`（含逐文件 SHA-256）。校验方式：`sha256sum -c SHA256SUMS.txt`。

**签名状态**：Windows 包**未签名**（首次运行会遇到 SmartScreen 提示）；macOS 为 ad-hoc 签名、**未公证**（首次打开需右键 → 打开，或 `xattr -dr com.apple.quarantine`）；Linux 包未做 GPG 签名。这是当前构建的已知事实，不是遗漏说明。

## 主要能力

多标签浏览、目录搜索、ZIP/7Z/TAR/TGZ 创建与解压、选中条目冲突策略、密码挑战与算法元数据、添加/删除/重命名、编码覆盖、原生 worker 隔离与真实取消；归档路径与链接防护、私有暂存、提交日志与异常恢复、Windows 长路径与 MOTW 传播；设置白名单迁移与自定义颜色 token（不执行旧 CSS）；六主题、18 语言、RTL。

发行包为 C++17 + Qt Quick 6.8.3 + bit7z 4.1.0 + 完整 7-Zip 26.04 动态库，不含 Electron 或 7z 命令行运行依赖。

## 已知限制

Win11 现代右键菜单、Finder/Nautilus shell 扩展、SFX 创建、密码恢复、复杂损坏修复未实现；这些需要 COM 扩展、签名证书与长期独立维护。完整清单见[已知限制](known-limitations.md)，逐条能力对照见[功能对照](feature-parity.md)。体积与启动数据见[性能记录](size-and-performance.md)。
