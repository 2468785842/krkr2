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

## 未验证与下一步

本批 Windows/macOS 原型尚未运行；三平台平台契约和 Linux 原型 CI 待推送后核对结果。
SDL_PushEvent 不更新物理设备状态，自测从模拟聚焦开始；X11 窗口并不意味着实际 OS 切焦/键盘验收。
继续按[平台契约](../../proposed/architecture/2026-11-03-platform-contracts.md)建立现有 Cocos 的真实失焦基线，
再迁移键盘/鼠标/手柄释放和 System 停用事件；坐标/DPI/IME、暂停恢复与销毁仍待处理。
