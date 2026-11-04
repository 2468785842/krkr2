# SDL3 / TJS 平台壳实验

这是独立桌面原型，默认关闭，不替代现有游戏运行器。
实际链接仓库 TJS2、`core_input` 和 SDL3，使用真实 SDL 窗口、事件队列及渲染读回。
脚本决定窗口/画布尺寸、标题、背景和矩形，键盘与鼠标回调修改图形，关闭回调第一次否决、第二次接受。
失焦时先清空共享按键状态，并分发对应 `onKey` / `onPointer` 抬起，再调用 `onFocus(false)`；
失焦期间忽略输入，恢复时调用 `onFocus(true)`，等待新的输入。
其他窗口的输入、焦点和关闭事件不影响本窗口，重复焦点通知及无对应按下的抬起不重复分发。
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
测试输出目录必须全新；程序正常退出、脚本计数、事件顺序和 BMP 指定像素都符合条件才通过。
`verification.json` 保存 exe/脚本/截图哈希，`report.json` 是原型自身的观察，二者含义不同。
去掉验证器直接运行 `exe fixture.tjs NEW_OUTPUT_DIR` 可手工按 A、关闭窗口两次。

## 验证与限制

自测经 [SDL_PushEvent](https://wiki.libsdl.org/SDL3/SDL_PushEvent) 投递输入，
由实际 SDL 轮询循环读取，再更新共享虚拟键状态并调用 TJS。
SDL 明确说明该 API 不更新物理设备状态，本原型也不查询 SDL 的设备状态来伪装真实键盘。
每帧重新绘制，再[呈现](https://wiki.libsdl.org/SDL3/SDL_RenderPresent)；BMP 来自 SDL 渲染器读回。
这里使用软件 renderer，不验证现有 OpenGL 渲染器或 GL context。

自测从模拟的已聚焦状态开始，依次投递 Shift/Ctrl/Alt+A 按下、失焦、失焦期间重复/抬起、恢复、
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

`onKey(key, down, current, pending, shift)` 的最后一个参数来自共享修饰键快照，
不使用 SDL 的设备状态查询伪装脚本状态。队列自测验证组合键标记为 7，失焦合成释放时全部归零；
X11 探针实际按住 Ctrl+A 切焦，验证 SDL 自动抬起的顺序及恢复后的 A 不再带 Ctrl。
虚拟键用 scancode 查询去除修饰键后的键码，Shift/Ctrl+A 均保持 VK_A，字符与 IME 输入另行处理。

## 鼠标与视口

`pointer_fixture.tjs` 用 800×480 窗口显示 640×480 画布，两侧各有 80 像素黑边。
`canvasWidth` / `canvasHeight` 可省略，默认等于窗口尺寸；窗口支持调整大小。
鼠标位置经 `SDL_RenderCoordinatesFromWindow` 处理窗口像素密度、逻辑呈现和视口，
再按共享规则向零截断，不裁剪画布外坐标。SDL 顶部为原点，不再翻转 Y。
滚轮位置使用同一变换；增量保持小数、不随视口缩放，`FLIPPED` 方向取反。

实验回调为 `onPointer(kind, x, y, button, shift, current, pending, wheelX, wheelY, synthetic)`。
`kind` 为 move/down/up/wheel，左/右/中键使用共享 VK 1/2/4；移动与滚轮的 button 为 0。
shift 沿用共享修饰键快照，左/右键标记为 8/16，中键不占标记。
current 查询会消费待查询的按下标记，因此其后的 pending 为 false；移动与滚轮不查询按键。
`synthetic` 只标识共享状态在失焦时补发的抬起。其他鼠标键尚未映射。

```sh
xvfb-run -a python tools/sdl3-prototype/verify.py out/sdl3-prototype/build/tools/sdl3-prototype/krkr2_sdl3_prototype out/sdl3-prototype/pointer-01 --pointer
xvfb-run -a python tools/sdl3-prototype/verify_x11.py out/sdl3-prototype/build/tools/sdl3-prototype/krkr2_sdl3_prototype out/sdl3-prototype/os-pointer-01 --pointer
```

前者用 SDL 队列检查黑边负坐标、窗口外位置、分数反向滚轮、失焦释放和八帧像素。
后者经 XTest 移动、按键、滚轮及 XResizeWindow，验证 SDL 自动捕获的窗口外拖动，
以及窗口调整为 1200×720 后相同逻辑位置的换算；各步等待本轮回调和呈现尺寸确认。
`pointer-events.jsonl` 单独记录鼠标回调；`presentation-events.jsonl` 只在初始呈现和尺寸/视口变化时记录。
读回的 BMP 为 640×480 逻辑画布，窗口实际像素尺寸另记在报告中。
CI 还故意偏移脚本绘制位置，要求进程退出 0 时验证器仍因错误像素失败。

键码映射字母、数字、空格、回车、Escape、方向键和左右 Shift/Ctrl/Alt，其他键记录 unsupported-key。
左右修饰键沿用现有 Cocos 的合并虚拟键规则，尚不独立跟踪同时按住两侧再抬起一侧的状态。
触摸/手柄、IME、后台挂起、媒体、菜单、存储及完整窗口契约均未接入。
鼠标只在 Linux Xvfb/X11 实测；调整窗口产生的 1.5 倍缩放不等于高 DPI 屏幕或多显示器验证。
Windows/macOS 鼠标、物理设备、多设备及捕获销毁路径仍待验证。
失焦批量释放尚未接入现有 Cocos 游戏适配器；原型的 `onFocus` 也不是生产 System 激活事件。
SDL3 能力实验不能作为 B2 全部完成或 B3 游戏兼容验收通过的依据。
