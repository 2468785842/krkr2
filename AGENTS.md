# 开发约定与项目导航

KrKr2 是 C++17 的跨平台吉里吉里运行环境。先读 [README_CN.md](README_CN.md)、
[文档索引](docs/README.md) 和 [当前架构](docs/architecture.md)。
本文适用于人工和 AI 协作；用户本次任务的明确指令优先。以本地代码、CMake 和实际验证为准，外部教程及旧规划不能替代实现事实。

## 代码地图

| 位置 | 职责 |
|---|---|
| `cpp/core/tjs2/` | TJS2 编译、字节码、值、对象、原生绑定与执行 |
| `cpp/core/base/` | 脚本入口、事件、KAG、系统启动；部分存储实现仍在此目录 |
| `cpp/core/io/` · `storage/` · `archive/` | 文件后端、存储、XP3/ZIP/7z/TAR 与工具专用存储 |
| `cpp/core/imageops/` | 不暴露 OpenCV 类型的 CPU 图像接口、原生最近邻及现有 OpenCV 后端 |
| `cpp/core/visual/` | Layer/Bitmap、字体、渲染、混合、转场、窗口接口 |
| `cpp/core/environ/` | 平台实现、cocos 应用壳、输入、窗口及模拟器 UI |
| `cpp/core/sound/` · `movie/` | 音频、视频、FFmpeg 集成 |
| `cpp/core/plugin/` · `cpp/plugins/` | 插件协议、ncbind、内建插件与 motionplayer |
| `platforms/` | Windows/Linux/macOS/Android 启动与打包 |
| `vcpkg/` · `cmake/` | overlay ports、平台依赖与构建辅助 |
| `tests/` · `tools/` | Catch2/语料、XP3 等工具 |

## 工作流程

1. 开始先看 `git status --short`，保留已有改动。按任务读接口、调用方、实现和对应测试，不做全仓格式化。
2. 明确要保持的脚本可观察行为及最小复现。性能或后端替换先记录同机基线。
3. 小步实现、验证、更新相关专题；新增专题加入 `docs/README.md`。文档使用中文，事实引用仓库路径；明确区分现状、提案和历史记录。
4. 交付说明改动、验证命令/结果、未验证平台及剩余问题。编译通过不等于游戏兼容；零测试不等于测试通过。

## 构建与验证

设置 `VCPKG_ROOT`，Windows 使用 MSVC x64 开发环境和 PATH 内的 win_bison。
完整准备及 Linux/macOS/Android 命令见 [构建与验证](docs/building.md)。Windows 示例：

```sh
cmake --preset "Windows Debug Config" -DENABLE_TESTS=ON
cmake --build --preset "Windows Debug Build"
ctest --test-dir out/windows/debug -N
ctest --test-dir out/windows/debug --output-on-failure --no-tests=error
python scripts/check_core_module_deps.py
```

- C++ 使用仓库 `.clang-format`，仅格式化修改文件，CI 使用 clang-format 20。
- 测试源文件在各 `tests/unit-tests/**/CMakeLists.txt` 的 SOURCES 中登记，Catch2 经 `catch_discover_tests` 注册；放入目录不会自动成为测试。
- 改基础模块依赖运行依赖检查。该工具容许既有 domain 环，退出成功不代表整个依赖图无环。
- 环境阻塞应给出实际命令和错误，不自行修改全局工具链或下载无关依赖。只修改文档可做链接、路径与 diff 检查，无需触发完整依赖构建。

## 修改前先读

| 改动 | 入口及约束 |
|---|---|
| TJS/字节码/绑定 | `tjs2/tjsInterface.h`、`tjsVariant.h`、`tjsNative.h`；保持 dispatch flags、objthis、错误码及 AddRef/Release 契约 |
| 脚本加载 | `base/ScriptMgnIntf.*`、`base/impl/ScriptMgnImpl.*`；检查源码/表达式/字节码、文件名行号、上下文与异常 |
| 文件/归档 | `io/IFileBackend.*`、`io/FileBackendRegistry.*`、`storage/impl/`；保持路径规范化、查找顺序、流所有权及移动端资源访问 |
| 窗口/输入 | `environ/impl/TVPWindow.*`、`visual/impl/WindowImpl.*`、`base/EventIntf.*`；坐标、DPI、IME、事件顺序及销毁期回调 |
| 图像/渲染 | `visual/RenderManager.*`、`visual/ogl/RenderManager_ogl.cpp`、`LayerBitmap*`；pitch、裁剪、alpha、混合/插值、纹理寿命及读回 |
| 视频/音频 | `movie/ffmpeg/`、`sound/`；时间基、线程、暂停恢复、设备及资源释放 |
| 插件 | `core/plugin/ncbind.*`、`plugins/tp_stub.h` 及插件注册；保持导出接口和原生实例身份 |
| 构建/依赖 | 根及模块 CMake、vcpkg 清单/配置/overlay；区分目标依赖与配置期依赖，不把 PRIVATE 当作静态库依赖隔离保证 |

路径表中的 `base/` 等均相对 `cpp/core/`。现有子模块文档优先补充局部细节，不重复维护整份项目说明。

## 后端精简约束

目标是隔离并逐步替换 cocos2d/OpenCV，减小依赖及编译范围，见 [后端精简路线](docs/backend-modernization.md)。
SDL3/Sokol/GLFW 尚未在本仓库完成迁移；不要把 `environ/sdl/tvpsdl.cpp` 的目录名当作有效 SDL 后端。
平台替换与 Ciallang/TJS2 替换分开验收。保持现有 TJS、Layer、存储及插件行为；移除依赖须同时处理 CMake、vcpkg、资源/平台打包与消费者，不用空实现静默吞掉游戏功能。

外部实现的借鉴范围和已发现差异见 [参考资料](docs/references.md)。引用代码时记录来源提交、具体文件及原许可/版权；外部 README 的能力声明不等于本仓库已验证能力。
