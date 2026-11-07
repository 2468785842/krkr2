# KrKr2 模拟器

<p align="center">
  <a href="https://github.com/2468785842/krkr2/actions/workflows/ci.yml"><img src="https://github.com/2468785842/krkr2/actions/workflows/ci.yml/badge.svg?branch=main" alt="CI 状态"></a>
  <a href="https://github.com/2468785842/krkr2/actions/workflows/ci.yml"><img src="https://img.shields.io/github/actions/workflow/status/2468785842/krkr2/ci.yml?branch=main&label=format-check&logo=githubactions" alt="格式检查"></a>
  <a href="https://github.com/2468785842/krkr2/actions/workflows/ci.yml"><img src="https://img.shields.io/github/actions/workflow/status/2468785842/krkr2/ci.yml?branch=main&label=linux&logo=githubactions" alt="Linux 构建"></a>
  <a href="https://github.com/2468785842/krkr2/actions/workflows/ci.yml"><img src="https://img.shields.io/github/actions/workflow/status/2468785842/krkr2/ci.yml?branch=main&label=windows&logo=githubactions" alt="Windows 构建"></a>
  <a href="https://github.com/2468785842/krkr2/actions/workflows/ci.yml"><img src="https://img.shields.io/github/actions/workflow/status/2468785842/krkr2/ci.yml?branch=main&label=android&logo=githubactions" alt="Android 构建"></a>
</p>

KrKr2 模拟器是一款跨平台的模拟器，旨在运行使用吉里吉里引擎（也称为 T Visual Presenter）制作的游戏。该模拟器支持在 Android、Windows 和 Linux 等多个平台上运行，帮助用户在不同设备上体验吉里吉里引擎制作的游戏。

**语言 / Language**: 中文 | [English](README.md)

开发与 AI 协作入口：[开发约定](AGENTS.md) · [文档索引](docs/README.md) · [当前架构](docs/architecture.md) · [构建与验证](docs/guides/building.md) · [后端精简路线](.agents/notes/proposed/architecture/2026-11-03-backend-modernization.md)。

## 目录

- [KrKr2 模拟器](#krkr2-模拟器)
  - [目录](#目录)
  - [支持平台](#支持平台)
  - [依赖构建工具](#依赖构建工具)
  - [编译环境配置](#编译环境配置)
    - [环境变量](#环境变量)
    - [编译步骤](#编译步骤)
  - [可执行文件位置](#可执行文件位置)
  - [文本脚本回归测试](#文本脚本回归测试)
  - [代码格式化](#代码格式化)
  - [游戏验证范围](#游戏验证范围)
  - [插件资源](#插件资源)
  - [许可证](#许可证)

## 支持平台

- **Android**:
  - `arm64-v8a`
  - `x86_64`
- **Windows**:
  - x86_64
- **Linux**:
  - x86_64
- **MacOS**:
  - arm64

## 依赖构建工具

- **Android**:
  - [ninja@latest](https://github.com/ninja-build/ninja/releases)
  - [cmake@3.31.1+](https://cmake.org/download/)
  - [vcpkg@latest](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started)
  - [Android SDK@33](https://developer.android.com)
  - [Android NDK@28.0.13004108](https://developer.android.com/ndk/downloads)
  - [JDK@17](https://jdk.java.net/archive/)
  - `bison@3.8.2+`
  - `python3`
  - `YASM@latest`
- **Windows**:
  - [ninja@latest](https://github.com/ninja-build/ninja/releases)
  - `Visual Studio 2022`
  - `vcpkg@latest`
  - [cmake@3.31.1+](https://cmake.org/download/)
  - [winflexbison@2.5.25](https://github.com/lexxmark/winflexbison)
  - `python3`
  - `YASM@latest`
- **Linux**:
  - [ninja@latest](https://github.com/ninja-build/ninja/releases)
  - `GCC`
  - `vcpkg@latest`
  - [cmake@3.31.1+](https://cmake.org/download/)
  - `bison@3.8.2+`
  - `python3`
  - `YASM@latest`
- **MacOS**:
  - Xcode
  - `vcpkg@latest`
  - [ninja@latest](https://github.com/ninja-build/ninja/releases)
  - [cmake@3.31.1+](https://cmake.org/download/)
  - `bison@3.8.2+`
  - `python3`
  - `YASM@latest`

## 编译环境配置

### 环境变量

请根据所使用的平台配置以下环境变量：

- **Android**:
  - `VCPKG_ROOT`: `/path/to/vcpkg`
  - `ANDROID_HOME`: `/path/to/androidsdk`
  - `ANDROID_NDK`: `/path/to/androidsdk/ndk/28.0.13004108`
- **Windows**:
  - `VCPKG_ROOT`: `D:/vcpkg`（注意使用正斜杠 `/` 或双反斜杠 `\\`）
  - `bison`: 将 `winflexbison` 的路径添加到 `PATH` 环境变量中。
- **Linux OR MacOS**:
  - `VCPKG_ROOT`: `/path/to/vcpkg`

> **注意**: 在 Windows 上，环境变量路径必须使用 `/` 或 `\\`，避免使用单一的 `\`。例如：
>
> - **错误示例**: `D:\vcpkg`（cmake 不转义 `\`，导致路径错误）
> - **正确示例**: `D:/vcpkg`

### 编译步骤

- **Android**:
  - 运行: `./platforms/android/gradlew -p ./platforms/android assembleDebug`
> 旧版 `glib` 安装问题可参考[历史排错记录](.agents/notes/proposed/bug-fix/2026-11-03-android-glib-install-path.md#安装glib失败)，先核对当前工具版本与实际错误。
  
- **Windows**:
  - 运行: `./scripts/build-windows.bat`
  
- **Linux**:
  - 运行: `./scripts/build-linux.sh`

- **MacOS**:
  - 运行:
  ```
    cmake --preset="MacOS Debug Config"
    cmake --build --preset="MacOS Debug Build"
  ```

## 可执行文件位置

- **Android**:
  - Debug 版本: `platforms/android/out/android/app/outputs/apk/debug/*.apk`
  - Release 版本: `platforms/android/out/android/app/outputs/apk/release/*.apk`
- **Windows**:
  - 可执行文件: `out/windows/debug/bin/krkr2/krkr2.exe`
- **Linux**:
  - 可执行文件: `out/linux/debug/bin/krkr2/krkr2`
- **MacOS**:
  - 可执行文件: `out/macos/debug/bin/krkr2/krkr2.app`

## 文本脚本回归测试

`KRKR2_ENABLE_SCRIPT_SMOKE` 默认为 `OFF`，可在已有桌面构建中单独开启；
无需开启全部 `ENABLE_TESTS`。两项测试运行真实引擎和自动生成的脚本，检查 mode 0/1
文本读取、BOM 与读取边界，以及语法错误抛出异常、保留文件/行号诊断和错误后的继续执行。
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
  ctest --test-dir out/linux/release -R '^(textstream|parser-error)-runtime$' \
  --output-on-failure --no-tests=error
```

进程日志和 JSON 报告保存在构建目录 `script-smoke/`，每次运行使用独立子目录。
这些回归覆盖文本与编译错误处理，不代表全部游戏、音视频或存读档兼容性。

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
