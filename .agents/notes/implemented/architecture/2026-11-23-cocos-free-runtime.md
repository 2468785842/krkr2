# Linux 纯 SDL3 运行时的依赖闭包

Status: implemented

当前 Linux 独立目标已实现，验收见文末「2026-11-24 独立应用闭包验收」。
以下前期分批记录保留其当时状态，不作为最终现状。

## 目标与基线

目标是 Linux 的正式 Window/Layer/KAG 运行时不安装、不配置、不链接 Cocos，
同时保留现有脚本可观察行为。基线 `1fddb56`，当前工作区含本记录对应的未提交改动。
目前 SDL3 仅为完整应用的可选宿主；`core/CMakeLists.txt` 与 `environ/CMakeLists.txt`
仍无条件查找并链接 Cocos，不能把 SDL3 的游戏验收记录当作依赖移除证据。

## 首批：配置与语言边界

- `LocaleConfigManager.*` 不再暴露 Cocos 类型，资源经 `PackagedAsset.h` 读取。
  Text/Button 的四个重载移至 `cocos2d/CocosLocale.*`，所有现有调用点显式使用适配函数。
- 语言选择仍为用户设置优先、系统语言其次，缺文件时尝试英文。
  原实现的默认英文缺失路径会无限递归；现在有限回退并保留键名查询行为。
- `IndividualConfigManager.cpp` 经 `TVPHostFileExists` 查询文件，当前 Cocos 适配仍委托原有 FileUtils，
  保留本地/打包文件和相对路径搜索语义。配置 XML 的实际读写方式没有在本批重写。
- GL 默认值探测移至 `cocos2d/CocosPreferenceDefaults.cpp`；SDL3 的非 GL 默认值路径保持原样。
- 依赖检查约束语言和单游戏配置源码不得直接访问 Cocos/UI。
  当前实现说明见 `docs/subsystems/management-ui.md`。

## 验证与限制

Windows MSVC 19.44、clang-format 20。`locale_config_tests` 在 full 的
`tests/unit-tests/core/environ/CMakeLists.txt` 显式登记，5 项测试覆盖 UTF-8 BOM/实体、
空文本、用户语言优先、英文回退、英文也缺失、重复键与重新加载。
测试宿主提供配置值和打包字节，不验证真实宿主资源搜索或配置持久化。

本地独立验证目录为 `out/locale-config-validation/`，复用已存在的 Catch2 和
`out/windows/cocos-slim-build/lib/ext_tinyxml2.lib`，只编译本次生产语言读取源码和测试宿主。
`validate.cmd` 完成配置、构建、`ctest -N` 和 `ctest --output-on-failure --no-tests=error`，
实际注册 5 项、通过 5 项。链接行不含 Cocos 运行库，但 XML 库仍来自旧 Cocos 包，
因此不能证明干净环境中已无需安装 Cocos。

首次独立链接因旧 XML 头默认声明 dllimport 失败（LNK2019/LNK1120）；
测试目标显式使用与当前静态 XML 库一致的 `CC_STATIC` 后通过，未替换生产逻辑。
原始命令、日志和构建文件在上述 out 目录。

完整应用尝试在 MSVC x64 环境执行：

```powershell
cmake -S . -B out/locale-config-validation/app-build -G Ninja -DWINDOWS=ON -DCMAKE_BUILD_TYPE=Debug -DVCPKG_TARGET_TRIPLET=x64-windows-static-md -DVCPKG_INSTALLED_DIR=D:/Users/LiDon/VSCodeProjects/krkr2/out/windows/debug/vcpkg_installed -DVCPKG_MANIFEST_INSTALL=OFF -DENABLE_TESTS=ON -DBUILD_TOOLS=OFF
```

退出码 1：`cpp/core/environ/CMakeLists.txt` 的 `find_package(cocos2dx)` 找不到
`cocos2dxConfig.cmake` / `cocos2dx-config.cmake`。本机 CLion 原有配置进程正在重新安装该包；
本次使用独立目录并禁止依赖安装，没有干预该进程或修改全局工具链。
因此尚未执行本次完整应用链接、真实资源/设置读写或游戏回归，Linux/macOS/Android 也未验证。
日志为 `out/locale-config-validation/app-check.log`。

为检查适配层调用点，在独立 MSVC x64 环境使用原 compile_commands 的宏和核心包含目录、
已存在的 `out/windows/cocos-slim-source` 头文件，对 CocosLocale、CocosPreferenceDefaults、
IndividualConfigManager 与五个 UI 调用文件执行 `/Zs`，8 个翻译单元语法检查通过。
这不是当前完整应用编译/链接或 Android 构建，不能替代上述未完成验收。
最终文件查询接回宿主后，另对 IndividualConfigManager 与 CocosPackagedAsset 执行 `/Zs`，
两项均通过，日志为 `syntax-final.log`。`python scripts/check_core_module_deps.py`、
`git diff --check`、修改文档的相对链接和 environ 源文件登记路径检查通过。

