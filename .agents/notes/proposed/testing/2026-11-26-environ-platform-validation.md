# environ 平台拆分后的原生交互验证

Status: proposed

## 已实现的结构与验证

基于 `ff120ab910a9a86ac38f8ea7bc96208b25ba8dcf` 的未提交改动，将系统实现迁入
`cpp/core/environ/platform/windows/`、`platform/linux/`，SDL 平台服务迁入
`sdl3/windows/`、`sdl3/linux/`。统一接口保持不变，公共 SDL 实现不再判断操作系统，
由模块 CMake 显式选择实现。现状说明见 `docs/subsystems/native-runtime.md`。

Windows 普通 Debug/full 构建，`KRKR2_ENABLE_UI_TEST_DRIVER=OFF`：

- `cmake --preset "Windows Debug Config"` 首次等待 vcpkg 文件锁；CLion 同时配置相同目录，
  两次配置均停滞。结束这两次配置后，使用现有 SDK 加
  `-DVCPKG_MANIFEST_INSTALL=OFF` 完成配置，未改预设或依赖清单。
- `cmake --build --preset "Windows Debug Build" --parallel 2`：退出 0。
- `ctest --test-dir out/windows/debug -N`：实际注册 119 项。
- `ctest --test-dir out/windows/debug --output-on-failure --no-tests=error`：119/119 通过，退出 0。
- `python scripts/check_core_module_deps.py`：11 个 foundation 目标约束检查通过，不证明 domain 无环。
- `python scripts/check_native_runtime.py out/windows/debug`：退出 0，375 个编译单元、109 个 SDK ports。
- `python tools/game-smoke/imageops_repro.py out/windows/debug/bin/krkr2/krkr2.exe out/game-smoke/environ-refactor-imageops-01 --renderer software`：
  退出 0，四项缩放、模糊、仿射、透视通过，正常退出。

exe SHA256：`dbc7018360042396159de62b455a76c35882d23b32b200b69a93204641773b68`。
原始日志：`out/environ-refactor-*.log`；Windows 编译清单确认只选择新的 Windows 平台源。
Linux 系统源码去掉构建已确定的平台宏；Windows 系统、Linux 菜单接线和 GTK 对话框迁移内容保持一致。
这些静态对照不能代替 Linux 编译和交互验证。

## 未完成的原生交互验证

两次运行 `tools/game-smoke/sdl_windows_repro.py`，分别使用全新目录
`out/game-smoke/environ-refactor-windows-01`、`environ-refactor-windows-02`，均退出 1。
窗口创建、初始键盘输入和切焦释放已有事件；焦点辅助窗口切回主窗口时，等待
`foreground ownership` 超时，`forced_stop=true`、`passed=false`。
后续菜单、文件对话框、关闭否决和接受未完成，不作为通过证据。
已查看首轮 `native-window.png`，实际窗口存在。原因尚未定位，不能认定为平台拆分导致或纯环境问题。

## 下一步与完成条件

在可稳定取得前台焦点的 Windows 桌面重跑全新目录，检查
`tools/game-smoke/sdl_windows_repro.py` 中辅助窗口后的 `foreground(main)`。
若继续复现，对照同机基线区分宿主行为与输入控制器问题；不要修改通过条件或绕过焦点检查。
要求菜单、消息框、文件选择、关闭否决和接受全部完成，退出 0、`forced_stop=false`，并查看截图。
Linux 另需编译及 GTK 对话框轮询/关闭验收。本记录在这些条件满足前保持 proposed。
