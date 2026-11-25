# 游戏验证范围

本仓库尚无覆盖完整剧情、音视频、输入和存读档的游戏兼容清单。
此前列出的本地样本为：天神乱漫 LUCKY or UNLUCKY!?（2009-05-29）。

该样本早期调查的证据如下，后续验证见本页日期章节：

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

## 2026-11-24 Windows SDL3 独立宿主

在私有归档副本验证《天神乱漫》的 `title.ks:33` 标题、SendInput 鼠标开始游戏、
Enter 推进 `１.ks`、存档槽写入和同进程读档；正常确认退出后，将该私有副本的 savedata
复制到另一份全新副本，新进程从标题读取保存槽并继续推进剧情。两轮结束均退出 0、未强停。
原始截图、归档哈希、二进制身份与失败/修复过程见
[实施记录](../../.agents/notes/implemented/testing/2026-11-24-windows-sdl3-independent.md)。

本次发现并修复 WindowEx 未初始化覆盖位图指针导致的窗口销毁崩溃。旧 Cocos 基线和修复前
SDL3 游戏退出都曾为 `0xc0000005`；只有 SDL3 失败保存了匹配符号栈，不能据相同退出码
推断所有历史访问冲突都已修复。未验证完整剧情、音视频同步和 Windows 物理 DPI/IME/手柄。

## 2026-11-25 删除 Cocos 后的 Windows 验收

在全新私有副本使用删除后的普通 SDL3 Debug/full 应用，完成标题、新游戏短剧情、File01 新建存档、
同进程读档及两个新进程读档。三轮均从保存位置 `１.ks:24` 恢复并继续推进，最后一轮到 `１.ks:38`，
截图可见 CG、文字和按钮。三轮均通过游戏确认框正常退出 0，未强停；原始 13 个资源文件哈希未变。

本轮 exe 身份、控制器早期失败、最终截图及存档证据见
[删除与验收记录](../../.agents/notes/implemented/architecture/2026-11-25-desktop-only-cocos-removal.md)。
这是 Windows 单样本短流程验证，不扩展为完整剧情、音视频同步或其他平台的兼容结论。