## 续批：独立资源入口和 XML

`core_host_assets` 包含通用资源入口与 C++17 本地读取器；Cocos AppDelegate 安装旧服务提供器，
Linux SDL3 在 Run 期间安装程序旁 `Resources` 目录。全局资源函数不再由 Cocos 文件定义。
`core_config_xml` 使用已有 LibXml2，Global/Locale 配置不再包含旧 tinyxml2。
全局设置路径和写入错误提示仍借用现有平台层；移动端最近列表等 UI 仍使用 Cocos 的 XML 库。
当前实现和边界见 `docs/subsystems/host-resources.md`。

独立 `config` 范围已在根 CMake、vcpkg feature 和测试 SOURCES 中登记。
实际只编译 ConfigXml、LocalAssetReader、HostAssets、LocaleConfigManager 和三个测试源，共 7 个源文件。
Windows 与 WSL Ubuntu/GCC 15.2 各注册 15 项、各通过 15 项，链接列表均无 Cocos/tinyxml2/TJS/SDL。
Windows 配置显式禁用查找 cocos2dx；WSL 使用独立 prefix 中的 Catch2 3.15.0、LibXml2 2.15.3，
从现有缓存源码构建，未安装系统包或下载依赖。WSL 的 LibXml2 关闭 iconv/zlib/programs/tests，
结论只覆盖 UTF-8 配置和本地资源支持层，不能替代正式 vcpkg 完整应用配置。
命令和日志在 `out/config-native-validation/{verify.cmd,verify-complete.log,verify-wsl.sh,wsl-complete-file.log}`。
新增三平台 config CI job，检查包闭包、7 源文件和 15 项注册；本轮没有推送或触发云端工作流。

full 额外用旧 tinyxml2 检查两份自带语言 XML 和新写出的设置，
并以原 Cocos 提供器对照 19,484,784 字节字体及语言文件的原始字节。
XML 测试包含长属性、实体、重复参数、重复键、制表符和 CR/LF 兼容规则。
开发期同进程字体 I/O 单次基线为 Cocos 226.382 ms、native 39.2306 ms；
native 后读取有缓存优势，这不是性能结论，也未据此声称构建速度或包体改善。

最初原样切换 LibXml2 时，自带语言文件的属性原始 `<300MB` 导致整份解析失败。
增加引号属性的兼容规范化后，英文文件又暴露 `&#10Report` 缺分号；
该资源已补分号，恢复此前被旧解析器吞掉的英文说明。早期失败在 app.log / app-final.log，
后续两解析器逐项对照通过；没有删除失败的对照用例。

Windows 完整应用构建、18 项配置/资源对照、17 项 Cocos 契约、3 项真实脚本回归、
15 项 CLI 检查及普通构建的软件/OpenGL 自制场景通过。
运行时验证另发现并修复普通静态库构建丢失 OpenGL 注册及构造/退出清理的问题，
包括失败、匹配 exe/PDB、最终哈希和验收边界，见
[专项记录](../../implemented/bug-fix/2026-11-23-renderer-static-registration.md)。

## 第三批：配置管理器与宿主存储

`ConfigManager/PreferenceStorage.*` 引入显式宿主提供器与作用域恢复，
配置管理器不再包含 Platform.h、TJS 或 Cocos。`core_config` 独立链接真实的全局、游戏、语言管理器，
`GlobalConfigManager` 的读写与失败提示经提供器分发；保留保存失败后清除 dirty 标记的旧行为。
语言测试已去掉假 GlobalConfigManager 实现，改为真实管理器加内存宿主提供器。

从 MainScene 迁出 `TVPGetInternalPreferencePath` 和 `TVPGetDataPath`；后者的 TJS 包装在
`environ/HostPaths.cpp`。Cocos 桌面在 CLI 校验前、移动端在 AppDelegate 构造时安装适配器，保留原 FileUtils 路径、
目录创建与 Android 权限写入、提示接口。Linux SDL3 安装标准库本地提供器，
沿用旧 XDG/HOME + 实际可执行文件名目录规则；作用域覆盖引擎初始化和 TVPSystemUninit。
未安装提供器时报错，不加入空实现或静默使用工作目录。

独立 config 范围增加 5 项存储/真实管理器测试，总计 20 项、11 个编译源文件。
覆盖 UTF-8 路径和精确二进制读写、空文件、写入失败提示、嵌套提供器恢复、Linux 目录规则、
XML 持久化、游戏覆盖/继承及启动参数不写回。
CI 源清单/注册数量同步更新，云端工作流未执行。

Windows `cmd /c out/config-native-validation/verify.cmd` 和 WSL
`bash out/config-native-validation/verify-wsl-complete.sh` 均退出 0、发现并通过 20 项。
最终日志为 `storage-windows-final.log` / `storage-wsl-final.log`，沿用上一批同一独立构建目录。
Windows Ninja 完整命令列表无 Cocos/tinyxml/TJS/SDL，两个 compile_commands 各 11 源。
同进程另运行管理器 5 项（40 断言）和语言 5 项（16 断言），均通过，检查单例状态不依赖
CTest 的逐用例进程隔离。依赖边界检查、clang-format 20.1.8、YAML 与文档链接检查通过。

