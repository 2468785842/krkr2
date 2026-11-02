# 构建、验证与测量

依赖准备见根 README，实际预设见 `CMakePresets.json`，CI 命令见 `.github/workflows/`。
本页命令用于仓库根目录，不依赖个人绝对路径。

## 桌面构建

需要 CMake/Ninja、C++ 工具链、Python、Bison 和 `VCPKG_ROOT`；Windows 在 MSVC x64 开发环境运行，并把 win_bison 加入 PATH。vcpkg 基线与 overlay 已记录在配置文件中，不为修复局部问题随意升级全仓依赖。

| 平台 | 配置预设 | 构建预设 | 测试目录 |
|---|---|---|---|
| Windows | `Windows Debug Config` | `Windows Debug Build` | `out/windows/debug` |
| Linux | `Linux Debug Config` | `Linux Debug Build` | `out/linux/debug` |
| macOS | `MacOS Debug Config` | `MacOS Debug Build` | `out/macos/debug` |

```sh
cmake --list-presets
cmake --preset "Windows Debug Config" -DENABLE_TESTS=ON -DBUILD_TOOLS=ON
cmake --build --preset "Windows Debug Build"
ctest --test-dir out/windows/debug -N
ctest --test-dir out/windows/debug --output-on-failure --no-tests=error
```

其他桌面平台替换表中的名字和目录。Release 预设将 Debug 替换为 Release。
根开关为 `KRKR2_BUILD_APP`、`ENABLE_TESTS`、`BUILD_TOOLS`，默认均开启。CMake 在 `project()` 前按开关选择 vcpkg 的 foundation/engine/tests/tools/imageops 等 features；关闭测试不会再因根直接依赖而安装 Catch2。默认 full 测试仍依赖完整引擎；strings 范围可独立构建。
选择目标可用 `cmake --build --preset "Windows Debug Build" --target xp3`；仅选择目标不会缩小根配置范围。

## 仅构建 XP3 解包工具

在 MSVC x64 开发环境执行（Linux/macOS 去掉 WINDOWS 并选择对应 triplet）：

```sh
cmake -S . -B out/windows/xp3 -G Ninja -DWINDOWS=ON -DVCPKG_TARGET_TRIPLET=x64-windows-static-md -DCMAKE_BUILD_TYPE=Debug -DKRKR2_BUILD_APP=OFF -DENABLE_TESTS=OFF -DBUILD_TOOLS=ON
cmake --build out/windows/xp3 --target xp3
python tools/xp3/smoke_test.py tools/bin/win/dbg/xp3.exe
```

使用新的构建目录可核实独立依赖安装；不要复用全量引擎的安装目录来证明最小闭包。此模式只添加日志、TJS2、消息、IO、XP3 和共享归档基础层，不配置 cocos/OpenCV/FFmpeg，也不需要 libarchive/unrar。Boost.Locale、fmt、spdlog、Oniguruma、zlib 和 argparse 等依赖仍存在。直接使用 vcpkg 命令时默认安装 foundation；完整引擎还需选择 `--x-feature=engine` 等能力。若同时指定 `--x-no-default-features`，则必须显式加入所需的 foundation。CMake 会自动按构建开关选择。
工具仅解包本地 XP3；工具专用 `TVPCreateStream` 通过文件后端重开只读归档句柄，不支持引擎存储介质、嵌套归档或写入，错误会抛出异常。共享引擎存储实现未改变。smoke 脚本生成原始及 zlib 索引/数据段，验证解包内容并检查无效签名行为；不等于全游戏兼容测试。

