# Linux 无 Cocos 运行时

`KRKR2_SDL3_ONLY=ON` 构建正式的 Linux SDL3 应用。它使用生产 TJS、Window/Layer、
存储、插件、音频和 FFmpeg 视频模块；配置、编译、链接和运行均不需要 Cocos。
Windows 使用同一独立构建开关，见[Windows SDL3](windows-sdl3.md)；macOS、Android 仍使用原宿主。
旧 Linux 开关 `KRKR2_COCOS_FREE` 保留为兼容别名，新配置统一使用 `KRKR2_SDL3_ONLY`。

## 构建与运行

先安装 CMake 3.28 以上、Ninja、C++17 编译器、Python、Bison、Yasm、Autotools、
pkg-config，以及 Linux X11/Wayland、GTK3 的系统开发依赖。可执行的 Ubuntu 安装清单见
[独立构建工作流](../../.github/workflows/cocos-free-linux.yml)。设置 `VCPKG_ROOT`，
保持 manifest 安装开启，由仓库 overlay 构建 FFmpeg 开发库等依赖。

```sh
cmake --preset "Linux SDL3 Config" \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_INSTALLED_DIR="$PWD/out/native-sdk"
cmake --build out/linux/sdl3 --parallel 2
out/linux/sdl3/bin/krkr2/krkr2 --game /path/to/game
```

此预设开启完整通用测试、脚本 smoke，关闭 UI 测试驱动和命令行工具。
程序默认选择 SDL3；显式 `--host=cocos` 会报错。使用独立安装目录可确认未复用旧 Cocos SDK。
公共 `engine` feature 不含 Cocos；旧宿主额外选择 `cocos` feature，正式 Linux SDL3 宿主额外选择 `native-dialogs` 引入 GTK3，独立实验不需要此依赖。
不能同时开启依赖旧宿主的崩溃上传功能。

## 宿主实现与边界

- 启动入口 `platforms/linux/sdl_main.cpp` 经共享 `sdl3/NativeServices.cpp` 工厂，在归档校验之前安装本地资源和配置服务，
  SDL3 宿主驱动生产帧循环、窗口、键盘、鼠标、文本输入和视频呈现。
- Layer 使用软件渲染，经 SDL3 呈现。已有 `renderer=opengl` 设置回退到软件工厂；
  未知工厂名称继续报错。原 OpenGL 渲染器属于旧宿主，不在独立构建中注册。
- GTK3 提供菜单窗口、嵌套菜单、输入框、任意按钮消息框和文件选择。
  菜单保留脚本对象，选择结果经引擎输入事件队列返回；普通按钮、勾选、可见性和禁用状态均由真实控件处理。
- 控制台经统一日志输出；旧 Cocos 叠加控制台通过可选观察器接收消息。
  文件复制、外部 URL 打开和 PVRTC 解码也有独立实现，不依赖空桩或旧宿主单例。
- 两平台共享 `visual/PVRTCDecoder.cpp` 的原有 2/4 bpp 解码算法和许可，保留既有像素行为。
  来源见[上游参考](../reference/upstream.md)。Linux 早期试用的现代 PowerVR 解码器已移除，
  不再保留两套算法或已发现的低位颜色差异。
- 打包保留字体、语言、图标和许可；排除 Cocos 管理 UI 的 `img/`、`out_ui/`。
  本程序从命令行选择游戏，不包含旧管理器界面。

## 验证

```sh
python3 scripts/check_native_runtime.py out/linux/sdl3
ctest --test-dir out/linux/sdl3 -N
xvfb-run -a ctest --test-dir out/linux/sdl3 --output-on-failure --no-tests=error
python3 tools/game-smoke/cli_smoke.py out/linux/sdl3/bin/krkr2/krkr2 --host=sdl3
xvfb-run -a python3 tools/game-smoke/sdl_host_repro.py \
  out/linux/sdl3/bin/krkr2/krkr2 out/native-evidence/host
xvfb-run -a -s '-screen 0 1280x800x24' python3 tools/game-smoke/sdl_dialog_repro.py \
  out/linux/sdl3/bin/krkr2/krkr2 out/native-evidence/dialogs
LIBGL_ALWAYS_SOFTWARE=1 ALSOFT_DRIVERS=null xvfb-run -a \
  python3 tools/game-smoke/imageops_repro.py out/linux/sdl3/bin/krkr2/krkr2 \
  out/native-evidence/imageops --renderer software
```

依赖检查核对 CMake 身份、不含 Cocos 的 SDK 闭包、实际编译/链接命令、动态库和资源包，
并检查两份 engine 清单的公共依赖是否漂移。完整通用测试不注册仅对 Cocos 适配器有意义的
窗口及旧资源对照测试。窗口/对话框控制器使用 X11/XTest 真实系统输入，保存引擎哈希、
事件、截图、退出状态；脚本控制文件只请求打开界面，不代替用户输入。
KAG 存读档和生成音视频验收命令见工作流，使用固定 KAG3 提交和自制素材。

当前独立应用已在 Linux X11 软件呈现下完成上述运行验收；Wayland、物理高 DPI、
其他桌面和移动平台不包含在这次独立构建的运行验收结论中。

## Release 打包

```sh
python3 scripts/package_native_runtime.py out/linux/sdl3 out/native-packages
```

打包器先检查独立构建身份，只接受 Release、普通应用配置；按当前资源清单打包，
排除旧构建目录残留。压缩包含程序、选中资源、许可、每个文件的 SHA256、源码版本/dirty 状态
和动态依赖清单；旁边另有压缩包 SHA256。Linux 输出 tar.gz，Windows 输出 zip。
系统动态库不随包分发，运行环境需安装清单中的库；Linux glibc 不能低于构建主机。
CI 在 Ubuntu 24.04 和 Windows 2022 构建两份 Release 包，商业游戏素材/存档不进入 CI 或发布包。

首次音轨另有 `audio-first-buffer-runtime` 回归：测试专用预加载库延迟工作线程启动，
通过真实 OpenAL PCM 验证首次播放不会因异步启动确认而丢失。辅助库不链接进生产程序或发布包，
正常媒体探针继续要求 BGM、SE、视频三条音轨同时存在。
