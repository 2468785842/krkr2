# libarchive 静态 XAR 依赖漏传 XML 链接

Status: implemented

## 问题与证据

JSON 配置 PR #61 已合入 dev `3f95a3678492e9c6fa0c81c81cc864e9aa332f5c`。
三平台独立配置、无头基础、Android 输入、Linux 完整/SDL3 应用 CI 已通过。
Windows SDL3 job `138920891225`（run `45529470559`）在最终链接报 11 个 LNK2019/LNK1120，
全部来自 `archive.lib(archive_read_support_format_xar.c.obj)` 引用的 xmlTextReader/xmlCleanupParser。

仓库的 libarchive overlay 3.7.7 显式依赖 libxml2，并实际编译 XML XAR 支持。
CMake 自带 FindLibArchive 只创建 archive.lib/libarchive.a 的导入目标，没有传递 XML 静态依赖。
之前 base/配置模块偶然直接链接 LibXml2 掩盖了这个问题；JSON 迁移移除直接边后 Windows 暴露失败。
配置格式库与 XAR 格式解析是两个独立消费者，不能通过给 JSON 模块重新加 XML 链接来修复。

普通 Windows job `138921059950`（run `45529470614`）另在下载
`boostorg/functional/archive/boost-1.91.0.tar.gz` 时 HTTP 503，尚未进入应用编译。
已单独重试该下载失败 job。

## 修复与验收

libarchive overlay 安装 `vcpkg-cmake-wrapper.cmake`，在正常 FindLibArchive 后将
LibXml2::LibXml2 加入 LibArchive::LibArchive 的 INTERFACE_LINK_LIBRARIES。
port-version 从 0 升为 1，确保旧 SDK 不复用没有包装器的包；保留 XAR 功能和 JSON 模块的独立性。

Linux 已重新安装实际 overlay，完整普通 SDL3 应用构建成功、119/119 项 CTest 通过；
独立最小 CMake 消费者确认导入目标传递 LibXml2::LibXml2。
Windows 普通/SDL3 CI 已完成实际构建、链接及对应验收，结果见下。
日志 `out/libarchive-xml-build.log`；后续跟踪对应修复 PR，不将旧 PR 下载重试当作修复头部的验证。


## 最终 CI 与合并

验证源提交 `c503cb183b1cce625467bb9f34c29c7801ef2017`，PR #62 已在全部 CI 成功后合入 dev，
合并提交为 `7632067f2b14b8dc316cf01ff0acf85783afd7b2`。

- 标准 CI `45559780997` 成功：Windows/Linux/Android 完整应用构建通过。
  Windows job `139020634450` 完成实际链接，15 项 CLI 检查及 17/17 项 Cocos 窗口契约通过；
  Android 完整应用构建与产物上传成功。单独的可选 Android runtime-contracts job 按配置跳过，
  不将该跳过计为通过的游戏运行验证。
- SDL3 原生 CI `45559780864` 成功：Windows job `139020449380` 的 67/67 项测试、
  15 项 CLI、原生交互、发布打包通过；依赖审计为 353 个编译单元、109 个 SDK ports，无 Cocos。
  Windows zip SHA256：`2facafec7a100667a7ae0b1665f2baf7700d65af552885f8b0063e6b2eb2ee70`。
  Linux job `139020448892` 的构建、119 项测试、窗口/对话框、KAG 存读档、音视频及打包通过。
- 无头基础 CI `45559780973` 的 16 个任务成功，其中三平台 config 各 26 项测试通过。
- Android 输入 CI `45559780932` 成功，保留其真实 Framework/ART/JNI 验证范围。

JSON 配置格式保持不变，XML 链接依赖归属于归档库。
本次不声称重跑过商业游戏，也没有做 macOS 完整应用验收；先前 Windows 商业游戏验收仍属于基线记录。
本记录更新只修改文档，不改变上述已验收的代码。
