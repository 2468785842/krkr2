# 游戏验证范围

本仓库尚无覆盖完整剧情、音视频、输入和存读档的游戏兼容清单。
此前列出的本地样本为：天神乱漫 LUCKY or UNLUCKY!?（2009-05-29）。

该样本的当前证据如下：

| 范围 | 已有证据与限制 |
|---|---|
| 归档入口与 KAG 标题 | 管理界面 ON/OFF 的私有副本各运行 25 秒，到达 `title.ks:33`；心跳及启动参数探针通过 |
| 标题图层 | 已导出 KAG snapshotLayer 快照；不包含宿主 UI 和独立视频视口 |
| 剧情交互 | 曾在 CLion 验证运行中新游戏进入剧情并有后续资源加载，未完成整段剧情对照 |
| 正常退出 | 上述定时 smoke 由控制器结束，不能证明游戏正常退出；自制窗口场景另有关闭回调验证 |
| 完整散装资源 | 曾在标题合成图片查找/加载处失败，问题待调查 |
| 音视频、存读档、多平台 | 无完整验收结论 |

细节见[游戏插装记录](../../.agents/notes/implemented/testing/2026-11-02-game-smoke-investigation.md)、
[窗口验证](../../.agents/notes/implemented/testing/2026-11-03-window-interface-validation.md)及
[散装资源问题](../../.agents/notes/proposed/bug-fix/2026-11-03-loose-game-resource-lookup.md)。
测试方法见[游戏与交互测试](../guides/game-testing.md)。本地商业样本及素材不随仓库分发。