首个完整应用（SHA256 `6e744ca21f867c55ed595045a955991d996f2be1ad93f7a335408d47522ffe09`）
构建和 26 项配置/脚本测试通过，但 CLI smoke 在无 startup 的有效 XP3 处失败，
输出 `Host preference storage has not been initialized`，错误码仍为 2。
原因是 PrepareDesktopLaunch 的归档校验间接读取配置，发生在 AppDelegate 之前；
仅靠应用对象构造安装提供器不够。已将三个桌面入口的存储安装提前到 CLI 校验之前，
AppDelegate 只在尚未安装时补装，不删除 CLI 用例或加入静默默认提供器。
失败日志保留在 `out/config-native-validation/storage-app.log`。

修正后 `cmd /c out/config-native-validation/storage-app.cmd` 退出 0：完整应用构建成功，
CTest 先发现并通过 26 项（独立配置 20 + 旧接口对照 3 + 真实脚本 3），CLI help/error 15 项通过。
最终日志 `storage-app-final.log`；full/Debug、脚本 smoke ON、UI test driver OFF，
AppDelegate 实际编译参数为 `KRKR2_USE_COCOS_UI=0`。
最终 exe SHA256 `87905a6d2e6811d28ae495b63bfb5b9b1d1f4865b62d91fe8ed567ba4d642cc5`。
最新错误退出报告位于 `out/windows/debug/script-smoke/renderer-error/renderer-error-nfdulikv/report.json`，
匹配最终 exe，caught 错误与 exit-requested 各一次、exit 0、未强停。

`python tools/game-smoke/imageops_repro.py out/windows/debug/bin/krkr2/krkr2.exe out/config-native-validation/storage-runtime`
退出 0，软件/OpenGL 场景的算法检查与 ready/finished 均通过，exit 0、未强停。
报告在 `storage-runtime/report.json`，匹配最终 exe；OpenGL Layer BMP 已实际查看。
脚本 SHA256 `646b99f2f46d3c7bddf18b4ac130c5c5951058a6220b5f18ded3678edc9e561e`，
软件 BMP `395f5bb131813c513681734ed29859ea0682c6640e0fe80bebfc47e1375d711d`，
OpenGL BMP `8f01fb5c23ca9b5bc6db7f324e05d6cfdf5e6fcee5998a1b8043b4eab2225e86`。
本批没有系统窗口截图、商业游戏兼容、Linux SDL3 完整应用或 macOS/Android 应用验收；
Linux/macOS 桌面入口安装顺序已同步修改，尚未经这些平台的完整构建。

## 下一步与完成条件

1. 新资源、设置和 CLI 接线已完成 Linux Release 完整应用与 SDL3 原生窗口探针验收，
   见同日续验。移除 Cocos 构建依赖后须在新的独立闭包中重跑，不能沿用过渡构建作为完成证据。
2. 继续迁出剩余平台服务。平台默认值宏、CLI/XP3 校验前安装服务和 SDL3 平台名查询
   已接通并完成本轮完整应用验证，脚本 UI 等仍待处理。
3. 接通脚本菜单、文件/输入对话框和剩余平台服务，禁止用空实现满足链接。
4. 为应用配置纯 SDL3 的目标、manifest feature 与资源打包分支；生产源码不编译 Cocos 适配层。
5. 在独立 Linux 安装目录证明不安装、不查找、不链接 Cocos，并跑现有窗口、IME、KAG
   存读档、音视频、输入与正常退出探针。其余平台仍保持旧后端并单独验收。

风险集中在资源搜索语义、脚本 UI、平台服务遗漏和 XML ABI；
接口拆分或单元测试通过均不能作为纯 SDL3 里程碑完成结论。

## 2026-11-24：CLI 前的 native 服务作用域

基于 `74be243`，云端工作区 `/workspace/krkr2`；本节对应未提交改动。
先从此前聊天恢复仓库和分支信息，再取回远端 dev，没有访问或修改 Windows 本地工作区。

`DesktopLaunch` 增加可选宿主准备回调：完成参数/目标解析后、设置引擎参数和校验归档前调用。
Linux 入口据解析出的宿主选择安装提供器；SDL3 使用 `ConfigManager/LocalHostServices.*`，
同时安装程序旁 `Resources` 的标准库资源读取器与原 XDG/HOME 规则的本地设置存储。
作用域由 main 持有，覆盖归档校验、游戏运行与 TVPSystemUninit；Run 不再二次替换提供器。
SDL3 分支移到 TVPAppDelegate 构造之前，避免该构造函数重新安装 Cocos 服务。
帮助、设置列表、非法宿主和目标解析失败不调用准备回调。
Windows/macOS 的既有提前安装逻辑仍使用默认空回调。

