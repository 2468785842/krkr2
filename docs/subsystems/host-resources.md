# 宿主资源与配置 XML

## 本地资源读取

`cpp/core/environ/resources/LocalAssetReader.*` 是只依赖 C++17 的本地资源读取器。
它按构造时给定的资源根顺序查找相对文件名；绝对文件名直接访问本地文件，不追加资源根。
不把工作目录隐式加入查找路径，目录不算文件。二进制和空文件均可读取。
文件在调用同步字节回调前关闭；回调返回后字节失效，回调抛出的异常正常向上传递。

`ScopedReader` 在宿主线程安装、恢复读取器。其他线程取得的 shared_ptr 保持读取器存活，
每次查询使用一个完整的读取器快照。
Linux SDL3 入口在 CLI 解析完成后、XP3 校验前，通过 `ConfigManager/LocalHostServices.*`
同时安装本地资源和设置存储。资源根为 `/proc/self/exe` 指向的实际程序所在目录下的
`Resources`，对应现有 Linux 应用的资源打包位置。
作用域覆盖归档校验、脚本启动、主循环和引擎清理；SDL3 分支不再构造 Cocos AppDelegate。

`TVPHostFileExists` / `TVPReadPackagedAsset` 定义在通用模块的 `resources/HostAssets.cpp`。
Cocos AppDelegate 在构造时安装 `CocosPackagedAsset.cpp` 的提供器，保留原有文件搜索与移动端打包资源访问。
SDL3 入口直接安装本地提供器，结束后恢复原提供器；服务构造中途失败也恢复此前的资源提供器。
未初始化提供器属于宿主接线错误，入口抛出 logic_error，不返回伪造的空资源。
通用资源库可以独立链接，完整应用组合根仍依赖 Cocos。

## 配置 XML

`ConfigManager/ConfigXml.*` 的 `core_config_xml` 只链接仓库已有的 LibXml2，
不依赖 Cocos、TJS、窗口或平台设置路径。
`GlobalConfigManager` 和 `LocaleConfigManager` 使用该模块解析运行时设置与语言目录。
`core_config` 包含真实的全局、游戏和语言配置管理器，以及 `PreferenceStorage` 接口；
它只链接 XML、资源和系统线程库，不再包含 Platform.h 或 TJS。
宿主安装存储提供器后才能初始化配置；未安装时抛出 logic_error。
`ScopedPreferenceStorage` 按宿主线程作用域恢复原提供器，持有者使用 shared_ptr 保持提供器存活。

`TVPGetDataPath` 的 TJS 包装位于 `environ/HostPaths.cpp`；
`TVPGetInternalPreferencePath` 由存储提供器提供，调用方使用返回引用时必须保持宿主作用域有效。
Cocos 适配器保留原 FileUtils 路径、目录创建、平台写入（包括 Android 权限路径）及本地化失败提示。
桌面入口在 CLI/XP3 目标校验之前安装存储提供器；移动端由 AppDelegate 构造安装。
Cocos 工厂只创建提供器，路径和目录在实际查询时初始化；Linux SDL3 在有效目标解析后创建
标准库本地存储，帮助、列举设置和目标解析失败不创建本地设置目录。设置目录保持
`$XDG_CONFIG_HOME/<实际可执行文件名>/.preference/`，未设置 XDG 时使用
`$HOME/.config/<实际可执行文件名>/.preference/`，不迁移用户文件。
显式为空的 XDG 仍沿用旧路径规则；两个环境变量均缺失时报错，不写入当前目录。
本地存储使用 UTF-8 文件名、二进制读写并检查关闭结果，失败提示由宿主回调提供。
设置保存失败后仍清除 dirty 标记，保持旧行为；本次没有改变重试策略。
移动端最近游戏列表等 Cocos UI 内部代码仍使用旧 tinyxml2。

设置文件保留 `GlobalPreference`、`Item(key,value)`、有序 `Custom(key,value)` 和
`KeyMap(key,value)` 的结构。值的重复键取最后一项，自定义参数保留重复项和顺序，
按键映射重复键取第一项并忽略零值和无效整数；只处理根下的直接子节点。
序列化写 UTF-8 XML，转义属性中的特殊字符和换行，不再使用固定 4096 字节的格式化缓冲区。
语言目录支持 UTF-8 BOM、XML 实体、空文本和重复 id，未知文本查询继续返回键名。
旧语言文件在引号属性中直接使用 `<`（如 `<300MB`）；解析入口只将这种字符转为
XML 的 `&lt;` 后交给解析器，读取后的文字保持原样。注释、CDATA 和正常标签不做这种转换。
属性中的原始制表符也保留；原始 CR、LF、CRLF、LFCR 按旧解析器规则归一化为 LF，
通过字符引用避免新解析器将它们折叠为空格。已经写成字符引用的 CR/LF 保留原值。

LibXml2 已是完整引擎依赖。使用它避免在保留 Cocos 的过渡构建中再链接一份不同 ABI 的 tinyxml2。
文件编码、空白排版和属性顺序可能与旧写入器不同；验收比较读取后的值，不要求 XML 字节相同。
自带英文语言文件中一处 `&#10Report` 缺少实体终止分号，已修为 `&#10;Report`；
旧解析器此前会吞掉“Report bugs here:”片段，修正后两个解析器都保留该说明及换行。

## 独立验证

在配置好编译器和 VCPKG_ROOT 后执行；Windows 增加 `-DWINDOWS=ON` 和对应 triplet：

```sh
cmake -S . -B out/config-tests -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DKRKR2_BUILD_APP=OFF -DBUILD_TOOLS=OFF -DENABLE_TESTS=ON \
  -DKRKR2_TEST_SCOPE=config -DCMAKE_DISABLE_FIND_PACKAGE_cocos2dx=TRUE
cmake --build out/config-tests --parallel 4
ctest --test-dir out/config-tests -N
ctest --test-dir out/config-tests --output-on-failure --no-tests=error
```

该范围选择 vcpkg 的 config/tests features，只配置 LibXml2、Catch2 及其依赖，
不配置 TJS、Cocos、SDL、图像或媒体模块。实际登记 5 项 XML、5 项资源、5 项真实管理器/存储
和 5 项语言回退测试，另有 2 项 native 服务作用域与构造失败恢复测试，共 22 项；
编译 12 个源文件。语言测试使用真实全局配置管理器，
仅替换宿主存储与资源提供器。
`.github/workflows/headless-tests.yml` 的 config job 在三平台检查注册数量、源码清单及安装闭包。

full 范围额外登记 `config_xml_legacy_tests`，用原 tinyxml2 比较自带语言文件和新写出的设置 XML，
并对照原 Cocos 服务读取字体/语言文件的实际字节。
这些用例不创建游戏窗口；它们不能替代 Linux SDL3 完整运行、移动端权限写入或游戏兼容验收。

共享配置默认值通过 `common/TVPPlatform.h` 的 `TVP_PLATFORM_IOS` / `TVP_PLATFORM_ANDROID`
选择平台行为，不再要求预先包含 Cocos 平台头。完整应用的构建与链接仍依赖 Cocos，
此处的 native 服务验收不表示整个运行时依赖已移除。
