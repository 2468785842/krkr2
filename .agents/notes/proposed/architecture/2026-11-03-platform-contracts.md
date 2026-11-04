# B2 窗口之后的输入、主循环与生命周期契约

Status: proposed

## 问题与当前边界

窗口首批已提取 `visual/WindowLayer.h`，Cocos 窗口实现仍在 environ；
视频通过专用适配器借用 Cocos 节点。当前 dirty 实现和已验证范围见
[窗口接口](../../../../docs/subsystems/window-interface.md)。
这只完成 B2 的窗口接口子批次，输入、主循环及生命周期仍与现有宿主相关。
SDL3 已选作后续平台壳，尚未完成迁移，见[选型](2026-11-03-ui-platform-shell.md)。

## 建议与下一步

已完成[初始化测试链接闭包](../../implemented/testing/2026-11-03-ffmpeg-test-link-closure.md)修复，
继续保留[访问冲突](../bug-fix/2026-11-03-ui-contract-access-violation.md)的诊断能力。
随后从 `TVPWindow.*`、`WindowImpl.*`、`EventIntf.*` 和 `MainScene.*`
列出事件生产者、排队、脚本分发及销毁回调的调用链，确定下一项最小可独立验证的接口。
输入批次先定义坐标空间、DPI、按下/释放及焦点丢失顺序；
再处理主循环的绘制/模态/计时关系，最后处理暂停恢复与窗口销毁。

## 完成条件

每个接口在现有 Cocos 适配器上保持脚本可观察行为，有对应最小场景和验证证据，
通用头不引入新的 Cocos 类型。记录 Windows 实测及其他平台缺口。
基线建立后再按[后端路线](2026-11-03-backend-modernization.md)推进 SDL3 原型。

## 取舍与风险

直接在一次改动中换窗口、输入和渲染会使回归原因难以区分，采用分接口批次。
本提案尚未确定新的函数签名；先依据调用链和测试定义契约，避免为未来后端堆积空接口。

## 输入状态子批次调查

现有 `SystemImpl.cpp` 为读取异步按键而直接包含 Cocos 窗口实现头。
键盘、鼠标和手柄适配器共同维护 `TVPWindowLayer::_scancode[0x200]`：
低位表示当前按下，0x10 表示待查询的按下；释放保留未消费的按下位。
`getcurrent=true` 的查询也清除 0x10，这是现有脚本可见行为，不能顺手改成常见的独立边沿查询。
鼠标短点另有直接写入“已释放但待查询”的状态路径，重复 key-down 会重新置位按下标记。

已接入纯标准 C++ 的 `core_input` 状态模块，System 查询不再包含 `TVPWindow.h`。
六个状态契约测试通过，依赖检查扩至 11 个基础目标。
迁移前 `input-state-baseline-01` 完成一轮交互并正常退出，
opt-in 驱动在 Cocos EventDispatcher 投递分开的 A-key down/up，
真实 Window 回调分别记录 `input:key-down:65:0:1:0`、`input:key-up:65:0:0:0`。
基线应用 SHA256 为 `03253e42a8d0e7a00d0dee35cffee040462f07766ad3a571dc2a259a7a3b2995`。
接入后 `input-frame-migrated-01` 以相同脚本完成三轮交互与正常退出，
A-key 的两条记录与基线完全一致；应用 SHA256 为
`a0a7a510a751fa15ce93ce618ef6ca75ef39c70ebfc403054469c87b237e6824`。
CDB 附加轮退出 0，无访问冲突标记或 dump。full 配置实际注册、运行输入/帧钩子共 9 项，全部通过。
键码翻译、坐标/DPI 变换、TJS 输入事件类和排队/合并策略仍由现有路径承担。

