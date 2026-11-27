# macOS SDL3 正式运行时恢复

Status: implemented

## 问题与基线

`1833e8079186342a749f48f27adff681eadaad92` 的无 Cocos 桌面分支仅允许 Windows/Linux。
macOS 被根配置和 vcpkg supports 显式拒绝，旧入口和平台实现已经删除。
本轮基于该提交的 dirty 工作区实现，脚本后端保持默认 TJS2。
参考同仓库 `c494ea0:cpp/core/environ/apple/macos/platform.mm`（沿用仓库许可），
只迁移系统接口，不恢复旧 Cocos/GLFW/UI 依赖。

用户要求清理两个 codex 分支：远端 `codex/ciallang-runtime` 与
`codex/remove-cocos-desktop-only` 经 fetch 后都指向完整提交 `1833e80...`，没有内容差异。
保留任务名称对应的 remove-cocos 分支，删除远端重复的 ciallang-runtime 分支。
HTTPS push 因本机没有凭据失败，随后通过已有 SSH 身份删除成功；未修改全局 Git 认证配置。

## 实现

- 恢复 Darwin 平台识别、Objective-C++17、Debug/Release 预设和 `.app` 资源同步。
- 使用 `platforms/macos/main.cpp` 进入现有 DesktopLaunch/SDL3 生产运行时。
- `platform/macos/Platform.mm` 恢复 Mach 内存统计、文件时间/操作、语言和系统服务。
  修正旧目录创建返回值、HOST_VM_INFO64 的 flavor/count、空闲内存宽度与 host port 释放。
- `sdl3/macos/HostServices.mm` 提供 bundle 资源和用户 Application Support 配置目录。
- `NativeHost.mm` 实现 AppKit 打开/保存面板、输入框、消息框与菜单。
  菜单跟踪期间持有脚本对象，在 AppKit 回调退出之后发布引擎菜单事件。
- Cocoa 通过 SDL Metal 呈现 CPU RGBA，保持引擎软件渲染和其他平台的驱动选择。
- macOS FFmpeg 禁止自动探测 XCB；依赖审计和打包脚本识别 Mach-O 与 `.app`。
  Apple CPU 特征检查限制为 arm64，避免把 Intel Mac 当成 NEON。

## 实际验证

环境：macOS 26.5.1、arm64、Apple Clang、CMake/Ninja、clang-format 20.1.0、
vcpkg baseline `aa40adda5352e87655b8583cfb2451d5e9e276fd`，普通应用 UI driver OFF。
所有命令在仓库根运行，原始输出在忽略的 `out/macos/`。

```sh
cmake --preset 'MacOS Debug Config' -DBUILD_TOOLS=OFF -DENABLE_TESTS=ON -DKRKR2_ENABLE_SCRIPT_SMOKE=ON
cmake --build --preset 'MacOS Debug Build' --parallel 8
ctest --test-dir out/macos/debug -N
ctest --test-dir out/macos/debug --output-on-failure --no-tests=error
python3 scripts/check_native_runtime.py out/macos/debug
python3 scripts/check_core_module_deps.py
python3 scripts/check_desktop_sources.py
python3 -m unittest discover -s scripts/tests
```

最终 Debug：实际注册并执行 **119 项，全通过**，其中 4 项集成测试（3 项脚本、1 项
WindowEx 生命周期）。耗时 11.70 秒，退出 0；最终证据 `ctest-list-final.log` 和 `ctest-final.log`。
Python 10 项通过；12 个 foundation 目标依赖守卫、无旧宿主源码检查、diff 与格式检查通过。
编译数据库 368 单元；SDK 107 ports，无 cocos2dx。
最终 Debug 二进制 SHA256：`df5113b3fbdab47075ac173b796d1ff8ddc1720dee8ef942350cdbe89bbbefb9`。

