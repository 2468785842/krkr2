# 桌面命令行启动与设置

Windows、Linux 与 macOS 使用命令行启动游戏和设置引擎。Linux 可通过显式开关构建 SDL3 游戏宿主。

## 使用

```sh
krkr2 --game /games/example
krkr2 --game /games/example/data.xp3
krkr2 --xp3 /games/example/data.xp3
krkr2 /games/example/data.xp3
krkr2 --game /games/example -- -debug=yes -contfreq=60
krkr2 --game /games/example --set renderer=opengl --set fps_limit=60
krkr2 --list-settings
krkr2 --help
```

Windows 路径含空格时加引号。所有桌面入口把参数转换为 UTF-8 后交给同一解析器。
`--game=PATH`、`--xp3=PATH` 也可使用；只允许一个目标。
无目标时返回 2 并向 stderr 输出用法。`--help` 与 `--list-settings` 可直接运行。

目录优先使用散装 `startup.tjs`，否则使用目录内 `data.xp3`；无两者时返回错误。
显式文件沿用既有归档读取器检查是否包含根 `startup.tjs`，因此仍可使用已有的其他受支持归档。
目录中其他归档、patch 优先级及存档路径由原引擎负责，不解包合并，不更换存储后端。

参数错误、路径缺失、目录无入口、无效归档或归档缺启动脚本返回 **2**，写 stderr，
并在创建 GL 窗口前结束。`--help` 输出用法、返回 **0**。
这不等于所有游戏运行期脚本异常均已转换为非零退出码；运行期仍使用现有异常/对话框机制。
Windows 可直接在 PowerShell 或 cmd 中查看输出和退出码。

`--` 后仅接受吉里吉里 `-name` / `-name=value`，`-name` 等同 `yes`，保留空值和第二个等号。
通过原 `TVPSetCommandLine` 写入，同名后项覆盖前项，不改变 TVPGetCommandLine。
旧单横线引擎参数与 `-loglevel` / `--loglevel` 写法保留。
未知双横线选项明确拒绝，`--config` 等设计草案中的开关当前不能使用。

## 引擎设置

`--list-settings` 列出支持的键与取值。`--set KEY=VALUE` 和 `--set=KEY=VALUE` 可重复使用，放在 `--` 前面，
同一键取最后一次值。未知键和无效值以退出码 2 报告。`-name=value` 继续传给原有引擎参数和脚本接口。

游戏级配置的优先级为命令行、游戏的 `Kirikiroid2Preference.xml`、全局 `GlobalPreference.xml`、内建默认值。
命令行值只在当前进程内生效，原有 XML 会继续作为后续启动的默认配置。布尔设置用 `0` 或 `1`，
路径含空格时用引号。显示帧率、帧率上限、菜单透明度及虚拟鼠标是 Cocos 宿主设置。
`virtual_mouse=1` 在 Cocos 游戏宿主启动时启用虚拟鼠标。

## Linux SDL3 游戏宿主

在完整应用构建中显式开启 SDL3 选项，使用同一个 `krkr2` 可执行文件选择宿主。
Debian 和 Ubuntu 的 SDL3 构建依赖包含 `libdecor-0-dev`，用于 Wayland 客户端窗口控件。

```sh
cmake --preset "Linux Release Config" -DKRKR2_ENABLE_SDL3_HOST=ON -DBUILD_TOOLS=OFF
cmake --build --preset "Linux Release Build" --target krkr2
out/linux/release/bin/krkr2/krkr2 --host=sdl3 --game /path/to/game
```

