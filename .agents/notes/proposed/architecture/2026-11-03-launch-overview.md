# KrKr2 启动与命令行层

Status: proposed

方案材料：本页包含待实施的接口或目录设计，不能作为当前构建与可用功能说明；事实以源码及 docs 为准。

> **状态：** Desktop L1 已实现，其余为设计草案。实际可用语法、代码边界及验证见 [Desktop CLI](../../../../docs/subsystems/desktop-cli.md)。
> **原则：** argparse 管外壳语法 · TVPGetCommandLine 管引擎运行时 · 适配器单向灌入 · Desktop CLI / Mobile Intent 共用 `apply()`

---

## 背景

KrKr2 当前启动参数来源分散：

| 来源 | 现状 | 问题 |
|------|------|------|
| `platforms/*/main.cpp` | Desktop 共用 PrepareDesktopLaunch，支持 --game / --xp3 / 位置参数 | 尚无 launch.json / Intent 适配 |
| `TVPCheckStartupArg()` | Desktop 仅保留平台启动服务；移动端保留 | 命令行解析已移至 Launch 层 |
| `TVPProgramArguments` | `TVPGetCommandLine` / `TVPSetCommandLine` | 吉里吉里兼容层，**不应删除** |
| `PushAllCommandlineArguments()` | 重放初始化前通过 TVPSetCommandLine 设置的启动参数 | 保持初始化重建参数后的值及优先级 |
| `GlobalConfigManager` | tinyxml2 XML | 设置 UI 用，与 CLI 未统一 |

新方向：

- **Desktop**：无内置 UI，靠 CLI + 可选 `launch.json` + 外部 launcher\
- **Mobile**：RN/Kotlin Settings → Intent / JSON → 同一套 C++ `apply()`\
- **CliParser**：基于 vcpkg **argparse**（与 `tools/xp3` 一致）

---

## 阅读顺序

| 顺序 | 文档 | 内容 |
|:----:|------|------|
| 1 | [architecture.md](2026-11-03-launch-architecture.md) | 分层、数据流、平台入口 |
| 2 | [cli-parser.md](2026-11-03-launch-cli-parser.md) | argparse 子命令与参数 |
| 3 | [adapter.md](2026-11-03-launch-adapter.md) | 适配器与 TVPGetCommandLine |
| 4 | [launch-config.md](2026-11-03-launch-launch-config.md) | JSON schema、与 XML 配置关系 |

---

## 设计目标

| 目标 | 做法 |
|------|------|
| **统一入口** | 各平台最终调用 `LaunchOptionsAdapter::apply()` |
| **不破坏吉里吉里** | 引擎模块继续 `TVPGetCommandLine`，不改调用点 |
| **外壳现代化** | `--xp3` / `--config` / `--help` 由 argparse 解析 |
| **跨端共享** | `LaunchOptions` 结构体 + 同一 `apply()`；Mobile 跳过 argparse |
| **渐进迁移** | XML GlobalConfig 可保留；launch.json 作 overlay |

---

## 不在范围内

- 替换 `TVPGetCommandLine` 全库实现\
- 一次性将 GlobalConfig XML 改为 JSON（可 Phase 2）\
- Cocos / Electron 内置文件选择 UI（见 [UI 层](2026-11-03-ui-overview.md)）

---

## 源码规划（待建）

```text
cpp/core/environ/launch/
├── LaunchOptions.h           # 结构化启动选项
├── CliParser.cpp             # argparse（Desktop）
├── LaunchOptionsAdapter.cpp  # apply() → TVPSetCommandLine + LaunchContext
├── LaunchContext.h           # xp3Path、是否已指定启动目标
└── LaunchConfigJson.cpp      # launch.json 读写

platforms/*/main.cpp          # parse → apply → AppDelegate
```

---

## 相关代码（现状）

| 路径 | 说明 |
|------|------|
| `cpp/core/sysinit/impl/SysInitImpl.cpp` | `TVPGetCommandLine` / `TVPProgramArguments` |
| `cpp/core/environ/win32/Platform.cpp` | Windows `TVPCheckStartupArg` |
| `tools/xp3/main.cpp` | argparse 用法参考 |
| `vcpkg.json` | `argparse` 依赖（desktop） |
