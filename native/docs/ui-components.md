# T00 UI接口与状态

MainWindow：1120×740；TabBar：模型id/title/modified；Toolbar：open/create/extract/test/add/delete/rename/settings；Breadcrumbs：path段。

EntryModel roles：entryId/name/path/kind/size/packedSize/modified/encryption。模型提供folder/filter/sort与多选ID，虚拟化显示；右侧ArchiveInfo显示未知值，不推断AES。

CreateDialog：inputs/output/format/level/volume/password/headerEncryption/deleteSources；ExtractDialog：destination/smart/policy/selected/openAfter。

JobDialog：jobId/state/progress/currentFile/error；ChallengeDialog：jobId/challengeId/password或冲突四策略。关闭挑战=取消，不执行默认覆盖。

SettingsDialog：General/Appearance/Associations/Language/About，与旧版布局一致。Settings服务提供language/theme/paths/restoreTabs；Theme服务返回白名单颜色，Translation服务按原key返回文案。

UI只发语义动作；路径、库调用、文件修改不放QML。对未实现动作禁用并解释，不用按钮空回调伪装功能。
