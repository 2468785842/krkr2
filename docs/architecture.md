# 当前架构与代码导航

核对日期：2026-10-31；文档建立时本地 HEAD 为 `9e0ca5d`。以下描述本地实现，不代表外部二开仓库。后续实现变化应同步更新。

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
- OpenCV 的实际调用包括 boxFilter、resize、仿射和透视变换，见 `visual/RenderManager.cpp`；OpenGL 路径也有 resize。SDL 窗口库不负责替代这些算法。
- `environ/sdl/tvpsdl.cpp` 虽进入源清单，但核对时未检索到其中的 SDL 调用；当前 vcpkg 清单也未声明 SDL。不能据目录名推定输入系统已使用 SDL2。
- 标准库/图像库/编解码依赖以 `vcpkg.json`、`vcpkg-configuration.json` 和各模块 CMake 为准；overlay ports 也包含自定义补丁。

## 已有解耦与剩余边界

`tools/xp3/CMakeLists.txt` 当前链接 `tjs2`、`core_io`、`core_storage_tool`；
`core_storage_tool` 位于 `cpp/core/storage/tool/`，与引擎存储分开。
因此旧 [Core 分析](CORE_MODULE_DECOUPLING.md) 中“xp3 直接链接 core_base_module”的示例属于历史现状，不能继续当作当前事实。

不过根配置仍添加整个 core/plugins 并执行重依赖查找。**工具目标链接变小，不等于根 CMake 配置或 vcpkg 安装已经独立。** 后续最小构建应分别验证配置、依赖安装、目标编译和链接。

`scripts/check_core_module_deps.py` 可检查基础层新增环并输出 DOT；它保留既有 domain 互依赖例外，解析也不是完整 CMake 求值器。检查成功不能证明全工程已经单向依赖。

现有测试由 `tests/CMakeLists.txt` 添加 movie、visual、tjs2 和插件子目录；核对时 visual 的 SOURCES 为空。不能把这个目录的存在当作已有图像兼容回归。
