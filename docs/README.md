# 开发文档

这里维护当前实现、可执行的操作指南和使用边界。项目概览见[中文 README](../README_CN.md)，
代码职责见[当前架构](architecture.md)。文档中的命令默认在仓库根目录执行。

## 操作指南

| 文档 | 内容 |
|---|---|
| [构建与验证](guides/building.md) | 平台预设、独立构建、测试范围与测量方法 |
| [游戏与交互测试](guides/game-testing.md) | 隔离样本、TJS 插装、截图、事件与退出验证 |

## 已实现的子系统

| 文档 | 内容 |
|---|---|
| [Desktop 启动](subsystems/desktop-cli.md) | `--game` / `--xp3`、参数兼容及错误退出码 |
| [可选管理界面](subsystems/management-ui.md) | Desktop 管理页面开关、资源与默认配置边界 |
| [窗口接口](subsystems/window-interface.md) | 通用窗口接口与现有 Cocos 适配边界 |
| [ImageOps](subsystems/imageops.md) | CPU 图像契约、独立测试与微基准 |

## 参考与工具

| 文档 | 内容 |
|---|---|
| [插件来源](reference/plugins.md) | 插件原始资料；不表示全部内建或兼容 |
| [游戏验证范围](reference/game-compatibility.md) | 本地样本、已有证据与未验证能力 |
| [外部实现参考](reference/upstream.md) | 资料关系、适用范围与平台库入口 |
| [仓库工具](../tools/README.md) · [XP3 工具](../tools/xp3/README.md) | 实际工具的构建和使用 |
| [motionplayer](../cpp/plugins/motionplayer/docs/README.md) | 模块专属实现与契约 |

AI 工作流程和技能在 [.agents](../.agents/README.md)，
问题调查、待实现方案及历史验证在 [notes](../.agents/notes/README.md)。
UI 外壳、Launch 配置和 Rust 迁移草案不作为当前可用功能说明。

新增稳定指南或实现说明时更新本页；新提案和问题记录按 notes 约定归档。
