# Cocos 桌面焦点订阅与游戏输入取消

Status: implemented

## 问题与实现

实现提交 `1c7b877`（基于 `6d5a163`）。此前 GLView 的普通焦点事件没有接入游戏输入，
AppDelegate 只经最小化通知前后台；不能直接以后台入口替代普通切焦。
现有模拟鼠标的 InternalKeyUp 会生成点击，且 GetMouseButtonState 会消费待查询标记，
因此不能直接拿它补发取消抬起。

- `CocosInputFocus.h` 持有两个专用监听器；首次失焦先禁止输入并 releaseAll，
  再调用场景取消。重复失焦无副作用，恢复不重建旧按下；销毁仅移除自己的监听器并释放 dispatcher 引用。
- `InputCancellation.h/.cpp` 提供纯标准 C++ 的取消分类与 mouseKeyButton 映射。
  模拟鼠标与物理按钮合并，只有确实发过鼠标按下的模拟按钮补发抬起；其他 VK 保持释放顺序。
  Window 的常规按下、抬起共用同一映射，同时修正右键模拟原先误发左键按下的问题。
- 当前 Window 的 CancelInput 清理触摸、模拟鼠标标记、加速度与速度，直接排队抬起，
  不生成点击，也不通过 GetMouseButtonState 消费未读标记。
- MainScene 清理虚拟鼠标的触摸及长按任务；游戏键盘、鼠标、触摸、手柄、文本与鼠标键计时在失焦时停止输入。
  迟到的旧手势结束/取消、已释放鼠标的抬起被忽略。
- 普通焦点不调用 HostLifecycle，不改变 System 激活、音量或动画泵。移动端通知没有迁移。

稳定规则见[共享输入](../../../../docs/subsystems/input-state.md)。没有复制外部实现代码，
事件名来自仓库 pin 的 Cocos 3.17.2 GLView；窗口焦点与 iconify 的区别由其桌面实现核对。

## 本地验证

Linux GCC Debug，`out/linux/platform-focus` 缓存确认 app=OFF、tests=ON、scope=platform。

```sh
cmake --build out/linux/platform-focus --target input_state_tests frame_hook_tests -j 4
ctest --test-dir out/linux/platform-focus -N
ctest --test-dir out/linux/platform-focus --output-on-failure --no-tests=error
python scripts/check_core_module_deps.py
```

实际重新编译 InputCancellation.cpp / InputCancellationTest.cpp，发现并通过 21 项。
新增 3 项分别检查键/鼠标分类、5 种模拟键映射与普通键不被误分类、32 种物理/模拟按钮组合。
共享模块没有新增引擎或后端依赖，11 个 foundation 目标约束检查通过。
只对修改的 C++ 源/头执行 clang-format 20；工作流 YAML、链接和 diff 检查在推送前执行。

