# Windows SDL3 无 Cocos 构建与真实游戏验收

Status: implemented

## 目标和构建身份

从干净的 `dev` 开始，`git fetch --all --prune`、`git pull --ff-only` 确认已与
`origin/dev` 同步，基线 `74be243`；实现位于 `codex/windows-sdl3-independent` 的 dirty 工作树。
本页确认 Windows 独立宿主和以下已运行范围，不能外推完整游戏或其他平台兼容。

- MSVC 19.44.35217、Ninja、Debug、x64-windows-static-md；SDL 3.4.8。
- 最终 exe：`out/windows/sdl3-debug/bin/krkr2/krkr2.exe`，SHA256 `9ac132ee21cda9facf51d119d5322afb6a6c9022c782974d24eed5740d8e58d0`。
- 匹配 PDB SHA256：`67e03afd2d51ebfa851698513487d8b3baed1984b9b6bf1117ca8c1c8255bb89`。
- `KRKR2_SDL3_ONLY=ON`、`KRKR2_ENABLE_SDL3_HOST=ON`、`KRKR2_ENABLE_UI_TEST_DRIVER=OFF`、
  `ENABLE_TESTS=ON`、`KRKR2_TEST_SCOPE=platform`、`BUILD_TOOLS=OFF`、PSD 开启。

## 实现决定

Windows 启动直接进入 SDL3，沿用生产 TJS/KAG、Window/Layer、归档、插件和媒体实现，
不构造 Cocos delegate。Windows SDL3 只允许独立模式，拒绝误配成缺少完整服务的双宿主。
Cocos 移到单独的 vcpkg feature；旧构建自动选择它，独立构建不查找/编译/链接/安装它。

原生服务提供配置路径、菜单、任意按钮文本的消息框、Unicode 文件选择、文件复制和 URL；
资源通过 LocalAssetReader 读取，打包剔除管理 UI 图片/CSD。软件渲染保持原 Layer 算法，
拒绝显式 OpenGL 请求；PVRTC 解码器独立保留原 2/4 bpp 实现及许可。

全新依赖闭包暴露了两处旧安装目录会掩盖的引用：未使用的 minizip 写入器依赖 Cocos ioapi，
以及 PSBFile 的未使用 RapidJSON 头。前者只供已关闭的上传器使用，独立构建不再配置它；
后者删除未使用包含。未替换归档读取器或 PSB 功能。文件对话框初始目录必须转换为
Windows 分隔符，修复前实际落在程序目录，修复后选中文件路径与隔离场景相符。

## 构建和契约结果

```powershell
cmake --preset "Windows SDL3 Debug Config"
cmake --build --preset "Windows SDL3 Debug Build" -j 12
ctest --test-dir out/windows/sdl3-debug -N
ctest --test-dir out/windows/sdl3-debug --output-on-failure --no-tests=error
python scripts/check_core_module_deps.py
python tools/game-smoke/sdl_windows_repro.py out/windows/sdl3-debug/bin/krkr2/krkr2.exe out/windows-sdl3/native-final
```

最终构建退出 0；CTest 实际注册并运行 **67 项，全部通过**，其中 WindowEx 回归启动真实引擎。
基础依赖守卫通过其声明的 11 个 foundation 范围；不代表 domain 环全部消除。
`ctest-final.log`、`ctest-registered.log`、`build.log` 位于 `out/windows-sdl3/`。

正式 `sdl3-debug/vcpkg_installed/vcpkg/status` 无 cocos2dx 包；编译清单 351 个单元中
没有 Cocos 或 environ UI 源，最终链接命令没有 Cocos。21/91 项运行资源被选择，
不包含 img/out_ui。原始审计在 `dependency-audit.txt`、`final-link.txt`、`final-validation.json`。
早期 `sdl3-local` 只用既有依赖加快编译排错，不用它证明独立闭包。

`native-final/interaction-report.json`：全部菜单/对话框/文件路径/关闭事件齐全，
`passed=true`、退出 0、`forced_stop=false`。键盘来自 SendInput，切焦来自原生前台窗口，
最小化/恢复检查原生窗口状态；菜单和消息框/文件选择使用实际系统 UI 输入。
截图实际查看了窗口、标题、存读档和游戏确认框，不把组件分发当作系统输入。
早期探针错误的按下标记预期、辅助窗口点击及跨进程 Edit 文本读取已纠正；失败目录未覆盖。

