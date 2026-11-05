# Android 提交文本与按键桥

## 当前链路

`DummyEdit` 的硬件监听与 IME 的 `SDLInputConnection.sendKeyEvent` 共用 `handleKeyEvent`。
类名为历史命名，不依赖 SDL 后端。打印键由 `getUnicodeChar()` 得到完整码点，
按下时经 `Character.toChars` 提交一次，抬起不再提交；空格、ACTION_MULTIPLE 字符串走同一入口。
Ctrl/Meta 快捷键不当作字母提交；AltGr 的 Ctrl+Alt 组合仍允许字符查询。
死键先保存重音，再与后续字符组合；重复重音可提交重音字符，退格可取消尚未提交的重音。

IME `commitText` 只通知一次原生 sink，并保留 BaseInputConnection 的 editable/组合区域维护。
`setComposingText` 只更新 Android 的预编辑缓冲，最终 commit 才进入引擎；本桥不实现候选窗。
普通退格、deleteSurroundingText(1,0) 和 deleteSurroundingTextInCodePoints(1,0)
均走原生 DEL 的按下/抬起；后者按码点维护 Android 缓冲中的代理项。
其他范围删除仍由 BaseInputConnection 处理，没有新增任意游戏文本范围编辑协议。

`KR2Activity.nativeCommitText` 与 `nativeInsertText` 使用同一个
`platforms/android/cpp/AndroidTextInput.h::readCommittedText`：
读取 jstring 的显式 UTF-16 长度，以 RAII 释放 GetStringChars，再调用
`core_input::encodeTextInput` 得到标准 UTF-8。完整复制的字节串经既有 Android 事件队列进入引擎线程，
不保留 JNIEnv、jstring 或 JVM 字符指针到回调中；提交与原生键事件使用同一队列。

不能将 GetStringUTFChars 返回的 Modified UTF-8 交给严格 UTF-8 解码器。
补充平面字符必须编码成四字节 UTF-8；NUL 保留在显式长度内；未配对的代理项整段拒绝。
空串是有效转换，桥不为其排队；null 或转换失败同样不提交。
引擎端仍逐 UTF-16 单元产生 `onKeyPress`，零单元沿用窗口适配器的过滤规则。
原来把 Android KEYCODE 发送到 `onCharInput` 的额外接口已移除。
`newCursorPosition` 保留原 Java 接口，原生端仍未新增按此参数调整游戏光标的能力。

## 验证

共享编码器的 8 项编解码用例随 platform 范围运行；full 范围另与真实 Cocos 转换结果对照所有合法标量。
独立 Android 测试工程在 `tests/android-input/`，实际编译生产 SDLInputConnection、core_input 与 JNI 读取器，
使用 Android Framework 和 ART，不使用 Android 方法空实现或 JVM 桩替代。

准备 JDK 17、Android SDK 34、NDK 28.0.13004108、CMake 3.31.1 和已启动的 x86_64 模拟器后，
从仓库根目录执行：

```sh
bash platforms/android/gradlew -p platforms/android --no-daemon :app:compileDebugJavaWithJavac
bash platforms/android/gradlew -p tests/android-input --no-daemon connectedDebugAndroidTest
```

`.github/workflows/android-input-tests.yml` 使用 API 34 模拟器，并检查实际 XML 中发现的 14 项测试，
拒绝缺失报告、零测试、失败或跳过。报告与测试 APK 上传到 android-input-contracts artifact。
覆盖一次提交、Unicode、NUL、预编辑到提交、键码与文本分工、普通/码点退格、死键及 JNI 存储生命周期。

这是独立测试 APK 的输入桥契约；Java 全量编译不等于生产游戏 APK 的 NDK 链接通过。
它没有运行 Cocos/TJS 游戏、物理键盘、真实厂商输入法、ARM64 或移动 surface 生命周期。
游戏内最终回调、退出和焦点恢复仍需自制 TJS 场景及完整 APK 独立验收。
