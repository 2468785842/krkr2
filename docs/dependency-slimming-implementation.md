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

## 归档模块隔离续批

基线为 dev `f4d3e2a`。将共享句柄缓存迁到 `storage/impl/ArchiveHandleCache.*`，保留 8 槽缓存、
退出回调和 XP3 compact 清理；关闭后归还的句柄会释放，重复关闭安全。ZIP 析构会清除其缓存条目。
`core_storage_archive` 不再链接 XP3，四个格式目标改为依赖共享层；工具显式链接 `archive_xp3`。
ZIP 的格式类型及流读取实现独立于 cocos 头，解码算法保持原实现；现有 `cpp/external/minizip` 主要是写入器，未作为 ZIP 读取替代。

注册器在所有创建器拒绝或抛异常时释放输入。成功返回才转移所有权；XP3、ZIP、7z、TAR 的失败路径同步遵循此契约。
XP3 直接构造和重打包子类的 `Init` 仍接管输入；注册器改走借用输入的索引解析。
归档名称转换改为检查实际转换结果，非法序列不再悄悄截成前缀。测试宿主记录错误和退出回调，不修改引擎 UI 行为。
Linux XP3 工具同时修复 stat 时间成员在取消宏后应访问 `st_*tim.tv_sec` 的编译问题。

新增 `archives` 最小配置、跨平台 headless CI 及 foundation 依赖检查。检查器只守护显式基础层边界，
不把 domain 环或传递包含判为已解决；完整应用仍需要 cocos、OpenCV、FFmpeg 等库。
具体测试命令及覆盖边界见[独立归档测试](building.md#独立归档测试)。

本机验证为 Debian 13、GCC 14.2、CMake 3.31.6、Bison 3.8.2、Debug；仅在临时目录解包
真实 Debian 依赖，未改全局工具链。Boost 1.83/fmt 10.1/spdlog 1.15/Oniguruma 6.9.9/Catch2 3.7.1
不同于固定 vcpkg 基线，不能替代 CI 的全新 manifest 安装。

| 检查 | 本机结果 |
|---|---|
| strings-only 回归 | 2 项通过 |
| 独立 ZIP/TAR | 7 项通过；生成图无 XP3、完整存储、cocos、OpenCV、FFmpeg 或 unrar/LibArchive，61 条编译记录无重复 |
| 加入 XP3 的联合验证 | 9 项 CTest 通过（8 个 Catch2 用例/41 个断言及 XP3 raw/zlib smoke） |
| ASan/UBSan | 8 个归档用例通过，启用泄漏检测和 UBSan 首错退出 |
| 依赖守卫 | 9 目标通过；注入 cocos/完整存储依赖的负向检查被拒绝 |

三平台 [headless CI](https://github.com/2468785842/krkr2/actions/runs/41365411170) 使用仓库 vcpkg 基线，
Windows x64/MSVC、Linux x64/GCC 与 macOS ARM64/AppleClang 均已通过构建及 9 项 CTest。
本机未编译完整应用、7z 或 Android，也未验证真实游戏，未测量体积或耗时收益。
三平台远端的 vcpkg status 和编译清单已核对，不含 cocos2dx、OpenCV、FFmpeg、libarchive、unrar、7zip 或 libjpeg-turbo，
每平台 67 条编译记录无重复；
最小配置仍有 Boost.Locale，且间接安装 Boost.Regex 等子包，不能把 PSB 局部去 Boost 等同于全仓移除。

## ImageOps 接口与像素回归续批

基线 dev `5e889a8`，实现提交 `6038329`。提取 `core_imageops`，软件/OpenGL 渲染器改用不暴露 OpenCV 的接口；
OpenCV core/imgproc 保留为后端，删除 layerex_draw 未使用的总头；消费者显式包含标准头，GL 清除纹理函数使用 GLuint，避免依赖 OpenCV 提供全局 uint 别名。新增独立 imageops 测试范围，
并将 TJS/日志公共依赖归入 foundation feature，避免纯图像测试安装 Boost.Locale 等无关库。
覆盖、命令及未验证边界见 [ImageOps](imageops.md)，同机微基准原始样本见该页报告链接。

本机 Release：9 项 CTest/900 断言通过；ASan/UBSan Debug 同样通过，参考 OpenCV 库未启用 sanitizer。
归档+XP3 9 项 CTest 回归通过。依赖守卫扩展至 10 个 foundation 目标，并阻止渲染器/插件重新直接包含或调用 OpenCV；注入公共头及渲染器泄漏的负向检查通过。
新建本机构建目录的目标图无 TJS/日志/存储/图形壳/媒体目标，编译清单恰有模块与测试两个源；本机使用现有真实依赖，vcpkg 安装闭包另由 CI 检查。
完整应用、Android 与真实游戏尚未验证；无构建时间/包体收益结论。

三平台 [ImageOps/归档 CI](https://github.com/2468785842/krkr2/actions/runs/41370645981)（源码 `6038329`）
共 6 个任务全部通过：Windows x64/MSVC、Linux x64/GCC、macOS ARM64/AppleClang 各通过 9 项图像 CTest
及原 9 项归档/XP3 CTest。各平台图像任务均核对到恰有两个项目编译源；实际 vcpkg status
仅含 catch2、opencv4、zlib 与三个 vcpkg host 构建辅助 port，无 Boost、日志、cocos 或 FFmpeg。
这些元数据及 CTest 日志分别保存在 imageops-tests-* 与 headless-tests-* 六份附件中。
固定 port 在 macOS 明确禁用 OpenMP，未把它当作三平台相同的并行后端；本轮也未裁剪 OpenCV port 的内部模块构建。

## 最近邻 resize 原生化续批

基线 `4a4a615`，实现 `32a3009` 与并行修正 `5c646f4`。公共校验/缓冲区和最近邻像素采样迁入
不包含 OpenCV 的 `ImageOps.cpp`，保留两次 double 除法舍入、Gray8/RGBA8、pitch、ROI、未对齐与恒等复制行为。
内部行任务接口仍复用 OpenCV 同步调度；Area/Linear/Cubic、blur、warp 与矩阵算法尚未替换，vcpkg 依赖没有减少。
依赖守卫额外拒绝原生源和内部头引入 OpenCV。

本机 13 项图像用例、41,377 个断言通过，含 4 万组最近邻尺寸对照及请求 1/3 线程的大图用例；
ASan/UBSan 启用泄漏检测也通过，参考 OpenCV 库未启用 sanitizer。原生源只使用标准头的 C++17 语法检查通过。
微基准及限制见 [ImageOps](imageops.md)；串行大图回退促使本批保留并行调度，未宣称运行/包体收益。

三平台 [最近邻 CI](https://github.com/2468785842/krkr2/actions/runs/41374071238)（`5c646f4`）六个任务均通过：
各平台 13 项图像 CTest 和 9 项归档/XP3 CTest，图像任务核对到三个项目编译源；
实际 vcpkg 清单仍是 catch2、opencv4、zlib 和三个 host 辅助 port，无额外引擎依赖。
Windows 完整应用 workflow 拆分配置与编译步骤，成功链接后检查两个真实渲染器及两个 ImageOps 源各出现一次，
并上传编译清单与依赖状态；该检查不把链接等同于脚本/游戏画面验收。

另已启动基线 `4a4a615` 的 [Windows 完整构建](https://github.com/2468785842/krkr2/actions/runs/41372678650)。
截至本条记录（2026-11-02 01:49 UTC）仍在旧 workflow 的 Build Windows 步骤，尚未取得配置/编译/链接结论；
不计为完整引擎通过，也不能替代用本轮新提交重跑消费者与脚本画面对照。
