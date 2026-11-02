# 当前架构与代码导航

核对日期：2026-11-02。初批构建瘦身基于 `853ec7d`，归档续批基于 `f4d3e2a`，实际改动与验证见[实施记录](dependency-slimming-implementation.md)。以下描述本仓库实现，不代表外部二开仓库。后续实现变化应同步更新。

## 运行链路

```text
platforms 启动 → environ/cocos2d 应用壳、UI 与窗口
                         ↓
          base 脚本管理 / 事件 / KAG → tjs2
                         ↓
     visual 图层与绘制 / sound / movie / 原生插件
                         ↓
          io 文件后端 / storage / archive
```

这是职责示意，当前模块链接仍有互依赖，不是已经实现的单向分层。
Windows 入口 `platforms/windows/main.cpp` 创建 TVPAppDelegate；脚本入口在
`cpp/core/base/ScriptMgnIntf.cpp`，包含源码执行、表达式、字节码加载及存储加载。

## 按问题定位

| 问题 | 优先阅读 |
|---|---|
| 启动、窗口、输入、模拟器界面 | `cpp/core/environ/cocos2d/`、`environ/impl/TVPWindow.*`、`environ/ui/`、`visual/impl/WindowImpl.*` |
| 脚本错误、宿主回调 | `base/ScriptMgnIntf.*`、`tjs2/tjsInterface.h`、`tjs2/tjsNative.*`、`tjs2/tjsInterCodeExec.cpp` |
| 图层混合、缩放、透视、纹理 | `visual/RenderManager.*`、`visual/ogl/RenderManager_ogl.cpp`、`visual/LayerBitmap*`、`visual/impl/DrawDevice.*` |
| 文件路径、归档和解包 | `io/`、`storage/impl/`、`archive/`、`base/StorageIntf.cpp`、`tools/xp3/` |
| 视频画面/同步 | `movie/ffmpeg/`、`environ/cocos2d/YUVSprite.*`、`visual/impl/VideoOvlImpl.cpp` |
| 插件/角色动画 | `core/plugin/`、`plugins/`、`plugins/motionplayer/docs/` |

表内缩写路径相对 `cpp/core/`，`plugins/` 指 `cpp/plugins/`。

## 依赖事实

- 根 CMake 使用 C++17。`cpp/core/CMakeLists.txt` 的 `krkr2core` 为 INTERFACE 聚合目标，不是单个已隔离实现库。
- cocos2d 除窗口/绘制外，还被 UI、配置、文件后端、Android JNI 辅助及视频等路径使用；移除它不只是改启动函数。
- `environ/impl/TVPWindow.h` 的接口仍返回 `cocos2d::Node*`。抽象层本身也需要去除后端类型。
- CPU 图像操作已集中到 `imageops/ImageOps_opencv.cpp`；软件/OpenGL 渲染器通过不含 OpenCV 类型的 `ImageOps.h` 调用缩放、模糊、仿射、透视及矩阵求解。最近邻 resize 的像素采样已用原生 C++ 实现；其他算法及并行调度仍链接 OpenCV core/imgproc。
- dev 已删除旧 `environ/sdl/tvpsdl.cpp`；当前没有 SDL 后端，vcpkg 清单也未声明 SDL。
- 内建插件实现源只属于各自静态库，`krkr2plugin` 通过 WHOLE_ARCHIVE 保留注册对象；PSD 可选。崩溃上报默认关闭，CocoStudio reader 已移除，管理 UI 仍使用 cocos UI。
- 标准库/图像库/编解码依赖以 `vcpkg.json`、`vcpkg-configuration.json` 和各模块 CMake 为准；overlay ports 也包含自定义补丁。

## 已有解耦与剩余边界

`tools/xp3/CMakeLists.txt` 当前链接 `core_log`、`core_storage_archive`、`archive_xp3` 及 argparse，
通过 `tools/xp3/tool/CreateStream.cpp` 只读重开本地 XP3，不再链接完整 `core_storage`。
根 CMake 的 `KRKR2_BUILD_APP=OFF`、`ENABLE_TESTS=OFF`、`BUILD_TOOLS=ON` 组合
只配置日志、TJS2、消息、IO、XP3 和共享归档基础层；vcpkg 同步选择 tools feature。
完整引擎和 full 测试仍配置全部域模块；strings 测试范围只添加日志、TJS2 和 Catch2。操作见 [构建与验证](building.md)。

归档格式目标统一链接 `core_storage_archive`，不再反向链接完整 `core_storage`。
共享句柄缓存已从 XP3 迁到 `storage/impl/ArchiveHandleCache.*`；基础归档层不再链接 XP3。
ZIP 读取器使用自身格式类型及 zlib，不再包含 cocos 的 minizip 头。完整应用的其他 cocos/archive 消费者仍保留。
`archives` 测试范围可独立配置 ZIP/TAR；同时开启工具时增加 XP3 回归，见[构建指南](building.md#独立归档测试)。

`scripts/check_core_module_deps.py` 已恢复，检查 10 个 foundation 目标的显式 CMake 链接及直接平台包含。
它不解析所有生成表达式或传递头，也不代表 domain 层既有环已消除。

`imageops` 测试范围仅配置 `core_imageops` 与 Catch2，不添加 TJS、日志或平台目标；其 vcpkg features 为 tests/imageops，见 [ImageOps](imageops.md)。默认 full 测试也注册这组像素用例。依赖守卫额外拒绝渲染器/插件中的直接 OpenCV 调用；完整渲染路径与游戏画面仍需单独回归。
