# Linux 纯 SDL3 运行时的依赖闭包

Status: proposed

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

1. 用正式 Linux SDL3 主机验收新资源和设置接线；独立库已完成，完整 SDL3 运行本轮未验收。
2. 整理平台默认值宏与剩余平台服务；纯 SDL3 入口还须在 CLI/XP3 校验前安装 native 提供器，
   当前桌面入口仍先装 Cocos 提供器。全局设置路径、读写及失败提示已从管理器拆出。
3. 接通脚本菜单、文件/输入对话框和剩余平台服务，禁止用空实现满足链接。
4. 为应用配置纯 SDL3 的目标、manifest feature 与资源打包分支；生产源码不编译 Cocos 适配层。
5. 在独立 Linux 安装目录证明不安装、不查找、不链接 Cocos，并跑现有窗口、IME、KAG
   存读档、音视频、输入与正常退出探针。其余平台仍保持旧后端并单独验收。

风险集中在资源搜索语义、脚本 UI、平台服务遗漏和 XML ABI；
接口拆分或单元测试通过均不能作为纯 SDL3 里程碑完成结论。
