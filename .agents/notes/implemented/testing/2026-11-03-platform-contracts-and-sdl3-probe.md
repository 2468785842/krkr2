# 输入、帧钩子、宿主通知与 SDL3 壳实验验证

Status: implemented

## 范围与实现

基于 `0f163a2` 加当前工作区改动，Windows x64、MSVC 19.44、Debug。
完整 B2 仍未完成，活动边界及输入前后基线见[后续契约](../../proposed/architecture/2026-11-03-platform-contracts.md)。
本记录只描述已实现子批次，不替代[原访问冲突调查](../../proposed/bug-fix/2026-11-03-ui-contract-access-violation.md)。

- `core_input` 接管原 512 项共享状态，保留 current 查询也消费按下标记、释放保留未读标记等行为。
- `PostUpdateEvent.cpp` 保存唯一回调，修复原内部链接存储在跨编译单元间不可见的问题。
  提取原定义的双单元实验为 0 次调用（预期 1 次），不将该实验归因为原游戏访问冲突。
- `RenderUtils` 的 Cocos 绘制移入 cpp，`HostLifecycle.h` 提供无 Cocos/TJS 类型的前后台通知。
  不改变旧退出接线与销毁顺序。
- SDL3/TJS 独立实验使用自定义脚本全局变量，不是生产 Window/Layer 或 KAG 后端。

## 验证与证据

正式独立配置使用 `KRKR2_TEST_SCOPE=platform`、应用/工具关闭、测试开启。
`out/platform-contract-validation/verify.log` 包含配置、实际构建、CTest 注册及运行结果：
6 个输入与 3 个帧钩子用例全部通过。独立安装树只有 Catch2 和两个 vcpkg CMake 辅助包。
此前 full 配置同样实际注册并运行了 9 项用例。
最终 full 配置复核还构建应用与 FFmpeg 初始化测试，10 项定向 CTest 全部通过，
成功日志为 `out/windows/ffmpeg-link-closure/final-contracts-cold.log`。
本轮首次增量重链接长时间低 CPU 等待，保存线程栈后终止，保留 ILK 并重新冷链接成功；
没有从缺少链接器私有符号的栈推断工具链根因。
临时 batch 的 `if errorlevel 1` 曾漏判终止产生的负退出码，继续运行了旧 exe 的 CTest；
该次结果明确排除。改为显式非零判断后再执行上述成功轮，新 exe 时间与哈希均已核对：
FFmpeg 测试 SHA256 为 `09a4cc21801b88e9f6bac312310ce421fd11525e74f1d759ac2c202845a2b782`。
此陷阱已写入验证技能，避免仅看日志末尾的 CTest 全绿。
依赖检查扩至 11 个基础目标，并约束 System 查询与三个宿主公共头不重新引入具体后端。

输入迁移前后分别保存在 `input-state-baseline-01`、`input-frame-migrated-01`：
同一 TJS 场景的 A-key down/up 查询序列一致；后者附加 CDB，三轮完整交互通过，退出 0、未强停。
`lifecycle-migrated-01` 检查真实 System 停用、激活回调，再完成一轮交互与正常退出。
以上目录位于 `out/ui-game-smoke/`，包含 exe/PDB 副本、哈希、请求轨迹与截图。
自动控制器的 Python 测试 13 项通过，包括拒绝旧事件确认与驱动异常假通过。

真实游戏副本 `input-frame-game-01`、`input-frame-game-image-01` 各观测约 25 秒，
KAG/心跳及 `title.ks:33` 条件通过，到期均强停，不能证明正常退出。
第二轮实际查看 `smoke-frame.bmp`：为启动警告图层；这是 KAG snapshot，不是 OS 窗口或宿主 UI。

SDL3 使用仓库 pin baseline 的 3.4.8#1，独立安装和构建目录在 `out/sdl3-prototype/`。
安装树未包含 Cocos、OpenCV、FFmpeg。GitHub archive 下载停滞时，
从官方 codeload 获取同一 tag，SHA512 与 port 中的固定值完全一致后放入下载缓存，
终止本次停滞的下载子进程，再由 vcpkg 使用缓存完成安装；未修改工具链或依赖源码。
相关日志为 `configure.log`（首次失败）、`download.log`、`configure-cached-source.log`、`final-build.log`。

最终 `final-run-01/verification.json` 为 passed，进程退出 0、未强停；
真实窗口尺寸/标题、software renderer、四帧、键盘和关闭计数、事件顺序及 BMP 像素均符合条件。
exe SHA256 为 `13ff28c6752c5d729e1c099c3cd510d31e9847ac4d93bc878abc56b20738ce5f`，
fixture SHA256 为 `e69792b406389bbb2046a87138d01b0cd53654a26ab4b009c27d12d9ef74512c`。
实际查看 pressed/released 图像，矩形颜色和范围符合脚本变化。
`final-negative-01` 故意画错颜色，程序仍退出 0，但验证器退出 1、passed=false，
明确报告 pressed 像素与预期不符，避免仅凭事件计数通过。

## 未覆盖及下一步

只验证 Windows；Linux/macOS/Android 未构建或实测。
组件按键不是 OS 设备输入，宿主通知探针没有暂停 Cocos 动画泵；
SDL 自测由 SDL_PushEvent 投递，不更新物理设备状态。
尚未验证 IME、DPI、失焦释放、移动端挂起与 surface 重建、媒体或完整游戏兼容。
后续应先建立这些场景与现有后端基线，再接入 SDL 到生产窗口/图层契约；
不能用实验脚本 API 冒充已有 Window/Layer 行为或删除现有 Cocos 功能。
