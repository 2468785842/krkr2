# 依赖瘦身实施记录

日期：2026-11-01。基于 dev 合并提交 `853ec7d`，具体变更以对应 Git 提交为准。
本文记录实际改动；完整后端替换仍按 [迁移路线](backend-modernization.md) 单独验收。

## 已落地的构建边界

- 根 CMake 在 `project()` 前选择 vcpkg features，区分 engine、tools、tests、psd、crash-upload。
- `KRKR2_BUILD_APP=OFF`、`ENABLE_TESTS=OFF`、`BUILD_TOOLS=ON` 仅配置 XP3 所需模块；工具通过本地只读文件后端重开归档，不链接完整存储或使用空实现吞掉读写请求。
- `KRKR2_BUILD_APP=OFF`、`BUILD_TOOLS=OFF`、`ENABLE_TESTS=ON`、`KRKR2_TEST_SCOPE=strings` 仅配置日志、TJS2 和 Catch2；不再添加归档、存储、图像、媒体和插件目标。
- `KRKR2_ENABLE_PSD=OFF` 排除 PSD 插件及其 Spirit/Phoenix/Iostreams 直接依赖。默认 ON 保留已有 PSD 能力。Boost.Locale 尚未替换。
- 崩溃上报默认关闭，开启 `KRKR2_ENABLE_CRASH_UPLOAD` 同时启用 cocos network feature、DumpSend 源和启动调用。关闭后保留本地崩溃捕获/文件，不加入空上报函数。

## 源码与链接

`cpp/plugins/CMakeLists.txt` 提供 `krkr2plugin_api` 头文件/依赖接口、`krkr2plugin_impl` 实现库和兼容的 `krkr2plugin` 聚合接口。子插件只依赖 api，消除原先反向链接整个插件聚合库的环。
实现源改为 PRIVATE；聚合接口对含静态注册对象的库使用 CMake WHOLE_ARCHIVE。仅改 PRIVATE 会让未被普通符号引用的注册对象被丢弃，因此两项必须同时保留。完整应用生成图中核对到 82 个插件源各有一条编译命令。

完整链接暴露 spdlog 头文件模式与静态库模式混用。`cpp/core/log/CMakeLists.txt` 现在向消费者传播 `spdlog::spdlog` 的编译定义，确保包括 spdlog 头的引擎/插件与 sink 使用同一模式，不通过允许重复符号隐藏问题。
`cpp/core/base/CMakeLists.txt` 显式使用 LibXml2 CONFIG 及导入目标，传播头文件、静态库定义和配置对应的链接依赖。

## cocos 与公共头

- UI 的 C++ 工厂已经接管创建路径。删除 `UIExtension.cpp` 中遗留的 CocoStudio NodeReader、注册函数及启动调用，manifest 不再选择 cocostudio。
- desktop 且未启用 network 时不添加 cocos 网络源码；Android 保留 JNI 启动仍需的原生 downloader。overlay port-version 增加，以使二进制缓存重新计算。
- `tjsString.h` 移除 Boost.Locale 总头，将宽字符串和格式化后的 UTF 转换移入 `tjsString.cpp`；转换仍调用同一 Boost 函数。为此前依赖传递包含的消费者补上 functional、memory、set、sstream 等直接包含。
- TextStream 删除无用途的 OpenCV 头；渲染器只包含 core/imgproc，CMake 显式查找这两个组件。尚未替换 OpenCV 算法。
- PSB 纹理索引读取由 Boost.Regex/LexicalCast/字符串算法改为 C++17 string_view/from_chars，保留最后一个 tex、首个数字段、特殊零后缀及 uint32 溢出行为，并新增边界测试。
- Bison、日期词表及消息头全部在构建期生成，依赖声明覆盖输入语法、脚本和数据表；配置阶段不再重写输出。

## 复核后保留的库

`vcpkg/ports/ffmpeg` 是 3.3.9 自定义 overlay，已经启用 small，关闭文档、命令行程序和 avdevice。`VideoCodecFFmpeg.cpp` 实际调用 avfilter，故保留该库，并补充 CMake 对 avformat 的显式依赖。没有按上游默认 features 盲目裁剪。
JPEG/PNG/WebP/JXR/BPG、Vorbis/Opus、Oniguruma、OpenAL 等实际消费者仍在；未用格式更少的库静默替换它们。

