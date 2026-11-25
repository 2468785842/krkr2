# 窗口接口拆分（B2 首批）

> 2026-11-25 状态：当前仅构建 Windows/Linux SDL3，Cocos 宿主和专属测试已删除，Android/Apple 暂停。
> 下文中的 Cocos 适配过程和验收属于历史记录；当前运行入口见[桌面运行时](native-runtime.md)。

## 可选的 KAG 按钮方向导航

启动参数 `-controllerfocus=yes` 开启 `LayerManager` 的空间按钮导航，默认关闭。
没有 Shift/Ctrl/Alt 修饰的方向键按当前图层位置选择可见、启用且参与焦点链的
`ButtonLayer`，包括由脚本关闭 focusable 的鼠标按钮；选中时恢复其 focusable，
仍经现有 onFocus 处理高亮。Enter 在按下、抬起时均选中同一有效按钮才通过原鼠标
命中测试与 down/click/up 路径激活，避免只实现鼠标 onExecute 的按钮忽略键盘确认；
此时不再把 Enter 交给按钮键盘回调，避免重复激活。候选集合每次重算，过滤父级裁剪、
透明/隐藏/禁用节点及当前模态层之外的按钮。方向无候选时按屏幕阅读顺序循环。
焦点在其他独立控件上时保留原方向键行为，不抢占文本编辑。

这一兼容选项只依赖 KAG 通用 ButtonLayer 类型和图层状态，不包含游戏名、按钮名、
固定坐标或商业脚本。任意自绘菜单不一定遵循该协议，仍需游戏自身导航或鼠标输入。
`KRKR2_ENABLE_X11_INPUT_SMOKE` 同时注册 `x11-controller-focus-runtime`，
使用独立合成界面和真实 X11 按键覆盖默认关闭、反向创建顺序、隐藏/禁用/越界按钮、
文本控件、模态范围和 Enter 回调；不等于所有 KAG 游戏均已验证。

状态：从 Cocos 窗口实现中拆出通用接口，保留现有后端。
本批不替换窗口、输入、渲染、媒体后端或平台库。

## 边界

- `cpp/core/visual/WindowLayer.h` 承载 `iWindowLayer`、修饰键别名和方向常量，
  包含已有 `WindowIntf.h`，不包含 Cocos 或控制台 UI。
  消息接收器注册枚举移至 `WindowIntf.h`，枚举值保持不变。
- `environ/impl/TVPWindow.h` 保留 `TVPWindowLayer`、Cocos ScrollView、输入适配及窗口管理实现。
  `visual/impl/WindowImpl.cpp` 和 `BasicDrawDevice.cpp` 改为包含通用接口。
- `movie/VideoPresentation.h` 声明窗口视频呈现工厂。Cocos 实现位于
  `TVPWindow.cpp`，由宿主把视频节点挂到主图层区域并注册帧回调。
  工厂需要 Cocos 窗口，传入其他后端的窗口会抛出 `std::bad_cast`。
- 原接口中的空实现及既有限制原样迁移；本批不宣称补齐全屏、位置、触摸查询等功能。
  当前接口仍依赖 TJS/WindowIntf，并非完全独立的宿主 API。

`scripts/check_core_module_deps.py` 额外拒绝上述消费者重新包含 `TVPWindow.h`，
并检查通用头不直接暴露 Cocos 类型；该静态检查不能代替传递包含检查或全模块无环证明。

## 验证方法

真实交互使用自制 [窗口场景](../../tools/game-smoke/interaction/README.md)，
通过原脚本入口、原游戏菜单/文件选择与消息框、原关闭事件运行。
脚本请求文件仅负责打开对话框和发起 close；不能伪造用户点击或成功返回值。
控制器要求实际菜单事件、确认/取消返回值、选中文件名及关闭回调，空运行不能通过。

测量、排序补丁来源、截图与本机运行结果见 [窗口验证记录](../../.agents/notes/implemented/testing/2026-11-03-window-interface-validation.md)。

## Cocos UI 约束

