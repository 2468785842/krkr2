# 可选管理界面（U1）

状态：已实现首批 Desktop 裁剪。`KRKR2_USE_COCOS_UI` 默认 `ON`，在根 `CMakeLists.txt` 定义。
本开关控制启动选择器及全局/游戏设置页面；游戏窗口、游戏内菜单、脚本对话框仍使用现有 Cocos 实现。
Cocos 依赖、渲染器、移动端外壳替换尚未完成。

## 使用与行为

在既有 Desktop 构建目录配置并构建：

```sh
cmake -S . -B out/windows/debug -DKRKR2_USE_COCOS_UI=OFF
cmake --build out/windows/debug --target krkr2
out/windows/debug/bin/krkr2/krkr2.exe --game D:/games/example
```

构建环境、生成器及依赖继续按 [构建与验证](../building.md) 设置。
关闭后必须传入游戏目标（`--game`、`--xp3` 或位置参数）；无目标返回 2 并显示用法，`--help` 返回 0。
重新配置为 `ON` 恢复原选择器与设置页。Android/iOS 的 `OFF` 在配置时明确拒绝，等待独立启动入口。

## 代码及资源边界

- `cpp/core/environ/CMakeLists.txt` 按开关登记 `MainFileSelectorForm.cpp`、
  `GlobalPreferenceForm.cpp`、`IndividualPreferenceForm.cpp`；关闭时不编译这三个实现。
- `AppDelegate.cpp` 按开关引用选择器；`launch/DesktopLaunch.cpp` 在 GL 初始化前检查目标。
  `ui/csd/CsdUIFactory.*` 与 `CsdUIWidgets.*` 同时裁掉专用工厂函数。
- `ui/FileSelectorForm.cpp` 包含脚本文件选择实现，`ui/PreferenceForm.cpp` 的控件用于游戏菜单，
  两者继续编译。`GameMainMenu`、`InGameMenuForm`、`MessageBox` 及共享资源保留。
- `ConfigManager/PreferenceDefaults.inc` 是运行时与页面共用的默认值表。
  `PreferenceDefaults.cpp` 在 GL 就绪后初始化配置，沿用未设置时的 Adreno 扩展规避行为，保留已有用户值。
  页面只在实际打开时创建配置描述，不再承担启动期默认值初始化。
- `cmake/RuntimeAssets.cmake` 生成完整/选中资源清单。关闭时排除
  `img/syssetting_btn_ff.png`、`img/syssetting_btn_on.png`、`out_ui/MainFileSelector.csd`、
  `out_ui/MenuList.csd`、`out_ui/RecentListItem.csd`。
  Windows/Linux 通过 `cmake/scripts/sync_runtime_assets.py` 复制选中资源并移除输出目录中的上述排除项，
  因此同目录从 `ON` 切换为 `OFF` 不会残留它们；其他自定义文件不清理。Apple 包资源使用同一选中清单。

## 验证

```sh
cmake --build out/windows/debug --target krkr2 launch_tests
ctest --test-dir out/windows/debug -R "desktop|runtime|Adreno|launch_cli_smoke" --output-on-failure --no-tests=error
python -m unittest discover -s tools/game-smoke -p "test_*.py" -v
python scripts/check_core_module_deps.py
```

`PreferenceDefaultsTest.cpp` 验证持久配置、Adreno 条件覆盖和初始化幂等性；
资源测试验证双向切换、保留自定义文件、非法清单及缺失源文件时不进行清理。
`launch_cli_smoke` 在关闭模式增加无目标退出检查。纯 `launch` 测试仍不依赖 Cocos/GL。
真实游戏插装见 [游戏测试](testing.md)。

## 本地 Windows 验证记录（2026-11-02）

复用本机已安装依赖，在 `out/windows/engine-config-check` 先构建 `ON`，再同目录切为 `OFF`：

| 项目 | ON | OFF |
|---|---|---|
| 应用与 launch_tests 构建 | 通过 | 通过 |
| 相关 CTest | 10/10 | 10/10 |
| CLI 子进程 help/错误检查（包含于 CTest） | 10 项 | 11 项，增加无目标返回 2 |
| compile_commands 中唯一源码（含测试及外部源码） | 342 | 339 |
| 其中 environ 源码 | 38 | 35 |
| 打包资源 | 83 个，19,934,480 字节 | 78 个，19,868,162 字节 |
| tslm 隔离副本 | 25 秒通过，33 个状态样本 | 25 秒通过，42 个状态样本 |

两种游戏运行都到达 `title.ks:33`，截止时心跳新鲜且无 smoke-error，
真实 TJS 的中文、重复覆盖、空值和标志参数探针通过。到期由控制器终止，不能作为正常退出验收。
原始游戏 `D:/krkr/tslm` 未写入；本轮副本位于 `out/ui-game-smoke/tslm-u1-on` 与 `tslm-u1-off`，
XP3 只硬链接到先前隔离副本，存档独立。

`ON` 无参数运行 6 秒，日志确认选择器 Form 创建；
`OFF` 静态库成员清单确认三个管理页面被移除，脚本文件选择、共享偏好控件、游戏菜单及消息框实现保留。
同一资源目录完成 `ON → OFF → 恢复完整资源 → OFF` 检查，排除项实际清除且可恢复。
按平台条件排列的 29 条原页面默认值与新表逐项一致；单独 launch 构建的 8 项 CTest、
Python 控制器/资源共 7 项测试、clang-format 20、依赖检查、diff 检查及 CI YAML 解析通过。
Android 关闭模式在配置阶段按预期拒绝。

编译清单及资源比较记录在 `out/windows/u1-on.json`、`u1-off.json`；
构建日志为 `u1-on-build.log`、`u1-off-build.log`；游戏状态与日志在各副本目录。
该验证目录沿用 L1 的临时关闭 PDB/增量链接 hook，不据此宣称发布体积或编译耗时收益。

Linux/macOS 应用及移动端默认开启构建尚未实机验证，远程 CI 未运行。
游戏内菜单和脚本对话框本轮只核对依赖、源码及链接保留，未完成交互回归；
存读档、音视频、正常退出、像素对照及完整游戏兼容不在通过范围中。
完整散装游戏的资源加载问题仍见 [L1 记录](../launch/desktop-cli.md)。
