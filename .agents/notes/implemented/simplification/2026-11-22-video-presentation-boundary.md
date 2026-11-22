# 视频覆盖层与 Cocos 宿主解耦

Status: implemented

基于 `codex/continue-render-migration` 的 `a2896f3` 接续 Visual 构建边界。
生产代码最终版本为 `c06a92c`，之后的提交只涉及测试、CI 和文档。

## 实现

- `movie/VideoPresentation.h` 提供紧密排列的 YUV420P 借用帧、目标矩形、两层显隐控制和窗口呈现工厂。
- 游戏覆盖层和管理媒体预览共用 Cocos 适配。游戏由窗口节点定时回调，预览由自己的更新循环交帧。
  适配器持有呈现节点引用，销毁时解绑并注销调度。窗口传入空指针或已没有 form 时只解绑旧呈现。
- 保留原有 Cocos YUVSprite 的上传、坐标换算和混色。FFmpeg 播放器继续管理帧队列、PTS、跳帧及脚本事件。
  Layer 模式向引擎位图输出。窗口与预览都由现有 Cocos 应用壳承载。
- `core_movie_module` 删除直接 Cocos 包查找、链接和源码引用。依赖守卫会识别 Cocos 节点、CMake 包及
  `platform/CCPlatformConfig.h` 等直接头文件引用。

## 验证

| 范围 | 结果 |
|---|---|
| 本地 | `platform` 42 项、`imageops` 7 项、游戏探针 Python 49 项和检查脚本 Python 5 项通过。独立 C++17 头编译、clang-format 20.1.8、依赖守卫、三项注入反例与 diff 检查通过。 |
| Windows 完整应用 | [44972866269](https://github.com/2468785842/krkr2/actions/runs/44972866269) 完整构建，定向 Cocos 契约 17 项通过，其中两项覆盖已挂载和已脱离的节点清理。五组原生窗口交互、八组渲染配置及 49 项 Python 用例通过。 |
| Android ARM64 | [44972351234](https://github.com/2468785842/krkr2/actions/runs/44972351234) 编译 353 个项目源并成功打包 Debug APK。工作流核验 ABI 与原生依赖，APK SHA-256 为 `9569b0aa7b7533e3a793fee1df2b996ed8e9349b011fedb15f9f8f9840d67d61`。 |
| Linux 完整应用 | [44973634715](https://github.com/2468785842/krkr2/actions/runs/44973634715) 完整构建，17 项 Cocos 契约与八组渲染配置均通过。自制视频完成首播、关闭、同一影片重开及正常退出。构建产物 SHA-256 为 `aa85e349d2fdfc1ef11d613246b57d8b97c2d02819ef7021d78ae0c2c824675a`。 |

Linux 自制 MPEG-4 样本为 96×64、YUV420P。首播和重开两次截图的四个采样点相同，
依次为背景 (16, 32, 48)、红色 (255, 0, 0)、绿色 (0, 254, 1)、背景 (16, 32, 48)。
关闭后四点均为背景色。脚本使覆盖层失效并退出，进程返回 0。截图和日志由工作流归档。

## 首轮失败与修正

- [Linux 44972031518](https://github.com/2468785842/krkr2/actions/runs/44972031518) 和
  [Windows 44972031411](https://github.com/2468785842/krkr2/actions/runs/44972031411) 发现
  `VideoPlayer.cpp` 残留未使用的 Cocos 平台头，移除该头并补齐守卫。
- [Windows 44972351316](https://github.com/2468785842/krkr2/actions/runs/44972351316) 已完成编译，
  注册数断言仍预期旧的 15 项。更新为 17 项后复跑通过。
- [Linux 44972352906](https://github.com/2468785842/krkr2/actions/runs/44972352906) 的 LZ4 配置未收到
  首次换图事件并超时。保持原通过条件，将视频与图像步骤独立执行。随后的两轮图像矩阵均通过。
- [Linux 44973137967](https://github.com/2468785842/krkr2/actions/runs/44973137967) 的首次视频探针
  把 TJS 写出的带 BOM 文本当作 UTF-8 读取，导致等待 ready 超时。按 BOM 选择编码后复跑通过。

运行验收范围为自制覆盖层视频及既有桌面回归。管理媒体预览路径随完整应用编译，
其 UI 播放操作、Android 真机、macOS 完整应用和真实游戏单独验收。
