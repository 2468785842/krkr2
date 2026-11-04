# 触摸按钮归属、双编译单元与窗口切换取消

Status: implemented

## 基线复现

基线 `01a509b` 的 `TVPWindow.h` 含 `static tTVPMouseButton _mouseBtn;`，
MainScene.cpp 和 TVPWindow.cpp 分别写读自己的副本；首批修正为 `d648dff`。
改成单个全局变量仍会让多个窗口/设备互相覆盖。
从原头文件逐字提取声明，包含真实 `tvpinputdefs.h`，用两个编译单元分别设置右键和读取按钮：

```sh
c++ -std=c++17 -Icpp/core/visual -Icpp/core/tjs2 out/linux/touch-button-baseline-01/writer.cpp out/linux/touch-button-baseline-01/reader.cpp -o out/linux/touch-button-baseline-01/baseline
```

编译退出 0，实验程序退出 1，输出 `expected=1, observed=0`（TJS 枚举右键=1、左键=0）。
原头文件 SHA256 为 `11d70fa9e09a071ebb46a661e597e79606bcb60c3643f651ca955dc34ac0f749`。
声明、构建命令、日志及哈希保存在 `out/linux/touch-button-baseline-01/`。
这是对原声明的双单元实验，不是已执行的真实双指或 TJS Window 场景。

## 实现

- `core_input/TouchButton.h` 保存每个手势的按钮选择与实际按下状态。
  一/二/三触点在按下前选 VK 1/2/4；实际按下后新增手指不能改变配对的抬起。
  结束/取消幂等，未实际按下的延后点击不凭空补发鼠标抬起。
- Window 直接触摸各有 TouchButton 与开始时间；MainScene 的虚拟触摸、位置、开始时间、光标比例及移动状态归场景实例。
  虚拟光标仍只处理最多两指移动，三指移动不再使用默认原点。
- 删除头文件静态按钮；物理鼠标、虚拟鼠标的 down/up/click 显式传按钮。
  物理右/中键不会覆盖尚未结束的触摸选择；物理鼠标与滚轮只交给当前游戏窗口。
- `SetCurrentWindow` 在改变接收者前取消旧窗口输入，窗口管理界面、虚拟模式切换同样清理旧手势。
  迟到且不属于新手势的 move/end/cancel 被忽略；析构阶段仅丢弃状态，避免访问可能已失效的原生实例。
  本批选择“切换时取消”，不把旧拖动转交新窗口，也不新增物理按键的逐设备所有者跟踪。
- 复查发现 InvalidateClose 原先只移除节点；autorelease/外部引用可使节点晚于原生实例销毁。
  补充修正让 InvalidateClose 立即清除原生指针、撤销当前接收者，析构复用同一注销逻辑；
  后继选择和显式激活均拒绝已失效节点。此寿命路径依据代码与 Cocos 引用机制修正，尚无真实 TJS 场景复现。
- 原生 OnReleaseCapture 受 CanDeliverEvents 可见性检查约束；现在正常切换统一释放旧窗口捕获，
  隐藏窗口前先注销输入，避免先隐藏后排队抬起和释放捕获均被拒绝。失效/析构仍走无原生回调的丢弃路径。
- 手势自己的 held 标记参与取消分类，即使共享状态已被其他路径释放，也按原按钮补全配对；同按钮仍合并。

