# B2 窗口之后的输入、主循环与生命周期契约

Status: proposed

## 问题与当前边界

窗口首批已提取 `visual/WindowLayer.h`，Cocos 窗口实现仍在 environ；
视频通过专用适配器借用 Cocos 节点。当前 dirty 实现和已验证范围见
[窗口接口](../../../../docs/subsystems/window-interface.md)。
这只完成 B2 的窗口接口子批次，输入、主循环及生命周期仍与现有宿主相关。
SDL3 已选作后续平台壳，尚未完成迁移，见[选型](2026-11-03-ui-platform-shell.md)。

## 建议与下一步

先完成[测试链接闭包](../testing/2026-11-03-ffmpeg-test-link-closure.md)调查，
并保留[访问冲突](../bug-fix/2026-11-03-ui-contract-access-violation.md)的诊断能力。
随后从 `TVPWindow.*`、`WindowImpl.*`、`EventIntf.*` 和 `MainScene.*`
列出事件生产者、排队、脚本分发及销毁回调的调用链，确定下一项最小可独立验证的接口。
输入批次先定义坐标空间、DPI、按下/释放及焦点丢失顺序；
再处理主循环的绘制/模态/计时关系，最后处理暂停恢复与窗口销毁。

## 完成条件

每个接口在现有 Cocos 适配器上保持脚本可观察行为，有对应最小场景和验证证据，
通用头不引入新的 Cocos 类型。记录 Windows 实测及其他平台缺口。
基线建立后再按[后端路线](2026-11-03-backend-modernization.md)推进 SDL3 原型。

## 取舍与风险

直接在一次改动中换窗口、输入和渲染会使回归原因难以区分，采用分接口批次。
本提案尚未确定新的函数签名；先依据调用链和测试定义契约，避免为未来后端堆积空接口。
