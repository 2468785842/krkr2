# 当前架构与代码导航

核对日期：2026-11-03；Windows SDL3 独立宿主补充于 2026-11-24。初批构建瘦身基于 `853ec7d`，归档续批基于 `f4d3e2a`，实际改动与验证见[实施记录](../.agents/notes/implemented/simplification/2026-11-01-dependency-slimming.md)。以下描述本仓库实现，不代表外部二开仓库。后续实现变化应同步更新。

## 运行链路

```text
platforms 启动 → Windows SDL3 独立宿主 / Cocos 应用壳、UI 与窗口
                         ↓
          base 脚本管理 / 事件 / KAG → tjs2
                         ↓
     visual 图层与绘制 / sound / movie / 原生插件
                         ↓
          io 文件后端 / storage / archive
```

这是职责示意，当前模块链接仍有互依赖，不是已经实现的单向分层。
Desktop 启动先经过 `environ/launch/` 的纯参数/路径解析与引擎适配；指定目标时
适配器在 GTK/GL 初始化前校验归档，`AppDelegate` 在场景就绪后直调 `MainScene::startupFrom`，
无目标仍保留原选择器。现状及命令见 [Desktop CLI](subsystems/desktop-cli.md)。移动端启动路径未迁移。
Windows 入口 `platforms/windows/main.cpp` 按构建选择 SDL3 或 TVPAppDelegate；脚本入口在
`cpp/core/base/ScriptMgnIntf.cpp`，包含源码执行、表达式、字节码加载及存储加载。

## 按问题定位

| 问题 | 优先阅读 |
|---|---|
| 启动、窗口、输入、模拟器界面 | `cpp/core/environ/cocos2d/`、`environ/impl/TVPWindow.*`、`environ/ui/`、`visual/impl/WindowImpl.*` |
| 脚本错误、宿主回调 | `base/ScriptMgnIntf.*`、`tjs2/tjsInterface.h`、`tjs2/tjsNative.*`、`tjs2/tjsInterCodeExec.cpp` |
| 图层混合、缩放、透视、纹理 | `visual/RenderManager.*`、`visual/ogl/RenderManager_ogl.cpp`、`visual/LayerBitmap*`、`visual/impl/DrawDevice.*` |
| 文件路径、归档和解包 | `io/`、`storage/impl/`、`archive/`、`base/StorageIntf.cpp`、`tools/xp3/` |
| 视频画面/同步 | `movie/ffmpeg/`、`movie/VideoPresentation.h`、`environ/cocos2d/CocosVideoPresentation.*`、`visual/impl/VideoOvlImpl.cpp` |
| 插件/角色动画 | `core/plugin/`、`plugins/`、`plugins/motionplayer/docs/` |

表内缩写路径相对 `cpp/core/`，`plugins/` 指 `cpp/plugins/`。

## 依赖事实

