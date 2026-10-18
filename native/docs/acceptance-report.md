# ZENzip 0.4.1 验收范围

以[0.4.1 发布验证报告](../../docs/verification-0.4.1.md)和本次 Release 的源码 SHA、六目标清单及 SHA256SUMS.txt 为准。

PR #3 纳入设置页多语言布局、保存值回显、暗色下拉箭头和 RTL 修正；PR #4 纳入标签字体与图标居中、字高自适应和真实点击回归。

原生 GUI 自动检查覆盖每目标 648 个设置布局和 108 个标签布局。六目标 CTest 另检查读写、密码与冲突、取消、路径安全、异常恢复、真实磁盘满及大文件。安装包还需通过架构和隔离运行检查，Windows 需通过安装注册与卸载检查。

实际 Explorer/Finder/Linux 桌面的全量手工交互、跨应用拖出、全部译文母语审校与辅助功能验收仍有缺口，见[已知限制](known-limitations.md)。历史报告和失败日志保留在 [Git 历史](https://github.com/whh110112/ZENzip/tree/601e6048824c0b36fa28267970833494f98a137d/native/docs)，不作为当前失败或当前通过的证据。
