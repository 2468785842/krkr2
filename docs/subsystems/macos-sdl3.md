# macOS 桌面宿主

macOS 接入正式 SDL3 宿主，使用 TJS2、生产 Window/Layer、存储和内建插件。
引擎使用软件渲染器，SDL Cocoa 驱动通过 Metal 上传和呈现 CPU RGBA 帧。
Cocoa 不提供原生软件窗口表面，因此不能沿用 X11 的 `software` 呈现驱动。
没有恢复 Cocos 依赖。当前本机验收架构为 Apple Silicon arm64。

## 构建和启动

安装 Xcode 命令行工具、CMake 3.28+、Ninja、Bison 3、Python 3.11+、vcpkg；
依赖由仓库 manifest/overlay 提供。设置 `VCPKG_ROOT`，并确保 PATH 中优先使用新版 Bison。
默认使用本机架构 triplet，交叉编译和 universal bundle 尚未验收。

```sh
cmake --preset "MacOS Debug Config" -DBUILD_TOOLS=OFF -DKRKR2_ENABLE_SCRIPT_SMOKE=ON
cmake --build --preset "MacOS Debug Build" --parallel 8
ctest --test-dir out/macos/debug -N
ctest --test-dir out/macos/debug --output-on-failure --no-tests=error
python3 scripts/check_native_runtime.py out/macos/debug
out/macos/debug/bin/krkr2/krkr2.app/Contents/MacOS/krkr2 --game /path/to/game/data.xp3
```

支持目录或 XP3 的 CLI 入口，与其他桌面平台相同；直接双击 `.app` 不会打开游戏选择器。
`--help` 和 `--list-settings` 可在无窗口环境使用。
Release 使用 `MacOS Release Config/Build`，产物位于 `out/macos/release/bin/krkr2/krkr2.app`。
`package_native_runtime.py` 支持生成保留 `.app` 布局、资源和第三方许可的 macOS ZIP；
没有签名、公证或安装器流程。

FFmpeg overlay 在 macOS 禁止 XCB 自动探测，防止偶然链接 Homebrew 的 Linux 窗口库。
若复用曾启用 XCB 的旧构建缓存，可重新配置时加 `-U 'FFMPEG*'` 清除其库查找结果；
无 Cocos 依赖检查会拒绝 bundle 外的非系统绝对动态库路径。

## 平台接线

- `platforms/macos/main.cpp` 使用 SDL main 接线进入通用 DesktopLaunch 和 SDL3 帧循环。
- `cpp/core/environ/platform/macos/Platform.mm` 提供文件时间、Mach 内存统计、语言和平台接口，
  从 Git 基线 `c494ea0` 的同仓库 macOS 实现迁移并修正目录创建和内存统计。
- `cpp/core/environ/sdl3/macos/HostServices.mm` 从 `.app/Contents/Resources` 读取资源；
  全局配置写到用户 `Library/Application Support/krkr2`，游戏存档仍由游戏路径规则决定。
- `NativeHost.mm` 提供 AppKit 菜单、打开/保存面板、消息框和文本输入框。
  菜单持有脚本对象直到原生菜单关闭，之后将点击交给引擎事件队列；取消不触发动作。
- 窗口、指针、键盘、文本提交、视口缩放和视频呈现复用 `sdl3/SdlHost.cpp`。
  Cocoa 只改变最终呈现驱动，不改变 Layer 软件渲染算法。

## 验证范围

本机证据见 [macOS 恢复记录](../../.agents/notes/implemented/simplification/2026-11-27-macos-sdl3-runtime.md)。
已检查完整构建、单元/脚本测试、软件图像场景、AppKit 对话框、菜单和真实 `tslm` 标题、
开始点击、键盘推进、关闭取消/确认。原始游戏及存档未修改。

未覆盖 Intel Mac、跨机发行、中文 IME 组合、物理手柄、完整剧情和存读档回归；
音频设备日志不等于实际音轨验收，FFmpeg 初始化测试不等于视频播放验收。
