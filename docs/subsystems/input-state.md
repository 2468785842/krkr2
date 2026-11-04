# 共享输入状态与坐标

`cpp/core/input/InputState.h` 的 `KeyState` 只依赖 C++17 标准库，
`core_input` 无引擎或图形后端链接依赖。键盘、鼠标与手柄适配器通过
`sharedState()` 使用同一份进程内状态；访问仍在引擎输入线程，不提供并发访问保证。

## 保持的脚本行为

状态覆盖虚拟键码 `[0, 512)`，越界查询为 false，越界写入忽略。
按下同时设置当前按下及待查询标记；重复按下重新设置待查询标记。
释放保留未查询的按下标记，返回释放前是否处于按下状态。
鼠标短点路径可直接设置“已释放但仍待查询”的状态。
Cocos 最后一个触摸取消时释放实际按下的对应鼠标键，再采样抬起事件的修饰位，不生成点击；
尚未转成鼠标按下的手势不补发抬起。

`System.getKeyState(key, current)` 经 `SystemImpl.cpp` 查询该模块。
每次查询都会消耗待查询标记，包括 `current=true`；此时返回当前按下状态。
`current=false` 返回本次查询前是否有待查询的按下。
修饰键组合使用不消耗标记的 `isDown()`。
这些行为保持原 `TVPWindowLayer::_scancode` 的实现，未改成独立的边沿查询。

`releaseAll()` 先释放所有当前按下的键，再按虚拟键码升序返回这些键，
供宿主发送对应的抬起事件。与单键释放相同，保留尚未查询的按下标记和已释放的鼠标短点；
重复调用返回空集合。适配器负责在这些释放回调之后通知失焦，恢复焦点不会自动按回旧键。
独立 SDL3 原型和 Cocos 桌面焦点订阅使用此入口；移动端后台入口尚未接入批量释放。

`InputCancellation.h` 将已释放的 VK 分为鼠标与键盘抬起，合并物理鼠标与键盘模拟鼠标的重复按钮。
`mouseKeyButton()` 统一 Return/Space/Pad1 → 左键、Escape/Pad2 → 右键；
Cocos 的常规按下/抬起也使用该映射，修正了旧右键模拟误发左键按下的问题。
取消只产生抬起，未曾转为鼠标按下的模拟键不会凭空产生鼠标抬起。

## 触摸按钮与窗口切换

`TouchButton.h` 保存单个手势的按钮状态：一/二/三触点在实际按下前选择左/右/中键，
按下后锁定按钮，直到最后一个触点结束或取消；取消后迟到的抬起不重复分发。
直接触摸由每个 Window 保存状态，虚拟光标由 MainScene 保存；物理鼠标事件显式携带按钮，
不修改触摸选择。头文件中原先按编译单元复制的 `_mouseBtn` 已删除。

窗口切换先取消旧窗口指针输入并释放其捕获，再更换事件接收者；隐藏窗口也在变为不可见前执行，
不会把原手势延续到新窗口。
内部切窗/模式切换通过 `releasePointers()` 只释放左/右/中键，保留键盘、手柄与未读标记；
取消抬起仍携带当前键盘修饰位。OS 失焦继续使用 `releaseAll()`。
取消产生的鼠标抬起在输入队列中保留取消标记；分发时允许窗口已隐藏，
但仍要求原生窗口及脚本 Owner 存在。普通鼠标抬起继续要求窗口可见。
这是因为隐藏前排队不等于隐藏前分发；失效窗口仍通过原事件撤销及窗口有效性检查拒绝回调。
键盘模拟鼠标的迟到抬起，若当前窗口没有该按钮的模拟按下，也不生成点击。
虚拟鼠标模式与窗口管理界面切换也取消旧手势。窗口失效时立即清除原生指针并注销接收者；
延后的节点析构仅丢弃状态，不回调脚本，也不会重新选中已失效窗口。
虚拟光标保持最多两指的移动规则，直接触摸支持三指选中键；这里只定义按钮生命周期，不提供完整多设备仲裁。

