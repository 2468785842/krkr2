# libarchive 静态 XAR 依赖漏传 XML 链接

Status: proposed

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
还需 Windows 普通/SDL3 CI 的真实链接、原生交互及发布打包通过后完成验收。
日志 `out/libarchive-xml-build.log`；后续跟踪对应修复 PR，不将旧 PR 下载重试当作修复头部的验证。
