# 原生候选体积与性能（Windows x64 首轮）

同一台 Windows 开发机，原 Electron v0.2.1 与 Qt 6.8.3 / MinGW 13.1.0 原生候选。正式六目标候选以各 CI artifact-manifest.json 为准，下面本机包包含 Qt、worker、完整7-Zip、许可证与对应源码；不是只算主 EXE。

| 项目 | v0.2.1公开资产 | 本机原生候选 |
|---|---:|---:|
| x64安装EXE | 121,076,488 B | 26,674,682 B |
| x64便携ZIP | 176,038,581 B | 36,573,627 B |
| 原生解包内容 | — | 112,538,764 B |

本机包的SHA和内容清单位于 .native-build/artifacts-win。CI使用MSVC，包体积会不同；后续安全修复和文档也会略改大小，不把这组数字当作最终发行哈希。

普通窗口平台、同机暖缓存、每场景3次。空窗口工作集的中位数：Electron约356.9MB（4个应用进程合计），原生硬件渲染约184.1MB；打开ISO后约380.2MB和189.5MB。进程工作集包含共享页，Electron合计可能重复计算共享页，不能直接等同于系统实际增量RAM。

内部应用入口至首次绘制/ready-to-show的中位数：空窗口Electron281ms、原生1557ms；ISO场景257ms、830ms。原生首帧存在明显波动，首样本曾达9秒。本轮**没有证明原生启动更快**，需继续分析驱动初始化、文件扫描和缓存。两种运行时进入应用入口前的加载开销也不同，这些不是严格的端到端启动时间。

另测Qt软件渲染：空窗口约126.8MB、477ms；ISO约128.2MB、692ms。作为可选环境变量 QT_QUICK_BACKEND=software 的测量，当前生产默认仍由Qt选择硬件后端；没有通过删除功能或省略依赖压低包体积。

原始12条/组记录及中位数见 performance/windows-x64-hardware.json、windows-x64-software.json。复现：配置本机Qt运行路径后，运行 native/tools/benchmark.py --native <reazip.exe> --output <directory> --samples 3。基线使用隔离Electron用户目录，不读取真实配置。
