# 外部参考与适用边界

核对日期：2026-10-31。以下是阅读过的项目首页/文档，不是全量代码审计或兼容性背书。
“本项目二开”和“同生态移植”分别标明；不因同名 krkr 就假定代码可以直接合并。

| 资料 | 关系与可借鉴内容 | 应注意的边界 |
|---|---|---|
| [colorcard/KrKr2Emulator_Tutorial](https://github.com/colorcard/KrKr2Emulator_Tutorial) | 面向本项目的教程，可帮助理解目录、KAG 和媒体链路 | 教程将 SDL2 描述为输入层；当前本地清单无 SDL 依赖，tvpsdl.cpp 未见 SDL 调用，必须以本地实现复核。 |
| [Kirikiri SDL 项目](https://github.com/krkrsdl2/krkrsdl2) · [项目说明](https://krkrsdl2.github.io/krkrsdl2/en/) | 基于 Kirikiri 2/Z 的同生态移植；可研究平台层、draw device 和构建分层 | 项目说明明确不支持未经修改的商业游戏，不能直接作为本模拟器的兼容承诺。 |
| [Wamsoft 系统架构](https://wamsoft.github.io/krkrz_dev/en/specification/system/) | Kirikiri Z 衍生实现的文档；介绍 SDL3 GENERIC、原生 Windows 和静态库形态共享核心 | 文档属于该实现，非当前仓库现状；可参考“共用语义核心、分开平台绑定”，不能直接替换本项目渲染协议。 |
| [clevebitr/Krkr2Next](https://github.com/clevebitr/Krkr2Next) | 相关 Android 二开，README 描述 Kotlin/Compose 壳、C++ 核心及 EGL/GLES3 | Android 专用方案；可研究 bridge/engine_api 和宿主生命周期，不能推定跨桌面可复用或已经完整移除全部依赖。 |
| [OWCramer/KiriKit](https://github.com/OWCramer/KiriKit) | README 明确说明使用 krkr2 核心和独立环境、Metal、音视频及宿主胶水 | 聚焦 iOS/iPadOS；可参考壳与合成器分离，不能直接当作跨平台方案。 |

本地历史资料另见 [Core 模块解耦](CORE_MODULE_DECOUPLING.md) 和 [Rust 文档](rust/README.md)。
前者部分事实已被后续实现更新，例如 XP3 工具已使用 core_storage_tool，见 [当前架构](architecture.md)。

## 调研到实现的记录方式

1. 写清所解决的问题和本地对应接口，再选择外部资料，避免照搬整套架构。
2. 真正移植代码前固定来源仓库、提交、文件、依赖版本和许可/版权声明；本页浮动链接只用于导航。
3. 明确“文档声明”“已检查实现”“本地测试验证”三个层级。没有运行过的能力保留为待验证。
4. 用自制 fixture 验证事件顺序、像素、脚本与生命周期，再考虑切默认或删旧实现；外部截图和通过列表不能替代本地验收。

## 平台库原始资料

- [SDL3 文档](https://wiki.libsdl.org/SDL3/FrontPage)：平台窗口/输入及媒体 API，查具体平台支持和生命周期约定。
- [Sokol](https://github.com/floooh/sokol)：独立 C 头文件模块；sokol_app 以单窗口模型为主，平台壳与 sokol_gfx 可分别评估。
- [GLFW FAQ](https://www.glfw.org/faq)：桌面窗口/输入和图形上下文，当前官方平台范围不含 Android/iOS。
- [MiniFB](https://github.com/emoon/minifb)：像素缓冲呈现和事件，适合软件渲染验证；移动端能力有单独限制。

这些资料只辅助选型，不预设本项目必须引入更多库。优先复用已满足契约的代码，并用同机测量决定精简收益。
