# Android 打印键与提交文本的编码边界

Status: proposed

## 源码事实

基于 `16f8468` 检查以下调用链，尚未在 Android 设备上执行：

- `SDLInputConnection.sendKeyEvent` 的打印键分支先将 `getUnicodeChar()` 强转 char 并 commitText，
  随后又把 `getKeyCode()` 传给 `KR2Activity.nativeCharInput`。
- JNI nativeCharInput 把这个 Android 键码传给 MainScene::onCharInput，后者强转为 tjs_char 发送 onKeyPress。
  它不是桌面 Unicode 标量入口，不能仅用补充平面字符到代理项转换来修正。
- nativeCommitText / nativeInsertText 使用 GetStringUTFChars，得到 JNI Modified UTF-8；
  补充平面字符以代理项编码表示，与主场景 UTF-8 严格解码契约不同。
  新旧严格解码均拒绝 UTF-8 代理项编码，本次共享模块提取没有改变该行为。

## 下一步与完成条件

先用 Android 最小 TJS 场景记录打印 A、中文、emoji、退格、硬件键与 IME commit 的真实回调，
核对提交与物理键的分工，避免文本重复或把 Android KEYCODE 当 Unicode。
再从 jstring UTF-16 正确复制/转换，保持 JNI 字符与队列对象生命周期；
消除重复文本后同时验证有效 BMP、代理项、空串、组合键、迟到输入、正常退出及现有游戏编辑控件。
Gradle/NDK 编译和设备交互须单独记录，桌面 VK_PACKET 或 C++ 解码测试不能替代。
IME preedit、候选窗和移动端 surface 恢复不自动包含在打印键修正中。

## 2026-11-05 编码与 Java 接线子批次

`b8069c8` 新增 core_input::encodeTextInput，将显式长度 UTF-16 编码为标准 UTF-8，
保留 NUL/补充平面字符、整段拒绝未配对代理项；两条 Android 提交入口共用 RAII JNI 读取器。
字符串在 JNI 调用返回前复制完毕，所有提交和按键经既有 Android 事件队列执行，
不再捕获 scene/scheduler 指针或在回调中保留 JVM 字符地址。
移除 nativeCharInput/onCharInput；硬件监听和 InputConnection 共用打印键处理，
只在按下时提交一次文本，支持空格、多字符提交、死键和退格路径；Ctrl/Meta 不生成字母提交。
`80b0f28` 补充重复死键和退格取消未提交重音，修正新 CI 的 SDK 包选择。

本地 34 项平台 CTest、44 项 Python、15 源闭包和 11 个 foundation 依赖守卫通过。
完整 Android Java 源由新 CI 的 :app:compileDebugJavaWithJavac 验证，
独立 tests/android-input 工程在真实 API 34 模拟器上运行生产 InputConnection 和 JNI 读取器，
不将普通 JVM 桩或纯 C++ 转换测试计作 Android Framework 通过。
实际 14 项 Framework/ART 测试全通过（无跳过），结果与运行身份见[本次验证记录](../../implemented/testing/2026-11-05-android-input-contracts.md)；稳定链路和命令见[Android 输入桥](../../../../docs/subsystems/android-input.md)。

完整 Cocos/TJS Debug APK 已在 x86_64 和 ARM64 完成 NDK 编译、链接及包内 ELF 依赖验收，
来源、失败轮次、哈希及限制见[完整运行时构建记录](../../implemented/testing/2026-11-05-android-runtime-build.md)。
此调查仍保持 proposed：最小 TJS 场景、现有游戏编辑控件、迟到输入归属、焦点/销毁、
ARM64 设备运行、物理设备和正常退出条件尚未完成。
下一步在最小 TJS 场景中检查 A/中文/emoji/退格的最终回调及生命周期。
