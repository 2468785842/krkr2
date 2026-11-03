# SDL3 / TJS 平台壳实验

这是独立桌面原型，默认关闭，不替代现有游戏运行器。
实际链接仓库 TJS2、`core_input` 和 SDL3，使用真实 SDL 窗口、事件队列及渲染读回。
脚本决定窗口尺寸/标题、背景和矩形，按键回调修改图形，关闭回调第一次否决、第二次接受。
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
测试输出目录必须全新；程序正常退出、脚本计数、事件顺序和 BMP 指定像素都符合条件才通过。
`verification.json` 保存 exe/脚本/截图哈希，`report.json` 是原型自身的观察，二者含义不同。
去掉验证器直接运行 `exe fixture.tjs NEW_OUTPUT_DIR` 可手工按 A、关闭窗口两次。

## 验证与限制

自测经 [SDL_PushEvent](https://wiki.libsdl.org/SDL3/SDL_PushEvent) 投递输入，
由实际 SDL 轮询循环读取，再更新共享虚拟键状态并调用 TJS。
SDL 明确说明该 API 不更新物理设备状态，本原型也不查询 SDL 的设备状态来伪装真实键盘。
每帧重新绘制，再[呈现](https://wiki.libsdl.org/SDL3/SDL_RenderPresent)；BMP 来自 SDL 渲染器读回。
这里使用软件 renderer，不验证现有 OpenGL 渲染器或 GL context。

键码仅映射字母、数字、空格、回车、Escape 和方向键，其他键会记录 unsupported-key。
鼠标/触摸/手柄、IME、DPI、焦点丢失释放、后台挂起、媒体、菜单、存储及完整窗口契约均未接入。
SDL3 能力实验不能作为 B2 全部完成或 B3 游戏兼容验收通过的依据。