## 验证方式与边界

操作命令见 [构建指南](building.md)。XP3 smoke 使用自制 raw/zlib 索引和数据段，比较解包内容并检查无效签名；字符串测试覆盖中文/日文、宽字符串、格式化及空字符串；PSB 纹理测试覆盖后缀、首个数字段、多个 tex 和 uint32 边界。

Windows 验证使用 MSVC 19.44、CMake 3.31、Ninja、Debug、x64-windows-static-md。
完整引擎使用现有 `out/windows/debug/vcpkg_installed`，缺失的 LibXml2 单独安装到 `out/windows/validation-deps`，没有修改全局工具链或覆盖原依赖目录。独立 XP3 安装使用自己的 `out/windows/xp3/vcpkg_installed`。
cocos 裁剪验证在 `out/windows/cocos-slim-source` 的隔离副本应用 overlay，并补齐仓库已有的 Win64 补丁内容，关闭 network/editor 后单独构建；不修改 vcpkg 原源码缓存。
这些验证不能代替全新全量依赖安装、其他平台构建或实际游戏兼容验证。未测得可归因于本轮的体积/耗时缩减比例；尤其 vcpkg 的 /Z7 与普通 CMake 的 /Zi 调试库不能直接比较大小。

## 本轮验证结果

| 检查 | 结果 |
|---|---|
| 独立 XP3 依赖安装/配置/编译 | 通过；安装状态无 cocos/OpenCV/FFmpeg/Catch2/libarchive/unrar |
| XP3 raw/zlib 解包及无效签名 smoke | 通过；再次构建无工作 |
| strings-only 配置/编译、ctest | 2 项通过；再次构建无工作 |
| 裁剪 cocos | network/editor 实现不在生成图中，442 条源码编译命令，静态库构建通过 |
| 应用与插件测试链接 | 使用隔离构建的新 cocos 静态库，krkr2 与 psbfile-dll 均链接通过 |
| PSD 关闭的应用配置/链接 | 复用验证依赖，排除 PSD 源及其库后应用链接通过（configure-lean.log/build-lean.log） |
| PSB 纹理索引测试 | CTest 已注册，1 项通过 |
| PSB 测试程序完整运行 | 3 项、389 个断言通过，含现有 PIMG/PSB 语料 |
| 插件源去重 | 完整配置下 82 个源各编译一次，链接命令包含 MSVC WHOLEARCHIVE |
| Lean 依赖计划 | vcpkg dry-run 生成待安装计划，不含 curl/OpenSSL、Spirit/Phoenix/Iostreams、Catch2、argparse；未执行全新全量安装 |
| 预设/格式/文档 | cmake --list-presets、修改文件 clang-format 检查、本地链接与 git diff --check 通过 |

完整应用验证的依赖复用原安装目录，仅为 cocos2dx::cocos2d 的 Debug 导入位置指定了本轮隔离构建的库（通过 out/windows/use-slim-cocos.cmake 的 CMake 项目 hook），没有覆盖用户原库。验证构建为 out/windows/engine-config-check；具体日志位于该目录的 configure-slim.log、build-slim.log。字符串及 XP3 日志分别在 strings-only/final-build.log、xp3/final-build.log。
独立库大小因 /Zi 与 /Z7 不同不可用于宣称裁剪百分比。Linux/macOS/Android、实际游戏、IME/暂停恢复/音画同步尚未验证；不是全量 Catch2 套件通过，也不是全部迁移路线完成。旧依赖检查脚本在 dev 中已删除，未将其缺失当作检查成功。

## 仍需独立迁移的工作

SDL3/Sokol 平台层、完整 OpenCV 像素算法替换、PSD 解析器重写、音频设备统一及归档格式合并尚未实施。它们需要像素/事件/格式语料与移动真机验收，不能因本轮构建瘦身通过就删除旧功能。
