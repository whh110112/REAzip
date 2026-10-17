# 可复制到 GPT-6.1 Sol 的启动提示词

请在模型选择中选 GPT-6.1 Sol。下列提示词不会自动创建聊天；由用户选择一个聊天逐项执行，或按总览中的四条线分配给独立聊天。多聊天各用独立工作目录，前置任务结果经集成负责人合入共同基础后再开下游。

## 建议的首条消息

先复制 T00 的提示词。不要一开始要求 Sol 同时做完全部 11 项。T01 完成并有六目标引擎验证证据后，再开展后续实现。

## T00：行为与视觉基线

前置依赖：无。

```text
请执行 REAzip 原生迁移任务 T00。
任务书：D:\Workbuddy\2026-10-13-14-25-42\REAzip-Cpp-migration\tasks\T00-baseline.md
先读同一任务包中的 00-START-HERE.md、01-EXECUTION-RULES.md、02-CONTRACTS.md 和 03-REGRESSION-MATRIX.md，再读取当前任务。
原项目路径：D:\Workbuddy\2026-10-13-14-25-42\reazip。
检查当前 Git 状态及前置任务交接单，保护 Workbuddy 的未提交改动。按任务书实现和验收当前任务，保存 native/docs/handoffs/T00.md。多聊天协作时使用独立 worktree，不直接并发修改原检出目录。
技术路线为 C++17 + Qt Quick/QML + bit7z + 完整 7-Zip 原生动态库，保留现有界面设计。不要改回 Electron/QtWebEngine，也不要把调用 7z 命令行当作库接入完成。
只推进本任务范围，必要缺陷修复纳入本任务；不要自动创建新聊天或发布公开 Release。报告实际完成、验证环境、未运行/阻塞项和下一步。
```

## T01：原生工程与六目标可行性

前置依赖：T00。

```text
请执行 REAzip 原生迁移任务 T01。
任务书：D:\Workbuddy\2026-10-13-14-25-42\REAzip-Cpp-migration\tasks\T01-foundation.md
先读同一任务包中的 00-START-HERE.md、01-EXECUTION-RULES.md、02-CONTRACTS.md 和 03-REGRESSION-MATRIX.md，再读取当前任务。
原项目路径：D:\Workbuddy\2026-10-13-14-25-42\reazip。
检查当前 Git 状态及前置任务交接单，保护 Workbuddy 的未提交改动。按任务书实现和验收当前任务，保存 native/docs/handoffs/T01.md。多聊天协作时使用独立 worktree，不直接并发修改原检出目录。
技术路线为 C++17 + Qt Quick/QML + bit7z + 完整 7-Zip 原生动态库，保留现有界面设计。不要改回 Electron/QtWebEngine，也不要把调用 7z 命令行当作库接入完成。
只推进本任务范围，必要缺陷修复纳入本任务；不要自动创建新聊天或发布公开 Release。报告实际完成、验证环境、未运行/阻塞项和下一步。
```

## T02：原生读取、密码与编码

前置依赖：T01。

```text
请执行 REAzip 原生迁移任务 T02。
任务书：D:\Workbuddy\2026-10-13-14-25-42\REAzip-Cpp-migration\tasks\T02-engine-read.md
先读同一任务包中的 00-START-HERE.md、01-EXECUTION-RULES.md、02-CONTRACTS.md 和 03-REGRESSION-MATRIX.md，再读取当前任务。
原项目路径：D:\Workbuddy\2026-10-13-14-25-42\reazip。
检查当前 Git 状态及前置任务交接单，保护 Workbuddy 的未提交改动。按任务书实现和验收当前任务，保存 native/docs/handoffs/T02.md。多聊天协作时使用独立 worktree，不直接并发修改原检出目录。
技术路线为 C++17 + Qt Quick/QML + bit7z + 完整 7-Zip 原生动态库，保留现有界面设计。不要改回 Electron/QtWebEngine，也不要把调用 7z 命令行当作库接入完成。
只推进本任务范围，必要缺陷修复纳入本任务；不要自动创建新聊天或发布公开 Release。报告实际完成、验证环境、未运行/阻塞项和下一步。
```

## T03：解压、压缩与归档修改

前置依赖：T02。

