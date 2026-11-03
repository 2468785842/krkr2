# 迁移流程

Status: proposed

方案材料：本页包含待实施的接口或目录设计，不能作为当前构建与可用功能说明；事实以源码及 docs 为准。

[← 索引](2026-11-03-rust-overview.md)

---

## 1. 单模块生命周期

```text
① 新建 rust/crates/krkr-<m> + bridge 头 + 对应模块迁移 note
② C++ 包装委托 Rust（KRKR_USE_RUST_<M>）
③ 字节级 / 单测对齐
④ 删除 C++ 实现，CMake 去掉重复 find_package
⑤ bridge/manifest.toml 标记 stable
```

---

## 2. 目录方案对比

| 方案 | 评价 |
|------|------|
| `cpp/plugins/*/rust/` | ❌ 难扩展 |
| 根目录单 crate | △ 无分层 |
| **`rust/` + `bridge/` + `cpp/`** | ✅ 与 `cpp/` 对称，语言一目了然 |

---

## 3. 模块文档

| 模块 | 文档 |
|------|------|
| psbfile | [modules/psbfile.md](2026-11-03-rust-psbfile.md) |
