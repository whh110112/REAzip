# 所有 Sol 任务共用的执行规则

## 开工前

1. 阅读 `00-START-HERE.md`、`02-CONTRACTS.md`、`03-REGRESSION-MATRIX.md` 和当前任务书，检查实际项目适用的 AGENTS.md。
2. 检查 git 状态、实际 HEAD、未提交修改。基线是 163e17a，但 Workbuddy 可能继续开发；列出差异，吸收需要保留的有效功能，禁止 reset/clean 覆盖用户工作。
3. 用户仅执行当前任务书时，不要自动启动所有下游任务。缺少前置交接单时先验证依赖，不能凭文字“已完成”推断实现存在。
4. 实现放 `native/`，实验产物放被忽略的 build/evidence 目录；旧 Electron 源码作只读对照，根目录和旧 CI 的切换由 T09/T10 处理。
5. 单任务使用独立提交；多聊天协作使用独立 worktree。公共接口变更先记录影响，集成负责人统一落地，禁止各自发明同名不同义接口。

## 工程边界

- 生产归档操作必须调用原生库，不允许偷偷回到 Node spawn、7z CLI 或 tar.exe。
- C++ 是业务和归档实现语言；QML 仅表达界面/绑定。不得用 QtWebEngine 包裹旧网页来宣布完成原生迁移。
- 不照搬旧错误：路径、密码、冲突、加密算法、取消和架构问题要由回归测试证明修复。
- 测试中允许用独立的官方 7-Zip CLI 检查产物；不允许生产路径依赖此测试工具。
- 文件关联、安装/卸载、默认应用测试只在测试用户/虚拟机/CI 中执行，不能无提示更改开发者当前默认应用。
- 自己生成的归档样本可写入临时目录；测试必须有断言和失败退出码。禁止只打印 ALL PASS。
- 不需要为每个 getter 写镜像测试；重点覆盖文件内容、数据完整性、协议状态和平台集成。
- Qt 具体版本、平台 minimum deployment target、7-Zip 源哈希和 bit7z 版本以 T01 锁文件为准。不得写成运行时联网获取 latest。
- 禁止提交密钥、签名证书、私有临时文档或原版 Bandizip 可执行文件。有用的逆向行为记录可以独立整理为自有文档。

## 文件归属

| 范围 | 主责 |
|---|---|
| native/CMakeLists.txt、native/include/reazip/*、依赖锁与顶层 preset | T01；后续公共变更由集成人处理 |
| native/engine/read、tests/engine-read | T02 |
| native/engine/write、tests/engine-write | T03 |
| native/worker、native/jobs、tests/jobs | T04 |
| native/ui、tests/ui、设计 token | T05 |
| native/app、native/models、tests/integration | T06 |
| native/platform、tests/platform | T07 |
| native/settings、native/i18n、native/themes、tests/settings | T08 |
| native/packaging、CI、部署脚本、LICENSES | T09 |
| 验收脚本和报告、发布准备材料 | T10 |

子目录可以有自己的 CMakeLists.txt；不要并发争改根构建文件。T01 应预留扩展点。发现其他模块缺陷时先记录，再由负责人安排修复；不跨所有者目录大规模重构。

## 完成定义与交接

每个任务写 `native/docs/handoffs/Txx.md`，包含：

- 基线 SHA、实现提交 SHA、修改文件、对外接口变化。
- 功能完成项与仍缺项，禁止把占位按钮/桩实现标成完成。
- 本机/CI 具体 OS、CPU 架构、命令、退出码和证据链接。
- 回归矩阵 Case ID 对应结果：PASS、FAIL、NOT RUN、BLOCKED。
- 新依赖、运行库、体积影响和后续任务需注意的事项。
- 下一任务可以直接使用的构建/运行命令。

任务结束给用户中文简报：已做什么、测到什么、有哪些未验证、下一步是什么。CI 是编译还是运行通过必须区分。

## 发布边界

这些任务书授权编制方案，不等于现在实施或自动公开新版本。用户把任务书交给 Sol 后，按该任务实施范围执行。普通工程任务默认不建 Release、不改旧标签；T09/T10 只生成候选包/草稿与发布说明，用户明确要求发布时才公开。不要因为已有 v0.2.1 发布授权就自行发布未来的原生版本。