设置写入失败继续使用 SDL 消息框；显示失败时追加 stderr 诊断。
共享配置默认值改用 TVPPlatform.h 的 iOS/Android 宏，不再依赖 Cocos 头的包含顺序。
依赖守卫增加 LocalHostServices 和默认值宏约束。

环境初始无 CMake、Ninja、Catch2、LibXml2 开发文件和 VCPKG_ROOT。
仅在忽略的 `out/native-host-validation/` 内安装验证工具，构建 Catch2 3.8.1 与 LibXml2 2.14.6；
LibXml2 关闭 iconv/zlib/lzma/python/programs/tests。未安装全局工具链或系统包。
这验证 UTF-8 配置支持层，不代表正式 vcpkg 闭包或全部编码能力。

主要命令（PATH/PYTHONPATH 指向该 out 目录中的工具）：

```sh
cmake -S . -B out/native-host-validation/config -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_PREFIX_PATH="$PWD/out/native-host-validation/prefix" \
  -DKRKR2_BUILD_APP=OFF -DBUILD_TOOLS=OFF -DENABLE_TESTS=ON \
  -DKRKR2_TEST_SCOPE=config -DCMAKE_DISABLE_FIND_PACKAGE_cocos2dx=TRUE
cmake --build out/native-host-validation/config --parallel 4
ctest --test-dir out/native-host-validation/config -N
ctest --test-dir out/native-host-validation/config --output-on-failure --no-tests=error
```

配置/构建/发现/执行均退出 0，发现并通过 22 项。新增 2 项检查无窗口时资源读取、
真实管理器 XML 持久化、写失败回调、嵌套作用域和构造异常时旧提供器恢复。
compile_commands 实际 12 个源，无 Cocos/tinyxml/SDL 编译参数。
相同参数改用 scope=launch 的独立目录，发现并通过 10 项。
最终日志 `config-final.log`、`launch-final.log`。
同进程执行 config_manager_tests：7 项、54 断言通过；locale_config_tests：5 项、16 断言通过。
另用标准 C++17 对桌面/iOS/Android 的 TVP 平台宏分支执行静态断言，三个编译均退出 0；
此项是宏分支检查，不是移动平台应用构建。依赖守卫、clang-format 20.1.8 和 diff 检查通过。
CI config job 源清单和测试数已同步，云端 CI 尚未执行。

完整应用尝试：

```sh
cmake -S . -B out/native-host-validation/app -G Ninja -DLINUX=ON \
  -DKRKR2_ENABLE_SDL3_HOST=ON -DENABLE_TESTS=OFF -DBUILD_TOOLS=OFF \
  -DCMAKE_DISABLE_FIND_PACKAGE_cocos2dx=TRUE
```

退出 1，首先阻塞于 `cpp/external/libbpg/CMakeLists.txt:17` 找不到 FFMPEG 包配置，
日志 `app-configure.log`。未进入完整应用编译，Linux main/SDL3/生产 DesktopLaunch
本轮未完成应用编译或运行验收。不得把独立 launch 测试当作该生产接线的运行证明。
即使补齐 FFmpeg，当前 core/environ 的 Cocos REQUIRED 与 vcpkg engine 依赖仍须迁出；
本轮没有增加一个不能链接的“纯 SDL3”开关，也没有通过空实现或删除游戏功能绕过依赖。
脚本菜单、文件/输入对话框、其余平台服务、应用构建闭包与全部运行探针仍是完成条件。

### 同日续验：补齐应用依赖

前述缺 FFmpeg 是可解决的环境准备问题，不能作为停止完整应用验证的理由。
原命令没有接入 vcpkg 工具链，CMake 只查找库，没有安装 manifest 中的依赖。
本次在 `out/toolchains/vcpkg` 准备仓库固定基线 `aa40adda5352e87655b8583cfb2451d5e9e276fd`，
重新 bootstrap 对应的 vcpkg 可执行文件，避免新版工具不支持旧 tools JSON schema 的错误。

环境没有 sudo 和 cloud-agent-packages 助手。通过既有 Debian snapshot 源下载并校验
120 个系统开发/构建包，仅解包到 `out/system-deps/root`，没有写系统目录或修改 sudo 配置。
安装 Yasm/Bison/Autoconf/Automake/Libtool 和 X11/GTK/Wayland 开发文件，
将私有 pkg-config 路径与工具的数据目录重定位，并为系统现有运行库建立私有前缀中的符号链接。
GTK 3.24.49 的头文件与运行库编译/执行检查通过，Xvfb 下实际创建 GTK 窗口也退出 0。
环境激活脚本为 `out/native-host-validation/activate.sh`；后续完整构建使用 CI 同版 CMake 3.31.1。
私有 overlay triplet 只构建 Release，以控制该 8 GiB 磁盘环境的占用，不修改入库 triplet。