`vcpkg/ports/cocos2dx/patch/fix-node-order.patch` 直接比较 localZOrder 与 orderOfArrival，避免打包排序键的字节序依赖；更新 port 后需要重建依赖。
共享 UI 按实际分配的行高限制文件名、标题与底栏字号，嵌入节点使用父级局部原点。
`tests/unit-tests/core/environ/NodeOrderTest.cpp` 在 full 范围验证层级与同层顺序，不创建 GL 窗口。
`CocosPointerCoordinatesTest.cpp` 另验证真实节点变换下的主图层坐标、越界位置及光标父节点坐标。
两者共 5 项，由完整 Windows 工作流实际注册、运行。
焦点订阅另有 4 项 EventDispatcher 用例，提交文本有 2 项真实 Cocos 转换对照。
另有 4 项 context 订阅寿命、2 项视频节点解绑与回调注销用例。17 项均无需创建 OS/GL 窗口。

`tools/game-smoke/pointer_windows_repro.py` 通过显式测试驱动创建真实双 Window，
检查切窗、隐藏和失效后的输入归属及局部坐标。正常切窗与隐藏回退会恢复目标 Cocos 节点的原点；
管理 overlay 的动画仍使用原入口。运行方法和软件 OpenGL 边界见[交互场景](../../tools/game-smoke/interaction/README.md#跨帧触摸回调探针)。

## 后续已拆分的边界

共享虚拟按键状态、修饰键快照及指针局部坐标规则迁至无后端依赖的 `core_input`，见[输入状态](input-state.md)。
`SystemImpl.cpp` 不再为查询状态包含具体 Cocos 窗口头。

`event/PostUpdateEvent.h/.cpp` 保存唯一的 post-update 回调，供普通帧和模态绘制共同调用。
原头文件内部链接变量会造成跨编译单元的注册不可见，已有双单元实验及三个回归用例。
`environ/HostFrame.h` 提供普通帧、嵌套消息处理和模态绘制三个入口，见[帧循环](#帧循环)。

`environ/HostLifecycle.h` 提供前台、后台引擎通知，不含 Cocos/TJS 类型。
Cocos AppDelegate 先通知引擎，再启停动画；引擎的音量、混音锁、compact 与 System 事件顺序未变。
连续激活事件沿用原队列合并与分发时状态核对，不保证快速切换的每一边沿都通知脚本。
此接口不拥有平台事件泵、GL context 或窗口销毁，不改变旧 `OnExit` 的接线与退出次序。

Cocos 桌面已订阅独立焦点事件并取消游戏输入，见[输入状态](input-state.md#cocos-桌面焦点)。
这些子批次未完成 DPI、IME、真实切焦交互、移动端挂起恢复或完整生命周期迁移。
具体证据与剩余工作见[B2 后续记录](../../.agents/notes/proposed/architecture/2026-11-03-platform-contracts.md)。

## 主线程消息

`event/MainThreadMessageQueue.h` 提供工作线程投递、引擎线程分发和按所有者取消消息的 C++ 接口。
Cocos 应用壳持有队列，`TVPProcessEngineMessages` 先分发一批消息，再推进脚本定时器。
分发按投递顺序取本批消息，回调在锁外执行。新消息由下一次分发处理。若回调显式嵌套处理消息，
嵌套调用会取得新消息，外层批次随后照常处理。回调抛出异常时，本批未执行消息随批次释放。

本地事件适配器将消息 ID 和事件值一起投递。关闭视频或注销接收器时，可以取消待执行消息，
包括已取出的批次里尚未开始的回调。接收器需在主线程销毁，并先停止生产线程。

构建、取消语义和桌面回归结果见[主线程消息实施记录](../../.agents/notes/implemented/simplification/2026-11-22-main-thread-message-queue.md)。


## 帧循环

Cocos 的 `MainScene::update` 调用 `TVPUpdateEngineFrame`。普通帧依次推进应用、回收纹理并调用帧钩子，
随后宿主处理 FPS 标签和场景绘制。应用更新调用同一个 `TVPProcessEngineMessages` 入口处理正常和嵌套消息。
事件层的嵌套入口直接调用该函数。

模态窗口、同步文件选择和启动日志共用 `TVPDrawSceneOnce` 的毫秒时钟。
`event/FrameStep.cpp` 以无符号差值处理 32 位回绕。正间隔返回距下一帧的等待量，
到期只绘制一帧，零和负间隔立即绘制。执行顺序为帧钩子、宿主绘制、交换缓冲。
绘制未成功返回时保留上一帧时间，后续调用可重试。平台输入轮询与休眠由各调用者安排。

模态文件选择、帧节流及跨平台构建结果见[宿主帧循环实施记录](../../.agents/notes/implemented/simplification/2026-11-22-host-frame-boundary.md)。

## Linux SDL3 窗口契约

SDL3 宿主使用正式的 `krkr2 --host=sdl3 --game PATH` 启动，沿用生产 `Window`、`Layer`
和 `System` 事件。每个 TJS 窗口有自己的 SDL 窗口和软件呈现纹理。

- 原生尺寸变化触发 `onResize`，画布保持宽高比。脚本 `setZoom(n, d)` 以主图层大小为基准调整原生客户区，
  并保留约分后的比例。位置、尺寸限制、边框、置顶、焦点开关和全屏请求使用 SDL 窗口 API。
- 鼠标坐标从原生窗口转换为主图层坐标。黑边中的新按下被忽略，已接受的拖动可在黑边和窗口外结束。
  鼠标按钮、滚轮、单击和双击使用现有 Window 回调。失焦时先清空虚拟按键和按钮状态，再投递释放回调。
- 当前焦点窗口管理 SDL 文本输入。图层的 IME 模式可覆盖窗口默认值，取消焦点或隐藏时暂停。
  已提交的 UTF-8 经共享输入模块解码成 TJS 的 UTF-16 单元。候选区锚点换算为窗口坐标。
- 模态窗口运行同一个 SDL 事件泵，并限制本应用其他游戏窗口的输入及关闭请求。脚本否决后仍继续收取事件，
  同意关闭后模态调用返回。
- 系统文件拖放按一次操作收集文件，完成后用 `onFileDrop(files)` 投递引擎存储路径数组。
- `Window.useMouseKey=true` 让方向键移动指针，Shift 加速，Enter 或空格操作左键，Esc 操作右键。
  手柄方向和 `VK_PAD1`、`VK_PAD2` 也可移动指针及操作左右键。关闭模式、失焦或拔掉最后一个
  按住模拟按钮的设备时取消按下动作。Wayland 的模拟指针在游戏画布内绘制。
- SDL 识别的手柄支持热插拔。方向键和左摇杆共用 `VK_PADLEFT` 至 `VK_PADDOWN`，摇杆及扳机在约半程
  触发、回到约 38% 行程时释放。面键、肩键、扳机和摇杆按压沿用 Cocos 的 `VK_PAD*` 映射。
  多个来源按住同一个虚拟键时只投递一次按下和一次释放。`System.getKeyState(VK_PADANY, true)`
  返回是否有已接收的手柄按键按住。窗口失焦后释放脚本状态，恢复焦点时需重新操作手柄。
- `Window.enableTouch=true` 开启触点事件、触点查询、速度和双指缩放及旋转。窗口通过同一逻辑画布
  坐标接收触点，黑边忽略新按下。SDL 未提供接触面积，回调的面积参数为 0。关闭触摸、隐藏或失焦时结束已登记的触点。

基础回归通过 XIM Compose 提交中文和补充平面字符。桌面回归使用 Openbox 和嵌套 Weston
检查全屏、退出全屏、最小化、恢复和焦点切换，并在 Weston 的 2× 输出验证画布像素与鼠标坐标。
Fcitx5 回归检查候选窗随两个图层输入锚点移动，并通过空格选择候选词。
KAG 回归通过场景内菜单操作，用两个独立游戏进程验证存档恢复剧情状态和文字。
X11 文件拖放来自 GTK 拖放源，键盘由 XTest 操作。触摸回归由显式测试构建的 SDL 事件驱动，
检查多指 ID、黑边、位置、手势和取消回调。
手柄回归通过 SDL 虚拟设备验证热插拔、失焦释放、重复来源、指针模拟和游戏重绘。
这组测试的 Weston 输出是虚拟显示，覆盖范围为整数缩放。
[构建与探针命令](desktop-cli.md#linux-sdl3-游戏宿主)
