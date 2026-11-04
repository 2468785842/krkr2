# 触摸按钮归属、双编译单元与窗口切换取消

Status: implemented

## 基线复现

基于 `01a509b`，`TVPWindow.h` 含 `static tTVPMouseButton _mouseBtn;`，
MainScene.cpp 和 TVPWindow.cpp 分别写读自己的副本；改成单个全局变量仍会让多个窗口/设备互相覆盖。
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

## 未验证与下一步

未实际执行触摸屏双指/三指、物理鼠标交错、多游戏窗口或原生 OS 切焦交互。
状态测试不运行真实 TJS Window 回调；完整应用编译不等于这些交互通过。
全局 VK 状态仍不做逐物理设备引用计数，同一个 VK 的混合设备仲裁未完成。
后续先用原生或真实组件输入轨迹核对取消不点击、双指右键、窗口切换后旧触点不影响新窗口，
再按[平台契约](../../proposed/architecture/2026-11-03-platform-contracts.md)继续 DPI/IME 与完整生命周期工作。
