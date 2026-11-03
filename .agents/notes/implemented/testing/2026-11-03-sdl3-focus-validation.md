# SDL3 原型失焦释放与平台 CI

Status: implemented

## 实现范围

基于 `bfeb707` 加本批改动；只扩展共享输入状态与独立 SDL3/TJS 原型。
没有接线 Cocos 失焦路径，也没有完成 B2 或生产 Window/Layer、KAG 后端迁移。

- `KeyState::releaseAll()` 释放全部当前按下的键，按 VK 升序返回；保留未查询的按下标记。
  返回前全部状态已更新，回调读取修饰键时不会看到一半释放的状态；重复调用为空。
- SDL 原型先逐个发送释放回调，再发送 `onFocus(false)`；恢复不重新按下旧键。
  忽略失焦期间输入、重复焦点通知、没有对应按下的抬起和其他窗口的键盘/焦点/关闭事件。
- 自测扩至七帧，覆盖重复按下、恢复前后迟到的抬起、外来窗口事件及关闭否决/接受。
  验证器保存实际 video driver，并检查脚本计数、事件顺序与像素。
- 无窗口 CI 增加三平台 `platform` 范围，检查 12 项实际注册、5 个项目编译源及仅 Catch2 的安装闭包。
  独立 SDL 原型另在 Linux Xvfb/X11 验证，增加正常退出但像素错误必须拒绝的反向场景。