```text
请执行 REAzip 原生迁移任务 T03。
任务书：D:\Workbuddy\2026-10-13-14-25-42\REAzip-Cpp-migration\tasks\T03-engine-write.md
先读同一任务包中的 00-START-HERE.md、01-EXECUTION-RULES.md、02-CONTRACTS.md 和 03-REGRESSION-MATRIX.md，再读取当前任务。
原项目路径：D:\Workbuddy\2026-10-13-14-25-42\reazip。
检查当前 Git 状态及前置任务交接单，保护 Workbuddy 的未提交改动。按任务书实现和验收当前任务，保存 native/docs/handoffs/T03.md。多聊天协作时使用独立 worktree，不直接并发修改原检出目录。
技术路线为 C++17 + Qt Quick/QML + bit7z + 完整 7-Zip 原生动态库，保留现有界面设计。不要改回 Electron/QtWebEngine，也不要把调用 7z 命令行当作库接入完成。
只推进本任务范围，必要缺陷修复纳入本任务；不要自动创建新聊天或发布公开 Release。报告实际完成、验证环境、未运行/阻塞项和下一步。
```

## T04：worker 与任务调度

前置依赖：T01。

```text
请执行 REAzip 原生迁移任务 T04。
任务书：D:\Workbuddy\2026-10-13-14-25-42\REAzip-Cpp-migration\tasks\T04-worker-jobs.md
先读同一任务包中的 00-START-HERE.md、01-EXECUTION-RULES.md、02-CONTRACTS.md 和 03-REGRESSION-MATRIX.md，再读取当前任务。
原项目路径：D:\Workbuddy\2026-10-13-14-25-42\reazip。
检查当前 Git 状态及前置任务交接单，保护 Workbuddy 的未提交改动。按任务书实现和验收当前任务，保存 native/docs/handoffs/T04.md。多聊天协作时使用独立 worktree，不直接并发修改原检出目录。
技术路线为 C++17 + Qt Quick/QML + bit7z + 完整 7-Zip 原生动态库，保留现有界面设计。不要改回 Electron/QtWebEngine，也不要把调用 7z 命令行当作库接入完成。
只推进本任务范围，必要缺陷修复纳入本任务；不要自动创建新聊天或发布公开 Release。报告实际完成、验证环境、未运行/阻塞项和下一步。
```

## T05：QML 视觉迁移

前置依赖：T01。

```text
请执行 REAzip 原生迁移任务 T05。
任务书：D:\Workbuddy\2026-10-13-14-25-42\REAzip-Cpp-migration\tasks\T05-native-ui.md
先读同一任务包中的 00-START-HERE.md、01-EXECUTION-RULES.md、02-CONTRACTS.md 和 03-REGRESSION-MATRIX.md，再读取当前任务。
原项目路径：D:\Workbuddy\2026-10-13-14-25-42\reazip。
检查当前 Git 状态及前置任务交接单，保护 Workbuddy 的未提交改动。按任务书实现和验收当前任务，保存 native/docs/handoffs/T05.md。多聊天协作时使用独立 worktree，不直接并发修改原检出目录。
技术路线为 C++17 + Qt Quick/QML + bit7z + 完整 7-Zip 原生动态库，保留现有界面设计。不要改回 Electron/QtWebEngine，也不要把调用 7z 命令行当作库接入完成。
只推进本任务范围，必要缺陷修复纳入本任务；不要自动创建新聊天或发布公开 Release。报告实际完成、验证环境、未运行/阻塞项和下一步。
```

## T06：操作集成与缺口补齐

前置依赖：T02/T03/T04/T05/T07/T08。

```text
请执行 REAzip 原生迁移任务 T06。
任务书：D:\Workbuddy\2026-10-13-14-25-42\REAzip-Cpp-migration\tasks\T06-integration.md
先读同一任务包中的 00-START-HERE.md、01-EXECUTION-RULES.md、02-CONTRACTS.md 和 03-REGRESSION-MATRIX.md，再读取当前任务。
原项目路径：D:\Workbuddy\2026-10-13-14-25-42\reazip。
检查当前 Git 状态及前置任务交接单，保护 Workbuddy 的未提交改动。按任务书实现和验收当前任务，保存 native/docs/handoffs/T06.md。多聊天协作时使用独立 worktree，不直接并发修改原检出目录。
技术路线为 C++17 + Qt Quick/QML + bit7z + 完整 7-Zip 原生动态库，保留现有界面设计。不要改回 Electron/QtWebEngine，也不要把调用 7z 命令行当作库接入完成。
只推进本任务范围，必要缺陷修复纳入本任务；不要自动创建新聊天或发布公开 Release。报告实际完成、验证环境、未运行/阻塞项和下一步。
```

## T07：系统打开与关联

前置依赖：T01。