## Cocos 桌面焦点

`environ/cocos2d/CocosInputFocus.h` 订阅 GLView 的 focused/unfocused 事件，
在场景销毁时移除自己的监听器。首次失焦先禁止游戏输入、释放全部共享 VK，
再由当前 `TVPWindowLayer::CancelInput` 清理手势、模拟按键与速度，并排队鼠标/键盘抬起。
这条取消路径不调用会产生点击的常规模拟键抬起，也不消费未读按下标记。
重复失焦不重复抬起；失焦期间忽略游戏键盘、鼠标、触摸、手柄与文本事件，恢复不自动按回旧键。
旧手势的迟到结束/取消与无对应按下的鼠标抬起被忽略，虚拟光标的长按任务会取消。

普通焦点通知不调用 HostLifecycle，不改变 System 激活通知、音量或动画泵；
最小化/恢复仍沿用 AppDelegate 的原有前后台入口。
初始状态沿用已聚焦假定，后续由 GLView 通知更新。
Windows 原生切焦探针已通过真实前台窗口切换与 A 键 SendInput 验证取消、失焦输入抑制和恢复；
触摸仍通过 GLView 注入。Windows 单宿主窗口另已通过原生最小化/恢复、
一秒脚本请求暂停、迟到抬起隔离和新输入检查，恢复前后的填充层中心 RGB 均为 `(48,64,80)`。
该场景使用已安装的英文布局，不代表 IME、音视频恢复或移动端 surface 验收。
多游戏窗口与系统切焦组合、其他平台焦点及最小化仍需分别验收，
用法见[原生焦点探针](../../tools/game-smoke/interaction/README.md#windows-原生焦点探针)。

## 修饰键快照

`InputModifiers.h` 的 `legacyShiftFlags(const KeyState&)` 读取当前按下状态，不消耗待查询标记。
位值保持原引擎约定：Shift=1、Alt=2、Ctrl=4、鼠标左键=8、右键=16。
原实现没有将中键加入快照，本接口保留该行为；双击和重复标记仍由事件路径提供。
`InputModifiers.h` 同时声明这六类键的标准虚拟键码，不依赖 Cocos、TJS 或系统头。

引擎导出 `TVPGetCurrentShiftKeyState()` 的实现已从 `MainScene.cpp` 移到
`base/impl/SystemImpl.cpp`，委托共享逻辑；公共声明及现有 Cocos 事件调用方不变。
编译期断言核对共享位值与引擎定义，Cocos 适配器也核对共享虚拟键码。

## 指针坐标

`PointerCoordinates.h` 提供不依赖后端的浮点位置、32 位整数位置、Y 轴翻转和整数转换。
坐标是连续位置，翻转使用 `height - y`；整数转换向零截断，保留负数及图层之外的拖动位置。
非有限值或超出 int32 范围时抛出 `std::out_of_range`，避免未定义的浮点转整数。
此处不执行裁剪、DPI 或 viewport 缩放。

Cocos 的 `environ/cocos2d/CocosPointerCoordinates.h` 先使用原 Node 世界到局部变换，
再调用共享规则；鼠标、滚轮、触摸及取消事件复用它，事件排队与坐标采样点不变。
脚本设置光标时，缓存保存主图层左上角坐标，光标节点位置单独换算到其父节点；
修复了旧 setter 把显示父节点坐标写入缓存、导致读回错位的问题。

平台独立契约现为 31 项，另有 3 项真实 Cocos 节点测试覆盖缩放/旋转/锚点、
负坐标捕获与不同父节点下的光标放置；后者随 Windows 完整应用构建定向运行。
独立 SDL3 原型另已验证鼠标事件、X11 窗口外拖动与调整大小，见[原型工具](../../tools/sdl3-prototype/README.md#鼠标与视口)。
OS 高 DPI、多显示器及物理设备仍需单独验收。

## 提交文本

`input/TextInput.h/.cpp` 以标准 C++17 将 UTF-8 提交转换为 TJS 使用的 UTF-16 单元，
不依赖 Cocos、OS 或 TJS 类型；`MainScene::onTextInput` 复用该入口。
保持逐 UTF-16 单元的 `Window.onKeyPress` 顺序：补充平面字符使用高/低代理项两次回调，
不进行 Unicode 规范化。解码保留嵌入 NUL，Cocos 窗口适配器沿用旧规则忽略零单元，
后续有效字符仍正常分发，不能把输入按 C 字符串在 NUL 处截断。
无效、截断、过长编码、代理项编码及超范围 UTF-8 整段拒绝，不发送有效前缀。
焦点检查和当前窗口有效性检查仍由适配器承担。

平台范围有 5 项解码测试；full 范围另用真实 Cocos 的旧转换函数对照所有合法标量与
10,000 组任意字节输入，不将纯解码测试计为 IME 或设备输入验收。
Windows 的 `text_repro.py` 通过原生 VK_PACKET 和组件 UTF-8 分发核对真实 TJS 回调，
用法见[提交文本探针](../../tools/game-smoke/interaction/README.md#windows-提交文本探针)。
这里不定义 IME preedit、候选窗、组合键，也没有改变 Android 的键码/JNI 接口。

Cocos 桌面字符回调的 GLFW 参数是 32 位码点；overlay port #12 修复了旧强转为 char16_t
导致补充平面字符截断的问题，改为 UTF-32 → UTF-8 后进入上述提交路径。
采用修复时需重建 Cocos 依赖；只重建应用并继续链接旧 port #11 不会获得该修复。

## 适配边界与验证

Cocos 仍承担键码转换、OS/窗口坐标与 DPI、IME、触摸判定及 TJS 事件排队；
共享模块处理虚拟状态、修饰位和图层局部坐标规则，尚未完成全部输入后端替换。
`tests/unit-tests/core/environ/InputStateTest.cpp` 的十项测试在 full/platform 范围登记，
覆盖标记消费、释放、重复、鼠标短点、键码边界和失焦时的批量释放。
`InputModifiersTest.cpp` 另覆盖全部 64 种键盘/鼠标组合、快照不消耗标记以及批量释放后的快照。
`InputCancellationTest.cpp` 的 3 项检查键/鼠标分类、模拟映射及 32 种物理/模拟按钮组合。
`TouchButtonTest.cpp` 另有 4 项，跨两个编译单元验证按钮实例隔离、手指数变化、冻结和取消。
`CocosInputFocusTest.cpp` 的 4 项经真实 EventDispatcher 检查释放顺序、重复/恢复、
无按下状态的手势取消及订阅销毁；不创建 OS/GL 窗口，不等于设备输入或真实 Window 回调验收。
独立构建命令见[构建指南](../guides/building.md#独立平台契约与-sdl3-原型)。

交互工具的 `--keyboard-probe` 在 Cocos EventDispatcher 投递分开的 A 键按下和抬起，
等待真实 Window 回调记录查询值；它不等同于操作系统键盘设备测试。
用法见[交互场景](../../tools/game-smoke/interaction/README.md)。
独立的 `pointer_repro.py` 经 GLView 分帧触摸操作核对真实 TJS Window 回调，
覆盖拖动取消、双指右键短点及隐藏/显示后的旧触点隔离；它不替代 OS 或物理设备验收。
`pointer_windows_repro.py` 另以两个真实 TJS Window 标记回调来源，检查切窗取消、
旧触点与新拖动交错，以及当前窗口隐藏/失效后的回退路由；同时检查局部坐标位于场景范围。
`BringToFront` 和隐藏后的回退会将目标节点恢复到原点，避免窗口仍停在屏幕外。
Windows 手动工作流的
`pointer_contracts=true` 可在显式驱动构建上运行这两个探针，说明及证据边界见
[交互场景](../../tools/game-smoke/interaction/README.md#跨帧触摸回调探针)。
