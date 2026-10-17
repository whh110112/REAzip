# REAzip

参考 Bandizip 行为、使用完整 7-Zip 原生库的跨平台归档管理器。界面沿用本项目原有设计，核心与程序独立实现；不包含 Bandizip 程序、代码或专有资源，不声称已还原其内核。

当前开发分支 `native-migration`：**C++17 + Qt Quick 6.8.3 + bit7z 4.1.0 + 7-Zip 26.04**。QML 是 Qt 原生界面描述，发行包不包含 Electron、WebEngine、Node 或 7z 命令行运行依赖。根目录旧 JS/HTML 仍保留作 v0.2.1 基线，新实现位于 `native/`。

原生版本 **0.3.0-dev 是候选版本**。公开 v0.2.1 仍是旧 Electron 版；当前没有公开原生 Release。候选只由 CI 上传，具体执行 SHA、验收范围和未验证项目见 [验收报告](native/docs/acceptance-report.md)。

## 原生功能

- 多标签、目录导航、全包搜索、自然排序、虚拟文件列表。
- ZIP / 7Z / TAR / TAR.GZ 创建、列表、测试与解压；选中条目解压；询问、覆盖、跳过、重命名冲突策略。
- ZIP AES / 旧 ZipCrypto 读取、7z 文件头加密、密码挑战与重试、分卷、ZIP64。
- 添加、删除和重命名；条目ID随归档元数据变化，拒绝过期选择。
- RAR / RAR5 / ISO / WIM 等只读浏览和解压；写入能力按真实格式限制。
- GBK / Big5 / Shift-JIS 显式编码；六套主题、18种语言和 RTL；旧设置及颜色 token 的保守迁移。
- 原生 worker 隔离、任务排队与取消、安全解压、提交日志和中断恢复。
- Windows 每用户关联与经典菜单、macOS bundle 文件类型、Linux desktop MIME；系统默认应用由用户选择。

完整支持边界见 [已知限制](native/docs/known-limitations.md) 和 [功能对照](native/docs/feature-parity.md)。Win11现代菜单、Finder/Nautilus扩展、SFX、密码恢复、复杂损坏修复、密码管理器和自动更新客户端未实现。

## 获取候选

[Native candidate 构建记录](https://github.com/whh110112/REAzip/actions/workflows/native-candidate.yml) 的成功运行提供六种目标：

| 系统 | CPU | 产物 |
|---|---|---|
| Windows | x64、ARM64 | 安装 EXE、便携 ZIP |
| macOS | x64、ARM64 | DMG、ZIP |
| Linux | x64、ARM64 | AppImage、tar.gz、deb |

GitHub artifacts 下载需要登录，属于限期候选附件。每包附版本、源码 SHA、CPU、SHA256、签名状态、许可证和对应源码。Windows 未签名；macOS ad-hoc 签名、未公证。安装和旧设置迁移见 [安装指南](native/docs/installation-guide.md)。

## 原生构建

先安装 Python 3.12、平台 C++17 工具链、CMake 和 Ninja。Windows ARM runner 使用 VS2026/CMake4.2；Windows x64 使用 VS2022；macOS 使用 Xcode，Linux 使用 GCC。依赖版本与摘要锁在 `native/deps.lock.json`，完整复现以 CI workflow 为准。

```sh
python -m pip install aqtinstall==3.3.0 ninja==1.11.1.4 cmake==3.31.6
python native/bootstrap/dependencies.py --arch x64
python native/bootstrap/qt-sdk.py --arch x64 --output .native-build/Qt
```

将生成的 Qt SDK 加入 PATH/CMAKE_PREFIX_PATH，指定当前目标实际引擎目录后配置：

```sh
cmake -S native -B .native-build/release -DREAZIP_ENGINE_DIR=/absolute/path/to/native/.deps/engine/x64 -DREAZIP_HEAVY_TESTS=ON
cmake --build .native-build/release --config Release --parallel 3
ctest --test-dir .native-build/release -C Release --output-on-failure
python native/packaging/package.py --build .native-build/release --arch x64 --qt-prefix /absolute/path/to/Qt/6.8.3/target-kit --output .native-build/artifacts
```

ARM64 将架构参数改为 `arm64`，使用对应原生 runner 和 Qt SDK；Windows ARM 还需 target Qt 的 QT_HOST_PATH。首次下载和完整7-Zip编译需要网络，发行包离线运行无需安装这些开发依赖。大文件测试会读取4GiB可压缩稀疏数据。磁盘满测试仅在Linux CI临时挂载2MiB tmpfs。

旧 Electron 基线可用 `npm ci`、`npm start` 运行，不用于原生打包。不要通过推送标签触发原生公开发布；当前候选 workflow 没有发布权限。

## 资料与许可

[任务进度](native/docs/task-status.md)、[发布说明草稿](native/docs/release-notes.md)、[体积与性能原始测量](native/docs/size-and-performance.md)、[T00–T10交接](native/docs/handoffs/)。

本项目自有代码采用 MIT；发行包包含 Qt LGPL、bit7z MPL、7-Zip LGPL/unRAR限制等第三方许可，**整个发行包不能简写成“全部MIT”**。第三方声明、许可文本及对应源码随候选提供，见 [打包声明](native/packaging/NOTICE.md)。Bandizip 为其权利人商标，本项目与其无关联。
