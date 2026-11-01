# KrKr2 UI 与外壳层

> **状态：** 设计草案（已修订）  
> **原则：** 引擎 CLI 化 · 外壳 UI 可选且分离 · Mobile 设置与游戏分 Activity · 渐进替换 Cocos2d-x

---

## 背景

KrKr2 当前使用 **Cocos2d-x 3.17.2** 同时承担：

1. **外壳 UI** — 文件选择、设置、菜单等（`cpp/core/environ/ui/`）
2. **游戏视口** — KAG 画面、LayerBitmap、YUV（`MainScene`、`YUVSprite`）

Cocos UI 已停更且表现力不足。**修订后的目标架构：**

| 平台 | 外壳 UI | 引擎入口 | 游戏渲染 |
|------|---------|----------|----------|
| **Desktop** | **无内置 UI**；可选独立 **Launcher**（Electron/React） | `krkr2` CLI + [`launch.json`](../launch/launch-config.md) | Native GL（`krkr2-render`） |
| **Mobile** | **RN Settings Activity**（库/设置） | Intent → `LaunchOptionsAdapter` | **Native Game Activity** + GL |

> Desktop **不**用 Electron BrowserWindow 嵌游戏；Mobile **不**用 RN 包 GL 视口。

---

## 与 Launch 层分工

```text
┌─────────────────────────────────────────────────────────┐
│  UI 层（本文档）                                         │
│  · 可选 Launcher / RN Settings                         │
│  · 写 launch.json、Intent extras、GlobalConfig overlay   │
└───────────────────────────┬─────────────────────────────┘
                            │ launch.json / Intent
┌───────────────────────────▼─────────────────────────────┐
│  Launch 层（docs/launch/）                               │
│  · CliParser (argparse) · LaunchOptionsAdapter           │
│  · TVPSetCommandLine · LaunchContext::xp3Path           │
└───────────────────────────┬─────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────┐
│  cpp/ 引擎（krkr2core）                                  │
└─────────────────────────────────────────────────────────┘
```

详见 [Launch 层](../LAUNCH_LAYER.md)。

---

## 阅读顺序

| 顺序 | 文档 | 内容 |
|:----:|------|------|
| 0 | [../launch/architecture.md](../launch/architecture.md) | CLI 与适配器（Desktop 必读） |
| 1 | [architecture.md](architecture.md) | UI 分层、Launcher / RN 模型 |
| 2 | [monorepo.md](monorepo.md) | `apps/launcher` + `apps/mobile-settings` |
| 3 | [bridge.md](bridge.md) | launch.json / Intent，非 Electron 嵌引擎 |
| 4 | [migration.md](migration.md) | 迁移阶段 |
| 5 | [rendering.md](rendering.md) | `krkr2-render` |

---

## 设计目标

| 目标 | 做法 |
|------|------|
| **Desktop 轻量** | 引擎仅 CLI；图形设置交给可选 Launcher |
| **Mobile 清晰** | Settings（RN）与 Game（Native GL）分 Activity |
| **跨端一致** | 共享 `launch.json` schema + `packages/shared` 类型 |
| **不嵌 GL 到 Web** | 拒绝 BrowserWindow / RN Canvas 跑 KAG |
| **渐进迁移** | 先 Launch + 去 Cocos UI，再抽 `krkr2-render` |

---

## 明确不做

| 方案 | 原因 |
|------|------|
| Electron 作为主壳嵌 GL | BrowserWindow 嵌入差、体积大 |
| Electron + WASM 引擎 | 等于重写运行时 |
| Flutter/Electron 统一包游戏画面 | 纹理共享复杂，收益低 |
| Desktop 内置 Cocos 文件选择（长期） | 由 CLI / Launcher 替代 |

---

## 相关链接

| 资源 | 说明 |
|------|------|
| [LAUNCH_LAYER.md](../LAUNCH_LAYER.md) | CLI、适配器、launch.json |
| [RUST_LAYER.md](../RUST_LAYER.md) | Rust 插件 |
| `cpp/core/environ/ui/` | 待废弃 Cocos UI |
| `tools/xp3/` | argparse 参考 |
