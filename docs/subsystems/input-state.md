# 共享输入状态与坐标

`cpp/core/input/InputState.h` 的 `KeyState` 只依赖 C++17 标准库，
`core_input` 无引擎或图形后端链接依赖。键盘、鼠标与手柄适配器通过
`sharedState()` 使用同一份进程内状态；访问仍在引擎输入线程，不提供并发访问保证。

## 保持的脚本行为

状态覆盖虚拟键码 `[0, 512)`，越界查询为 false，越界写入忽略。
按下同时设置当前按下及待查询标记；重复按下重新设置待查询标记。
释放保留未查询的按下标记，返回释放前是否处于按下状态。
鼠标短点路径可直接设置“已释放但仍待查询”的状态。

`System.getKeyState(key, current)` 经 `SystemImpl.cpp` 查询该模块。
每次查询都会消耗待查询标记，包括 `current=true`；此时返回当前按下状态。
`current=false` 返回本次查询前是否有待查询的按下。
修饰键组合使用不消耗标记的 `isDown()`。
这些行为保持原 `TVPWindowLayer::_scancode` 的实现，未改成独立的边沿查询。

`releaseAll()` 先释放所有当前按下的键，再按虚拟键码升序返回这些键，
供宿主逐个发送 key-up。与单键释放相同，保留尚未查询的按下标记和已释放的鼠标短点；
重复调用返回空集合。适配器负责在这些释放回调之后通知失焦，恢复焦点不会自动按回旧键。
目前只有独立 SDL3 原型使用此入口，现有 Cocos 焦点路径尚未迁移。

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

平台独立契约现为 18 项，另有 3 项真实 Cocos 节点测试覆盖缩放/旋转/锚点、
负坐标捕获与不同父节点下的光标放置；后者随 Windows 完整应用构建定向运行。
OS 物理像素/DPI、实际设备输入及 SDL 鼠标事件接入仍需单独验收。

## 适配边界与验证

Cocos 键码转换、坐标与 DPI、IME、触摸判定、焦点变化以及 TJS 事件排队仍使用原路径；
本批次只迁移状态所有权，不能据此声称完成全部输入后端替换。
`tests/unit-tests/core/environ/InputStateTest.cpp` 的九项测试在 full/platform 范围登记，
覆盖标记消费、释放、重复、鼠标短点、键码边界和失焦时的批量释放。
`InputModifiersTest.cpp` 另覆盖全部 64 种键盘/鼠标组合、快照不消耗标记以及批量释放后的快照。
独立构建命令见[构建指南](../guides/building.md#独立平台契约与-sdl3-原型)。

交互工具的 `--keyboard-probe` 在 Cocos EventDispatcher 投递分开的 A 键按下和抬起，
等待真实 Window 回调记录查询值；它不等同于操作系统键盘设备测试。
用法见[交互场景](../../tools/game-smoke/interaction/README.md)。
