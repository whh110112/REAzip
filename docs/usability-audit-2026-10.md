# ZENzip usability and settings audit · 2026-10-18

本次以 ZENzip 0.4.0 的 `main` 为基线，检索 Bandizip、NanaZip、7-Zip 的公开反馈，再检查自己的实现。投诉是具体用户、版本与场景的反馈，不是市场占比，也不等于竞品当前版本仍有同样问题。下面的修正属于后续源码变更；已发布的 0.4.0 安装包不会自动改变。

## 用户反馈与本项目对照

| 反馈与原始来源 | 来源时点、状态 | ZENzip 核查与处理 |
| --- | --- | --- |
| Bandizip 免费版广告、功能分层：[官方版本比较](https://en.bandisoft.com/bandizip/help/edition-comparison/)、[中文用户讨论](https://www.v2ex.com/t/629683) | 2019 年讨论是历史背景；当前官网仍明确区分广告与付费功能 | 原生界面、打包脚本没有广告 SDK、付费开关或捆绑下载。保持离线包和少干扰界面；这不构成对全部第三方依赖的安全审计。 |
| 7-Zip 暗色模式需求：[官方讨论区](https://sourceforge.net/p/sevenzip/discussion/45797/thread/768a550c16/) | 2025 年提出，讨论持续到 2026 年 | 已有六套即时切换配色，但实际发现 Qt Basic 下拉箭头仍取默认 `palette.dark`，在夜墨/青石中对比不足；本次将该角色绑定到皮肤的次要文字色。 |
| NanaZip 深色控件、地址栏和 Mica 不一致：[issue #469](https://github.com/M2Team/NanaZip/issues/469) | 反馈对象是 3.0.1000.0；检索时 issue 仍 open。[6.0 发布说明](https://github.com/M2Team/NanaZip/releases/tag/6.0.1630.0)已有部分主题/布局修复 | 不推断当前 NanaZip 完整复现。ZENzip 使用统一调色板；本次同时修正设置导航截字、RTL 复选框文字与勾选框重叠，六皮肤使用同一份响应式布局。 |
| 拖出先占临时盘、再复制，速度和空间不符合预期：[7-Zip 官方 FAQ](https://7-zip.org/faq.html)、[用户报告 #2056](https://sourceforge.net/p/sevenzip/bugs/2056/) | FAQ 解释 Explorer 拖放为何需要落地文件；报告是历史问题 | **仍有相同机制**：`AppController::prepareDrag()` 先解压到配置目录的 `temporary/drag-*`，再由 `QDrag` 提供文件 URL。正常退出清理、启动清理旧目录不能消除双重空间与复制。大包应直接“解压到”。自选临时盘、预检可用空间、原生延迟数据拖放列为后续工作，不能宣称已解决。 |
| NanaZip 不能把归档图片直接拖入 GIMP：[issue #698](https://github.com/M2Team/NanaZip/issues/698) | 2025 年、5.0.1263.0，检索时仍 open | ZENzip 有准备拖出和 URL MIME 实现，但 Explorer/GIMP/其他应用之间的交互尚未全面验收。代码存在不能证明跨应用兼容。 |
| NanaZip 某些压缩比 7-Zip 慢：[issue #69](https://github.com/M2Team/NanaZip/issues/69) | 2021 年报告，后续讨论涉及样本、方法差异；2026-02 标记 fixed/closed | 不把“四倍慢”作为当前结论。ZENzip 使用同系 7-Zip 引擎，但不能因此承诺同速；需要同格式、等级、线程、样本、冷热缓存的对照。十万条模型测试不是十万文件归档端到端性能证明。 |
| NanaZip 取消后再次压缩时临时文件异常增长：[issue #680](https://github.com/M2Team/NanaZip/issues/680) | 特定 LZ4 场景；2025-10 已 fixed/closed | ZENzip 当前不提供该 LZ4 创建选项，因此不能说复现同一错误。保留取消后再次操作、事务恢复和真实磁盘满测试；不要将不同编码器的问题混为一谈。 |
| 中文社区看重智能解压、旧 ZIP 编码，反感多余目录和乱码：[V2EX 原讨论](https://www.v2ex.com/t/629683) | 用户诉求，非当前版本缺陷统计 | 已有智能解压与 GBK/Big5/Shift-JIS/UTF-8 选择；发现设置页编码下拉框未回显已保存值。本次绑定真实配置，并覆盖重启恢复和运行时更新。自动编码判断仍不能保证每个历史 ZIP 正确，须保留手选。 |
| Windows 11 右键菜单需要多一步：[7-Zip 用户讨论](https://sourceforge.net/p/sevenzip/discussion/45797/thread/14f6621a45/) | 系统菜单集成诉求 | **仍有限制**：ZENzip 只有经典菜单，位于“显示更多选项”；现代菜单需要独立平台实现与签名。本次只让关联页面说明完整可见，不以 UI 修正冒充功能完成。 |

检索覆盖 Exa、GitHub issue/发布说明、官网 FAQ、SourceForge 与中文 V2EX；Reddit 使用公开搜索索引补查，没有登录态后端，未把搜索摘要当作已读完整评论。未声称穷尽全部平台或手工复现三个竞品。

## 设置界面的实际修正

- **导航截字**：原侧栏固定宽度，德语 `Zuordnungen` 实际显示为 `Zuordnu…`。固定可用导航宽度并允许文字换行，长标签不再依赖省略号。
- **内容可达性**：常规路径输入改为标签在上、输入在下。六个子页均独立纵向滚动，弹窗最大高度受窗口约束，操作按钮留在固定底部；滚动页为滚动条预留宽度。
- **保存状态回显**：语言与 ZIP 编码原先停留在模型第一项。现在绑定后端真实设置；主题列表明确标出当前主题。
- **暗色箭头**：Qt 6.8.3 Basic ComboBox 的 indicator 使用 `palette.dark`；补齐主题角色，避免暗色背景上的默认暗箭头。
- **输入框字高**：Windows ARM64 原生测试稳定复现印地语密码页两处 placeholder 内容高 34px、内框只有 24px，六皮肤都出现。取消控件固定高度，改为背景提供 36px 最小高度，保留 Qt 对内容和 placeholder 的自动计算。
- **跨平台字高**：macOS ARM64 回归在字体切换的中间帧观察到内容高度超过标签高度；这不能证明稳定后仍裁切。设置标签增加实际内容高度约束，测试等待字体回退与布局 polish 稳定后再判定。
- **RTL**：阿拉伯语复选框的文字留白从左侧改到实际指示器所在的右侧，修正文字/勾选框重叠。
- 中英文 README 列出全部 **18 种语言及代码**，保留各一张对应语言主截图。

## 验证方式与证据范围

`native/tests/integration/gui.cpp::settingsLayoutAndSavedSelections` 使用真实 AppController、真实翻译/主题资源和实际 QML：六皮肤 × 18 语言 × 六子页，共 **648 个设置布局**，窗口为最小 880×560。检查文字截断、文字内容尺寸、横向越界、弹窗高度、RTL 指示器留白、语言/编码回显与重新打开。测试另先写配置、销毁控制器，再重新加载，避免只测手工构造值。

`native/tools/capture-native.py --full` 捕获实际编译应用：六皮肤的中英同场景，以及中英德阿的 100/125/150/200% 设置子页。图片与 stderr、源码 SHA、字体 SHA、清单一起留在 Actions 附件，不加入 main 的截图展示目录。

本地 Qt 6.8.3 先运行真实 QML 的隔离布局探针，四档缩放（100/125/150/200%）各 648 项、合计 2,592 项检查无截断或横向溢出；探针数据为模拟后端，不能代替上述原生集成测试。离屏字体加载仍有 OpenType fallback 诊断；抽查的阿拉伯语、泰语、印地语截图有实际字形，但不能据此宣称所有字符与复杂排版均正确。人工抽查了夜墨关联页、青石阿拉伯语常规页与德语最小窗口，并据此修正了箭头和 RTL 问题。不能把“648 项自动几何检查”表述为“648 张截图全部人工验收”。

颜色计算采用 sRGB 相对亮度：六主题正文对卡片背景最低 **11.00:1**，次要文字最低 **5.86:1**，强调色最低 **5.56:1**。这只核查这些颜色对，不代表完整 WCAG/屏幕阅读器验收。禁用状态、半透明混合、焦点、系统高对比模式仍需单独验证。

## 本次原生验证结果

[Native candidate 39131074544](https://github.com/whh110112/ZENzip/actions/runs/39131074544) 已全部通过。测试源码提交为 `ff593fa5156b60e3c71415364a1e7ccd148e1627`，`native/` 树为 `ff00de26aa7645abb20cc1cd535526388a11e589`；其后的补充只修改根 README 与本报告，原生代码树一致。

| 目标 | CTest | 设置布局 |
| --- | --- | --- |
| Windows x64 | 15/15 | 648 项通过 |
| Windows ARM64 | 15/15 | 648 项通过，含印地语 placeholder 回归 |
| macOS x64 | 15/15 | 648 项通过 |
| macOS ARM64 | 15/15 | 648 项通过，按稳定后的字体/布局尺寸检查 |
| Linux x64 | 15/15 | 648 项通过 |
| Linux ARM64 | 15/15 | 648 项通过 |

实际编译应用的 192 张截图已下载并通过 ZIP 完整性核对；清单源码 SHA 与上述提交一致，192 份 stderr 均为空。人工补查六款皮肤的常规页、深色外观页、德语 150% 关联页、阿拉伯语 200% 常规页与英语语言选择页，未见本次修复项残留。

六目标打包/隔离运行及汇总也通过。本次没有替换公开 0.4.0 发布资产，也不把本轮设置检查算作完整 Explorer/Finder/GIMP 交互验收。

## 后续优先级

1. 临时盘可配置、拖出空间预检，并用大包验证取消/崩溃后的回收；跨应用拖放逐项实测。
2. Windows 11 现代菜单；macOS/Linux 默认关联与桌面操作的交互验收。
3. 同参数真实大包/十万文件性能基准；把 CPU 占用、磁盘吞吐与总耗时分开呈现。
4. macOS Keychain/Linux Secret Service；当前权限保护密码库仍不是加密密码库。
5. 18 语言母语审校与缺字检测，完整键盘/屏幕阅读器和系统缩放验收。字典键齐全不能保证译文与字形正确。
6. 关联页的平台注册诊断仍含英语（如 `not registered`、`missing`），尽管导航、按钮和解释已本地化。后续应给诊断增加本地化呈现层，保留技术标识用于排错；不把 18 份字典等同于所有底层消息均已翻译。

## English summary

The audit fixes settings navigation truncation, bounded scrollable pages, saved language/codepage selection display, dark-theme ComboBox indicators, an RTL checkbox collision and Hindi input-placeholder clipping on Windows ARM64. Both READMEs enumerate all eighteen locales. The native regression matrix covers 648 layouts; screenshots stay in CI artifacts. Temp-file duplication during drag-out, modern Windows menus, cross-application drag compatibility, full translation review and equal-parameter performance comparisons remain explicit follow-up work. Historical competitor bugs marked fixed are not presented as current failures.
