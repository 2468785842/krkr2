# Android SDL3 宿主构建验证

Status: implemented

基线为 `codex/remove-cocos-desktop-only` 的 `1d61099`。Android 使用原生 Material 3
启动器和独立 SDL3 进程，与桌面共用启动解析及设置清单。SAF 目录导入保留游戏文件
布局，归档及补丁仍由正式引擎读取。具体操作见 [Android SDL3 宿主](../../../../docs/subsystems/android-sdl3.md)。

- x86_64 完整依赖安装、CMake Release 编译与 Gradle `:app:assembleDebug` 通过。
  Gradle 复用本机已安装的同一份 vcpkg manifest 依赖，原生 Debug 编译使用 `-O1 -g0`。
  `scripts/check_android_runtime_apk.py` 验证四个原生库的 ABI、ELF 依赖闭包、JNI 入口和
  APK 资源，`apksigner verify` 验证签名。四个原生库的 LOAD 段均为 16 KiB 对齐。
  产物在 `out/android/artifacts/`，SHA-256 为
  `be8ff43e2775ffea3bfa1632305f145198d0290a599162ad9635e857c4a13eaf`。
- arm64-v8a `:app:assembleDebug` 通过。为节省本机磁盘空间，使用仅构建 Release
  依赖的本地 triplet，应用原生 Debug 编译仍为 `-O1 -g0`。APK 签名、ABI、四个原生库的
  ELF 依赖和 16 KiB LOAD 对齐、JNI 入口及资源检查均通过。SHA-256 为
  `c1468f25ebcce5d110b62e36bfec617449d5a078adee960b4f351a49b3776855`。
- Linux 主机 `launch` 范围 11 项、`platform` 范围 52 项通过。
  `python -m unittest discover -s scripts/tests -v` 通过 10 项。
  基础模块依赖和生产源码检查通过。
- 用桌面 JDK 加载实际设置 JNI 源码构建的共享库，验证默认值、旧 XML 读取、原键位及
  自定义字段保留、JSON 写回、无效值拒绝和补充平面 Unicode 路径。
- x86_64 工具准备中出现过缺少 Yasm、Bison 找不到 M4、Gradle 缺少 JDK 的错误。
  使用仓库 `out/` 中的工具补齐后重建。JNI、配置、构建和包校验日志位于
  `out/android-tooling/`。

`adb devices -l` 没有已连接的设备。本轮未验证实际游戏、Android 触摸、IME、
后台恢复或设备窗口呈现。旧输入桥的设备结果属于 Git 基线 `c494ea0`。
