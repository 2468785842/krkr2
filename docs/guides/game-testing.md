# 游戏与交互测试

工具位于 `tools/game-smoke/`，需要 Python 3.11 或更新版本。
真实游戏验证使用独立副本；公共 CI 使用可分发的自制场景，不提交商业素材、脚本或存档。

## 游戏副本与心跳

在已有完整游戏目录之外准备全新目录：

```powershell
python tools/game-smoke/game_smoke.py prepare D:/games/example out/game-smoke/run-01
python tools/game-smoke/game_smoke.py run out/windows/debug/bin/krkr2/krkr2.exe out/game-smoke/run-01 --seconds 25 --launch-style game --probe-engine-args
```

`prepare` 复制 XP3 和兼容 patch，不复制 savedata，记录 SHA256，安装自编 `AfterStartup.tjs`。
源目录若已有该钩子则拒绝，需要显式设计组合顺序。
钩子在真实 startup 返回后记录 KAG、脚本位置和 Timer 心跳，不修改原 startup 或归档查找顺序。
运行目录不可重复使用，控制器拒绝旧状态和请求。

`--launch-style game` 使用 `--game`；`xp3` 使用 `--xp3`，默认 `legacy` 保留位置参数路径。
`--target directory` 显式测试目录入口，否则有 `data.xp3` 时工具可自动选择该归档。
`--engine-arg=-name=value` 可重复；`--probe-engine-args` 通过真实 `System.getArgument`
检查中文、重复覆盖、等号、空值和默认 yes。
实际入口语法见 [Desktop 启动](../subsystems/desktop-cli.md)。

报告包括 exe/hash、命令、PID、状态序列、退出码、期限时存活状态和 `forced_stop`。
游戏 smoke 在期限后结束自己启动的进程，通过只表示该报告中的 KAG/心跳/参数条件满足。
标题画面、剧情输入、音视频、存读档和正常退出需要各自的实际证据。

当前引擎的 `System.exePath` 指向项目目录，默认 savedata 也在项目目录；
游戏自定义的绝对写入路径仍应单独核对。

## 截图

对支持该钩子的 KAG 场景，在运行副本中创建 `smoke-snapshot.request`，
Timer 经 `lockSnapshot` / `snapshotLayer.saveLayerImage` / `unlockSnapshot` 输出一次 `smoke-frame.bmp`。
该图仅覆盖 KAG 合成层，不包含 Cocos 宿主 UI、窗口边框或独立视频视口。

需要检查最终窗口、菜单或原生对话框时使用可用的窗口自动化，并在输入后重新观察。
若本机窗口采集已确认失败，可用下述显式组件测试构建的 `capture` 导出 Cocos 帧缓冲。
比较画面时记录脚本位置、操作和构建身份；动画区域可单独处理，不仅按等待秒数对齐。

## 自制窗口交互

[交互场景说明](../../tools/game-smoke/interaction/README.md)给出准备、运行与操作请求：

```powershell
python tools/game-smoke/interaction_smoke.py prepare out/game-smoke/interaction-01
python tools/game-smoke/interaction_smoke.py run out/windows/debug/bin/krkr2/krkr2.exe out/game-smoke/interaction-01 --seconds 300
```

场景覆盖菜单 checked 切换、原生消息框确认、文件选择取消/确认、close veto 与接受关闭。
请求文件只打开对话框或发起关闭；返回值和菜单回调必须由真实输入产生。
报告要求事件齐全、选中文件路径正确、无脚本/驱动错误、未强停且退出 0。

可选 `KRKR2_ENABLE_UI_TEST_DRIVER=ON` 仅用于桌面测试构建，要求应用与测试开启。
通过 `--component-input` 显式启用，且目标必须是带标记文件的隔离场景。
其 `tree`、`capture`、`click-name`、`click-text` 用法和完成确认见场景说明；原生消息框仍需系统输入。
默认构建不编译该驱动。组件分发测试不能替代操作系统设备、DPI 或 IME 测试。

指针验收可单独运行 `tools/game-smoke/pointer_repro.py`（取消、双指右键与隐藏/显示）
和 `pointer_windows_repro.py`（双窗口切换、失效与旧触点隔离），无需原生消息框。
两者各自使用全新目录并比较真实 TJS 回调序列；不能作为完整文件/菜单场景的通过证据。
Windows CI 可手动开启 `pointer_contracts`，用独立目录的 Mesa 软件 OpenGL 运行应用，
操作、artifact 和验收边界见[交互场景说明](../../tools/game-smoke/interaction/README.md#跨帧触摸回调探针)。

Windows 的 `tools/game-smoke/focus_repro.py` 另使用原生辅助窗口与 `SendInput`，
实际切换前台窗口并核对取消/恢复后的 TJS 输入状态；A 键走系统路径，触摸仍走 GLView。
该探针与默认文件/菜单场景分开判定，不覆盖最小化或完整暂停恢复。
操作及平台要求见[原生焦点探针](../../tools/game-smoke/interaction/README.md#windows-原生焦点探针)。

同一工具加 `--minimize` 可单独验收 Windows 原生最小化/恢复：检查 `IsIconic`，
最小化期间发布脚本请求并观察一秒内未被 Timer 处理，恢复后再核对取消、迟到输入及新输入。
System 激活事件按 `base/impl/SystemImpl.cpp` 的同类替换与当前状态检查规则验收，
允许后台通知在停止动画泵期间被合并，但必须收到恢复通知；不改变生产事件语义。
截图来自恢复后的 Cocos 帧缓冲，不覆盖移动端 surface 重建、音频或视频恢复。
最小化报告还检查恢复前后 PNG 中心像素为自制图层的 RGB `(48,64,80)`，
缺图、坏 PNG 或黑色中心像素均失败；这只是固定填充层检查，不代表完整游戏画面。
输入法过滤 A 键时可显式加 `--us-keyboard-layout`，仅向自有引擎线程请求已安装的英文布局，
并记录实际布局；该选择不作为 IME 兼容通过证据。

## 验收范围

按改动选择标题、剧情推进、相同位置画面、菜单/脚本对话框、存读档、关闭、音视频、
DPI/IME/焦点和移动生命周期中的相关项，分别保存结果。
已有本地记录见[游戏插装调查](../../.agents/notes/implemented/testing/2026-11-02-game-smoke-investigation.md)
与[窗口交互验证](../../.agents/notes/implemented/testing/2026-11-03-window-interface-validation.md)。
当前样本的具体通过与失败边界见[游戏验证范围](../reference/game-compatibility.md)。