```text
请执行 REAzip 原生迁移任务 T07。
任务书：D:\Workbuddy\2026-10-13-14-25-42\REAzip-Cpp-migration\tasks\T07-os-integration.md
先读同一任务包中的 00-START-HERE.md、01-EXECUTION-RULES.md、02-CONTRACTS.md 和 03-REGRESSION-MATRIX.md，再读取当前任务。
原项目路径：D:\Workbuddy\2026-10-13-14-25-42\reazip。
检查当前 Git 状态及前置任务交接单，保护 Workbuddy 的未提交改动。按任务书实现和验收当前任务，保存 native/docs/handoffs/T07.md。多聊天协作时使用独立 worktree，不直接并发修改原检出目录。
技术路线为 C++17 + Qt Quick/QML + bit7z + 完整 7-Zip 原生动态库，保留现有界面设计。不要改回 Electron/QtWebEngine，也不要把调用 7z 命令行当作库接入完成。
只推进本任务范围，必要缺陷修复纳入本任务；不要自动创建新聊天或发布公开 Release。报告实际完成、验证环境、未运行/阻塞项和下一步。
```

## T08：设置、语言与主题迁移

前置依赖：T01。

```text
请执行 REAzip 原生迁移任务 T08。
任务书：D:\Workbuddy\2026-10-13-14-25-42\REAzip-Cpp-migration\tasks\T08-settings-i18n.md
先读同一任务包中的 00-START-HERE.md、01-EXECUTION-RULES.md、02-CONTRACTS.md 和 03-REGRESSION-MATRIX.md，再读取当前任务。
原项目路径：D:\Workbuddy\2026-10-13-14-25-42\reazip。
检查当前 Git 状态及前置任务交接单，保护 Workbuddy 的未提交改动。按任务书实现和验收当前任务，保存 native/docs/handoffs/T08.md。多聊天协作时使用独立 worktree，不直接并发修改原检出目录。
技术路线为 C++17 + Qt Quick/QML + bit7z + 完整 7-Zip 原生动态库，保留现有界面设计。不要改回 Electron/QtWebEngine，也不要把调用 7z 命令行当作库接入完成。
只推进本任务范围，必要缺陷修复纳入本任务；不要自动创建新聊天或发布公开 Release。报告实际完成、验证环境、未运行/阻塞项和下一步。
```

## T09：打包与发布门禁

前置依赖：准备依赖T01，最终依赖T03/T04/T06/T07/T08。

```text
请执行 REAzip 原生迁移任务 T09。
任务书：D:\Workbuddy\2026-10-13-14-25-42\REAzip-Cpp-migration\tasks\T09-packaging.md
先读同一任务包中的 00-START-HERE.md、01-EXECUTION-RULES.md、02-CONTRACTS.md 和 03-REGRESSION-MATRIX.md，再读取当前任务。
原项目路径：D:\Workbuddy\2026-10-13-14-25-42\reazip。
检查当前 Git 状态及前置任务交接单，保护 Workbuddy 的未提交改动。按任务书实现和验收当前任务，保存 native/docs/handoffs/T09.md。多聊天协作时使用独立 worktree，不直接并发修改原检出目录。
技术路线为 C++17 + Qt Quick/QML + bit7z + 完整 7-Zip 原生动态库，保留现有界面设计。不要改回 Electron/QtWebEngine，也不要把调用 7z 命令行当作库接入完成。
只推进本任务范围，必要缺陷修复纳入本任务；不要自动创建新聊天或发布公开 Release。报告实际完成、验证环境、未运行/阻塞项和下一步。
```

## T10：独立验收与发布准备

前置依赖：T00–T09。

```text
请执行 REAzip 原生迁移任务 T10。
任务书：D:\Workbuddy\2026-10-13-14-25-42\REAzip-Cpp-migration\tasks\T10-acceptance.md
先读同一任务包中的 00-START-HERE.md、01-EXECUTION-RULES.md、02-CONTRACTS.md 和 03-REGRESSION-MATRIX.md，再读取当前任务。
原项目路径：D:\Workbuddy\2026-10-13-14-25-42\reazip。
检查当前 Git 状态及前置任务交接单，保护 Workbuddy 的未提交改动。按任务书实现和验收当前任务，保存 native/docs/handoffs/T10.md。多聊天协作时使用独立 worktree，不直接并发修改原检出目录。
技术路线为 C++17 + Qt Quick/QML + bit7z + 完整 7-Zip 原生动态库，保留现有界面设计。不要改回 Electron/QtWebEngine，也不要把调用 7z 命令行当作库接入完成。
只推进本任务范围，必要缺陷修复纳入本任务；不要自动创建新聊天或发布公开 Release。报告实际完成、验证环境、未运行/阻塞项和下一步。
```
