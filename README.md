# KrKr2 模拟器

KrKr2 模拟器是一款跨平台的模拟器，旨在运行使用吉里吉里引擎（也称为 T Visual Presenter）制作的游戏。该模拟器支持在 Android、Windows 和 Linux 等多个平台上运行，帮助用户在不同设备上体验吉里吉里引擎制作的游戏。

## 目录

- [KrKr2 模拟器](#krkr2-模拟器)
  - [目录](#目录)
  - [支持平台](#支持平台)
  - [依赖构建工具](#依赖构建工具)
  - [编译环境配置](#编译环境配置)
    - [环境变量](#环境变量)
    - [编译步骤](#编译步骤)
  - [可执行文件位置](#可执行文件位置)
  - [代码格式化](#代码格式化)
  - [支持的游戏](#支持的游戏列表)
  - [插件资源](#插件资源)
  - [贡献指南](#贡献指南)
  - [许可证](#许可证)

## 支持平台

- **Android**:
  - `arm64-v8a`
  - `x86_64`
- **Windows**:
  - Win32
- **Linux**:
  - x64
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
  - `NASM@latest`
- **Windows**:
  - [ninja@latest](https://github.com/ninja-build/ninja/releases)
  - `Visual Studio 2022`
  - `vcpkg@latest`
  - [cmake@3.31.1+](https://cmake.org/download/)
  - [winflexbison@2.5.25](https://github.com/lexxmark/winflexbison)
  - `python3`
  - `NASM@latest`
- **Linux**:
  - [ninja@latest](https://github.com/ninja-build/ninja/releases)
  - `GCC`
  - `vcpkg@latest`
  - [cmake@3.31.1+](https://cmake.org/download/)
  - `bison@3.8.2+`
  - `python3`
  - `NASM@latest`
  - `YASM`
- **MacOS**:
  - Xcode
  - [ninja@latest](https://github.com/ninja-build/ninja/releases)
  - `bison@3.8.2+`
  - `python3`
  - `NASM@latest`

## 编译环境配置

### 环境变量

请根据所使用的平台配置以下环境变量：

- **Android**:
  - `VCPKG_ROOT`: `/path/to/vcpkg`
  - `ANDROID_SDK`: `/path/to/androidsdk`
  - `ANDROID_NDK`: `/path/to/androidndk`
- **Windows**:
  - `VCPKG_ROOT`: `D:/vcpkg`（注意使用正斜杠 `/` 或双反斜杠 `\\`）
  - 将 `winflexbison` 的路径添加到 `PATH` 环境变量中。
- **Linux OR MacOS**:
  - `VCPKG_ROOT`: `/path/to/vcpkg`

> **注意**: 在 Windows 上，环境变量路径必须使用 `/` 或 `\\`，避免使用单一的 `\`。例如：
>
> - **错误示例**: `D:\vcpkg`（cmake 不转义 `\`，导致路径错误）
> - **正确示例**: `D:/vcpkg`

### 编译步骤

- **Android**:
  - 在 Windows 上运行: `./gradlew.bat assemble`
    - 如果遇到`glib`无法安装查看[FAQ#安装glib失败](./doc/FAQ.md#安装glib失败)
  - 在 Linux 上运行: `./gradlew assemble`
  
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
  
- **使用Docker容器**:
  - Android: `docker build -f dockers/android.Dockerfile -t android-builder .`
  - Linux: `docker build -f dockers/linux.Dockerfile -t linux-builder .`

## 可执行文件位置

- **Android**:
  - Debug 版本: `out/android/app/outputs/apk/debug/*.apk`
  - Release 版本: `out/android/app/outputs/apk/release/*.apk`
- **Windows**:
  - 可执行文件: `out/windows/debug/bin/krkr2/krkr2.exe`
- **Linux**:
  - 可执行文件: `out/linux/debug/bin/krkr2/krkr2`
- **MacOS**:
  - 可执行文件: `out/macos/debug/bin/krkr2/krkr2.app`

## 代码格式化

- **Linux**:
  - 使用 `clang-format` 进行代码格式化:
    ```bash
    clang-format -i --verbose $(find ./cpp ./linux ./windows ./android/cpp ./apple -regex ".+\.\(cpp\|cc\|h\|hpp\|inc\)")
    ```

- **MacOS**:
  - 使用 `clang-format` 进行代码格式化:
    ```bash
    clang-format -i --verbose $(find ./cpp ./linux ./windows ./android/cpp ./apple -name "*.cpp" -o -name "*.cc" -o -name "*.h" -o -name "*.hpp" -o -name "*.inc")
    ```

- **Windows**:
  - 使用 `clang-format` 进行代码格式化:
    ```powershell
    Get-ChildItem -Path ./cpp, ./linux, ./windows, ./android/cpp ./apple -Recurse -File | 
    Where-Object { $_.Name -match '\.(cpp|cc|h|hpp|inc)$' } | 
    ForEach-Object { clang-format -i --verbose $_.FullName }
    ```

## 支持的游戏列表
- [games](./doc/support_games.txt)

## 插件资源

您可以在 [wamsoft 的 GitHub 仓库](https://github.com/orgs/wamsoft/repositories?type=all) 中找到相关的插件和工具库。

## 贡献指南

欢迎对 KrKr2 模拟器项目提出建议、报告问题或提交代码贡献。请遵循以下步骤：

1. Fork 本仓库。
2. 创建一个新的分支：`git checkout -b feature-branch`
3. 提交您的更改：`git commit -m '添加新功能'`
4. 推送到分支：`git push origin feature-branch`
5. 创建一个 Pull Request。

## 许可证

此项目遵循 MIT 许可证。详细信息请参阅 [LICENSE](./LICENSE) 文件。
