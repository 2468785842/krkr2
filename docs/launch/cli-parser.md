# CliParser（argparse）

[← 索引](README.md)

---

## 1. 依赖

| 项 | 说明 |
|----|------|
| 库 | [argparse](https://github.com/peter-winter/argparse)（vcpkg，`krkr2` manifest 已声明） |
| 参考实现 | `tools/xp3/main.cpp` |
| 平台 | Desktop：`linux` / `windows` / `osx` |
| Mobile | **不使用** CliParser；见 [adapter.md](adapter.md) Intent 路径 |

---

## 2. 职责边界

**CliParser 只解析外壳参数**，解析完交给 `LaunchOptionsAdapter::apply()`。

| CliParser 管 | CliParser 不管 |
|--------------|----------------|
| `--xp3` `--config` `--help` `--version` | 引擎内部 `-fsbpp` 等语义解释 |
| `--` 后参数收集为 `engineArgs` | 写入 `TVPProgramArguments`（交给 Adapter） |
| 非法参数、缺参 | `TVPGetCommandLine` 查询 |

---

## 3. 命令行界面（草案）

```text
krkr2 [OPTIONS] [-- ENGINE_ARGS...]

Options:
  --xp3 PATH        XP3 文件或含 startup.tjs 的项目目录
  --config PATH     启动配置文件（launch.json）
  --print-data-path 打印数据目录并退出
  -h, --help        显示帮助
  -V, --version     显示版本

ENGINE_ARGS:
  `--` 之后原样传给吉里吉里引擎，例如 -debug=yes -contfreq=60
  若省略 `--`，未识别的 -xxx 参数也可按 engineArgs 收集（实现二选一，推荐显式 `--`）
```

### 示例

```bash
# 直接启动
krkr2 --xp3 /games/foo.xp3

# 使用 launcher 生成的配置
krkr2 --config ~/.config/krkr2/launch.json

# 启动并传引擎参数
krkr2 --xp3 /games/foo.xp3 -- -debug=yes -contfreq=60

# 仅传引擎参数（需已有默认项目路径或后续扩展）
krkr2 -- -startup=boot.tjs
```

---

## 4. LaunchOptions 结构

```cpp
#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace krkr::launch {

struct LaunchOptions {
    std::optional<std::filesystem::path> xp3;
    std::optional<std::filesystem::path> config;
    std::vector<std::string> engineArgs;

    bool printHelp{false};
    bool printVersion{false};
    bool printDataPath{false};
};

} // namespace krkr::launch
```

---

## 5. CliParser API（草案）

```cpp
namespace krkr::launch {

// 解析失败抛出 std::runtime_error 或返回 expected
LaunchOptions parseCommandLine(int argc, char** argv);

// 仅生成 LaunchOptions，不访问 TVP / Config
} // namespace krkr::launch
```

### argparse 骨架

```cpp
#include <argparse/argparse.hpp>

LaunchOptions parseCommandLine(int argc, char** argv) {
    argparse::ArgumentParser program("krkr2", KRKR2_VERSION);

    program.add_argument("--xp3")
        .help("XP3 file or project directory");

    program.add_argument("--config")
        .help("Launch profile JSON");

    program.add_argument("--print-data-path")
        .flag()
        .help("Print data directory and exit");

    program.add_argument("engine_args")
        .remaining()
        .help("Kirikiri engine arguments after --");

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n' << program;
        std::exit(2);
    }

    LaunchOptions opts;
    if (program.is_used("--xp3"))
        opts.xp3 = program.get<std::string>("--xp3");
    if (program.is_used("--config"))
        opts.config = program.get<std::string>("--config");
    opts.printDataPath = program.get<bool>("--print-data-path");
    opts.engineArgs = program.get<std::vector<std::string>>("engine_args");
    return opts;
}
```

> 实际实现需处理：`--help` / `--version` 与 argparse 默认行为、Windows UTF-8。

---

## 6. 平台入口模板

```cpp
// platforms/linux/main.cpp（示意）
int main(int argc, char** argv) {
    using namespace krkr::launch;

    LaunchOptions opts;
    try {
        opts = parseCommandLine(argc, argv);
    } catch (...) {
        return 2;
    }

    if (opts.printHelp) { /* print and return 0 */ }
    if (opts.printVersion) { /* print and return 0 */ }

    LaunchOptionsAdapter::apply(opts);

    if (LaunchContext::hasStartupTarget()) {
        /* 无 UI：直接进引擎 */
    }

    gtk_init(&argc, &argv);
    /* ... AppDelegate ... */
}
```

---

## 7. 与现有 main 的差异

| 平台 | 现状 | 目标 |
|------|------|------|
| Windows | `CommandLineToArgvW` + `MainFileSelectorForm::filePath` | `parseCommandLine` → `apply` |
| Linux | `argv[1]` → `filePath` | 同上 |
| macOS | `argv[1]` → `filePath` | 同上 |
| Android | 无 CLI | `LaunchOptionsFromIntent` → `apply` |

---

## 8. 错误码约定

| 码 | 含义 |
|----|------|
| 0 | 正常启动或 `--help` / `--version` 成功 |
| 1 | 运行时错误（引擎） |
| 2 | CLI 解析错误、缺少 `--xp3` 且无 config |

方便外部 launcher 脚本判断。

---

## 9. 测试建议

| 类型 | 内容 |
|------|------|
| 单元测试 | `parseCommandLine` 各组合（Catch2，不链 TVP） |
| 集成测试 | `apply` 后 `TVPGetCommandLine(L"-debug")` 为真 |
| 快照 | `--help` 输出文本 |