当前使用及边界见[输入状态](../../../../docs/subsystems/input-state.md)、
[构建指南](../../../../docs/guides/building.md#独立平台契约与-sdl3-原型)和
[原型工具](../../../../tools/sdl3-prototype/README.md)。

## 本地验证

Linux Debian 13、GCC 14.2、CMake 4.4.4、Ninja 1.13.2、Debug、Catch2 3.7.1。
依赖放在 `/tmp/krkr-dev-*` 隔离目录，没有修改全局工具链。
从仓库根配置 `out/linux/platform-focus`：

```sh
cmake -S . -B out/linux/platform-focus -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DKRKR2_BUILD_APP=OFF -DBUILD_TOOLS=OFF -DENABLE_TESTS=ON -DKRKR2_TEST_SCOPE=platform -DCMAKE_PREFIX_PATH=/tmp/krkr-dev-system/usr
cmake --build out/linux/platform-focus --target input_state_tests frame_hook_tests --parallel 4
ctest --test-dir out/linux/platform-focus -N
ctest --test-dir out/linux/platform-focus --output-on-failure --no-tests=error
```

全部命令退出 0；实际注册和通过 9 个输入、3 个帧钩子用例。
`compile_commands.json` 只有 InputState、InputStateTest、PostUpdateEvent、PostUpdateTest、PostUpdateRegistration 五个源。
`python scripts/check_core_module_deps.py` 检查 11 个 foundation 目标通过；交互控制器 13 项 Python 测试通过。

SDL 使用官方 release-3.4.8 源码，压缩包 SHA512 与仓库 pin 的 port 完全一致：
`df5a323af7ac366661a3c0e887969c72584d232f3cc211419d59b0487b620b6b2859d4549c9e8df002ee489290062e466fcfddf7edc0872a37b1f2845e81c0f3`。
本地构建关闭音频、GPU、Wayland、OpenGL/GLES 等原型不使用的能力，启用 X11 和软件 renderer；
这不是完整 vcpkg 安装闭包或发行配置测量。

`out/linux/sdl3-focus` 用 `KRKR2_BUILD_SDL3_PROTOTYPE=ON`、应用/工具/测试关闭独立配置并实际构建。
隔离 Bison 首次因其编译期 `/usr/bin/m4` 不存在失败，指定真实 M4 路径后构建成功。
前几次 X11 初始化失败保存在 `sdl3-focus-run-01/02`、`sdl3-focus-x11-01`：
隔离 SDL 曾误选静态 X11 库作为 dlopen 名，后一次 Xvfb 又因启动命令已结束而退出；
修正隔离库查找并将 Xvfb 生命周期与验证器放在同一命令中，未修改生产源码来绕过失败。

最终 `out/linux/sdl3-focus-x11-02/verification.json` 为 passed=true，退出 0、未强停，
video_driver=x11、renderer=software；同一程序在 dummy 后端也通过。
七帧、两次按下/释放、一次失焦/恢复、一次合成释放、两次失焦输入忽略、三次外来窗口事件均符合条件。
实际检查 pressed/unfocused/restored/repressed 图像，失焦背景与矩形颜色、恢复和重新按下符合脚本。
应用 SHA256：`6d18ee7c4b0098b8de55f80481f423573f3db22faa378860536b43789955849c`；
fixture SHA256：`6800308e31d8921fe76583f94b56d96b231f3d5a09d74898e64e6a809d97bc6d`。

反向场景 `out/linux/sdl3-focus-x11-negative-01` 只把失焦背景改错，程序仍退出 0，
验证器退出 1、passed=false、未强停，报告 `unfocused.bmp(10,10): 203040 != 102030`。

## 后续 CI 与系统输入验证

`6df41e8` 的 CI [41736796911](https://github.com/2468785842/krkr2/actions/runs/41736796911)
三平台 platform、archives、imageops、launch 共 12 个 job 全部通过，含每平台 12 项平台契约及闭包检查。
Linux SDL 原型 job 链接失败：`SDL_ime.c` 引用的 `SDL_IBus_*` 未定义。
SDL 3.4.8 的 `SDL_ibus.c` 实现在 `HAVE_IBUS_IBUS_H && SDL_USE_LIBDBUS` 下，
而调用方只检查前者；vcpkg 默认启用 ibus 却未启用 dbus。
后续修正为 Linux 显式选择 SDL3 dbus feature，保留默认 IBus/Wayland/X11 能力，等待新 CI 复核；
未通过关闭 IME 或空函数消除链接错误。
`ca05f3c` 的后续 [41737820166](https://github.com/2468785842/krkr2/actions/runs/41737820166)
在依赖安装阶段失败：DBus 传递依赖 libxcrypt 的 autoreconf 检查要求
`autoconf autoconf-archive automake libtoolize`。补齐 CI 宿主 Autotools 后继续复核，
不把这一阶段的失败归为原型源码或输入测试失败。

### X11 系统输入探针与关闭修复

队列自测通过后，另用 X server 的实际焦点和 XTest 键盘注入调查原型；没有直接投递 SDL 事件。
`out/linux/sdl3-x11-os-focus-01` 暴露关闭缺陷：第一次 WM_DELETE_WINDOW 即触发
`close-accepted:0`、`close-accepted:1` 并退出 0，没有等待第二次 OS 关闭。
SDL 3.4.8 的 `src/events/SDL_windowevents.c` 在最后一个窗口请求关闭后默认追加全局 QUIT，
导致脚本被查询两次；此前 SDL_PushEvent 自测不经过这条生成路径。

正式探针 `tools/sdl3-prototype/verify_x11.py` 按 PID 查找窗口，经 XSetInputFocus、
XTestFakeKeyEvent、WM_DELETE_WINDOW 操作，按本轮事件确认推进；首次否决后观测 200 ms，
检查进程仍存活且未接受，再提交第二次关闭。20 秒全局截止，超时保存失败并明确标出强停。
`sdl3-os-close-baseline-01` 仅增加日志即时刷新，应用 SHA256 为
`51ceacf942a2047a767b9b4258bc65b6c7e52a3fc633dc5d2f18a2d4fa65e962`，
探针实际失败：`First OS close bypassed the script veto`，程序退出 0、未强停。

修复为以 override 优先级关闭 SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE，
独立的应用 QUIT 事件仍执行脚本查询。开启环境变量 `SDL_QUIT_ON_LAST_WINDOW_CLOSE=1`
验证应用契约不受默认 hint 优先级影响，执行：

```sh
xvfb-run -a python tools/sdl3-prototype/verify_x11.py out/linux/sdl3-focus/tools/sdl3-prototype/krkr2_sdl3_prototype out/linux/sdl3-os-close-fixed-01
xvfb-run -a python tools/sdl3-prototype/verify.py out/linux/sdl3-focus/tools/sdl3-prototype/krkr2_sdl3_prototype out/linux/sdl3-close-fixed-queue-01
```

两项退出 0、passed=true、未强停；应用 SHA256 为
`1719a45f55ec0dc51fe4fefc853d329a394335b59ebb1cba806a276d1cb75904`。
系统探针观察到 SDL 在失焦通知前已自动释放 A，synthetic_releases=0，
两次按下/抬起、一次失焦/恢复与两次独立关闭顺序正确，没有重复 key-up。
队列自测仍为七帧，显式合成释放一次；两条路径分别验收，不能要求系统探针也合成一次释放。
Linux CI 增加同一 X11 探针并保存证据。

同步后的完整 Windows 应用基线 `bfeb707` 在
[41734855326](https://github.com/2468785842/krkr2/actions/runs/41734855326) 构建成功，
两个渲染器及两个 ImageOps 编译源检查通过并生成二进制附件。
旧 `4a4a615` 的 Bison 生成失败未在这次重跑复现，没有据此推断根因或修改语法/工具链；
该 full 结果对应基线，不冒充本批最终提交或游戏交互验收。

## 未验证与下一步

本批 Windows/macOS 原型尚未运行；DBus 修正与新增系统探针的 CI 待推送后核对结果。
SDL_PushEvent 不更新物理设备状态，自测从模拟聚焦开始；X11 窗口并不意味着实际 OS 切焦/键盘验收。
系统探针另覆盖 X server 切焦与 XTest 虚拟键盘，不是物理设备、IME 或现有 Cocos 验收。
继续按[平台契约](../../proposed/architecture/2026-11-03-platform-contracts.md)建立现有 Cocos 的真实失焦基线，
再迁移键盘/鼠标/手柄释放和 System 停用事件；坐标/DPI/IME、暂停恢复与销毁仍待处理。
