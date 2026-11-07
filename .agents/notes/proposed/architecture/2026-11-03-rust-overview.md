# KrKr2 Rust 层

Status: proposed

方案材料：本页包含待实施的接口或目录设计，不能作为当前构建与可用功能说明；事实以源码及 docs 为准。

> **构建：** CMake + vcpkg + Corrosion · 产物 `out/<platform>/<config>/`\
> **原则：** Rust 在 `rust/` · C++ 在 `cpp/` · 契约在 `bridge/`

---

## 阅读顺序

| 顺序 | 文档 | 内容 |
|:----:|------|------|
| 1 | [architecture.md](2026-11-03-rust-architecture.md) | 设计目标、`rust/` / `bridge/` / `cpp/` 分层、多 crate 扩展 |
| 2 | [ffi.md](2026-11-03-rust-ffi.md) | C ABI、Corrosion、cbindgen、内存约定、工具对比 |
| 3 | [dependencies.md](2026-11-03-rust-dependencies.md) | vcpkg 共用、krkr-sys-*、zlib/lz4 决策 |
| 4 | [build.md](2026-11-03-rust-build.md) | `KrkrRust.cmake`、输出目录、cbindgen 挂钩 |
| 5 | [migration.md](2026-11-03-rust-migration.md) | 单模块迁移步骤、目录方案对比 |

---

## 模块迁移文档

| 模块 | 文档 | C++ 胶水 | Rust crate |
|------|------|----------|------------|
| **psbfile**（首个拟迁移模块） | [modules/psbfile.md](2026-11-03-rust-psbfile.md) | `cpp/plugins/psbfile/` | `rust/crates/krkr-psb` |

---

## 源码布局（规划）

```text
rust/                         # ★ 全部 Rust 源码（无 C++）
├── krkr-ffi/
├── krkr-sys/{zlib,lz4,…}
├── crates/krkr-psb/
└── tools/krkr-psb-export/
bridge/include/krkr/          # C ABI 头（cbindgen + common.h）
cpp/plugins/                  # C++ 胶水（TJS、包装类）
```

本次核对未在仓库找到所设计的 Cargo workspace 或 `cmake/KrkrRust.cmake`；以下路径和构建方式均为提案，不能作为可执行指南。

---

## 相关链接

| 资源 | 说明 |
|------|------|
| [motionplayer 文档索引](../../../../cpp/plugins/motionplayer/docs/README.md) | psbfile 主要消费者 |
| `vcpkg.json` | 全工程原生依赖清单 |