首批（2026-11-01）Windows 验证：MSVC 19.44、Debug、x64-windows-static-md，以上独立配置、`cmake --build out/windows/xp3 --target xp3 -j 6` 及 smoke 命令通过；再次构建输出 `ninja: no work to do`。独立 vcpkg 安装状态中没有 cocos2dx、opencv4、ffmpeg、catch2、libarchive、unrar，compile_commands 中没有 visual/movie/plugins 源码且没有重复源文件。首次安装使用部分本地缓存，未测量同机全量基线，不能据此宣称耗时缩减比例。
完整引擎另用 `out/windows/engine-config-check`、`VCPKG_MANIFEST_INSTALL=OFF` 和现有 `out/windows/debug/vcpkg_installed` 检查配置，在 `cpp/core/base/CMakeLists.txt:60` 因 `Could NOT find LibXml2 (missing: LIBXML2_LIBRARY LIBXML2_INCLUDE_DIR)` 停止；后续已在隔离目录补齐 LibXml2，改用导入目标，并完成使用新裁剪 cocos 的应用链接，详见 [实施记录](dependency-slimming-implementation.md)。未重新安装整套引擎依赖。`python scripts/check_core_module_deps.py` 因 dev 已删除该文件而无法执行。该批尚未验证 Linux/macOS/Android；归档续批的独立验证见实施记录。

Android 使用 README 所列 JDK/SDK/NDK，在对应平台运行 `platforms/android/gradlew`，例如：

```sh
./platforms/android/gradlew -p ./platforms/android assembleDebug
```

Windows 使用 `platforms/android/gradlew.bat`。Gradle/NDK 验证不能用桌面 CMake 通过代替。

## 独立字符串测试与可选能力

Windows 还提供 `Windows XP3 Config/Build`、`Windows Strings Config/Build`、`Windows Lean Debug Config/Build` 三组预设。Lean 构建应用但关闭工具、测试、PSD 和上报；依赖 PSD 的游戏应使用默认完整预设或重新开启 PSD。新预设名称和 JSON 已由 `cmake --list-presets` 核对，Lean 的全新依赖安装尚未实测。

```sh
cmake -S . -B out/windows/strings -G Ninja -DWINDOWS=ON -DVCPKG_TARGET_TRIPLET=x64-windows-static-md -DCMAKE_BUILD_TYPE=Debug -DKRKR2_BUILD_APP=OFF -DBUILD_TOOLS=OFF -DENABLE_TESTS=ON -DKRKR2_TEST_SCOPE=strings
cmake --build out/windows/strings --target tjsString
ctest --test-dir out/windows/strings --output-on-failure --no-tests=error
```

此范围仅配置日志、TJS2 和 Catch2 测试，不配置存储、图像、音视频及插件。测试宿主提供键名回退的 locale 回调，不代表游戏本地化系统。默认 `KRKR2_TEST_SCOPE=full` 保留原测试范围。
`KRKR2_ENABLE_PSD=OFF` 可排除 PSD 插件及 Boost.Spirit/Phoenix/Iostreams（Boost.Locale 仍存在）。
`KRKR2_ENABLE_CRASH_UPLOAD` 默认 OFF，保留本地崩溃文件；开启时同时选择 crash-upload manifest feature 并编译上报代码。其开关不控制 Android 的本地 Breakpad 捕获。
CocoStudio reader 残留已移除；管理 UI 继续使用现有 C++ 工厂和 cocos UI。插件实现只编译在自己的静态库中，krkr2plugin 接口通过 WHOLE_ARCHIVE 保留静态注册对象，不能用普通静态链接替代而丢失注册。

## 独立归档测试

```sh
cmake -S . -B out/archive-tests -G Ninja -DCMAKE_BUILD_TYPE=Debug -DKRKR2_BUILD_APP=OFF -DBUILD_TOOLS=OFF -DENABLE_TESTS=ON -DKRKR2_TEST_SCOPE=archives
cmake --build out/archive-tests --target archive_tests
ctest --test-dir out/archive-tests -N
ctest --test-dir out/archive-tests --output-on-failure --no-tests=error
python scripts/check_core_module_deps.py
```

