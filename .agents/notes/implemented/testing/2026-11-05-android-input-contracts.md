# Android 输入桥的分层契约验证

Status: implemented

## 实现范围

`b8069c8` 引入独立 UTF-16 → 标准 UTF-8 编码器和 RAII jstring 读取器；
Java 打印键、硬件监听、空格、多字符提交、退格统一走 InputConnection，移除错误的 nativeCharInput/onCharInput。
`80b0f28` 补充重复死键和取消重音的退格语义。
当前链路、接口限制和命令见[Android 输入桥](../../../../docs/subsystems/android-input.md)。
这篇 implemented 只记录编码/桥契约子批次，整体游戏接入问题仍在
[原调查](../../proposed/bug-fix/2026-11-04-android-text-encoding.md)保留 proposed。

本机无 adb/javac，不伪造 Android 结果。Linux 复用核对后的独立 platform 配置：
本地 Catch2 v3.7.1、Debug、APP=OFF、TOOLS=OFF、TESTS=ON、scope=platform、测试驱动 OFF，
实际发现 34 项、15 个项目源，编译和 CTest 全部通过（约 0.06 秒）；
44 项 Python（约 8.31 秒）、11 个 foundation 目标守卫、改动范围 clang-format 20、actionlint 与 diff 检查通过。
本地 prefix 不能作为 vcpkg 安装闭包的证据；闭包由三平台 Headless CI 独立检查。
命令与日志保存在 `out/android-text-validation/`，构建目录 `out/linux/text-input/`。

## Android 初轮失败（保留）

1. 源 `b8069c8`，运行 `41943982458` / job `126971501106`：
   setup-android@v3 默认请求已不存在的 tools 包，Failed to find package 'tools'，退出 1。
   尚未编译 Java/NDK、没有执行设备测试。`80b0f28` 显式改为 platform-tools。
2. 源 `80b0f28`，运行 `41944175899` / job `126972140964`：
   完整生产 :app:compileDebugJavaWithJavac 成功，API 34 模拟器启动成功；
   独立工程的 Java 源目录设置误纳入无关 DialogMessage/KR2Activity 等，
   因缺少该工程有意不依赖的 Cocos/Material 类而出现 59 个 javac 错误，退出 1。
   configureCMake 已执行，但不能据此称 native 构建或 14 项测试通过。
   `749bae2` 在构建时同步精确的生产 SDLInputConnection 源文件到 generated 目录，
   并在 JavaCompile 前核对恰好两个实际源（该生产类和 NativeText 测试入口），
   不维护测试副本、不加宿主空实现；先构建测试 APK，再启动模拟器。

3. 源 `749bae2`，运行 `41944999640` / job `126974880723`：生产 Java、
   精确的独立 Java 源闭包和 NDK x86_64 JNI 库构建成功；AndroidTest 编译因缺少
   androidx.test.annotation.UiThreadTest 出现 15 个错误，测试未运行。
   已从固定版本 rules-1.6.1.aar 检查 classes.jar，确认该注解属于 androidx.test:rules，
   随后补全这个测试依赖，没有移除 UI 线程要求或跳过用例。

## Windows 回归

