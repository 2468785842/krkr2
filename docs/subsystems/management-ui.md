# 移动端管理界面与桌面资源边界

桌面构建通过命令行选择游戏和设置引擎。Android/iOS 构建继续包含游戏选择器及全局和单游戏设置页。
[桌面命令行](desktop-cli.md)

- `environ/CMakeLists.txt` 只为移动端编译 `MainFileSelectorForm.cpp`、`GlobalPreferenceForm.cpp` 和
  `IndividualPreferenceForm.cpp`。`AppDelegate.cpp` 在移动端入口选择文件列表或直接启动。桌面在启动图形宿主前解析参数。
- 桌面 Cocos 宿主的游玩菜单包含游戏脚本菜单、窗口切换、键盘输入和退出。虚拟鼠标模式通过 `--set virtual_mouse=1` 在启动时开启。
- `FileSelectorForm.cpp` 与 `InGameMenuForm.cpp` 继续提供脚本可用的文件选择和菜单。通用表单控件、脚本对话框及共享资源仍由 Cocos 提供。
- `ConfigManager/PreferenceDefaults.inc` 供运行时与移动端设置页共用。桌面 `--set` 值优先于持久设置，在进程内生效。
- `cmake/RuntimeAssets.cmake` 的桌面清单排除启动选择器专用的 `syssetting_btn_ff.png`、`syssetting_btn_on.png`、
  `MainFileSelector.csd`、`MenuList.csd` 和 `RecentListItem.csd`。Windows/Linux 的资源同步也移除旧构建目录中这些文件。macOS 打包使用选中清单。

Linux/Windows CI 的 `cli_smoke.py` 直接运行应用，检查无目标、帮助、设置列表和参数验证。
`test_resources.py` 检查同步清单、保留自定义文件和错误输入。