FFmpeg overlay 3.3.9#1 已通过 GCC 14.2 编译并安装（首轮构建约 3.4 分钟，安装退出 0）。
使用真实 vcpkg 工具链、显式 `VCPKG_INSTALLED_DIR` 和 `VCPKG_MANIFEST_INSTALL=OFF`
重试应用配置，日志 `app-ffmpeg-configure.log` 出现 `Found FFMPEG`，原 libbpg 阻塞已解除；
随后缺少 Boost，因此继续启动完整 manifest 依赖安装，未以 Boost 错误结束。
该首轮是单包安装；正式 manifest 会按仓库固定基线重新解析并重建不匹配的支持包。
完整安装首轮因私有 WebP 配置引用已有系统运行库而失败；补齐私有运行库链接后继续安装。
保留 `ffmpeg-install-final.log`、`app-full-configure.log` 与 `app-full-configure-retry.log`，
完整应用构建与运行结论待后续证据补充。

完整 manifest 安装已完成。其间还修正了私有 Autoconf 的 M4、工具及宏目录，
并通过 PKG_CONFIG_LIBDIR 让 Meson 在重设 PKG_CONFIG_PATH 后仍能找到系统 X11。
首次应用生成失败是安装脚本对 libpng.pc 符号链接和其真实文件各重定位一次造成重复前缀；
已按真实路径去重并规范化，删除本次产生的错误 Cocos 缓存包后，使用 SDK GTK 3.24.52 重建 Cocos。
没有创建假目录掩盖不存在的包含路径，也没有修改库 ABI 或升级仓库基线。

最终配置/生成目录为 `out/native-host-validation/app-verified`，安装阶段 manifest ON；
依赖全部安装后才设 `VCPKG_MANIFEST_INSTALL=OFF`，并开启 compile_commands。命令：

```sh
source out/native-host-validation/activate.sh
cmake -S . -B out/native-host-validation/app-verified -G Ninja -DLINUX=ON \
  -DCMAKE_BUILD_TYPE=Release -DKRKR2_ENABLE_SDL3_HOST=ON \
  -DENABLE_TESTS=OFF -DBUILD_TOOLS=OFF -DKRKR2_ENABLE_SCRIPT_SMOKE=ON \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_INSTALLED_DIR="$PWD/out/native-host-validation/vcpkg_installed" \
  -DVCPKG_OVERLAY_TRIPLETS="$PWD/out/native-host-validation/triplets" \
  -DVCPKG_INSTALL_OPTIONS=--clean-after-build
cmake --build out/native-host-validation/app-verified --target krkr2 --parallel 4
```

最终配置/生成和完整应用编译/链接均退出 0，应用 362 个编译源。
`KRKR2_ENABLE_UI_TEST_DRIVER=OFF`、AppDelegate 实际参数 `KRKR2_USE_COCOS_UI=0`。
完整编译同时核对出启动 banner 的 TVPGetOSName/TVPGetPlatformName 仍调用 Cocos Application 单例；
SDL3 跳过 AppDelegate 后不应访问它。已让 SDL3 活动期间直接返回 Linux，并在真实脚本与
窗口探针中检查 `platform:Linux:Linux`。这是源代码依赖修正，没有声称已复现 Release 崩溃。
旧 FMOD 二进制有 dynsym 链接警告、JPEG XR 有 tmpnam 警告，链接退出 0，未修改这些库来消除警告。

最终程序为 `app-verified/bin/krkr2/krkr2`，SHA256
`d2a891020671e45961ff7d6d4f8595c619bba15aa66eaf9bb2c6fe1313effc45`。
`app-verified-configure2.log`、`app-final-configure.log`、`app-build.log`、`app-build-final.log`
保留配置与构建证据。FFmpeg 的独立 CMake 编译/链接/执行探针从合成 PNG 解出准确 RGBA 像素，
输出 `FFmpeg 3.3.9: PNG decode passed; headers 57, library 57`，日志 `ffmpeg-probe.log`。

以下均针对最终程序，退出 0：

- `python3 tools/game-smoke/cli_smoke.py <engine>`：15 项。
- 同一命令加 `--host=sdl3`：15 项，覆盖 native 存储提前安装、有效但无 startup 的 XP3，
  并检查帮助/列举设置不创建 native 设置目录。CI Linux 的 SDL3 合约分支已登记该命令。
- `LIBGL_ALWAYS_SOFTWARE=1 ALSOFT_DRIVERS=null xvfb-run -a ctest --test-dir out/native-host-validation/app-verified -R '^(textstream|parser-error|renderer-error)-runtime$' --output-on-failure --no-tests=error`：先发现 3 项、执行通过 3 项。
- Xvfb 下 `sdl_host_repro.py <engine> out/native-host-validation/sdl-host-01`：
  OS 窗口像素、输入/Unicode、缩放/黑边、隐藏恢复、关闭否决、模态/主窗口退出及新增平台名事件通过，
  exit 0、forced_stop=false，报告 `sdl-host-01/report.json`。
  fixture SHA256 `6c22bb9373fe58232adc0330fd90be55985448c12575e30fc0a49e2ac1d77c6e`。
