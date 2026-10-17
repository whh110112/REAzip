# 公共接口与兼容约定（T00 确认，T01 落地）

此文件定义语义，不绑定未经验证的 bit7z 方法名。T00 可以明确细节，T01 编译成公共头与协议 schema；后续任务必须使用同一份类型。

## 模块

- archive-core：普通 C++ 类型与引擎接口；不引用 QML/UI，不调用外部命令。
- archive-worker：QtCore 或标准库实现的独立控制进程；调用 archive-core。包含 engine build identity 查询命令。
- job-controller：GUI 内的任务队列、worker 生命周期、密码和冲突事件路由。
- app/models：QAbstractItemModel 与 C++ 控制器；面向 QML 暴露视图数据。
- platform-services：文件对话框、打开/显示文件、实例转发、文件关联、便携路径。
- settings/themes/i18n：与界面解耦的数据服务。

## 数据类型

- EngineIdentity：引擎版本、bit7z 版本、OS、arch、构建 ID、实际加载库路径、能力列表；不只展示包版本。
- ArchiveInfo：格式、物理体积、卷信息、加密信息；未知算法显示 unknown，不能根据 encrypted=true 推断 AES。
- ArchiveEntry：稳定 entryId、原始索引、显示路径、entryKind、size/packedSize（可未知）、修改时间、加密算法/状态、可用操作。没有单独目录条目的路径也能生成虚拟目录。
- ArchiveHandle：规范化路径、修改时间/大小或指纹、只读状态；每次修改/刷新后旧 entryId 失效。路径名不是条目的唯一标识，考虑重复名称。
- JobRequest：协议版本、jobId、operation、归档路径、目标路径、entryIds、typed options。
- CreateOptions：格式、方法、级别、密码策略、文件头加密、分卷、是否在成功校验后删源；不支持的组合明确拒绝。
- ExtractOptions：所选条目/全包、目标、是否智能建目录、覆盖策略、代码页、路径/链接处理策略。
- JobError：稳定错误码、用户消息键、诊断 detail、recoverable；禁止只解析英文错误文本分类。
- 错误至少包含 PasswordRequired、WrongPassword、UnsupportedFormat、UnsupportedOperation、EngineMissing、EngineArchitectureMismatch、PermissionDenied、CorruptArchive、UnsafeEntry、Cancelled、IoError。

## worker 协议 v1

使用父进程创建的私有 stdin/stdout 管道；Qt QProcess 启动自身 worker，不能经过 shell。命令行只含运行模式/诊断选项，不含密码。stdout 只放协议消息，stderr 放已脱敏诊断，不开本地 TCP 端口。

先选 UTF-8 NDJSON（每行一个 JSON 对象）。每条消息含 `protocolVersion`、`jobId`、`type`，响应另含 sequence。字段中的换行必须被 JSON 转义。单条消息大小上限 16 MiB；列表分块（建议每块 512 条），禁止一次输出整个大目录。管道断流、半包、非法类型、版本不匹配必须显式失败。

64 位字节数、可能超过 JSON 精确数值范围的索引使用十进制字符串；时间用带时区的统一格式。文件系统路径使用平台可往返的转换层；不可解码文件名需要明确处理，不能静默改名后覆盖其他文件。

请求：hello / submit / respondChallenge / cancel。事件：ready / started / entriesChunk / progress / passwordRequired / overwriteRequired / warning / completed / failed / cancelled。

- 每个挑战有独立 challengeId，答复必须匹配 jobId 和 challengeId；关闭密码/冲突框视为取消，而不是空密码或覆盖。
- worker 工作线程做归档，控制线程继续读取取消/挑战回应，避免库调用阻塞后死锁。
- 密码可经管道内存传递，不进入日志、argv、磁盘、崩溃报告附件或持久设置；任务终结后清理保存的副本。不宣称标准字符串能保证所有内存副本可靠擦除。
- 状态：Queued → Running ↔ AwaitingPassword/AwaitingOverwrite → Succeeded/Failed/Cancelled；Cancelling 是 Running 到 Cancelled 的中间状态；每任务只产生一个终态。
- 正常取消用库进度回调/取消令牌；超时仅终止该任务 worker，并清理暂存文件；不能杀死别的任务或 UI。
- 重任务初始串行，列表/预览可独立；同一归档写操作排他锁，读写互斥。避免查询进程覆盖全局 currentProc。

## 文件操作规则

- 解压选择基于条目 ID/索引，不能把用户条目名解释为通配符或参数。目录选择包括其后代，兼容隐式目录。
- ask/overwrite/skip/rename 四种覆盖策略同时适用于整包、选中解压和目录；遇到未知冲突不得默认覆盖。
- 目标内每个条目的路径必须验证：拒绝父目录逃逸、绝对路径/盘符/UNC 注入；检测符号链接、重解析点和硬链接导致的目标逃逸；暂存/落盘前再次检查，不能只做字符串 startsWith。
- 默认不允许从不可信归档创建能跳出目标目录的链接；显式记录被拒条目和理由。
- 新归档/修改归档先写同卷临时文件，校验成功后提交；取消/错误不得损坏原归档。分卷按文件集合维护提交/回滚记录，不假设多个文件能原子替换。
- 删源仅发生在输出成功提交和验证之后；取消、警告导致的漏文件、源文件期间被修改、校验失败时不得删源。优先使用可恢复删除，必须记录失败项。
- TAR.GZ 采用库内 TAR 与 GZIP 两阶段处理，解包最后输出真实文件；选项不支持时禁用/报错，不能悄悄忽略。
- 精确保留用户选择的输入与归档内部相对路径映射；不泄露本机盘符/用户名目录，不吞重名冲突。

## UI 与兼容

- 保留窗口 1120×740、最小 880×560 的初始设计基线；按 DPI 使用逻辑尺寸；系统标题按钮遵循平台。
- 界面元素 ID/动作名由 T00 冻结；T05 可用假数据模型，T06 接真实服务。
- 表格虚拟化，归档列表与排序过滤在 C++ 模型中处理，QML 不重复扫描整个压缩包数组。
- 18 语言保留 key，Qt 系统对话框也需正确本地化；Mac 主快捷键用 Cmd，Windows/Linux 用 Ctrl。
- 旧设置迁移只导入白名单字段并保留备份；不导入密码。用户自定义 CSS 不执行，说明可迁移 token 与不兼容项。
- 便携模式用明确标记文件启用；安装模式走平台标准应用数据目录。可执行目录只读时应给出明确错误/用户选择，不悄悄丢配置。
