# Cocos 桌面焦点订阅与游戏输入取消

Status: implemented

## 问题与实现

基于 `6d5a163` 加本批改动。此前 GLView 的普通焦点事件没有接入游戏输入，
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
这些测试尚待完整 Windows 构建运行，不将本地 21 项当作 Cocos 编译或交互证据。
Windows 工作流预期实际发现 9 项（节点 2、坐标 3、焦点 4）；三平台独立范围预期 21 项与 11 个项目源。
远端结果在推送后补充。

## 未覆盖及后续

未实际执行原生 OS 切焦、最小化、多游戏窗口路由、触摸设备/手柄或输入法交互。
焦点订阅初始沿用已聚焦假定；普通事件通过当前游戏窗口分发，不实现每个物理按下的多窗口所有者跟踪。
Cocos CPU 订阅用例不创建 OS/GL 窗口，也不执行真实 TJS Window 回调；不能宣称全游戏或完整 B2 通过。
后续按[平台契约](../../proposed/architecture/2026-11-03-platform-contracts.md)建立上述场景的原生输入轨迹，
特别核对取消不发点击、恢复后的迟到抬起、多窗口及移动端 surface 生命周期。