Release 使用 `MacOS Release Config/Build`，关闭测试和工具，复用本轮已安装的 SDK：
`-DVCPKG_INSTALLED_DIR=<仓库>/out/macos/debug/vcpkg_installed`。构建通过，332 编译单元，
`otool -L` 仅列系统动态库/framework。Release SHA256：
`0194dd3736eb9b0751509031c7a76dbb0e79d4302b4484f001096c5a677c924b`。
`package_native_runtime.py` 生成 macOS ZIP；用 `ditto -x -k` 解包后执行 `--help` 和
软件 `imageops_repro.py --renderer software`，均退出 0。后者实际检查缩放、模糊、仿射、
透视和 Layer 导出像素，正常退出；证据 `package-imageops.log`。

### 失败及修正

1. 初次配置成功后，强制 SDL `software` 呈现时报
   `SDL3: Window framebuffer support not available`；首轮 imageops 失败，不能算通过。
   改为 Cocoa/Metal 后，自制图像全部像素检查和正常退出通过。
2. 系统 `/usr/bin/python3` 缺少 `hashlib.file_digest`，首个游戏 staging 未完成。
   改用已安装的 `/opt/homebrew/bin/python3.13`，没有改动测试工具来绕过要求。
3. 初次 FFmpeg 自动探测并链接 Homebrew XCB；overlay 关闭探测后，旧 `FFMPEG_FOUND`
   缓存仍保留旧库。使用 `cmake --preset 'MacOS Debug Config' -U 'FFMPEG*' ...` 重新查找。
   最终新 Release 和刷新后的 Debug 动态依赖均无 Homebrew/XCB；检查脚本拒绝此类外部绝对路径。
4. Release 配置复用 SDK 且关闭测试时 vcpkg 移除了 Catch2，造成并发的 Debug 构建报
   `libCatch2Maind.a missing`。顺序重新配置 Debug 恢复 Catch2 后完整构建及 119 项测试通过。
   不把失败期间旧二进制的测试计入最终验收。

### 真实游戏与系统输入

原游戏 `/Users/lidong/Documents/tslm` 保持原样，独立副本位于 `out/game-smoke/`。
`macos-tslm-02` 在聊天中断时终止，只有标题截图/心跳，未形成最终报告。
`macos-tslm-03` 用 `game_smoke.py prepare/run --seconds 25 --launch-style game --probe-engine-args`：
48 次状态样本、KAG 标题、持续心跳和中文/空/重复 CLI 参数均通过。
定时控制器结束时强停（退出 -9），该结果不代表正常退出。

另以 `macos-tslm-04` 用原生 CUA 输入：标题画面点击 START 进入剧情，Return 推进文本；
系统关闭按钮弹出游戏退出确认，选择 NO 后仍存活，再次关闭选 YES 后退出 0，未强停。
图像通过 CUA 实际观察（标题、剧情对话、退出确认），过程日志 `tslm-interaction.log`。
测试的是 FFmpeg 重建前的 Debug 二进制（SHA256 `5435baef0fe7985de171b697a8829c1d86b465a42844d23cd2b332ec37dccaba`），
后续仅改变 FFmpeg 探测/链接和文档；最终二进制另经 119 项测试及发行包图像场景验证。

`macos-dialogs-01` 使用仓库 `native-dialogs/startup.tjs` 自制场景，由 CUA 操作 AppKit 控件：
中文输入确认/取消、三按钮消息框选择第三项、文件确认/取消、菜单 checked 1→0 和嵌套菜单。
23 条脚本事件包含正确返回值与选中文件路径，无 error，quit 请求正常退出 0。
事件和明确的验收断言保存于该副本 `report.json`，不是 SDL_PushEvent 模拟输入。

## 边界与后续

本记录完成的是 Apple Silicon macOS 的生产 SDL3 接线、构建、脚本和上述交互。
没有验证 Intel Mac、跨机分发、universal、签名/公证、完整剧情、商业游戏存读档、
实际音轨/视频播放、IME 组合输入或物理手柄。游戏日志仍有既存的可选插件加载失败；
没有将其伪装为空插件或据此宣布整个游戏兼容完成。

后续按需要增加 macOS CI、IME/缩放/全屏和媒体场景验收；Windows/Linux 本轮没有实机复测。
操作指南已同步 `docs/subsystems/macos-sdl3.md` 和构建/架构入口。
