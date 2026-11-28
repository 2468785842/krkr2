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

## 当前桌面宿主验收

桌面探针用于 Windows/Linux/macOS SDL3；Android 的 APK 与设备验证见 [Android SDL3 宿主](../subsystems/android-sdl3.md)。
使用全新输出目录，记录实际 exe 和场景 SHA256、构建 scope、UI 驱动开关及退出状态。
游戏副本心跳只证明相应脚本阶段仍运行，不证明完整剧情或正常退出。

Windows 使用系统输入、菜单、文件对话框和真实窗口截图：

```powershell
python tools/game-smoke/sdl_windows_repro.py out/windows/debug/bin/krkr2/krkr2.exe out/game-smoke/native-windows-01
```

该工具使用自制场景和 Win32 SendInput，验证切焦、最小化恢复、菜单、消息框、文件选择、
关闭否决与接受。需要交互式桌面和 Pillow；不启用组件驱动。
报告须要求所有事件与路径正确、`forced_stop=false`、退出码 0，并实际查看截图。
命令和范围见[Windows SDL3](../subsystems/windows-sdl3.md)。

Linux 使用 X server/XTest 的真实系统输入：

```sh
xvfb-run -a python3 tools/game-smoke/sdl_host_repro.py out/linux/release/bin/krkr2/krkr2 out/game-smoke/native-host-01
xvfb-run -a -s '-screen 0 1280x800x24' python3 tools/game-smoke/sdl_dialog_repro.py out/linux/release/bin/krkr2/krkr2 out/game-smoke/native-dialogs-01
```

KAG 存读档、生成音视频与原生窗口验收命令见[桌面运行时](../subsystems/native-runtime.md)
及 `.github/workflows/native-desktop.yml`，不使用商业素材作为公共 CI 输入。

## 截图与像素

宿主截图应来自实际窗口或 SDL3 宿主截图接口；KAG `snapshotLayer` 只覆盖游戏合成层，
不能证明窗口边框、原生菜单/对话框或独立视频视口正常。
采集失败要保留失败记录，不能用旧图或模拟回调代替。

软件 ImageOps 自制场景使用正式 Layer 与插件：

```sh
python tools/game-smoke/imageops_repro.py /path/to/krkr2 out/game-smoke/imageops-01 --renderer software
```

检查缩放、模糊、仿射和透视结果、导出的图层及正常退出。
当前应用不注册旧 OpenGL 工厂。旧 Cocos 的 `--capture` / `--profiles` 组件探针和
`pointer_repro.py`、`text_repro.py`、`interaction_repro.py` 等旧宿主工具不用于当前验收；
历史代码和证据只用于追溯，不表示当前宿主仍支持旧测试协议。

## 结果与调试

通用测试先核对实际注册，再使用 `--output-on-failure --no-tests=error` 执行。
编译通过、单元测试通过、原生交互通过和商业游戏兼容是不同结论，分别记录。
崩溃调查保留匹配的 exe/PDB、异常码、栈、dump 和失败前操作顺序；旧 RVA 不直接用于新构建。
原始日志、图像及 dump 放在忽略的 `out/`，notes 保留必要摘要与定位信息。
物理高 DPI、IME 候选窗、手柄、Wayland、移动端生命周期和完整剧情需各自专项验收。