- Xvfb 下 `imageops_repro.py <engine> out/native-host-validation/imageops-01`：
  软件/OpenGL 场景均通过 resize/blur/affine/perspective、ready/finished，exit 0、无强停。
  报告 `imageops-01/report.json` 匹配最终程序；已实际查看 OpenGL Layer BMP，
  图像 SHA256 `eb505139dfacf19733d2e134ebae5a0de79fc3308d24706486cbd7db1aa3f637`。
  该 BMP 不是宿主 UI 截图，OpenGL 使用 Mesa 软件实现，不是实体 GPU 验收。

依赖守卫、修改 C++ 的 clang-format 20.1.8 和 diff 检查通过。
仍未完成纯 SDL3 的配置/安装/链接闭包、脚本 UI 替换、商业游戏与移动平台验收；
本节恢复完整应用基线并验证 native 接线，不能将其写成“完全移除 Cocos”。


## 2026-11-24 独立应用闭包验收

Linux 目标完成；其他平台仍使用旧宿主。实际使用说明归入
`docs/subsystems/native-runtime.md`，源仍为 dev `74be243` 加当前未提交改动。

- `KRKR2_COCOS_FREE=ON` 选择独立 Linux 启动入口、`engine-sdl3` manifest feature，
  排除 Cocos/UI 源、OpenGL 工厂与旧包装；GTK3 原生菜单、输入、消息和文件选择替换脚本 UI。
  普通应用 `KRKR2_ENABLE_UI_TEST_DRIVER=OFF`；默认 SDL3，显式 Cocos 请求报错。
- 使用新的 `out/native-sdk` 安装目录，共 162 ports，无 cocos2dx；不复用旧完整 SDK。
  新配置 `out/native-independent` 开启 app/full tests/script smoke，显式设置
  `CMAKE_DISABLE_FIND_PACKAGE_cocos2dx=TRUE`，完整配置与链接成功。
  `scripts/check_native_runtime.py` 核验 365 个编译单元、SDK status、实际 ninja 命令、
  ldd 和资源包，未发现 Cocos/FMOD/GLFW 依赖；资源实际选择 21/91。
- 全新闭包暴露两处旧宿主隐式依赖：未使用的 minizip writer 头和 PSB 未使用的
  `json/memorystream.h`。独立模式不构建仅崩溃上传消费者需要的 minizip，删除未用 JSON include。
  不引入伪造头或空实现。
- 初版 native 控制台额外 stderr 输出造成 parser marker 重复，现改为中立可选 ConsoleOutput
  观察器，由统一 logger 输出；旧 AppDelegate 安装真实 overlay 监听器。
- 初次完整测试暴露 core_launch 的公共 PreferenceDefaults 头缺少 common include 路径，已修正。
  首次无显示执行 renderer-error 失败，实际错误为 SDL 无可用视频设备；Xvfb 下完整测试通过。
- 原生 UI 探针前几轮分别因 fixture 未加载 fstat、Timer 未启用而失败；修正真实脚本初始化。
  后续查出 GTK 返回 UTF8 经旧窄字符转换丢失补充平面字符，现原生输入结果显式转换 UTF16。
  探针 XGetImage 曾与 GTK chooser 调整窗口大小竞争，改为抓取稳定 root 并裁剪，不影响交互判断。
  历史失败日志 `dialog-ui-01` 至 `04` 保留，不作为通过证据。
- PVRTC 使用独立 MIT PowerVR SDK，源码原样及固定提交见 external/pvr/UPSTREAM.md。
  与旧 Cocos 随机块对照并非逐字节相同：4 bpp 透明蓝低位展开存在旧实现错误；
  临时仅修正该差异后 4 bpp 对照通过，但 2 bpp 调制仍不同，因此未将临时兼容改动写入生产。
  使用现代原样 decoder，新增两项固定端点/透明颜色/输出边界真实测试，均覆盖 2/4 bpp；
  此颜色差异明确记录在正式说明，不声称任意压缩纹理商业游戏已兼容。

最终独立程序：`out/native-independent/bin/krkr2/krkr2`，SHA256
`3d2da8b6d5758f2df68af5ec2da505533e948095bad9fc226fa7b12a43384274`。
`independent-configure.log`、`independent-build-final.log`、`independent-tests-final.log`、
`native-closure-final.log` 均在 `out/native-host-validation/`。

最终验证：

- `xvfb-run -a ctest --test-dir out/native-independent --output-on-failure --no-tests=error --parallel 2`：
  **113/113**，含 3 个生产脚本 runtime；非零用例已实际发现，不是零测试成功。
- CLI 默认与显式 `--host=sdl3` 各 **15 项**；`--host=cocos --help` 拒绝。
  日志 `native-cli-default.log`、`native-cli-sdl.log`、`native-cocos-rejected.log`。