## 真实游戏和修复前失败

样本为 `D:/krkr/tslm` 的《天神乱漫 LUCKY or UNLUCKY!?》，只在新副本中安装自编
AfterStartup 心跳/快照钩子，不复制原始 savedata，也不修改原目录。
`data.xp3` SHA256 为 `6791e688118c95924281a51e9e182f6e4bc512c6ef29447f994a803364f93282`；
完整资源身份见各副本 `smoke-manifest.json`，最终逐项重算确认源文件未变。

旧 Cocos 基线 exe SHA256 为 `87905a6d2e6811d28ae495b63bfb5b9b1d1f4865b62d91fe8ed567ba4d642cc5`，
到达 `title.ks:33`，实际点击 EXIT 和确认后退出 `0xc0000005`，没有强停。
修复前 SDL3（SHA256 `48c26da0ffd3b3628c7104d126db2d6c3a3e487349375ee81207ec0887293a73`）
标题、剧情、同进程存读档成功，但退出同样为 `0xc0000005`。两者证据分别在
`tslm-cocos-baseline/`、`tslm-sdl-01/`；相同退出码不能单独证明根因相同。

在 `tslm-sdl-debug/` 附加本机 CDB，保存匹配 exe/PDB、首次/二次异常栈和 dump。
SDL 栈明确为 `ucrtbased!free_dbg` ← `WindowEx::deleteOverlayBitmap` ← `WindowEx::~WindowEx`
← ncbind Invalidate ← Window 关闭。释放目标是 `0xcdcdcdcdcdcdcdcd`：
`cpp/plugins/windowEx.cpp` 的 `ovbmp` 从未初始化。改为 `nullptr`，没有绕开清理逻辑。

`windowex_lifetime_repro.py` 使用 `Window.registerExEvent()` 创建真实惰性实例再 invalidate。
修复前最小场景退出 `3221225477`、无强停；修复后相同场景输出完成标记并退出 0，
已登记 `windowex-lifetime-runtime`。此修复不关闭没有匹配栈的其他历史访问冲突调查。

## 修复后的游戏范围

- `tslm-sdl-fixed/`：点击标题 START，Enter 推进剧情，打开 SAVE 并写入 File01；
  推进后打开 LOAD、确认 File01，回到保存的 `１.ks:25`；原生关闭请求经游戏确认后，
  `end.ks:33`、退出 0、未强停。exe SHA256 为 `c6146c50457bfd26faf7674565af972e07b8fcd8066f217fb3152115ba9d0c31`。
  自动流程曾在文本动画时过早点击 SAVE 而超时；保留失败日志，后续以真实页面/文件/脚本状态验收。
- `tslm-sdl-restart/`：全新副本只复制上述私有 savedata；新进程从标题 LOAD 读取 File01，
  恢复 `１.ks:25` 并继续推进，正常确认退出 0、未强停。
- 最终 exe 的 `tslm-sdl-final/`：再次以该私有存档启动新进程，从 `title.ks:33` 进入 LOAD，
  恢复 `１.ks:25`，实际 Enter 推进至 `１.ks:38`，截图可见 CG、文字和游戏按钮；
  再经游戏确认退出 0、未强停。`native-game-report.json` 的 exe 哈希与最终构建、
  `native-final` 原生交互报告完全一致。标题、读档、推进及退出确认截图均已实际查看。

## 验收边界和下一步

完成了 Windows SDL3 独立构建、定向契约、原生交互及该游戏标题/短剧情/存读档/正常退出。
未验证 Windows 音视频播放同步、完整剧情、物理高 DPI、IME 候选窗、物理手柄及其他平台独立模式。
旧平台中原有的未实现扩展接口不因本轮脱离 Cocos 而获得兼容保证。

后续按 [Windows SDL3 操作指南](../../../../docs/subsystems/windows-sdl3.md) 补充这些专项，
不要把本轮短剧情验收扩大为游戏全兼容。所有商业素材、存档、dump 和原始截图留在忽略的 out。
