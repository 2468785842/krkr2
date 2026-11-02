# Desktop 启动入口（L1）

状态：已实现的首批入口；现有 cocos 游戏窗口、管理界面和游戏内菜单仍保留。
`launch.json`、GlobalConfig overlay、RN 和新窗口后端尚未实现。

## 使用

```sh
krkr2 --game /games/example
krkr2 --game /games/example/data.xp3
krkr2 --xp3 /games/example/data.xp3
krkr2 /games/example/data.xp3
krkr2 --game /games/example -- -debug=yes -contfreq=60
krkr2 --help
```

Windows 路径含空格时加引号。所有桌面入口把参数转换为 UTF-8 后交给同一解析器。
`--game=PATH`、`--xp3=PATH` 也可使用；只允许一个目标。
默认构建不带目标时打开现有文件选择界面。
[关闭管理界面](../ui/optional-management-ui.md)（`KRKR2_USE_COCOS_UI=OFF`）后，无目标返回 2。指定目标时直接调度游戏启动，不创建文件选择 Form。

目录优先使用散装 `startup.tjs`，否则使用目录内 `data.xp3`；无两者时返回错误。
显式文件沿用既有归档读取器检查是否包含根 `startup.tjs`，因此仍可使用已有的其他受支持归档。
目录中其他归档、patch 优先级及存档路径由原引擎负责，不解包合并，不更换存储后端。

参数错误、路径缺失、目录无入口、无效归档或归档缺启动脚本返回 **2**，写 stderr，
并在创建 GL 窗口前结束。`--help` 输出用法、返回 **0**。
这不等于所有游戏运行期脚本异常均已转换为非零退出码；运行期仍使用现有异常/对话框机制。
Windows GUI 子系统的输出可通过子进程 stdout/stderr 重定向读取。

`--` 后仅接受吉里吉里 `-name` / `-name=value`，`-name` 等同 `yes`，保留空值和第二个等号。
通过原 `TVPSetCommandLine` 写入，同名后项覆盖前项，不改变 TVPGetCommandLine。
旧单横线引擎参数与 `-loglevel` / `--loglevel` 写法保留。
未知双横线选项明确拒绝，`--config` 等设计草案中的开关当前不能使用。

## 实现边界

- `cpp/core/environ/launch/LaunchOptions.*`：C++17 参数/文件系统解析，静态目标 `core_launch`，
  不依赖 TJS、cocos、日志或归档库。本批语法规模固定，未新增 argparse 依赖。
- `DesktopLaunch.*`：引擎适配，保存目标、设置引擎参数、在 GTK/GL 初始化前调用现有归档校验并记录退出码。
  编译在桌面 `core_environ_module`，不进入纯解析库。
- `platforms/windows/main.cpp`、`platforms/linux/main.cpp`、`platforms/apple/macos/main.cpp`：
  在应用启动前解析参数。Linux 的 help/参数错误不初始化 GTK，不要求显示服务。
- `environ/cocos2d/AppDelegate.cpp`：场景就绪后直接调用
  `MainScene::startupFrom`。无目标使用原选择器，Android/iOS 保留原启动路径。

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
额外登记 `launch_cli_smoke`，使用自制损坏及缺 startup 的 XP3，无需商业游戏或 GL 窗口。

真实游戏只在隔离副本运行，见 [UI 测试](../ui/testing.md)。
game-smoke 的 `--launch-style game --target directory` 测 `--game` 目录，
`--launch-style xp3` 测别名与自动选择的归档，默认 legacy 保留旧位置参数测试。
`--engine-arg=-name=value` 可重复，原样放入目标后的 `--` 分隔区。
加 `--probe-engine-args` 会通过真实 `System.getArgument` 断言中文、重复键最后值、
包含等号的值、空字符串存在性和默认 yes；参数不可见时即使心跳正常也判失败。
主循环心跳、KAG 状态与合成快照仍沿用已验证的 AfterStartup 钩子。

## Windows 本轮验证

本批同时修正 `sysinit/impl/SysInitImpl.cpp` 的两处旧行为：初始化清空参数库存时，
重放初始化前设置的参数；区分本地项目目录与存在的归档文件，避免给散装目录追加归档分隔符。
`TVPGetCommandLine` 查询接口、更新后项优先及参数代数更新仍保留。

| 验证 | 结果 |
|---|---|
| launch-only 配置、编译、CTest | 5/5；编译清单仅 LaunchOptions.cpp / LaunchOptionsTest.cpp |
| 应用与 launch_tests | 链接通过；复用本机现有依赖，不代表全新依赖安装 |
| 应用构建目录的启动 CTest | 6/6，含 CLI 集成检查 |
| CLI help/错误进程检查 | 10 项通过；缺参、损坏 XP3、缺 startup 的真实 XP3、未知选项等按契约处理 |
| 控制器单元检查 | 3/3，含冻结心跳和缺失参数探针拒绝 |
| 无启动目标 | 5 秒运行，日志确认创建选择器且未直接启动游戏；随后测试进程被终止 |
| tslm 目录自动选 data.xp3、--xp3、位置参数 | 各 25 秒通过；含中文/空格路径、持续心跳、参数断言及无选择器日志 |
| 散装 startup 引导、归档中保留素材 | 25 秒通过，实际 KAG 到 title.ks:33，内置参数探针通过 |

原始状态和日志在 `out/ui-game-smoke/tslm-cli-final-*`，内置探针报告在
`tslm-cli-probe/smoke-report.json`；散装引导验证在 `tslm-cli-loose-bootstrap-v2/`。
该散装引导只用自制 startup 注册 data.xp3 的搜索路径，再执行原启动脚本，没有改游戏主体。

**完整散装的商业游戏尚未通过**：解包 data.xp3 的全部文件后，原启动脚本能够执行，
但在 custom.ks 标题界面遇到 `title@背景%base` 图片查找/加载异常，测试明确失败。
不能将目录入口通过等同于全部解包素材兼容；这需要另行检查合成资源的路径映射。

为保留用户正在运行的调试游戏，使用独立 `out/windows/engine-config-check` 验证。
该目录原 zlib 缓存引用已失效的库名，已仅在此目录重新查找现有库；未改全局工具链或安装依赖。
源码使用 Debug 配置，最终验证 exe 通过该目录的临时 CMake hook 关闭 PDB/增量链接，
避免调试符号链接阻塞；不把它用于体积或耗时收益比较，也不替代带符号的调试构建验证。

clang-format 20、模块依赖检查和 diff 检查通过。新增无窗口 CI launch job 覆盖三桌面平台，
已检查 YAML 与本地构建，但未在本轮触发远程 CI；Linux/macOS 的应用入口及 Android/iOS 的
共享初始化改动尚未实机验证。存档恢复、音视频、正常退出和完整游戏兼容不在本轮通过清单中。