- `sdl_host_repro.py`：真实窗口/像素、输入/Unicode、缩放、隐藏恢复、关闭否决、模态退出通过；
  `native-host-final/report.json`，exit 0、forced_stop=false。
- `sdl_dialog_repro.py`：菜单两次勾选、嵌套菜单、禁用/隐藏条目、取消、
  输入 `native 中文 𠀀` 确认/取消、三个消息按钮、文件选择/取消、UTF8 文件复制及防止自复制通过；
  `dialog-ui-05/report.json`，exit 0、forced_stop=false。已查看实际 menu/file chooser 截图。
- `imageops_repro.py --renderer software`：Layer 变换和 BMP 像素断言通过，exit0、无强停，
  `native-imageops-final/report.json`，已查看 BMP。此项只验证软件工厂，不声称 OpenGL。
- 固定 KAG3 `1f3ab309106d210e3169bbbe0fb4e066ae463b42`，自制场景菜单和存读档 checkpoint42 通过；
  `native-kag-final/report.json`，exit0、无强停。
- 自制 440/660Hz WAV + MPEG4/AAC 880Hz 视频：实际窗口红绿帧及结束清除、自然结束和跳过、
  OpenAL PCM 输出幅度 4120.4/4115.8/4151.8 通过；`native-media-02/report.json`，exit0、无强停。
  已查看视频帧截图。首次媒体探针因私有 sysroot Openbox 找不到主题失败；设置正确 XDG_DATA_DIRS 后通过。
- 旧混合宿主重新构建，SHA256 `7fc250e86a8790cca7f22b5ea2f1ee40b60f42e57cc89b610e59a2aac7ebb802`；
  默认/SDL CLI 各15、脚本3/3、普通应用软件/OpenGL imageops 全通过。
  日志 `legacy-regression-build.log`、`legacy-cli*-regression.log`、`legacy-script-regression.log`，
  `legacy-imageops-regression/report.json`。原 FMOD/JPEG XR 链接警告仍存在，构建退出0。

新增专用 Cocos-free Linux CI，从空 SDK 安装，执行同类闭包、通用测试、X11 UI、图像和 KAG/媒体验证。
工作流已静态检查，远端 GitHub Actions 未运行。新增 Linux SDL3 配置预设与软件 renderer 过滤参数。
依赖守卫、clang-format20 和 diff 检查通过。未验收 Wayland/物理高DPI、实体GPU、
Windows/macOS/Android 无 Cocos 构建或任意商业游戏完整剧情。


## 2026-11-24 同步 Windows 并统一独立宿主

已 fetch 并快进至 dev `41c783d`，恢复本地 Linux 工作后整合；原 Linux stash 及二进制 diff/未跟踪文件归档
保存在本机，用于保留合并前状态。Windows 真实游戏测试已经完成，正式证据见
`../testing/2026-11-24-windows-sdl3-independent.md`；不再把“首次真实游戏验收”当作待办。

- 正式开关统一为 `KRKR2_SDL3_ONLY`，支持 Windows/Linux；Linux 旧 `KRKR2_COCOS_FREE` 仅作兼容别名。
  公共 engine feature 不含 Cocos；旧宿主显式选择 cocos feature。Linux 正式宿主单独选择 native-dialogs，
  不把 GTK 强加给独立 SDL3 实验。重新开启 manifest 安装核验，无需新增端口且 SDK 仍无 Cocos。
- `sdl3/NativeServices.cpp` 统一资源/配置作用域、文件复制/防自复制、URL、窗口/视频/截图接线。
  两平台都在 CLI 归档校验之前安装服务，生命周期持续到引擎清理后。
  GTK 菜单等只在 Linux 编译，Windows 保留真实 Win32 菜单/文件选择与调试日志。
- 保留远端 WindowEx 初始化修复与真实引擎生命周期回归。保留 SDL 截图和模态绘制接口。
- 删除 Linux 初版现代 PowerVR decoder/许可/专用端点测试，改用两平台共享的
  visual/PVRTCDecoder.cpp（来源/许可沿用 Windows 已入库记录）。对照旧 Cocos 3.17.2 decoder，
  两种码率/16,32,64 宽/8,16,32 高/各50组固定种子随机块共 **900** 组逐字节相同；
  对照程序仅在 out 中引用旧 Cocos 头，生产及独立测试不引用。之前记录的蓝通道/2bpp 差异不再适用最终实现。
- 最终普通 Linux Release 程序 SHA256 `1ea60dede329ca5afbf39b9aee0a88f337045664c3811668c6bf34e7784be3ba`；
  367 编译单元、162 SDK ports、21/91 资源，UI test driver OFF。完整 **114/114** 测试通过，
  包含 WindowEx、共享 PVRTC、3个脚本 integration；独立闭包检查通过。