- 根 CMake 使用 C++17。`cpp/core/CMakeLists.txt` 的 `krkr2core` 为 INTERFACE 聚合目标，不是单个已隔离实现库。
- Cocos 配置仍包含 UI、移动端文件后端、Android JNI 辅助及视频适配；Windows SDL3 独立配置不编译这些路径。
- `input/` 的 `core_input` 只依赖标准 C++，由输入适配器和 System 查询共同使用，包含状态、修饰位、局部坐标、取消分类与 UTF-8/UTF-16 提交文本编解码；OS 键码、坐标、IME 组合状态和事件排队仍由现有宿主承担。见[共享输入](subsystems/input-state.md)。
- `visual/WindowLayer.h` 已拆出不暴露 Cocos 类型的窗口接口；具体 Cocos 窗口仍在 `environ/impl/TVPWindow.h`。视频播放器通过通用呈现接口交付 YUV 帧，由 Cocos 宿主创建和管理节点。见 [窗口接口拆分](subsystems/window-interface.md)。
- `visual/TexturePresentation.h` 提供后端无关的 CPU 行/GL 画面描述；Cocos 纹理包装、上传与截图移到宿主适配层。CPU 渲染器不再直接创建 Cocos 纹理，GL宿主状态和重建订阅已迁入纯C++接口，配置与平台服务也已集中至 RenderHost，完整context资源恢复待实现，见[纹理呈现](subsystems/render-presentation.md)。
- `core_base_module` 的本地事件直接使用 C++ 主线程消息接口，已移除对 Cocos 的直接构建依赖。队列由应用壳持有并驱动。见[主线程消息](subsystems/window-interface.md#主线程消息)。
- `environ/HostFrame.h` 提供普通帧、消息泵和模态绘制入口。帧推进和模态节流位于无 Cocos 依赖的 `event/FrameStep.*`。见[帧循环](subsystems/window-interface.md#帧循环)。
- `core_visual_module` 自行声明图像、字体、哈希和原生 GL 依赖。内置字体经宿主读取，Cocos 的实际资源搜索仍在应用壳中。见[Visual 构建依赖](subsystems/render-presentation.md#visual-构建依赖)。
- CPU 图像操作统一经 `imageops/ImageOps.h` 调用。`ImageOps.cpp` 实现最近邻/线性缩放，`ImageOps_native.cpp` 实现其余缩放、模糊、仿射/透视、矩阵求解和标准 C++17 行任务；生产目标与 vcpkg 已移除 OpenCV，实时 OpenCV 对照仅为默认关闭的测试工具。移植来源及验证边界见 [ImageOps](subsystems/imageops.md)。
- Windows 的 `KRKR2_SDL3_ONLY` 配置已移除 Cocos 的配置、安装、编译及链接依赖，
  原生交互和真实游戏短剧情、存读档及退出已有验收记录。见
  [Windows SDL3 独立宿主](subsystems/windows-sdl3.md)。
- Linux 的 SDL3 宿主使用生产脚本、Window/Layer 与软件画面呈现；
  `KRKR2_SDL3_ONLY=ON` 可构建不安装、配置或链接 Cocos 的正式应用。
  见[独立运行时](subsystems/native-runtime.md)及
  [Linux SDL3 游戏宿主](subsystems/desktop-cli.md#linux-sdl3-游戏宿主)
- 独立 SDL3/TJS 原型继续验证实验脚本和事件契约。[原型说明](../tools/sdl3-prototype/README.md)
- 内建插件实现源只属于各自静态库，`krkr2plugin` 通过 WHOLE_ARCHIVE 保留注册对象；PSD 可选。崩溃上报默认关闭，CocoStudio reader 已移除，管理 UI 仍使用 cocos UI。
- 标准库/图像库/编解码依赖以 `vcpkg.json`、`vcpkg-configuration.json` 和各模块 CMake 为准；overlay ports 也包含自定义补丁。

## 已有解耦与剩余边界

`tools/xp3/CMakeLists.txt` 当前链接 `core_log`、`core_storage_archive`、`archive_xp3` 及 argparse，
通过 `tools/xp3/tool/CreateStream.cpp` 只读重开本地 XP3，不再链接完整 `core_storage`。
根 CMake 的 `KRKR2_BUILD_APP=OFF`、`ENABLE_TESTS=OFF`、`BUILD_TOOLS=ON` 组合
只配置日志、TJS2、消息、IO、XP3 和共享归档基础层；vcpkg 同步选择 tools feature。
完整引擎和 full 测试仍配置全部域模块；strings 测试范围只添加日志、TJS2 和 Catch2。操作见 [构建与验证](guides/building.md)。

归档格式目标统一链接 `core_storage_archive`，不再反向链接完整 `core_storage`。
共享句柄缓存已从 XP3 迁到 `storage/impl/ArchiveHandleCache.*`；基础归档层不再链接 XP3。
ZIP 读取器使用自身格式类型及 zlib，不再包含 cocos 的 minizip 头。完整应用的其他 cocos/archive 消费者仍保留。
`archives` 测试范围可独立配置 ZIP/TAR；同时开启工具时增加 XP3 回归，见[构建指南](guides/building.md#独立归档测试)。

`scripts/check_core_module_deps.py` 已恢复，检查 11 个 foundation 目标的显式 CMake 链接及直接平台包含。
它不解析所有生成表达式或传递头，也不代表 domain 层既有环已消除。

`imageops` 测试范围仅配置 `core_imageops` 与 Catch2，不添加 TJS、日志或平台目标；其 vcpkg features 为 tests/imageops，见 [ImageOps](subsystems/imageops.md)。默认 full 测试也注册这组像素用例。依赖守卫额外拒绝渲染器/插件中的直接 OpenCV 调用；完整渲染路径与游戏画面仍需单独回归。

桌面使用命令行启动和设置。管理界面随移动端构建打包，Cocos 桌面游戏窗口、脚本菜单与对话框继续可用。
[桌面命令行](subsystems/desktop-cli.md) · [管理界面资源边界](subsystems/management-ui.md)

语言目录读取经 `environ/PackagedAsset.h`，Cocos 控件文字绑定和 GL 默认值探测留在
`environ/cocos2d/` 适配层。配置与语言使用 JSON，独立格式模块保留旧 XML 的只读迁移；Linux SDL3 从程序旁的
`Resources` 目录读取字体、语言等资源。资源与配置存储入口由通用库持有，提供器由各宿主安装；
全局/游戏/语言配置管理器已可独立链接，Cocos 权限写入与提示仍在适配层，
见[宿主资源与 JSON 配置](subsystems/host-resources.md)。
Linux SDL3 通过 CLI 的宿主准备回调，在 XP3 校验前直接安装 native 资源与设置服务，
到引擎清理后恢复；该分支不再构造 Cocos AppDelegate。
旧混合应用保留 Cocos 链接；独立 SDL3 目标使用单独入口、依赖清单和 GTK3 原生 UI，完全排除 Cocos。
SDL3 活动期间的平台名与 OS 名返回 Linux，不访问 Cocos Application 单例。