主循环调查另发现 `RenderUtils.h` 的 post-update 回调存储带内部链接，
设置者在 OpenGL 渲染器、读取者在 MainScene 与模态绘制路径。
提取原定义的双编译单元实验实际得到调用 0 次（期望 1 次），
证据在 `out/frame-hook-validation/baseline.log`、`source.json`。
已将唯一存储移至 `event/PostUpdateEvent.cpp`，三个跨单元/替换/注销测试通过；
`RenderUtils.h` 不再包含 Cocos 或平台头，绘制与 swap 实现在 environ 的 cpp 中。
普通更新和模态绘制均经同一调用入口，保持原有相对顺序与计时公式。
上述三轮也覆盖此改动，实际检查文件/菜单截图；这不是原访问冲突的根因结论。

## 生命周期子批次

`environ/HostLifecycle.h` 提供不含 Cocos/TJS 类型的前台/后台通知。
Cocos AppDelegate 在通知引擎后再启停动画，保持原顺序；Application 中音量、混音锁、
compact 和 System 事件排队未改变，没有提前接线旧 `OnExit` 或更改销毁顺序。

`lifecycle-migrated-01` 启用 `--keyboard-probe --lifecycle-probe`，真实 System 回调依次记录
`lifecycle:deactivate`、`lifecycle:activate`，随后一轮完整交互及关闭退出 0，未强停。
应用 SHA256 为 `49bb28ee8aa874d105934ea72f8206f228bfbf7a62a76a4a2298c2864be4f003`。
该测试只发引擎通知，未停止 Cocos 泵，不证明 OS 挂起、移动端 surface 重建或恢复帧行为。
事件生产/分发之间仍按原实现合并同类激活事件，并检查当前激活状态，不能要求每次快速切换都产生回调。

## 剩余工作

指针坐标子批次已抽出 Y 轴翻转/向零截断和 Cocos Node 适配，修复光标缓存混入父节点坐标；
范围及验证见[指针契约](../../implemented/testing/2026-11-04-pointer-coordinates.md)。
独立 SDL3 原型已接入鼠标位置与视口换算、滚轮和共享按键状态，
Linux XTest 验证窗口外拖动、调整大小和失焦恢复，见
[鼠标子批次](../../implemented/testing/2026-11-04-sdl3-pointer-input.md)。
这尚未定义生产 Cocos 的 OS DPI/屏幕坐标到世界坐标转换，也不替代高 DPI 与物理设备验证。

修饰键快照已提到共享输入模块，导出实现移到 System 层，保持旧中键省略及标记不消费规则；
SDL 队列与 X11 的组合键释放/恢复验证见[子批次记录](../../implemented/testing/2026-11-03-shared-input-modifiers.md)。
这不改变 Cocos 激活与失焦路径，也不表示完成 IME 或物理设备验证。

独立 SDL3 原型已使用共享状态的 `releaseAll()`，验证先释放后通知失焦、重复通知、
失焦输入忽略及恢复后的新按键，Linux Xvfb/X11 七帧像素与反向场景通过，见
[失焦子批次](../../implemented/testing/2026-11-03-sdl3-focus-validation.md)。
另有 X11 切焦/XTest 键盘探针，验证了原型的系统事件路径并修复 OS 关闭绕过脚本否决的问题。
没有接线 Cocos；不能替代上一节要求的现有后端真实失焦证据。

本页仍为 proposed：OS 坐标/DPI/IME、焦点丢失释放、平台事件泵、完整暂停恢复和销毁路径尚未抽离。
桌面 Cocos 3.17.2 的 `cocos/platform/desktop/CCGLViewImpl-desktop.cpp` 将焦点通知
分发为 `EVENT_WINDOW_FOCUSED/UNFOCUSED`；最小化/恢复才调用 AppDelegate 的后台/前台入口。
当前 MainScene 未订阅这两个焦点事件，不能把已有 HostLifecycle 通知当作桌面切焦接入。
后续需分别验证普通切焦、最小化和触摸取消，避免为处理切焦直接停止动画泵或误发点击。
下一批应在实际失焦/后台与恢复场景建立证据，避免仅靠接口声明推断跨平台行为。
独立 SDL3/TJS 壳实验的可用范围另见 `tools/sdl3-prototype/README.md`，不作为 B2 全完成或 KAG 后端迁移验收。
