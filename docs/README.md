# 开发文档索引

新参与者与 AI 从根 [AGENTS.md](../AGENTS.md) 开始，再按任务选择专题。

| 主题 | 入口 | 状态/用途 |
|---|---|---|
| 项目概览 | [中文 README](../README_CN.md) · [English](../README.md) | 平台、依赖准备、使用入口 |
| 代码与依赖地图 | [当前架构](architecture.md) | 依据当前源码，含历史文档修正 |
| 编译与验证 | [构建与验证](building.md) · [FAQ](FAQ.md) | 可执行命令、测试边界、基线记录 |
| 后端精简 | [迁移路线与任务](backend-modernization.md) | 提案及验收；尚未实现新后端 |
| 依赖瘦身 | [依赖审计与替代方案](dependency-slimming.md) · [实施记录](dependency-slimming-implementation.md) | dev 源码证据、优先级、候选库与测量要求 |
| 图像模块 | [ImageOps 接口与验证](imageops.md) | 独立构建、像素契约、微基准及剩余边界 |
| 外部研究 | [参考资料与适用边界](references.md) | 教程、二开与原版资料 |
| 既有存储解耦 | [Core 模块分析](CORE_MODULE_DECOUPLING.md) | 历史分析；部分内容已落地，以当前架构为准 |
| 插件 | [插件说明](KRKR_PLUGINS.md) · [motionplayer 索引](../cpp/plugins/motionplayer/docs/README.md) | 插件专属契约与排错 |
| Rust | [Rust 专题](rust/README.md) · [早期说明](RUST_LAYER.md) | 设计及迁移资料，不代表整个引擎已迁移 |
| 命令行工具 | [工具](../tools/README.md) · [XP3](../tools/xp3/README.md) | 工具构建与使用 |

维护原则：现状写架构，操作写指南，待实现任务写路线，排错写 FAQ，原始测量写报告；不要在多个文档复制同一份状态清单。专题更新时同步真实入口和链接。
