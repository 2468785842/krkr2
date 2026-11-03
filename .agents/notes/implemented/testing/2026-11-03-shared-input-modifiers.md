# 共享修饰键快照与 SDL3 组合键

Status: implemented

## 问题与实现

基于 `e1e6413`。引擎导出 `TVPGetCurrentShiftKeyState()` 仍实现在 Cocos MainScene，
逻辑本身只读共享虚拟按键状态；SDL3 原型不支持 Shift/Ctrl/Alt，无法验证组合键恢复。

`input/InputModifiers.h/.cpp` 提供标准 C++ 的虚拟键常量与 `legacyShiftFlags(const KeyState&)`。
读取当前状态但不消耗按下标记，保持 Shift=1、Alt=2、Ctrl=4、左键=8、右键=16。
旧实现明确省略中键，本批不改变它，也不把事件的 repeat/double-click 位混入快照。
引擎导出移到 `base/impl/SystemImpl.cpp`，声明、ABI、原有事件调用方不变；
编译期断言核对引擎标志位，Cocos 适配器核对六个共享 VK 值。
System 与 environ 已有 core_input 链接，无新域依赖或第三方库。

SDL 原型增加左右 Shift/Ctrl/Alt 映射，按 scancode 查询无修饰键的键码，
避免将 Shift+A 的文本结果当作另一个虚拟键；`onKey` 最后增加 shift 快照参数，日志同时记录。
队列场景使用三种修饰键+A；X11 场景使用 Ctrl+A，按本轮实际回调等待，再切焦/释放/恢复。
仍然是实验脚本 API，没有接入生产 Window/Layer 或 IME，也没有接线 Cocos 失焦释放。

## 本地验证

沿用上一批 Linux Debian 13、GCC 14.2、CMake 4.4.4、Debug 及隔离依赖。

```sh
cmake --build out/linux/platform-focus --target input_state_tests frame_hook_tests --parallel 4
ctest --test-dir out/linux/platform-focus -N
ctest --test-dir out/linux/platform-focus --output-on-failure --no-tests=error
cmake --build out/linux/sdl3-focus --target krkr2_sdl3_prototype --parallel 4
xvfb-run -a python tools/sdl3-prototype/verify.py out/linux/sdl3-focus/tools/sdl3-prototype/krkr2_sdl3_prototype out/linux/sdl3-modifiers-queue-01
xvfb-run -a python tools/sdl3-prototype/verify_x11.py out/linux/sdl3-focus/tools/sdl3-prototype/krkr2_sdl3_prototype out/linux/sdl3-modifiers-os-01
```

以上均退出 0。实际注册并通过 15 项平台用例：9 项状态、3 项修饰键、3 项帧钩子。
新测试覆盖全部 64 种组合、快照不消耗标记、批量释放前后状态；
另从 Git 基线实际提取 MainScene 原函数到 `out/linux/modifier-baseline.cpp`，
与共享实现比较 64 种组合完全一致，程序退出 0。
原生窗口就绪回归沿用上一批，不因增加组合键而改变其判定。

两个 SDL 报告 passed=true、退出 0、未强停，渲染像素与上一批一致。
队列测试 5 次按下/抬起、4 次合成释放；全部键已释放后，释放回调的 shift 均为 0。
X11 测试 3 次按下/抬起：Ctrl down=4，A down/up=4，Ctrl up=0，
随后失焦/恢复与新的 A down/up=0；没有重复抬起，关闭否决与接受仍正确。
两条路径的计数、释放顺序不同是正常现象，不能要求 SDL 自动抬起也走全部清空的分支。
初轮应用 SHA256 为 `8de4fb3e28d3f5f9716daccfadc8908d4061c9959c1ad892328787c54dc95593`，
fixture SHA256 为 `0bf158f43d51a3520fc53e183df1e243ef984116c5a183fbf69d82e5b8dcd2af`。
依赖检查 11 个 foundation 目标及修饰键归属约束通过。

## 验收边界与下一步

三平台独立 CI 与完整 Windows 应用构建待推送后核对，不能用 Linux 独立测试冒充完整应用验收。
仍保留现有 Cocos 将左右修饰键合并到一个 VK 的规则，未补齐同时按住两侧再释放一侧的独立跟踪。
System 导出与旧宿主的实际游戏回调尚需交互对照；本批不更改 Cocos 事件顺序、激活、捕获或销毁。
后续继续按[平台契约](../../proposed/architecture/2026-11-03-platform-contracts.md)建立生产后端真实失焦基线，
再处理坐标/DPI/IME 和生产 SDL Window/Layer 接入。
