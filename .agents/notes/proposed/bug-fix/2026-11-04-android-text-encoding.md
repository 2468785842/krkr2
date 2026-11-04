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
