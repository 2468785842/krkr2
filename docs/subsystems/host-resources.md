# 宿主资源与 JSON 配置

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

## JSON 配置与旧文件迁移

`ConfigManager/ConfigFormat.*` 的 `core_config_format` 使用 nlohmann-json 读写 JSON，
使用 pugixml 只读迁移旧 XML；不依赖 LibXml2、Cocos、TJS、窗口或平台设置路径。
`core_config` 包含真实的全局、游戏和语言配置管理器，以及 `PreferenceStorage` 接口；
它只链接格式库、资源和系统线程库。

全局设置文件为 `GlobalPreference.json`，游戏目录设置为 `Kirikiroid2Preference.json`：

```json
{
  "version": 1,
  "values": { "renderer": "software", "volume": "20" },
  "arguments": [["arg", "first"], ["arg", "second"]],
  "keyMap": [[65, 66]]
}
```

`version` 和 `values` 必填；`arguments`、`keyMap` 可省略。设置值保留字符串类型，
由管理器沿用原整数、浮点和布尔转换规则。自定义参数用二元数组保留顺序与重复项；
按键映射为整数二元数组，重复键取第一项并忽略零键/零值。无效 JSON 或字段类型不载入部分设置。
写入格式化 UTF-8 JSON，特殊字符、制表符、换行和长文本均可往返，不使用固定长度缓冲区。

只有 JSON 路径不存在时，管理器才读取同名 `.xml`。成功读取后立即写入 JSON，原 XML 保留不变。
JSON 存在但为空、损坏或不可读取时，不回退旧 XML，避免恢复过时设置。
游戏设置的存在检查与移动端历史入口同时识别新旧文件。切换游戏时清空旧参数和按键映射。
旧 XML 的直接子节点、重复键、空值、属性实体和空白处理规则由迁移读取器保留；
pugixml 不加载外部 DTD 或网络资源。迁移写入失败仍保留当前内存设置和原 XML，
通过宿主报告写入失败；普通保存失败后清除 dirty 标记的既有策略未变。

语言资源改为 `assets/locale/<语言>.json` 的字符串字典。英文、中文由旧文件逐项转换，
原文件保存在 `tests/fixtures/legacy-locales/` 作为迁移对照，不再打包。
语言选择继续为用户设置优先、系统语言其次，缺文件时只尝试一次英文，缺少文本返回键名。
新增或修改语言文件时使用 JSON，不再加载语言 XML。

宿主安装存储提供器后才能初始化配置；未安装时抛出 logic_error。
`ScopedPreferenceStorage` 按宿主线程作用域恢复原提供器，shared_ptr 保持提供器存活。
`TVPGetDataPath` 的 TJS 包装位于 `environ/HostPaths.cpp`；
`TVPGetInternalPreferencePath` 由存储提供器提供，调用方使用返回引用时必须保持宿主作用域有效。
Cocos 适配器保留原 FileUtils 路径、目录创建、平台写入（包括 Android 权限路径）及本地化失败提示。
桌面入口在 CLI/XP3 目标校验之前安装存储提供器；移动端由 AppDelegate 构造安装。
Linux 设置目录仍为 `$XDG_CONFIG_HOME/<实际可执行文件名>/.preference/`，未设置 XDG 时使用
`$HOME/.config/<实际可执行文件名>/.preference/`。显式为空的 XDG 沿用旧规则；
两个环境变量均缺失时报错。本地存储使用 UTF-8 文件名、二进制读写并检查关闭结果。
命令行覆盖只影响当前进程的读取，不写入配置。

项目配置模块和 base 模块已移除直接 LibXml2 链接，vcpkg 的 engine/config features 也不再直接声明它。
本仓库的 libarchive overlay 为 XAR 显式依赖 libxml2，其 CMake 包装器将 XML 链接依赖附加到
`LibArchive::LibArchive`，避免静态链接依赖引擎其他模块偶然引入 XML。压缩库和 Windows CNG 也由此导入目标传递，
当前 Linux SDK 没有 XAR 必需的哈希后端，不应将安装 XML 库视为 XAR 已可用；边界见
[依赖审计](dependency-audit.md)。GTK 等第三方依赖也可能
间接引入 XML 库，不能据此宣称整个 SDK 没有 XML。
移动端最近游戏列表属于旧 Cocos 管理 UI，仍使用 Cocos 自带 tinyxml2 读写，与本配置格式库独立。

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

该范围选择 vcpkg 的 config/tests features，只配置 nlohmann-json、pugixml、Catch2 及构建辅助依赖，
不配置 LibXml2、TJS、Cocos、SDL、图像或媒体模块。实际登记 26 项测试，编译 12 个源文件。
覆盖 JSON 往返/格式校验、旧 XML 迁移与新文件优先级、两种语言逐项对照、真实管理器和资源/存储服务。
`.github/workflows/headless-tests.yml` 的 config job 在三平台检查注册数量、源码清单及安装闭包。

full 范围额外登记 `config_format_legacy_tests`，用 Cocos 原 tinyxml2 核对旧语言/设置的解析结果，
再验证旧设置迁移到 JSON 后的值，并比较两种资源提供器读取字体和 JSON 语言文件的字节。
这些测试不创建游戏窗口，不能替代移动端权限写入或真实游戏兼容验收。
