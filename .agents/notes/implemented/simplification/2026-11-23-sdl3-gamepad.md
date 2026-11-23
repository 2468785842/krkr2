# SDL3 手柄输入与热插拔

Status: implemented

Linux SDL3 宿主通过 SDL Gamepad API 接收设备添加、移除、按钮和轴事件，
沿用 Cocos 的脚本虚拟键映射。十字键与左摇杆共用方向键，同一个虚拟键可由多个控件和手柄共同按住。
失焦时清除脚本持有状态，保留物理位置直到用户重新操作。移除设备时释放其独占的键，
并以取消事件结束仍按住的模拟鼠标按钮。
[窗口和输入契约](../../../../docs/subsystems/window-interface.md#linux-sdl3-窗口契约)

自制场景从正式的 `krkr2 --host=sdl3 --game PATH` 启动。
显式启用测试驱动后，程序通过 SDL 的虚拟摇杆接口接入两台手柄，
使用 X11 窗口焦点变化检验释放、重复来源、扳机阈值、模拟指针和拔掉设备时的点击取消。
场景的画面颜色由脚本收到手柄按下后修改，测试从原生窗口读取像素。
本批使用 SDL 虚拟设备验收。

```sh
xvfb-run -a -s '-screen 0 1600x1050x24' dbus-run-session -- /usr/bin/python3 \
  tools/game-smoke/sdl_gamepad_repro.py out/linux/release/bin/krkr2/krkr2 \
  out/linux/sdl-desktop-runtime/gamepad
```

## 实际结果

- `e2052da` 的 Linux Release CI 启用 `sdl3_desktop_contract=true` 和
  `imageops_contracts=true`。正式入口通过 X11 手柄测试，退出码为 0。既有窗口、Fcitx5、
  KAG 存读档、游戏音视频、文件拖放、触摸及图像操作回归全部通过。
  [Linux CI](https://github.com/2468785842/krkr2/actions/runs/45132908010) ·
  [截图与运行报告](https://github.com/2468785842/krkr2/actions/runs/45132908010/artifacts/14799415330)
- 使用同一可执行文件在本地嵌套 Weston 的 2× 虚拟输出验证了手柄驱动画面重绘、指针像素与位置、
  拔出设备后的模拟按钮释放和正常退出。X11 和 Weston 场景均以 0 退出，无强制停止。
- 可执行文件 SHA-256 为
  `0ad2d192e454e5a87e5c2d983b0ae92c30c3fd5bcacf2af5f9faf5d97abf243a`，
  自制场景脚本 SHA-256 为
  `512da56db5549df970696e62c8abd8469ad8a3b0d14849c97bf51403b936b858`。
- 本地 52 项平台契约、49 项游戏工具测试及基础模块依赖检查通过。
