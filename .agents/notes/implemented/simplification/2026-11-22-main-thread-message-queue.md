# 主线程消息与 Base 构建边界

Status: implemented

基于视频呈现阶段 `1af6758`。生产代码和 CI 修正提交为 `47b538c`。

## 实现

- `event/MainThreadMessageQueue.*` 提供标准 C++ 队列。线程可投递消息，引擎线程按批分发。
  应用在脚本定时器前分发消息。显式嵌套分发可取新消息，回调抛出异常时释放当前批次余项。
- 本地事件投递携带消息 ID 与事件值副本。可按接收器和 ID 取消队列中或已进入待处理批次的消息。
  接收器注销清除该接收器全部未开始的回调。生产线程须在接收器销毁前停止。
- `core_base_module` 移除 Cocos 包查找与链接。依赖守卫检查 Base 直接引用以及本地事件对应用头的依赖。
- Linux 构建没有矩阵，原 ccache 键引用 `matrix.os`。actionlint 报告该属性不存在，现用显式 Linux Release 键和 `runner.arch`。
  Headless 校验增加 5 项消息测试及 3 个编译单元，保留编译源和依赖闭包核对。

## 验证

| 范围 | 结果 |
|---|---|
| 本地 | platform 47 项、ImageOps 7 项、检查脚本 Python 5 项通过。新增 5 项另经 ASan/UBSan 与泄漏检查通过。actionlint 1.7.10、依赖守卫、格式、20 源文件闭包和 diff 检查通过。 |
| 三平台独立测试 | [44991259093](https://github.com/2468785842/krkr2/actions/runs/44991259093) 全部 13 个任务通过。Linux、Windows 和 macOS 各注册并执行 47 项平台契约，vcpkg 闭包是 Catch2 和两个 vcpkg CMake 辅助 port。 |
| Windows 完整构建 | [44991258891](https://github.com/2468785842/krkr2/actions/runs/44991258891) 完整构建、17 项 Cocos 契约、5 组原生窗口交互和 8 组渲染配置通过。 |
| Linux 完整构建 | [44991258843](https://github.com/2468785842/krkr2/actions/runs/44991258843) 完整构建、17 项 Cocos 契约、8 组渲染配置及自制视频通过。首播与重开截图采样一致，关闭后仅有背景，最终进程退出 0。 |
| 总 CI | [44991258551](https://github.com/2468785842/krkr2/actions/runs/44991258551) 通过格式、Linux、Windows 和 Android 构建，工作流上传了 Android APK。 |

Linux 场景二进制 SHA-256 为 `2d213383dda8fc7fafe62022fd43a52726b158ac7ba179b304a1e573570106bd`，
Windows 场景二进制 SHA-256 为 `a09cc55615a8eceb73ac58d8628741f23ada9a9fb0759f828d4773f98fdeba4c`。

macOS 验证范围为无窗口用例。Android 真机、macOS 完整应用和商业游戏单独验收。
