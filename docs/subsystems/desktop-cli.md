# 桌面命令行启动与设置

当前支持 Windows/Linux/macOS，默认使用 SDL3；Android、iOS 暂停构建。
旧 Cocos 宿主与游戏管理界面已删除，脚本菜单及对话框使用原生平台服务。

## 使用

```sh
krkr2 --game /games/example
krkr2 --game /games/example/data.xp3
krkr2 --xp3 /games/example/data.xp3
krkr2 /games/example/data.xp3
krkr2 --game /games/example -- -debug=yes -contfreq=60
krkr2 --game /games/example --set renderer=software --set fps_limit=60
krkr2 --list-settings
krkr2 --help
```

路径含空格时加引号。入口将参数转换为 UTF-8 交给共享解析器。
`--game=PATH`、`--xp3=PATH` 也可使用，只允许一个目标。
目录优先使用散装 `startup.tjs`，否则使用 `data.xp3`；显式归档沿用原读取器检查根启动脚本。
patch 搜索顺序和存档路径继续由引擎处理，不解包合并游戏。

参数或目标无效时返回 2，写 stderr，并在创建游戏窗口前结束。
`--help` 和 `--list-settings` 返回 0，不安装宿主配置服务或创建窗口。
这不表示所有运行期脚本错误都已转换为非零退出码。
`--host=sdl3` 可显式指定唯一宿主，其他宿主名称返回 2。
SDL3 固定使用软件渲染，命令行显式指定其他渲染器会被拒绝。

`--` 后仅接受吉里吉里 `-name` / `-name=value`，省略值等同 `yes`；保留空值、第二个等号和覆盖顺序。
旧单横线引擎参数与 `-loglevel` / `--loglevel` 写法保留。
未知双横线选项明确拒绝；没有 `--config` 草案开关。

## 引擎设置

`--list-settings` 列出设置键和值，`--set KEY=VALUE` 和 `--set=KEY=VALUE` 可重复使用，同键最后值生效。
未知键和无效值返回 2；`-name=value` 仍传给原引擎参数和脚本。
部分旧宿主的显示/GL 偏好键保留用于配置兼容，不表示 SDL3 已实现对应旧 UI 或 GPU 后端。

优先级为命令行、游戏 `Kirikiroid2Preference.json`、全局 `GlobalPreference.json`、内建默认值。
命令行覆盖只在当前进程生效。JSON 不存在时只读迁移旧 XML，保留原文件。
布尔设置使用 `0` / `1`。路径和格式见[宿主资源与 JSON 配置](host-resources.md)。

## Linux SDL3 游戏宿主

```sh
cmake --preset "Linux Release Config"
cmake --build --preset "Linux Release Build" --parallel 2
out/linux/release/bin/krkr2/krkr2 --game /path/to/game
```

宿主运行正式 TJS、Window/Layer、存储、插件和音视频模块；平台服务在归档校验前安装。
原生构建和实际窗口、对话框、软件图像、KAG 存读档与音视频探针见[桌面运行时](native-runtime.md)。
工作流 `.github/workflows/native-desktop.yml` 构建 Windows/Linux 普通应用并执行原生验收。
旧 `sdl3_host_contract` / `sdl3_desktop_contract` 工作流开关已删除。

## 自动验证

```sh
cmake -S . -B out/launch-tests -G Ninja -DCMAKE_BUILD_TYPE=Debug -DKRKR2_BUILD_APP=OFF -DBUILD_TOOLS=OFF -DENABLE_TESTS=ON -DKRKR2_TEST_SCOPE=launch
cmake --build out/launch-tests --target launch_tests
ctest --test-dir out/launch-tests -N
ctest --test-dir out/launch-tests --output-on-failure --no-tests=error
python tools/game-smoke/cli_smoke.py /path/to/krkr2
```

launch-only 只配置 `core_launch` 和 Catch2，不安装引擎基础层或媒体库。
应用目标存在时登记 CLI 和资源同步回归。真实游戏测试使用[隔离副本](../guides/game-testing.md)。
