# SDL3 / TJS 平台壳实验

这是独立桌面原型，默认关闭，不替代现有游戏运行器。
实际链接仓库 TJS2、`core_input` 和 SDL3，使用真实 SDL 窗口、事件队列及渲染读回。
脚本决定窗口尺寸/标题、背景和矩形，按键回调修改图形，关闭回调第一次否决、第二次接受。
失焦时先释放所有按住的虚拟键并调用 `onKey`，再调用 `onFocus(false)`；
失焦期间忽略按键，恢复时调用 `onFocus(true)`，等待新的输入。
其他窗口的键盘、焦点和关闭事件不影响本窗口，重复焦点通知及无对应按下的抬起不重复分发。
实验脚本的全局变量不是生产 `Window`、`Layer` 或 `System` API；不能运行 KAG 或商业游戏。

## 构建

设置 `VCPKG_ROOT` 并使用 MSVC x64 开发环境，PATH 中需有 win_bison。
新构建目录独立选择 foundation 与 sdl3-prototype 依赖，不选择完整 engine feature：

```powershell
cmake -S . -B out/sdl3-prototype/build -G Ninja -DWINDOWS=ON -DCMAKE_BUILD_TYPE=Debug -DKRKR2_BUILD_APP=OFF -DENABLE_TESTS=OFF -DBUILD_TOOLS=OFF -DKRKR2_BUILD_SDL3_PROTOTYPE=ON -DVCPKG_TARGET_TRIPLET=x64-windows-static-md
cmake --build out/sdl3-prototype/build --target krkr2_sdl3_prototype
python tools/sdl3-prototype/verify.py out/sdl3-prototype/build/tools/sdl3-prototype/krkr2_sdl3_prototype.exe out/sdl3-prototype/run-01
```

仓库 pin 的 vcpkg baseline 提供 SDL3 3.4.8#1；没有更改全局工具链。
Linux 显式选择 SDL3 的 dbus feature，满足默认 IBus 路径的实现依赖；不能通过关闭 IME 掩盖链接失败。
Linux 宿主还需 Autoconf、autoconf-archive、Automake、Libtool，以及窗口/IBus/Wayland 开发包，
Ubuntu 的实际安装列表见[无窗口 CI](../../.github/workflows/headless-tests.yml)的 SDL3 job。
测试输出目录必须全新；程序正常退出、脚本计数、事件顺序和七帧 BMP 指定像素都符合条件才通过。
`verification.json` 保存 exe/脚本/截图哈希，`report.json` 是原型自身的观察，二者含义不同。
去掉验证器直接运行 `exe fixture.tjs NEW_OUTPUT_DIR` 可手工按 A、关闭窗口两次。

## 验证与限制

自测经 [SDL_PushEvent](https://wiki.libsdl.org/SDL3/SDL_PushEvent) 投递输入，
由实际 SDL 轮询循环读取，再更新共享虚拟键状态并调用 TJS。
SDL 明确说明该 API 不更新物理设备状态，本原型也不查询 SDL 的设备状态来伪装真实键盘。
每帧重新绘制，再[呈现](https://wiki.libsdl.org/SDL3/SDL_RenderPresent)；BMP 来自 SDL 渲染器读回。
这里使用软件 renderer，不验证现有 OpenGL 渲染器或 GL context。

自测从模拟的已聚焦状态开始，依次投递 A 按下、失焦、失焦期间重复/抬起、恢复、
再次按下/抬起及两次关闭，还插入其他窗口的抬起/关闭事件。
验证器检查释放早于失焦通知、失焦背景、恢复后无旧键按下以及正常退出。
交互模式以 SDL 的初始窗口焦点状态为准。Linux CI 在 Xvfb 的 X11 窗口运行同一自测，
不是实际 OS 切焦、后台挂起或物理键盘验收。

另有 Linux/X11 输入探针，需 Xvfb、xauth、libX11、libXtst：

```sh
xvfb-run -a python tools/sdl3-prototype/verify_x11.py out/sdl3-prototype/build/tools/sdl3-prototype/krkr2_sdl3_prototype out/sdl3-prototype/os-input-01
```

该探针按进程 PID 查找窗口并等待 `IsViewable`，用 `XSetInputFocus`、`XTestFakeKeyEvent` 和 `WM_DELETE_WINDOW`
经过 X server 验证按住 A 切焦、抬起、恢复、重新按键及两次独立关闭；按本轮事件确认逐步操作，
第一次关闭后继续观测事件泵，确保否决有效，再发送第二次关闭并等待正常退出。
不使用 `SDL_PushEvent`，也不代表物理键盘或生产 Cocos 后端验收。
原型关闭 SDL 的“最后一个窗口请求关闭时自动追加 QUIT”行为，避免一次 OS 关闭触发两次脚本查询；
独立的应用退出事件仍经过关闭查询。
`test_verify_x11.py` 在真实 X server 上检查带 PID 的未映射窗口不会被当作已就绪；
CI 将它与系统输入探针一起运行。

键码仅映射字母、数字、空格、回车、Escape 和方向键，其他键会记录 unsupported-key。
鼠标/触摸/手柄、IME、DPI、后台挂起、媒体、菜单、存储及完整窗口契约均未接入。
失焦批量释放尚未接入现有 Cocos 游戏适配器；原型的 `onFocus` 也不是生产 System 激活事件。
SDL3 能力实验不能作为 B2 全部完成或 B3 游戏兼容验收通过的依据。