源 `b8069c8c47bcbbfb60d07cd490b5dbb6cf2df58a`，
[41943996172](https://github.com/2468785842/krkr2/actions/runs/41943996172) / job `126971553431` 成功。
完整 Release 应用、测试驱动 ON、cocos2dx #12、Mesa 软件 OpenGL；
11 项 Cocos CPU（含所有合法 Unicode 标量的旧转换/新编解码对照）、44 项 Python、
五个真实交互场景全部通过。Windows 结果不替代 Android JNI/键盘结果。

轻量 artifact `13377302965`（71,939 字节），ZIP SHA256
`97921680530d791d1911ee895b6165f48e0974601b51ed7603bfe4ee126e94b1`，
已下载到 `out/windows-ci/41943996172/`，五份报告均 passed=true、exit_code=0、forced_stop=false、failures=[]。
五份报告的 exe/PDB 哈希相同：
exe `69a9827edbd21aaa219dd9aaaf836bb0d1c82d5c0a51865727ab3a46512da49a`，
PDB `165e5dca59745d2c3a6f2fbb66964fd431a7589c06d47733749fae418f62fc2b`；
文本 fixture（CRLF）`5ce4440476274f5bf860084a431c9ac66fb0e9aa57e0dd88ff54034a4e6f1b1b`。
完整共享包 `13378251378` 为 127,185,111 字节，服务端 SHA256
`b7517edc7bb3dd3437c941866e193ea8c58b37301861e95919aec497eaae1107`。

## Android 最终验证

源 `24b586bc3f07f6831683386b94c9d27cb710e2fc`，
[41945414442](https://github.com/2468785842/krkr2/actions/runs/41945414442) / job `126976298417` completed/success。
使用 JDK 17、SDK/API 34、NDK 28.0.13004108、CMake 3.31.1，x86_64 Google APIs 模拟器。

生产 :app:compileDebugJavaWithJavac、独立 assembleDebug/assembleDebugAndroidTest、NDK JNI 库和
connectedDebugAndroidTest 全部成功。已下载并解析实际 XML：
`org.tvp.kirikiri2.InputContractTest`，tests=14、failures=0、errors=0、skipped=0，时间 0.818 秒，
device=emulator-5554 - 14。完整断言保留：一次提交、BMP/emoji/NUL/组合码点、ACTION_MULTIPLE、
打印键按下/抬起去重、硬件共用入口/空格、预编辑到提交、普通/码点退格、
Ctrl/Meta 不提交字母、死键/重复重音/退格取消、非法 UTF-16/null/空串以及 JNI 读取结果独立存储。
这些是实际 Android Framework/ART 运行，不是安卓方法桩，也不是完整游戏的 TJS 回调证据。

artifact `13378104101`，2,034,912 字节，ZIP SHA256
`0f05576e9f5a869f6385b8ee2afa2f844b8faca535c7e9b19dec11228f3dbc3d`，
已下载并核对到 `out/android-ci/41945414442/`。运行 APK 身份：

- 目标测试宿主 APK：1,333,355 字节，SHA256
  `2ffd7bef1f95dbf32e40ca3161d5d6f4a6aa7d57690f68b3abd7b5cda9d4dd9b`。
- instrumentation APK：4,519,435 字节，SHA256
  `739b17fa1814c7a41e85f6e6afadffe0811477dd6015fb13a5bfea8fa6997fe2`。
- 目标 APK 中 lib/x86_64/libandroid_text_contracts.so：SHA256
  `660dff85a61f83eb5f5511d78f4ba045a6b0d420e8624fd8e3469275d36a9007`。
- 随包 libc++_shared.so：SHA256
  `d6bceb7915ddb75ec4f96f97837efd80cf6084b00ad38e2666728f6cc24a9f68`。

`80b0f28` 的 Headless [41944175979](https://github.com/2468785842/krkr2/actions/runs/41944175979)
13/13 job 成功，三平台各 34 项、15 源与 Catch2-only 安装闭包通过。
后续 `c63969b` 为生产 Android JNI 应用目标补上 PRIVATE core_input 直接依赖：
core_environ 的 PRIVATE 链接不会给应用导出 TextInput/TouchButton 头路径。
已重新配置本地最小范围并检查模块守卫；这处应用目标接线仍须由完整 Android NDK 构建独立验收。


## 未覆盖

独立 APK 仅编译生产 InputConnection、共享 core_input 和 JNI 读取器。
完整应用的 Java 编译不代表完整游戏 APK 的 NDK/Cocos/TJS 链接完成；
没有验证 ARM64、物理设备、厂商 IME 候选 UI、游戏内编辑控件、迟到输入归属、移动 surface、游戏回调和退出。
