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

构建环境、生成器及依赖继续按 [构建与验证](../guides/building.md) 设置。
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
真实游戏插装见 [游戏测试](../guides/game-testing.md)。


本机验证记录见 [管理界面验证](../../.agents/notes/implemented/testing/2026-11-02-management-ui-validation.md)。