- 默认与显式SDL CLI各15，系统窗口/输入/关闭否决、原生菜单/输入/文件对话框、
  软件图像操作、KAG菜单/存读档、生成音视频全部通过；报告哈希匹配最终程序，均退出0且无强停。
  已查看本轮勾选菜单和视频红绿帧截图。证据在 `out/integration-validation/` 的 tests/closure/*.log 和各 report.json。
- 旧混合宿主重新构建，默认/SDL CLI各15、3/3脚本 smoke、软件/OpenGL imageops全通过。
- 增加 Windows Release 预设与双平台独立CI；打包脚本仅接受通过闭包检查的普通Release，
  按选中资源清单打包并提供文件哈希、源码身份/dirty状态、外部动态依赖清单及压缩包SHA256。
  Linux包解压校验实际暴露资源循环变量覆盖包根目录名，已修正并保留失败压缩包；
  修正后检查根目录、全部文件哈希、0755执行权限、CLI与实际窗口脚本。

这轮本机只能运行 Linux；Windows 原有真实游戏记录是远端41c783d的结果，不能视为合并后的新Windows验收。
双平台远端CI结果将在评审分支记录，未完成项保持明确状态；macOS/Android不包含在独立宿主支持范围。


评审分支 `codex/unify-sdl3-native-hosts` 已创建草稿 PR #60，目标 dev，未合并。
首轮 Windows 专用CI在 bootstrap 前失败：MSVC setup 将 VCPKG_ROOT 覆盖为 Visual Studio 自带路径，
导致 clone 拒绝写入已存在目录。已按旧 Windows 工作流恢复 repository-local 路径的方式修复；
这是CI环境接线错误，不是Windows编译通过/失败结论。
包内所有文件哈希、根目录和实际CLI/窗口脚本已通过。归档执行位为0755；
本机解压默认 umask0077 会变为0700，已核对 tar header，未将此环境掩码当作打包权限故障。


### CI 暴露的首次音轨启动竞态

首个 Linux 干净SDK任务（run45510931599/job138858145801）完成独立编译、114项CTest、
真实X11窗口/菜单/输入/文件对话框、图像操作和KAG存读档，但混合PCM缺少440Hz BGM，
幅度36.2；660Hz SE和880Hz视频音频存在。原始证据与Ubuntu程序已下载到
out/integration-validation/ci-linux-first、ci-linux-binary。相同Ubuntu程序在本机普通调度下通过，
不能据此否定CI失败。

通过仅用于测试的LD_PRELOAD pthread启动延迟150ms稳定复现；未修复Ubuntu程序的最小
WaveSoundBuffer首播场景退出0且无强停，但440Hz幅度0，完整KAG媒体也只剩SE和视频音频。
原因为StartPlay已要求工作线程启动，FillBuffer却仍依赖工作线程稍后更新的
TVPPrimarySoundBufferPlaying；首次队列为空时调用OpenAL Play，后续仅填充而未重新播放。
现allowpause=false的同步预填充不再等待该异步确认；正常工作线程填充仍受主缓冲暂停状态约束。
这是共享音频逻辑修复，未降低PCM断言、未加入延时到生产路径。

新增audio-first-buffer-runtime，以真实引擎、真实OpenAL wave输出和测试专用调度扰动检查首播。
辅助库独立于生产链接，仅在Linux独立应用的脚本smoke配置注册，测试驱动仍OFF。
初版测试脚本漏传WaveSoundBuffer owner且未创建窗口，分别导致脚本异常/提前退出；
已修正测试初始化，保留失败记录。正确最小场景修复前幅度0、退出0，修复后通过；
原完整媒体延迟场景修复后440/660/880Hz幅度4120.4/4115.8/4191.7，退出0且无强停。

另一Linux任务因ftpmirror.gnu.org证书异常、ftp.gnu.org的gperf下载超时而停止，
并非C++构建错误。CI增加mirrors.kernel.org稳定HTTPS预取和与固定vcpkg端口相同的SHA512校验；
不关闭TLS校验、不替换依赖版本。增加源码下载缓存，避免反复请求GNU镜像。

修复后完整115/115测试通过；最小首播测试包含真实窗口维持事件循环、延迟150ms工作线程、
440Hz PCM幅度2159.4和正常退出。旧混合宿主也重新编译并用同一首播回归验证。
CI两平台均使用Release-only依赖triplet，避免为Release发布同时编译未使用的Debug依赖。


Windows首个完成代码构建的任务（run45511304458/job138859391715）通过独立Release编译、
353编译单元/107 SDK ports的无Cocos审计、67/67 CTest和15项CLI。
真实SendInput键盘、失焦释放、最小化恢复、两次原生菜单和消息框通过，文件对话框就绪判定超时。
证据中的Edit文字是fixture而非fixture.txt：托管Windows启用了隐藏已知扩展名。
探针现接受这两种原生显示形式；真实选中文件路径仍必须严格匹配fixture.txt，未更改生产文件对话框。
原失败报告/截图在out/integration-validation/ci-windows-first，仍记为失败，不计作完整交互通过。
