# KrKr2 Emulator

<p align="center">
  <a href="https://github.com/2468785842/krkr2/actions/workflows/ci.yml"><img src="https://github.com/2468785842/krkr2/actions/workflows/ci.yml/badge.svg?branch=main" alt="CI Status"></a>
  <a href="https://github.com/2468785842/krkr2/actions/workflows/code-format-check.yml"><img src="https://img.shields.io/github/actions/workflow/status/2468785842/krkr2/code-format-check.yml?branch=main&label=format-check&logo=githubactions" alt="Format Check"></a>
  <a href="https://github.com/2468785842/krkr2/actions/workflows/build-linux.yml"><img src="https://img.shields.io/github/actions/workflow/status/2468785842/krkr2/build-linux.yml?branch=main&label=linux&logo=githubactions" alt="Linux Build"></a>
  <a href="https://github.com/2468785842/krkr2/actions/workflows/build-windows.yml"><img src="https://img.shields.io/github/actions/workflow/status/2468785842/krkr2/build-windows.yml?branch=main&label=windows&logo=githubactions" alt="Windows Build"></a>
  <a href="https://github.com/2468785842/krkr2/actions/workflows/build-android.yml"><img src="https://img.shields.io/github/actions/workflow/status/2468785842/krkr2/build-android.yml?branch=main&label=android&logo=githubactions" alt="Android Build"></a>
</p>

This repository contains the **KrKr2 Emulator**, a cross-platform emulator designed to run games made with the **KiriKiri engine** (also known as T Visual Presenter).  
It supports **Android, Windows, Linux, and MacOS**, allowing users to play KiriKiri engine games on multiple platforms.  

**语言 / Language**: [中文](README_CN.md) | English

---

## KrKr2 Emulator

### Table of Contents

- [KrKr2 Emulator](#krkr2-emulator)
  - [Supported Platforms](#supported-platforms)
  - [Build Tools](#build-tools)
  - [Build Environment Setup](#build-environment-setup)
    - [Environment Variables](#environment-variables)
    - [Build Steps](#build-steps)
  - [Executable Location](#executable-location)
  - [Code Formatting](#code-formatting)
  - [Supported Games](#supported-games)
  - [Plugin Resources](#plugin-resources)
  - [License](#license)

---

## Supported Platforms

- **Android**:
  - `arm64-v8a`
  - `x86_64`
- **Windows**:
  - x86_64
- **Linux**:
  - x86_64
- **MacOS**:
  - arm64

---

## Build Tools

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
  - `YASM@latest`
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
  - `YASM@latest`
- **MacOS**:
  - Xcode
  - `vcpkg@latest`
  - [ninja@latest](https://github.com/ninja-build/ninja/releases)
  - [cmake@3.31.1+](https://cmake.org/download/)
  - `bison@3.8.2+`
  - `python3`
  - `NASM@latest`
  - `YASM@latest`

---

## Build Environment Setup

### Environment Variables

- **Android**:
  - `VCPKG_ROOT=/path/to/vcpkg`
  - `ANDROID_SDK=/path/to/androidsdk`
  - `ANDROID_NDK=/path/to/androidndk`
- **Windows**:
  - `VCPKG_ROOT=D:/vcpkg`
  - Add `winflexbison` path to `PATH`.
- **Linux / MacOS**:
  - `VCPKG_ROOT=/path/to/vcpkg`

> **Note**: On Windows, use `/` or `\\` instead of a single `\` in paths.

---

### Build Steps

- **Android**:
  ```bash
  ./platforms/android/gradlew -p ./platforms/android assembleDebug
  ```

  > If you encounter `glib` installation issues, see [FAQ#安装glib失败](./docs/FAQ.md#安装glib失败)

* **Windows**:

  ```powershell
  ./scripts/build-windows.bat
  ```

* **Linux**:

  ```bash
  ./scripts/build-linux.sh
  ```

* **MacOS**:

  ```bash
  cmake --preset="MacOS Debug Config"
  cmake --build --preset="MacOS Debug Build"
  ```

---

## Executable Location

* **Android**:
  * Debug: `platforms/android/out/android/app/outputs/apk/debug/*.apk`
  * Release: `platforms/android/out/android/app/outputs/apk/release/*.apk`
* **Windows**: `out/windows/debug/bin/krkr2/krkr2.exe`
* **Linux**: `out/linux/debug/bin/krkr2/krkr2`
* **MacOS**: `out/macos/debug/bin/krkr2/krkr2.app`

---

## Code Formatting
- **clang-format@20**
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


---

## Supported Games

* See [games list](./docs/support_games.txt)

---

## Plugin Resources

* Available at [wamsoft GitHub repositories](https://github.com/orgs/wamsoft/repositories?type=all)

---

## License

MIT License. See [LICENSE](./LICENSE) for details.

---