该模式配置日志、TJS2、消息、IO、共享归档、ZIP/TAR 和 Catch2，不添加 XP3、完整存储、插件或图像/媒体目标。
vcpkg 选择 foundation/tests/archives features；基础依赖仍含 Boost.Locale、fmt、spdlog 和 Oniguruma，archives 提供 zlib。
启用 `BUILD_TOOLS=ON` 并构建 `archive_tests xp3` 时，再添加 XP3 所有权回归及解包 smoke。
独立归档测试也登记在默认 full 测试中；它们提供测试宿主回调，不能证明完整引擎事件或 UI 已隔离。

测试语料由 `tests/unit-tests/core/archive/make_fixtures.py` 在构建期生成，覆盖存储/deflate、UTF-8 名称、
小文件的 ZIP64 本地头、TAR、随机访问、归档持有、拒绝/异常输入释放和退出后归还句柄。
这不是大于 4 GiB、全部 ZIP 压缩方法、CP932、加密或真实游戏验收。
非法名称会报告错误并抛异常，修复了原转换长度查询只截断前缀的问题；没有替换 TJS 编码转换器。

根 CMake 尊重显式 `CMAKE_TOOLCHAIN_FILE`；未指定工具链且设置 `VCPKG_ROOT` 时才自动选 vcpkg。
也可用 `CMAKE_PREFIX_PATH` 提供已安装的真实依赖，Linux 验证使用此方式，未修改全局工具链。
三平台无窗口 CI 见 `.github/workflows/headless-tests.yml`，保留 CTest、编译清单和 vcpkg status 附件。

## 独立图像测试

使用新目录及 `-DKRKR2_BUILD_APP=OFF -DBUILD_TOOLS=OFF -DENABLE_TESTS=ON -DKRKR2_TEST_SCOPE=imageops`，构建 `imageops_tests` 后运行 CTest。该范围仍需 OpenCV core/imgproc（其余算法及调度）与 Catch2，不配置 TJS、日志、存储或 cocos。完整命令、像素对照和可选微基准见 [ImageOps](imageops.md)。

## 按改动选择验证

| 改动 | 最低相关验证 |
|---|---|
| 文档 | 本地链接/路径、代码块中的预设与开关、`git diff --check` |
| 基础库/工具 | 受影响目标构建，相关 Catch2；改依赖再运行 `python scripts/check_core_module_deps.py` |
| TJS/插件 | 相关测试、最小脚本及宿主回调；记录异常与对象释放行为 |
| 图像算法 | 合成像素用例、旧实现对照、裁剪/非紧密 pitch/alpha/边界；约定精确或容差比较 |
| 平台/渲染 | 目标平台窗口、DPI、输入、IME、缩放、全屏、暂停恢复及关闭；截图和事件序列对照 |
| 音视频 | 播放/暂停/seek/结束、音画同步、异常文件、线程及资源释放 |

新增测试要在对应子目录 CMake 的 SOURCES 中登记。先 `ctest -N` 确认注册，再运行；visual 已登记 `ImageOpsTest.cpp` 的像素对照用例。CI 部分构建关闭测试，构建徽章不代表全量测试通过。
后端改动还需实际游戏或可分发的自制 TJS/KAG fixture。报告资源版本/哈希、输入步骤、平台和结果；不把仅能启动当作兼容完成。

## 编译提速的基线

分开记录：依赖首次安装、配置、冷编译、缓存编译、无改动构建、修改公共头后重建和链接。
每次记录提交、操作系统/CPU、编译器、构建类型、并发数、vcpkg 基线、缓存是否命中、耗时及产物大小。
同机相同配置比较；不要把缓存命中与换库收益混为一谈，不为测冷构建清除用户全局缓存。

TJS 的 Bison、日期词表以及消息头统一在构建期生成；日期词表显式依赖脚本与 dp_word_table.txt，消息头依赖脚本与 messages.json。配置不再重写这些输出。

## 无法完成验证时

保留命令、退出码和最早的有效错误，区分依赖安装/配置/编译/链接/测试/游戏回归阶段。
不要把“环境缺库”写成源码测试失败，也不要把未运行写成通过。本文建立时只核对命令与源码，未重新执行全平台完整构建。