Cocos full 范围新增 4 项，使用真实 EventDispatcher 和 GLView 事件名，显式启用测试 dispatcher：
全部状态先释放再回调；重复失焦与恢复；没有 held VK 的延后手势仍取消；销毁订阅后不影响其他监听器。
这些测试在下述完整 Windows 构建实际运行，不将本地 21 项当作 Cocos 编译或交互证据。
Windows 工作流预期实际发现 9 项（节点 2、坐标 3、焦点 4）；三平台独立范围预期 21 项与 11 个项目源。
该提交的[Headless CI](https://github.com/2468785842/krkr2/actions/runs/41773291275)全部 13 个 job 已通过，
包含三平台各 21 项平台契约、11 个源与 Catch2 依赖闭包检查，以及既有 SDL3 队列/原生探针/像素回归。
同一提交的[完整 Windows 构建](https://github.com/2468785842/krkr2/actions/runs/41773298528)
job `126387751412` 已成功。Release、ENABLE_TESTS=ON、BUILD_TOOLS=OFF，
实际编译 InputCancellation.cpp、MainScene.cpp、TVPWindow.cpp、CocosInputFocusTest.cpp，
应用及 cocos_window_tests 成功链接；实际发现并通过全部 9 项（约 0.65 秒），其中焦点 4 项分别通过。
两个渲染器编译范围、元数据及应用二进制上传亦通过。

## Windows 原生焦点探针补验

接续 `69d0c37`；`4e0c432` 增加独立的 `tools/game-smoke/focus/startup.tjs`、
`focus_repro.py` 与 `win32_input.py`，不修改生产 C++。
Windows UI 测试驱动仍默认关闭，手动 `pointer_contracts=true` 模式同时运行第三个场景。

Win32 helper 创建自己的标准窗口，按子进程 PID 查找唯一可见引擎 HWND；
用 `SendInput` 点击标题栏，并实际检查 `GetForegroundWindow`。
A 键也通过 SendInput；触摸 begin/move/end 仍经 GLView，而不是物理设备。
没有直接分发 Cocos 的焦点通知、调用取消函数或伪造脚本回调。
标题栏点击不制造游戏点击；退出清理自己按下的键与辅助窗口。

场景检查 A 和拖动同时按住时切走：抬起回调查询 A/左键均为 false，取消不得点击；
失焦期间组件按键/触摸忽略；恢复后旧触点结束及旧 A-up 不产生新输入，
然后新原生 A 键和新 GLView 拖动正常。普通切焦不得产生 System 前后台回调。
先验证完整回调与新确认，最后要求正常退出、未强停；截图只来自 Cocos 帧缓冲。

```sh
python -m unittest discover -s tools/game-smoke -p 'test_*.py' -v
python -m py_compile tools/game-smoke/focus_repro.py tools/game-smoke/win32_input.py
/tmp/krkr-dev-tools/bin/actionlint -shellcheck= .github/workflows/build-windows.yml
git diff --check
```

Linux 本机 30 项 Python 检查全通过，其中新增 6 项只验证报告拒绝错误证据，未运行 Win32。
Windows 手动验收首次两次 dispatch 返回 502；确认没有运行后，以 workflow 文件名路径提交成功，
运行 `41813851043`，job `126519518400` 的应用、9 项 Cocos、30 项 Python、原有单/双窗口场景通过，
新原生焦点探针失败：`automation:TimeoutError: helper-unfocused`，进程退出 1、forced_stop=true。
真实脚本只收到 `focus:held:mouse-down:0:0:1` 与 `focus:held:key-down:65:1:1`，
未收到取消；不能计为原生切焦或恢复通过，也不足以判断生产焦点订阅有缺陷。
轻量 artifact `13314360583`，SHA256
`20a9cc894d399e4c33e6517dfcfef58a9ecc6fda93c71cebb34d3fe15e7f4889`，
已下载到 `out/windows-ci/41813851043/` 并核对报告、轨迹和进程日志。
失败轮 exe SHA256 `253f32b366cd82d86333665c45ac48da1ee7956f4aeaf9e9e289760073e87748`，
PDB `5995fd4eec1c9d4d928b008266096628a6f109356a3bfde9fb9242bc4c191177`，
fixture `21ebba88583564de9926218dda51582ba07b9e37bea6a272fca3460797a6f960`。

`327fd28` 补了发键前的前台归属检查与逐次键盘轨迹，其 Headless `41814333242` 全部 13 项通过。
手动应用运行 `41814353662` 在发现旧 helper 切焦失败后主动取消，不计为应用测试结果。
怀疑辅助窗口标题栏被前台引擎遮住；首轮没记录命中点，不能断言具体原因。
`ef2e260` 使自有 helper 保持 TOPMOST，改为从实际 client 原点计算非客户区点击点，
点击前通过 `WindowFromPoint` / `GetAncestor` 检查实际命中自己的目标窗口；
记录矩形、client 原点、桌面范围、点击点及前台 HWND，再交由 OS 输入并确认前台切换。
这不直接注入引擎焦点通知，仍需新的实际运行验收。

`ef2e260` 的 Windows `41815105064` / job `126523753183` 中，原有场景仍通过，
新焦点探针仍因 `helper-unfocused` 超时失败，退出 1、forced_stop=true。
已收到两条取消回调 `focus:held:mouse-up:0:0:0` / `focus:held:key-up:65:0:0`；
前台 HWND 严格确认没有完成，不能计为整条原生验收通过。
记录表明虚拟桌面为 1024×768，helper 矩形 `[644,538,1004,738]`，点击点 `[824,557]`，
`WindowFromPoint` 的 root 确实等于 helper；首轮可能遮挡的位置也落在引擎矩形
`[48,89,1024,768]` 内。这次不能再以遮挡解释失败，进一步改掉 STATIC 控件的激活语义。
轻量 artifact `13314800588` SHA256
`de9aa1a40674a17bed607f97ca569c6181c7407726d49f305bb234268d76ca7d`，
已下载核对到 `out/windows-ci/41815105064/`。
exe `1739801a1205fe0c5e8bec46c61000939b6a185732021cdb0674791d9e77c8bb`，
PDB `b5556b2add26669638bf38a1b176f001b379025ee29abe1769e91e12603ac635`，fixture 与首轮相同。
其 Headless `41815063265` 13/13 job 已通过。

`b017747` 将 helper 改为 RegisterClassW 注册的普通顶层窗口，使用原生 DefWindowProcW，
保留真实 SendInput、命中与前台确认；结束时 DestroyWindow / UnregisterClassW。
同时记录清理前的前台 HWND，实际复验结果如下。

`b017747` 的 [Windows 运行](https://github.com/2468785842/krkr2/actions/runs/41816119830)
job `126527193672` 全部成功：Release/full/app/tests/测试驱动=ON、管理 UI 默认 ON，
应用构建、9 项 Cocos CPU 用例（约 0.64 秒）、渲染器范围检查、30 项 Python 均通过。
单/双窗口指针场景和新原生焦点场景均 passed=true、退出 0、未强停。
原生场景实际轨迹核对为 engine HWND `590156` → helper `393288` → engine `590156`，
三个点击点的实际命中 root 都等于目标；native A-down 发给 engine，切走后的 A-up 发给 helper，
恢复后新的 A-down/up 都发给 engine。触摸仍是 GLView 输入，没有把它记成物理设备。

TJS 完整记录了按住阶段的 mouse/key down、两条全释放的取消 up，
失焦期间没有新增输入回调；恢复时旧触点结束与旧 A-up 没有回调，新 A 键与新拖动正常。
没有额外 click 或 System 前后台事件。代码未降低失败判定，没有修改生产 C++ 来配合测试。
普通顶层窗口替换通过实际前台检查；前两轮 STATIC helper 的失败仍保留为失败证据。

exe SHA256 `f4efa513995f605cecdde81b4e62f4ef31a5d63ce24fdaa667b1e70b2107175d`，
PDB `1b8b4d1e696efc2bf4e91226028acf272a16d2c7881a5ceb92a9f78fe8d632c5`，
focus fixture `21ebba88583564de9926218dda51582ba07b9e37bea6a272fca3460797a6f960`。
轻量 artifact `13315196483` SHA256
`ad9af0b602d2a0ccf28d84ae7b1230ce6ae6f4b86db4a178e7db38c33ddd383e`，
已下载核对至 `out/windows-ci/41816119830/`，实际查看 ready/recovered 两张截图，
自制蓝灰层中心 `(480,320)` 都为 RGB `(48,64,80)`；只说明该填充层，不作为完整画面回归。
截图来自 Cocos 帧缓冲，不含外部 helper，原生切焦证据来自 HWND/命中点/SendInput 轨迹与真实回调。
同提交 [Headless 运行](https://github.com/2468785842/krkr2/actions/runs/41816118316)
最终 13/13 job 全通过，包含三平台各 26 项平台契约、30 项 Python 与现有 SDL3 正反向检查。

## 未覆盖及后续

Windows 单宿主窗口的普通 OS 切焦已完成上述验收；Linux/macOS 原生切焦、最小化、
多游戏窗口与 OS 切焦组合、物理触摸设备/手柄或输入法交互仍未验证。
焦点订阅初始沿用已聚焦假定；普通事件通过当前游戏窗口分发，不实现每个物理按下的多窗口所有者跟踪。
Cocos CPU 订阅用例不创建 OS/GL 窗口，也不执行真实 TJS Window 回调；不能宣称全游戏或完整 B2 通过。
后续按[平台契约](../../proposed/architecture/2026-11-03-platform-contracts.md)建立上述场景的原生输入轨迹，
特别核对取消不发点击、恢复后的迟到抬起、多窗口及移动端 surface 生命周期。