稳定规则见[共享输入](../../../../docs/subsystems/input-state.md#触摸按钮与窗口切换)。

## 本地验证

Linux GCC Debug，`out/linux/platform-focus`：app=OFF、tests=ON、scope=platform。

```sh
cmake --build out/linux/platform-focus --target input_state_tests frame_hook_tests -j 4
ctest --test-dir out/linux/platform-focus -N
ctest --test-dir out/linux/platform-focus --output-on-failure --no-tests=error
python scripts/check_core_module_deps.py
```

实际重新编译 TouchButtonTest.cpp、TouchButtonPeer.cpp 并链接，发现、运行 25 项，全部通过。
新增 4 项覆盖触点映射、按下后按钮锁定、重复按下/取消/迟到释放，以及跨单元的同实例可见性与两实例隔离。
最后一项同时防止旧静态副本和“改成一个共享全局按钮”的错误修正。
测试中的 1/2/4 是共享 VK，不是基线实验的 TJS 枚举值。
平台闭包预期 13 个项目源，无 TJS/Cocos 依赖；11 个 foundation 目标约束检查通过。
Cocos 生产路径本机未编译，完整 Windows 应用与既有 9 项 Cocos CPU 用例待推送后验证。
首批提交的[Headless CI](https://github.com/2468785842/krkr2/actions/runs/41775574846)全部 13 个 job 已通过，
包含三平台各 25 项及 13 个源/依赖闭包检查。
C++ 中间提交 `ee9a345` 的[完整 Windows 构建](https://github.com/2468785842/krkr2/actions/runs/41776743865)
job `126398753688` 成功：实际编译 MainScene.cpp / TVPWindow.cpp、链接应用与 cocos_window_tests，
发现并通过 9 项 Cocos CPU 测试（约 0.65 秒），两个渲染器编译范围及二进制上传通过。
中间提交 `68bfe8d` 的 Windows 与 Headless 亦均通过；未用它们替代上述 C++ 构建。
该 C++ 提交的 Headless 为 12/13：SDL 负向检查的 Xvfb 包装器清理失败覆盖验证器退出码，
报告明确已拒绝错误像素；CI 壳修正与复核见[SDL 记录](2026-11-03-sdl3-focus-validation.md#反向检查的显示服务退出码)。
工作流修正 `f462876` 的验证另行记录。

复查发现内部切窗复用 releaseAll 会意外清除仍按住的 Ctrl/Shift。最终拆分取消范围：
`releasePointers()` 只释放左/右/中键，保留键盘、手柄与未读标记；仅 OS 失焦使用 releaseAll。
窗口取消抬起使用不消费标记的当前修饰位快照。模拟鼠标的迟到键抬起，若当前窗口没有该按钮的模拟按下则忽略，
避免将旧窗口的 Enter/Space 抬起变成新窗口点击；没有新增逐键所有者追踪。
新增契约用例后，本地重新构建并实际发现、通过 26 项；13 个编译源及无 TJS/Cocos 依赖保持不变。
新用例检查所有指针均释放、Ctrl/Shift/Alt/普通键/Pad 状态与未读标记保留、重复释放幂等。
最终 C++ 提交 `bdbe78a` 的[完整 Windows 构建](https://github.com/2468785842/krkr2/actions/runs/41778850301)
job `126405482245` 成功：实际编译 InputState.cpp、MainScene.cpp、TVPWindow.cpp，应用与 cocos_window_tests 链接成功，
实际发现并通过全部 9 项（约 0.66 秒），两个渲染器范围检查和二进制上传通过。
[Headless 运行](https://github.com/2468785842/krkr2/actions/runs/41778825990)已通过三平台各 26 项、13 源闭包、
SDL 全部正向/反向检查及其他已结束范围。截至 2026-11-04 05:31 UTC 为 12/13；
Windows ImageOps 仍在配置依赖，尚无失败结论，不把整条工作流写成全通过。
收尾复核同一运行，Windows ImageOps 已成功，全部 13 个 job 均通过。
后续仅更新记录，无 C++ 或工作流改动。

## 未验证与下一步

### Windows 真实组件回调补验（2026-11-04）

基于 `73cb801` 的本地 dirty 改动，新增 `tools/game-smoke/pointer_repro.py` 和
GLView 跨帧触摸驱动，真实 TJS Window 记录 down/up/click，不直接调用脚本回调。
独立报告的通过范围是回调契约，不能替代文件/菜单完整回归或物理设备验证。

`pointer-contract-01` 复现隐藏窗口丢失取消抬起：拖动 down 后脚本设置 visible=false，
CancelInput 虽在隐藏前排队，实际分发时 OnMouseUp 的 CanDeliverEvents 拒绝隐藏窗口。
此前仅移动捕获释放时点不足以保证排队抬起到达脚本。
exe SHA256 `1bf355b3e918dd9defe3c022ced2ef013d358d76d756976b5e30116a4f76af1c`。
首版修正仍检查 GetFormEnabled，而 Cocos 此函数就是 isVisible，
所以 `pointer-contract-02` 同样超时失败（exe `c595d82cbce5bbeacca6bbd5dd94feb40564503f62a03baef86b5307511efa8f`）。
两轮均报告 `automation:TimeoutError: pointer:up:0`、forced_stop=true、进程退出 1；不能计为正常退出。

最终仅给 CancelInput 生成的鼠标抬起添加 cancellation 标记；WindowIntf 分发时单独检查
仍存在的 form 和 Owner，不要求可见。普通输入的门禁不变，Invalidate 仍撤销排队输入。
没有同步清空全局事件队列或新增重入脚本分发。

构建为 Debug、app/tests=ON、full、管理 UI=OFF、测试驱动=ON。
本机 CLion 同时在配置 debug，首次构建在重新配置阶段等待；停止的仅是本次自己的构建进程。
后续对 engine-config-check 使用 `-DVCPKG_MANIFEST_INSTALL=OFF` 复用已安装依赖；
配置及最终应用构建均退出 0，没有更改全局工具链。
命令及日志为 `out/windows/ffmpeg-link-closure/build-pointer.cmd`、
`pointer-probe-local-build.log`、`pointer-fix-final-build.log`。

```powershell
python tools/game-smoke/pointer_repro.py out/windows/engine-config-check/bin/krkr2/krkr2.exe out/ui-game-smoke/pointer-contract-03
python -m unittest discover -s tools/game-smoke
ctest --test-dir out/windows/engine-config-check -N -R '^cocos (window|pointer|focus)'
ctest --test-dir out/windows/engine-config-check --output-on-failure --no-tests=error -R '^cocos (window|pointer|focus)'
python scripts/check_core_module_deps.py
```

03 完成完整轨迹，passed=true、退出 0、未强停：拖动 cancel 不点击、双指短点右键 down/click/up、
隐藏后仍收到左键 up、显示后旧触点结束不点击，新手势 down/click/up 正常。
exe SHA256 `e378edc603033cb4b05b78726fecaa69897d60d56d75919ab89d57d7e0d2c14e`，
PDB SHA256 `7b0b24420e23aa65d0ac27e0954926f94f787df63ee8bb5764a7e0de618e4b5b`，
脚本 SHA256 `83e452ffb6c47b924f9dfe135cf016cbd2b1a8399e32003b4461853a77d5026b`。
Python 共 17 项通过（新增 4 项拒绝额外点击/抬起、错误按钮与不完整证据），
Cocos 实际发现并运行 9 项全通过，11 个基础目标依赖检查通过。

实际查看了 01 的起始截图及 03 的起始/结束截图：01 有蓝灰填充层，03 两张均黑底但宿主菜单图标可见。
03 的回调通过不代表渲染通过；尚未确定黑底的原因或与本批改动的关系，需单独检查场景绘制与截图时点。
同一 exe/脚本另以 `pointer-contract-04` 全新目录重复，回调再次 passed=true、退出 0、未强停；
实际查看其结束截图有预期蓝灰填充层。两轮画面不同，尚不能以 04 覆盖 03 的黑底，采集/绘制稳定性继续待查。
所有报告、逐步轨迹、截图及匹配二进制在 `out/ui-game-smoke/pointer-contract-01..04/`。

未实际执行触摸屏双指/三指、物理鼠标交错、多游戏窗口或原生 OS 切焦交互。
状态测试不运行真实 TJS Window 回调；完整应用编译不等于这些交互通过。
全局 VK 状态仍不做逐物理设备引用计数，同一个 VK 的混合设备仲裁未完成；
普通键盘/手柄仍沿用原有当前窗口路由，多个模拟键映射同一按钮时的逐键归属也未完成。
保留了原短点的 generic onClick 规则；右/中键触摸短点与物理按钮的点击事件是否应完全一致，需用真实 TJS 轨迹确认，
本批只修正按钮传递、配对与取消，不据纯状态测试改动该脚本事件规则。
后续先用原生或真实组件输入轨迹核对取消不点击、双指右键、窗口切换后旧触点不影响新窗口，
再按[平台契约](../../proposed/architecture/2026-11-03-platform-contracts.md)继续 DPI/IME 与完整生命周期工作。
