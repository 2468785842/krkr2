# psbfile 模块迁移

Status: proposed

方案材料：本页包含待实施的接口或目录设计，不能作为当前构建与可用功能说明；事实以源码及 docs 为准。

[← Rust 层索引](2026-11-03-rust-overview.md)

| 项 | 路径 |
|----|------|
| C++ 胶水 | `cpp/plugins/psbfile/` |
| Rust crate | `rust/crates/krkr-psb` |
| C 头 | `bridge/include/krkr/psb.h` |
| 消费者 | `motionplayer` → `PSB::PSBFile` |

通用规则见 [../ffi.md](2026-11-03-rust-ffi.md)、[../dependencies.md](2026-11-03-rust-dependencies.md)、[../build.md](2026-11-03-rust-build.md)。

---

## 1. 架构位置

```text
cpp/plugins/psbfile/
        │  #include <krkr/psb.h>
        ▼
bridge/include/krkr/psb.h
        ▼
rust/crates/krkr-psb/
```

---

## 2. 职责拆分

| 保留 C++ | 迁到 Rust |
|----------|-----------|
| ncbind、`PSBMedia`、`PSBFile` 壳 | 解析、解压、chunk 表 |
| `types/*`（二期） | zlib、RL、Emote LZ4 流 |
| stream → `vector<uint8_t>` | seed、decrypt |

---

## 3. C API

符号前缀 **`krkr_psb_`**。

```c
KrkrPsbDoc* krkr_psb_open_memory(const uint8_t* data, size_t len, int32_t seed);
void krkr_psb_close(KrkrPsbDoc* doc);
bool krkr_psb_read_chunk(KrkrPsbDoc* doc, int32_t chunk_id, uint8_t* buf, size_t len);
```

---

## 4. CMake

```cmake
krkr_rust_crate(NAME psb CRATE krkr-psb)
target_link_libraries(psbfile PRIVATE Krkr::Rust::psb)
```

---

## 5. 迁移阶段

| 阶段 | 内容 | 验收 |
|------|------|------|
| P0 | workspace + `KrkrRust.cmake` + 空 crate | 全平台链接 |
| P1 | 头 + chunk 表 | `psbfile-dll` |
| P2 | offset API | `motionplayer-dll` |
| P3 | 解压 | 字节 diff |
| P4 | 删 C++ 解析 | NEKOPARA 夹具 |
| P5 | CLI、types | JSON diff |

---

## 6. 测试

| 层级 | 位置 |
|------|------|
| Rust | `rust/crates/krkr-psb/tests/` |
| C++ | `tests/unit-tests/plugins/psbfile-dll.cpp` |
| 集成 | `motionplayer-dll` |
