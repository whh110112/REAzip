# ZENzip 0.3.1-dev 原生候选发布说明

**这是 pre-release（候选预发布），不是 T10 正式验收通过版本。** 自动化门禁在本次提交上跑六目标；人工项仍未运行：设置页并排截图与缩放/长译文目视签收、真实桌面文件关联与右键菜单点击、跨应用拖出、大包压力与干净物理机冷启动。详见[验收报告](acceptance-report.md)与[已知限制](known-limitations.md)。

## 本轮新增

**密码库（设置 → 密码）。** 保存常用密码，创建归档与解压密码弹窗可直接选用，并可把刚输入的密码存入库中。

- Windows：条目列表整体用 `CryptProtectData` 加密，附加固定应用熵；只有同一 Windows 账户在同一台机器上能解开，不需要主密码，也不需要我们自己保存任何密钥。
- macOS / Linux：没有免依赖的系统密钥库接入，因此以 `0600` 保存并在界面显示"未加密，仅受文件权限保护"——不把权限保护说成加密。
- 文件是原子写入；损坏文件改名为 `vault.json.corrupt-<ms>` 保留；其他平台写入的 DPAPI 库在本机不可解密时整库置为只读，拒绝覆盖；便携格式（`protection: permissions`）在 Windows 上首次保存即升级为 DPAPI。

**Windows 经典右键菜单。** 每个受支持扩展名的 `SystemFileAssociations` 下注册"解压到此处"和"解压到…"，带图标、`MultiSelectModel`（前者可多选）与所有权标记；`*` 与 `Directory` 下注册"添加到压缩包"，同样带图标与多选。扩展名从 11 个扩到 22 个（zipx/cab/lz/lzma/zst/zstd/arj/lzh/tbz/txz 等）。另注册 `Applications\zenzip.exe`（`FriendlyAppName`、`DefaultIcon`、`SupportedTypes`），让"打开方式"列表里能看到 ZENzip。**Windows 11 的现代右键菜单未实现**：它需要 `IExplorerCommand` COM 服务器 + 包标识 + 代码签名，本轮不塞假入口，动词出现在"显示更多选项"里。

**三平台文件关联。**

- Windows：ProgID `ZENzip.Archive` + `Capabilities\FileAssociations` + `RegisteredApplications` + 每扩展名 `OpenWithProgids`；注册后打开系统"默认应用"设置，不代用户改默认值。
- macOS：`Info.plist` 增加 `CFBundleTypeExtensions`、`LSHandlerRank=Default`、`LSSupportsOpeningDocumentsInPlace`，UTI 扩到 9 个（zip/7z/rar/tar/gz/bz2/xz/iso/wim）。默认处理程序只能由用户在 Finder「显示简介 → 打开方式 → 全部更改」确认。
- Linux：`.desktop` 增加 `TryExec`、`Actions`（解压到此处 / 解压到…）、15 个 MIME 类型，并在写入后调用 `update-desktop-database` 与 `xdg-mime default`，所以是"已选中"而不只是"已安装"。
- 设置页"文件关联"现在显示真实状态（ProgID、动词数量与图标数、默认应用项、Win11 说明；Linux 上为 desktop 项状态与 `xdg-mime` 查询结果），可随时刷新。

## 验证范围

- 新增 `native-vault` 测试：校验拒绝、Windows 密文断言（文件中不得出现明文）、非 Windows 的 0600 权限断言、二次加载、删除、损坏文件保留、便携库导入与 Windows 升级、DPAPI 库在非 Windows 上拒写且不修改原文件。
- `native-platform` 扩展断言：Windows 上检查动词图标、`MultiSelectModel`、`Applications/SupportedTypes`、摘要包含现代菜单说明，并覆盖卸载不留残留；Linux 上检查 `%F`、`MimeType=application/zip;`、`Actions=ExtractHere;ExtractTo;`、`--extract-here %f` 与摘要内容。
- 六目标构建与 CTest 结果、各包 SHA-256 见本次 Release 附件中的 `*-manifest.json` 与 `SHA256SUMS.txt`。

## 下载

Release 附件包含六目标全部产物：Windows x64/ARM64 安装 EXE + 便携 ZIP、macOS x64/ARM64 DMG + ZIP、Linux x64/ARM64 AppImage + deb + tar.gz，外加各平台 `*-manifest.json` 与总 `SHA256SUMS.txt`。校验方式：`sha256sum -c SHA256SUMS.txt`。

**签名状态**：Windows 包**未签名**（首次运行会遇到 SmartScreen 提示）；macOS 为 ad-hoc 签名、**未公证**；Linux 包未做 GPG 签名。这是事实，不是遗漏说明。

## 主要能力

多标签浏览、目录搜索、ZIP/7Z/TAR/TGZ 创建与解压、选中条目冲突策略、密码挑战与算法元数据、密码库、添加/删除/重命名、编码覆盖、原生 worker 隔离与真实取消；归档路径与链接防护、私有暂存、提交日志与异常恢复、Windows 长路径与 MOTW 传播；设置白名单迁移与自定义颜色 token（不执行旧 CSS）；六主题、18 语言（166 个 key）、RTL。

发行包为 C++17 + Qt Quick 6.8.3 + bit7z 4.1.0 + 完整 7-Zip 26.04 动态库，不含 Electron 或 7z 命令行运行依赖。

## 已知限制

Win11 现代右键菜单、Finder Quick Action / Nautilus 扩展、SFX 创建、密码恢复、复杂损坏修复未实现；这些需要 COM 扩展、签名证书与长期独立维护。密码库在 macOS/Linux 未接入系统密钥库。完整清单见[已知限制](known-limitations.md)，逐条能力对照见[功能对照](feature-parity.md)。
