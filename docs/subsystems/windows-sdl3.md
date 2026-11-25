# Windows 桌面宿主

默认 SDL3 构建使用正式 TJS2/KAG、Window/Layer、归档、插件和音视频实现，
由 SDL3 创建窗口、处理输入并呈现软件合成画面。此配置不查找、编译、链接或安装 Cocos。
Linux 同样默认提供[独立宿主](native-runtime.md)，macOS/Android 暂停构建。

## 构建和运行

在 MSVC x64 开发环境设置 `VCPKG_ROOT`，把 win_bison 放入 PATH 后执行：

```powershell
cmake --preset "Windows Debug Config"
cmake --build --preset "Windows Debug Build" -j 12
ctest --test-dir out/windows/debug -N
ctest --test-dir out/windows/debug --output-on-failure --no-tests=error
out/windows/debug/bin/krkr2/krkr2.exe --game D:/games/example
```

输出目录为 `out/windows/debug/bin/krkr2/`。默认宿主为 SDL3，也接受显式
`--host=sdl3`；其他宿主名称返回 2。只支持 `renderer=software`，显式请求其他渲染器
返回 2，不建立 Cocos/GL 回退。游戏目录、归档、引擎参数和临时设置语法沿用
[桌面命令行](desktop-cli.md)。

标准预设沿用根构建开关，默认构建应用、PSD、工具与 full 范围测试。
`KRKR2_TEST_SCOPE=platform` 可缩小测试范围；组件输入驱动默认关闭。
Cocos 专属测试已删除。

CMake 在配置前选择 `engine`、`sdl3-prototype` 等 manifest features；旧 `cocos`、`crash-upload` features 和端口已经删除。

## 平台服务

- `platforms/windows/main.cpp`：UTF-16 命令行转 UTF-8，直接进入 SDL3；不构造 Cocos delegate。
- `cpp/core/environ/sdl3/SdlHost.cpp`：生产窗口、事件、帧循环、视频呈现和最终画面截图。
- `cpp/core/environ/sdl3/NativeServices.cpp`：两平台共享的资源/配置安装、文件复制、URL、窗口和截图接线。
- `cpp/core/environ/sdl3/SdlServices.cpp`：Windows 原生文件对话框、弹出菜单及调试器日志。
- `cpp/core/environ/win32/Platform.cpp`：Windows 系统服务；独立模式通过 SDL3 原生消息框保留自定义按钮文本和返回索引。
- `cpp/core/visual/PVRTCDecoder.cpp`：独立保留原来的 2/4 bpp 解码算法，来源和许可位于
  `assets/licenses/PVRTCDecoder.txt`；软件模式没有删掉压缩图像读取能力。

全局设置继续位于 `%LOCALAPPDATA%/<可执行文件名>/.preference/`，与旧 Windows 宿主一致。
字体、语言等通过 LocalAssetReader 从程序旁的 `Resources` 读取；不打包 Cocos 管理界面的
`img/`、`out_ui/`。未使用的旧 minizip 写入器不进入此构建；正式归档读取器保持原实现。
PSBFile 清除了未使用的 Cocos RapidJSON 头文件引用。

## 原生交互验证

需要交互式 Windows 桌面及 Python Pillow；测试会操作自己启动的窗口。

```powershell
python tools/game-smoke/sdl_windows_repro.py out/windows/debug/bin/krkr2/krkr2.exe out/game-smoke/sdl-windows-01
```

工具使用全新自制场景，验证 SendInput 键盘、原生切焦后的按键释放、最小化恢复、
两次菜单切换、消息框、文件选择取消/确认、关闭否决和接受。动作等待对应脚本回调，
报告要求 `forced_stop=false`、退出 0、全部事件和选中文件路径正确。
截图采集实际窗口，不启用组件驱动。它不代表物理高 DPI、IME 候选窗或手柄设备验收。

真实游戏使用 [隔离副本方法](../guides/game-testing.md)。商业资源、存档和原始截图留在忽略的
`out/`，本轮构建身份、游戏步骤与限制见
[Windows SDL3 实施记录](../../.agents/notes/implemented/testing/2026-11-24-windows-sdl3-independent.md)。

Windows 音视频播放同步、完整剧情、物理高 DPI/IME/手柄仍需专项验收。
Release 构建使用 `Windows Release Config/Build`；打包命令为
`python scripts/package_native_runtime.py out/windows/release out/native-packages`，系统动态库要求见包内清单。

2026-11-25 删除旧宿主后，Windows 普通 Debug/full 构建的 119 项测试、软件图像与原生交互已通过；
本次证据及控制器修正见[删除记录](../../.agents/notes/implemented/architecture/2026-11-25-desktop-only-cocos-removal.md)。
删除后也已在新隔离副本重跑《天神乱漫》短剧情、新存档、同进程及新进程读档和正常退出；完整剧情未验证。
