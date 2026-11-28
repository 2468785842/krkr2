# Android SDL3 宿主

Android 应用使用原生 Material 3 启动器和设置页，游戏由独立进程中的 SDL3 Activity
运行。SDL3 使用 OpenGL ES 上传引擎的软件图层和视频纹理。SDL 的 Android
Surface、触摸、物理键盘、IME 和手柄事件接入共享宿主。

## 启动游戏

- **导入游戏目录**通过系统文件选择器复制目录内容。含有 `startup.tjs` 或
  `data.xp3` 的目录可以启动。同目录里的补丁、散装资源和存档会一起复制。
- **导入游戏归档**将选中的单文件存为独立游戏目录中的 `data.xp3`。依赖同目录
  文件的游戏使用目录导入。引擎在启动时验证归档及根 `startup.tjs`。
- **打开本地路径**接受应用可读的绝对路径，也保留上次输入。导入的游戏保存于
  应用内部 `files/games/`，副本与来源目录各自保存后续改动。
- 游戏中按系统返回键遵循脚本的关闭询问。后台切换取消活动输入，由共享宿主营
  理游戏帧和音量。Android SDL3 支持一个游戏窗口。

## 设置与命令行

设置页通过 JNI 读取 [桌面 CLI](desktop-cli.md) `--list-settings` 的同一份键和值域，
使用 `--set KEY=VALUE` 的原生校验。布尔项显示开关，枚举显示选项列表，数值、
路径与语言可编辑。兼容设置保留旧的 OpenGL、日志控制台与菜单透明度键，
这些键在当前宿主中不生效。

全局设置保存于应用内部 `files/.preference/GlobalPreference.json`，保留未知键、
键位和已有自定义参数字段。有旧 XML 且没有 JSON 时支持读取旧配置。游戏目录内的
`Kirikiroid2Preference.json` 优先于全局设置。设置页的「引擎参数」使用和桌面
`-- -name=value` 相同的解析规则，每行一个参数。以空格分隔的内容仍属于同一个值。
游戏进程使用与桌面共用的目标解析、归档校验和参数入口。

## 构建 APK

准备 JDK 17、Android SDK 35、Build Tools 35.0.0、NDK 28.0.13004108、
Android SDK CMake 3.31.1 和已 bootstrap 的 vcpkg。Linux 依赖包括 Python、Bison、
Yasm、NASM、Autoconf、Automake、Libtool 和 pkg-config。设置 `ANDROID_HOME` 与
`VCPKG_ROOT` 后在仓库根目录运行：

```sh
bash platforms/android/gradlew -p platforms/android :app:assembleDebug
# 仅构建一个 ABI，例如供 x86_64 模拟器使用。
bash platforms/android/gradlew -p platforms/android -Pkrkr2.androidAbis=x86_64 :app:assembleDebug
adb install -r platforms/android/app/build/outputs/apk/debug/app-debug.apk
```

默认 APK 包含 arm64-v8a 与 x86_64，最低 Android 9（API 28）。Gradle 校验 SDL
发布包的 SHA-512 并提取与原生库版本匹配的 Java 层，区域设置和默认字体直接从 APK
资源读取。GitHub Actions 的 **Android runtime** 可分别构建并检查两个 ABI。

旧输入与存储实现参考仓库 `c494ea0` 中的
`platforms/android/app/java/org/tvp/kirikiri2/` 和
`cpp/core/environ/android/AndroidUtils.cpp`。启动图标沿用同一提交中的
`platforms/android/app/res/mipmap-*/ic_launcher.png`，版权与许可随本仓库分发。