`--host=sdl3` 要求显式游戏目标。默认宿主是 Cocos，也可用 `--host=cocos` 明确指定。
未编译 SDL3 的构建收到 `--host=sdl3` 时返回 2。SDL3 固定使用软件渲染，显式指定其他渲染器会返回 2。
SDL3 使用正式的启动脚本、`Window`、`Layer` 和 `System`。
引擎的软件渲染结果以 RGBA 纹理上传。X11 通过软件窗口表面呈现，Wayland 通过 EGL 呈现。
Cocos 仍提供部分资源和共享服务。窗口接口覆盖缩放和重绘、焦点切换、鼠标捕获、键鼠输入及
`Window.showModal()` 的应用内模态行为。提交文本通过现有 TJS `onKeyPress` 投递。
[SDL3 窗口契约](window-interface.md#linux-sdl3-窗口契约)

在具备 X11、XTest 和 Xvfb 的 Linux 环境运行正式入口探针。探针在外部读取实际窗口像素，
通过 XTest 发送键鼠输入、焦点切换和 XIM Compose 序列，并发送窗口管理器的关闭消息。
探针验证窗口缩放和黑边、窗口外拖动、重新显示、模态关闭否决及正常退出。

```sh
xvfb-run -a -s '-screen 0 1024x768x24' python tools/game-smoke/sdl_host_repro.py \
  out/linux/release/bin/krkr2/krkr2 out/linux/sdl-host-runtime
```

GitHub 的 Build Linux 工作流通过 `sdl3_host_contract=true` 构建并运行上述探针。
`sdl3_desktop_contract=true` 同时运行桌面验收，用 Openbox 和 Weston 验证窗口恢复与输入，
用 Fcitx5 验证中文候选窗和提交文本，并启动两次自制 KAG 场景验证存档与读档。
同一开关还验证 KAG 的音乐、音效和视频，以及 SDL3 的文件拖放、鼠标模拟、触摸回调和手柄热插拔。
工作流将回调记录、截图、音频输出和运行报告上传至 `linux-sdl-desktop-runtime`。

## 实现边界

- `cpp/core/environ/launch/LaunchOptions.*`：C++17 参数/文件系统解析，静态目标 `core_launch`，
  不依赖 TJS、cocos、日志或归档库。本批语法规模固定，未新增 argparse 依赖。
- `DesktopLaunch.*`：引擎适配，保存目标、应用当次设置、在 GTK/GL 初始化前调用现有归档校验并记录退出码。
  编译在桌面 `core_environ_module`，不进入纯解析库。
- `platforms/windows/main.cpp`、`platforms/linux/main.cpp`、`platforms/apple/macos/main.cpp`：
  在应用启动前解析参数。Linux 的 help/参数错误不初始化 GTK，不要求显示服务。
- `environ/cocos2d/AppDelegate.cpp`：桌面场景就绪后调用 `MainScene::startupFrom`。
  Android/iOS 继续使用移动端入口及管理界面。

本批不处理 Intent、配置合并、文件关联注册、C ABI 或渲染器替换；不移除脚本可观察的 UI。
桌面旧 `TVPCheckStartupArg` 不再解析命令行，仅保留平台启动服务（如可选崩溃上报）；
移动端仍保留原入口及事件注册。

## 自动验证

设置仓库构建环境后：

```sh
cmake -S . -B out/launch-tests -G Ninja -DCMAKE_BUILD_TYPE=Debug -DKRKR2_BUILD_APP=OFF -DBUILD_TOOLS=OFF -DENABLE_TESTS=ON -DKRKR2_TEST_SCOPE=launch
cmake --build out/launch-tests --target launch_tests
ctest --test-dir out/launch-tests -N
ctest --test-dir out/launch-tests --output-on-failure --no-tests=error
python tools/game-smoke/cli_smoke.py /path/to/krkr2
```

Windows 添加对应 triplet 与 MSVC 开发环境。launch-only 只选择 tests feature，
配置 core_launch 和 Catch2，不安装或配置引擎基础层、cocos、OpenCV 或媒体库。
若同时启用工具或应用，仍加入其真实依赖。完整测试也登记 launch_tests；应用目标存在时
额外登记 `launch_cli_smoke`，在无 GL 窗口的情况下检查帮助、设置列表、参数错误和损坏归档。

真实游戏只在隔离副本运行，见 [UI 测试](../guides/game-testing.md)。
game-smoke 的 `--launch-style game --target directory` 测 `--game` 目录，
`--launch-style xp3` 测别名与自动选择的归档，默认 legacy 保留旧位置参数测试。
`--engine-arg=-name=value` 可重复，原样放入目标后的 `--` 分隔区。
加 `--probe-engine-args` 会通过真实 `System.getArgument` 断言中文、重复键最后值、
包含等号的值、空字符串存在性和默认 yes；参数不可见时即使心跳正常也判失败。
主循环心跳、KAG 状态与合成快照仍沿用已验证的 AfterStartup 钩子。


本机验证记录见 [Desktop CLI 验证](../../.agents/notes/implemented/testing/2026-11-02-desktop-cli-validation.md)。
