# T09：六目标打包、CI、依赖部署与发布门禁

## 依赖 / 范围

可在 T01 后准备框架；完整交付依赖 T03/T04/T06/T07/T08。负责 native/packaging、CI、LICENSES、部署和性能测量脚本。旧 release.yml 的最终替换由本任务统一实施。

## 实施

1. 六目标构建/测试矩阵与锁定工具链，构建配置使用 Release。Qt QML 插件和实际需要的系统插件必须随包部署，不能在开发机碰巧能找到库就通过。
2. 每个包仅包含对应架构的 worker 和完整引擎库；Linux/macOS 可执行位、rpath/install_name、共享库签名/装载路径在构建时确定。不在启动时 chmod 修正包。
3. Windows：x64/ARM64 安装 EXE，以及两架构各自的免安装 ZIP；ZIP 内含便携标记、worker、引擎、Qt 依赖、许可证与使用说明。若额外保留单 EXE 便携形式，单独说明，不能以它替代 ZIP。
4. macOS：x64/ARM64 DMG 和 ZIP（或经验证的 universal 包），正确 app bundle、UTI 与引擎加载。无签名/公证身份时明确记录，不能宣称已签名或给整个系统禁用 Gatekeeper 的步骤。
5. Linux：x64/ARM64 AppImage、tar.gz；deb 至少 x64，ARM64 有能力则同轮交付。只读 AppImage 中可直接运行 worker/库；干净桌面环境验证。
6. 打包安装/卸载元数据与 T07 对齐，便携 ZIP 的 data 目录逻辑与 T08 对齐。
7. 发行包记录源码 SHA、版本、OS/arch、Qt/bit7z/7-Zip 版本、SHA256SUMS、依赖来源和许可证。分发源和动态库的要求按实际许可核对，不笼统标全部 MIT。
8. 原 CI 的 v* 标签直接跑 Electron 并公开 Release 必须改造：旧线与新线触发条件互斥；原生线先构建→测试→上传 CI 候选 artifacts，聚合 job 等全部必须目标成功后才产生草稿/候选清单。没有用户发布授权不自动公开。
9. PR/分支 CI 做核心回归与构建；重型大文件/GUI系统集成可按环境单独执行，但正式候选必须汇总结果。
10. 同架构完整离线安装包与解包体积对比 v0.2.1，记录大小构成；实测启动到主界面就绪、空闲内存、代表压缩/解压耗时。不能省略运行库、上传空壳、压低功能来达成数字。
11. 不提供无法证明可运行的架构包；不能悄悄去掉任务目标。缺环境/签名等要列入候选发布限制并交由用户决定。

## 验收

R01/R12/R20/R22/R27/R28/R29/R30。至少在干净机器或隔离环境无 Qt/Node/7-Zip 预装时验证；确认所有二进制架构与引擎一致；包内没有 Electron/Chromium/WebEngine 和其他平台冗余引擎。

构建成功、归档核心通过、GUI启动通过、系统集成通过分别展示。验证某一矩阵任务失败时不会发布半成品 Release。

## 交接

native/docs/handoffs/T09.md、artifact-manifest.json、SHA256SUMS、size-and-performance.md、发布/回滚操作说明。候选版本统一来源，不手工给六个平台写不同版本号。
