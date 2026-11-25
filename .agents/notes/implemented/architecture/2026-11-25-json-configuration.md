# 配置和语言资源迁移为 JSON

Status: implemented

## 问题与实现

基线为 dev `4913486265bbf135fad6c847c6857b5711b3b2fe`，本次在
`feat/json-preferences` 分支实现（下列验证针对工作区 dirty 状态）。用户希望配置不再依赖 XML。
全局、单游戏设置及中英文语言资源已改用 JSON；结构与兼容边界见
[宿主资源与 JSON 配置](../../../../docs/subsystems/host-resources.md)。

`ConfigFormat` 使用 nlohmann-json；pugixml 只负责读取旧 XML，不再写 XML。
选用不同命名空间的迁移解析器，避免与 Cocos 内置 tinyxml2 的 ABI/重复符号冲突。
原 XML 保留；JSON 缺失时迁移，已有 JSON 损坏/为空/不可读时不回退旧设置。
初始化/切换游戏会清空旧参数和按键映射，避免配置串到下一游戏。
移除 ConfigManager 和 base 的直接 LibXml2 链接及清单直接依赖。
第三方 libarchive/GTK 等传递依赖仍可能使用 XML；旧移动端历史列表仍用 Cocos tinyxml2。

## 验证证据

- 全新 `out/json-config-tests`，Release、app/tools OFF、scope=config：
  配置、构建、CTest 26/26 通过，12 个编译源；独立 SDK 仅含 nlohmann-json、pugixml、Catch2、
  vcpkg-cmake、vcpkg-cmake-config，没有 LibXml2/Cocos/FFmpeg/SDL。
  覆盖格式拒绝、中文/空值/特殊字符/长文本、重复参数、整数边界、按键映射、语言逐项对照、
  迁移失败、损坏新文件优先、游戏旧文件识别/新文件生成及切换后状态清理。
- `out/native-independent`，Release、SDL3_ONLY ON、UI_TEST_DRIVER OFF：
  完整应用构建成功，Xvfb 下全量 CTest 119/119 通过。
  `check_core_module_deps.py` 通过；`check_native_runtime.py` 验证 368 个编译单元、164 个 SDK ports，无 Cocos。
- 复用旧混合 SDK 的 `out/native-host-validation/app-verified`，显式指定新依赖的 CMake 包路径，
  构建 `config_format_legacy_tests`；原 tinyxml2 对照、旧设置到 JSON、实际资源字节三项全部通过。
  首次配置失败是旧 SDK 缺新包/Catch2，补充真实包路径后通过；没有用旧二进制代替新构建。
- CLI 15 项帮助/错误退出码通过。`imageops_repro.py --renderer software` 的 JSON 配置场景在 Xvfb 下通过，
  图片 SHA256 为 `395f5bb131813c513681734ed29859ea0682c6640e0fe80bebfc47e1375d711d`。
  首次未提供 DISPLAY，SDL 返回无可用视频设备；提供 Xvfb 后通过。
- `out/json-runtime-migration` 隔离 XDG 和游戏目录，真实应用启动同时迁移 GlobalPreference.xml 与
  Kirikiroid2Preference.xml。验证中文/实体、重复参数、按键映射、新 JSON 和保留原 XML；
  自制图像场景通过控制文件正常退出，没有强停。应用 SHA256：
  `34a5554dcaf7423cffd432bc79df55e12ad1894ea4f3518c667d1a0850696622`。
- `package_native_runtime.py` 成功生成约 60 MiB 的发布包；归档语言资源仅含两个 JSON，没有旧 XML。
  tar.gz SHA256：`aca1bbbbbe65bef22b539d2d065c95122686b032bcd0e1b4d39b1339264b7c4a`。
- 日志为 `out/json-*.log`；资源/配置测试与 CI 注册数量同步，修改的 Python 工具语法及 diff 检查通过。

## 边界

本次本机验证为 Linux；尚未重跑 Windows、macOS、Android。三平台 config CI 已同步依赖与 26 项断言。
先前 Windows 真实商业游戏验收属于基线证据，不作为本次 JSON 修改后的游戏验收。
