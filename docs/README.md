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
| [PSBFile](subsystems/psbfile.md) | 文件/Octet 加载、资源介质、格式边界与合成测试 |
| [Windows SDL3 独立宿主](subsystems/windows-sdl3.md) | 无 Cocos 构建、原生平台服务及游戏验证边界 |
| [桌面命令行](subsystems/desktop-cli.md) | 游戏启动、引擎设置及错误退出码 |
| [管理界面](subsystems/management-ui.md) | 移动端管理页面、桌面资源、配置与语言读取边界 |
| [宿主资源与 JSON 配置](subsystems/host-resources.md) | 本地资源根、设置存储、JSON 配置、XML 迁移与独立管理器验证 |
| [依赖审计与瘦身](subsystems/dependency-audit.md) | 实际消费者、SDK 清单测量、精简结果与平台边界 |
| [Linux 无 Cocos 运行时](subsystems/native-runtime.md) | SDL3 独立构建、原生对话框、资源与依赖验证 |
| [窗口接口](subsystems/window-interface.md) | 通用窗口接口、可选 KAG 按钮导航与 Cocos 适配边界 |
| [共享输入状态](subsystems/input-state.md) | 虚拟键、修饰键、指针坐标、提交文本及 Cocos 适配边界 |
| [Android 输入桥](subsystems/android-input.md) | UTF-16/JNI、打印键去重、退格和独立模拟器测试 |
| [纹理呈现](subsystems/render-presentation.md) | 后端无关画面描述、Cocos 纹理适配、宿主截图与视频覆盖层 |
| [ImageOps](subsystems/imageops.md) | CPU 图像契约、独立测试与微基准 |

## 参考与工具

| 文档 | 内容 |
|---|---|
| [插件来源](reference/plugins.md) | 插件原始资料与 clipAlphaRect 实现/测试边界 |
| [游戏验证范围](reference/game-compatibility.md) | 本地样本、已有证据与未验证能力 |
| [外部实现参考](reference/upstream.md) | 资料关系、适用范围与平台库入口 |
| [仓库工具](../tools/README.md) · [XP3 工具](../tools/xp3/README.md) | 实际工具的构建和使用 |
| [motionplayer](../cpp/plugins/motionplayer/docs/README.md) | 模块专属实现与契约 |

AI 工作流程和技能在 [.agents](../.agents/README.md)，
问题调查、待实现方案及历史验证在 [notes](../.agents/notes/README.md)。
UI 外壳、Launch 配置和 Rust 迁移草案不作为当前可用功能说明。

新增稳定指南或实现说明时更新本页；新提案和问题记录按 notes 约定归档。
