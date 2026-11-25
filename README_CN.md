# KrKr2 模拟器

KrKr2 是 C++17 的吉里吉里运行环境。当前支持 Windows x64 和 Linux x64，使用 SDL3 窗口与软件呈现。
Android、macOS/iOS 暂停构建；Cocos 宿主、管理 UI、port 和构建配置已经删除。

## 构建

准备 CMake 3.28+、Ninja、Python、Bison 和 vcpkg。Windows 使用 MSVC x64 开发环境与 win_bison，
设置 `VCPKG_ROOT` 后运行：

```sh
cmake --preset "Windows Debug Config"
cmake --build --preset "Windows Debug Build"
ctest --test-dir out/windows/debug -N
ctest --test-dir out/windows/debug --output-on-failure --no-tests=error
```

Linux 使用 `Linux Release Config`，系统依赖与完整命令见[构建与验证](docs/guides/building.md)。
程序位于 `out/windows/debug/bin/krkr2/krkr2.exe`；Linux SDL3 位于 `out/linux/release/bin/krkr2/krkr2`。
通过 `--game /path/to/game` 启动，`--help`、`--list-settings` 查看命令和设置。

[开发约定](AGENTS.md) · [文档索引](docs/README.md) · [当前架构](docs/architecture.md) ·
[桌面运行时](docs/subsystems/native-runtime.md) · [Windows SDL3](docs/subsystems/windows-sdl3.md) · [English](README.md)

## 文本脚本回归测试

`KRKR2_ENABLE_SCRIPT_SMOKE` 默认为 `OFF`，可在已有桌面构建中单独开启；
无需开启全部 `ENABLE_TESTS`。三项测试运行真实引擎和自动生成的脚本，检查 mode 0/1
文本读取、BOM 与读取边界，以及语法错误抛出异常、保留文件/行号诊断和错误后的继续执行。
另检查未知渲染器的构造失败可被脚本捕获，并在退出时不重新初始化失败的渲染器。
所有测试素材均由脚本生成，不包含商业游戏文件。

文本格式对照本项目写入器及
[Kirikiroid2 的读取器](https://github.com/zeas2/Kirikiroid2/blob/d1c2b1259423542c893e0b65eaeb46c848848f2b/src/core/base/TextStream.cpp)：
`FE FE mode` 后跟 `FF FE`，UTF-16LE 正文从偏移 5 开始。修复沿用现有位变换，
没有整体移植其文本流实现。Linux 完成上述依赖配置并安装 `xvfb`、`xauth` 后运行：

```bash
cmake --preset="Linux Release Config" -DENABLE_TESTS=OFF -DBUILD_TOOLS=OFF \
  -DKRKR2_ENABLE_SCRIPT_SMOKE=ON
cmake --build --preset="Linux Release Build" --target krkr2
LIBGL_ALWAYS_SOFTWARE=1 ALSOFT_DRIVERS=null xvfb-run -a \
  ctest --test-dir out/linux/release -R '^(textstream|parser-error|renderer-error)-runtime$' \
  --output-on-failure --no-tests=error
```

进程日志和 JSON 报告保存在构建目录 `script-smoke/`，每次运行使用独立子目录。
这些回归覆盖文本、编译错误和渲染器构造失败后的退出，不代表全部游戏、音视频或存读档兼容性。

## 代码格式化
- `clang-format@20`
- **Linux**:
    ```bash
    clang-format -i --verbose $(find ./cpp ./platforms ./tests ./tools -regex ".+\.\(cpp\|cc\|h\|hpp\|inc\)")
    ```

- **MacOS**:
    ```bash
    clang-format -i --verbose $(find ./cpp ./platforms ./tests ./tools -name "*.cpp" -o -name "*.cc" -o -name "*.h" -o -name "*.hpp" -o -name "*.inc")
    ```

- **Windows**:
    ```powershell
    Get-ChildItem -Path ./cpp, ./platforms, ./tests, ./tools -Recurse -File | 
    Where-Object { $_.Name -match '\.(cpp|cc|h|hpp|inc)$' } | 
    ForEach-Object { clang-format -i --verbose $_.FullName }
    ```

## 游戏验证范围

见[本地样本与验证边界](docs/reference/game-compatibility.md)。

## 插件资源

您可以在 [wamsoft 的 GitHub 仓库](https://github.com/orgs/wamsoft/repositories?type=all) 中找到相关的插件和工具库。

## 许可证

此项目遵循 MIT 许可证。详细信息请参阅 [LICENSE](LICENSE) 文件。
