# 宿主帧循环与模态绘制边界

Status: implemented

基于主线程消息阶段 `2a9bb88`。生产代码、测试与工作流验证提交为 `47ec7a1`。

## 实现

- `environ/HostFrame.h` 提供普通帧、消息处理和按间隔绘制三个宿主入口。普通帧推进应用、回收纹理并调用帧钩子，Cocos 场景继续管理 FPS 标签和场景绘制。
- 普通帧与嵌套消息处理共用 `TVPProcessEngineMessages`，依次分发主线程消息和推进定时器。事件层经该入口处理嵌套消息。
- `event/FrameStep.*` 管理模态节流。用 32 位无符号差值处理计数器回绕，比较有界间隔后才将剩余毫秒转换为有符号数，避免长时间空闲后出现错误等待。正间隔首次调用开始计时，零和负间隔立即绘制。
- 模态窗口、同步文件选择及启动日志共用同一节流状态。帧钩子、绘制和交换缓冲按顺序执行，成功后更新最后绘制的时间。Cocos 绘制和平台换缓冲位于 `CocosModalFrame.cpp`。

## 验证

| 范围 | 结果 |
|---|---|
| 本地 | 52 项平台用例和 49 项 Python 用例通过。8 项帧钩子与节流测试经 ASan、UBSan 和泄漏检查通过。actionlint、依赖守卫和 diff 检查通过。 |
| 格式与三平台无窗口 | [格式检查](https://github.com/2468785842/krkr2/actions/runs/44996178403) 通过。[独立 CI](https://github.com/2468785842/krkr2/actions/runs/44996178433) 共 13 个任务通过，Linux、Windows 和 macOS 各 52 项平台用例通过。独立闭包增加 `FrameStep.cpp`，合计 21 个编译源。 |
| Linux 完整应用 | [44996178365](https://github.com/2468785842/krkr2/actions/runs/44996178365) 通过 17 项 Cocos 契约、8 组图像配置及自制视频首播、关闭、重开和正常退出。二进制 SHA-256 为 `85c1af3313026217836cd68446f11389ccac5c5731aebfe9ab856a132c171fef`。 |
| Windows 完整应用 | [44996179111](https://github.com/2468785842/krkr2/actions/runs/44996179111) 通过 17 项 Cocos 契约、8 组图像配置、5 组输入与生命周期探针及同步文件选择交互。二进制 SHA-256 为 `6117cb18c6a9a60dbe5a7179e88eaa102672611f8e9ae13f7a8f2ce16e2fd4a7`。 |
| Android ARM64 | [44996178701](https://github.com/2468785842/krkr2/actions/runs/44996178701) 编译 356 个项目源，Debug APK 打包、ABI 及原生依赖检查通过。APK SHA-256 为 `384fc4d7849dbc0f1ef7ce71b630b04405f52edfbde29f3f8ca95393aaf33f90`。 |

Windows 的新增回归通过 Cocos 组件输入实际点击同步文件选择器。脚本得到取消返回 0、确认返回 1
及规范化的 `fixture.txt` 路径，菜单开关和首次关闭否决也有实际回调记录。进程退出 0，未强制停止。
CI 使用 `--skip-native-dialog` 运行 Cocos 场景。默认的交互复现仍等待操作者确认系统原生消息框。

macOS 验证范围为无窗口构建和测试。Android 真机、商业游戏及系统原生消息框属于独立运行验收。
