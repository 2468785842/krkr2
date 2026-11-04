# 生产窗口指针坐标契约

Status: implemented

## 问题与实现

基于 `312cda2`。`TVPWindow.cpp` 的鼠标、滚轮、触摸及取消路径重复执行
世界坐标→Node 局部→height-y→整数转换；`SetCursorPos` 把窗口父节点坐标
写入 `_LastMouseX/Y`，但 GetCursorPos 和输入事件约定主图层左上角坐标。

`input/PointerCoordinates.h/.cpp` 提供标准 C++ 浮点/整数位置、Y 翻转及向零截断。
保留连续坐标 height-y、负坐标和边界外拖动，不在此层裁剪、缩放或处理 DPI。
新增对非有限值和 int32 越界的检查，防止浮点转整数的未定义行为。
`cocos2d/CocosPointerCoordinates.h` 使用原 Node 变换，再委托共享规则；
鼠标/滚轮/触摸路径接入，原事件顺序和触摸开始/结束点选择保持不变。
负坐标进入环境噪声编码时改用 uint32 移位和相加，避免旧有符号移位/溢出的未定义行为。

SetCursorPos 缓存请求的主图层坐标，显示位置单独换算到光标自身父节点；
无光标时不再为缓存更新强制访问父节点。此修正不实现 OS 鼠标移动或新的捕获策略。

## 本地验证与 CI 范围

Linux Debian 13、GCC 14.2、CMake 4.4.4、Debug，沿用隔离 Catch2 安装。

```sh
cmake --build out/linux/platform-focus --target input_state_tests frame_hook_tests --parallel 4
ctest --test-dir out/linux/platform-focus -N
ctest --test-dir out/linux/platform-focus --output-on-failure --no-tests=error
python scripts/check_core_module_deps.py
```

均退出 0；实际注册并通过 18 项平台用例，含 3 项新坐标用例。
覆盖连续坐标边缘、向零截断、图层外位置、int32 可表示边界、NaN 与正负无穷。
依赖检查 11 个 foundation 目标通过，输入模块没有增加引擎或平台链接依赖。

新增 3 项真实 Cocos Node 测试，结合既有 2 项排序测试，由 Windows 完整构建验收。
使用平移、缩放、旋转、锚点、负坐标及不同父节点，不创建 GL 窗口。
典型场景请求 (17,37)：旧窗口父节点缓存为 (54,916)，新光标父节点显示位置为 (64,911)，
显示位置转回主图层应为原请求；分别检查坐标空间，不能把显示位置当作脚本缓存。
这些 Cocos 断言尚待远端运行，不能把本地纯 C++ 通过当作 Cocos 验收。

完整 Windows 工作流改为开启 full 测试配置，定向构建 krkr2/cocos_window_tests，
检查实际发现 5 项窗口/坐标用例再运行；其他 full 测试不因配置成功而视为已通过。
三平台独立范围检查 18 项注册、9 个项目源及 Catch2 安装闭包。

## 剩余范围

完整应用构建与真实 Cocos 变换断言待推送后核对。脚本 Layer.setCursorPos 的交互读回、
OS DPI/屏幕映射、设备输入、捕获生命周期及 SDL 鼠标事件尚未实测。
不能用 CPU Node 测试宣称游戏画面、IME、GL 或 B2 已全部通过。
后续继续按[平台契约](../../proposed/architecture/2026-11-03-platform-contracts.md)推进实际输入基线与生产窗口接入。
