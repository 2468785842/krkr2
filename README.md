# KrKr2 Emulator

KrKr2 is a C++17 KiriKiri runtime. Current builds support Windows x64, Linux x64 and macOS arm64 using SDL3 and software rendering.
Android uses SDL3 and a native Material 3 launcher. iOS builds are temporarily disabled.
See the [Android SDL3 host guide](docs/subsystems/android-sdl3.md) for APK builds and settings.

Set `VCPKG_ROOT`, prepare CMake 3.28+, Ninja, Python and Bison, and use an MSVC x64 developer shell on Windows:

```sh
cmake --preset "Windows Debug Config"
cmake --build --preset "Windows Debug Build"
ctest --test-dir out/windows/debug --output-on-failure --no-tests=error
```

Linux uses `Linux Release Config`. Launch with `--game /path/to/game`; use `--help` or `--list-settings` for options.
See the [Chinese README](README_CN.md), [build guide](docs/guides/building.md) and [desktop runtime](docs/subsystems/native-runtime.md).

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

## Game Validation

See [local samples and validation limits](docs/reference/game-compatibility.md).

---

## Plugin Resources

* Available at [wamsoft GitHub repositories](https://github.com/orgs/wamsoft/repositories?type=all)

---

## License

MIT License. See [LICENSE](LICENSE) for details.

---
