# 文件交互后出现一次访问冲突

Status: proposed

## 问题与事实

Windows 的 `b2-complete-off-01` 交互运行在文件确认成功后、菜单命令处理前退出，
退出码 `3221225477`（`0xc0000005`），未被控制器强停。
Windows 错误记录为 2026-11-03 19:08:47，模块 krkr2.exe，偏移 `0x475abd`。
失败应用 SHA256 为 `0d2471f2612c79a5321f6fd6149d3a5ed0a1499f3bec0d68db9bafae23fa6b5f`，
当时 `KRKR2_USE_COCOS_UI=OFF`、`KRKR2_ENABLE_UI_TEST_DRIVER=ON`。
报告在 `out/ui-game-smoke/b2-complete-off-01/interaction-report.json`。
没有该失败构建的异常栈或 dump，根因未知。

后续 `b2-acceptance-off-01`（附加 CDB）、`b2-acceptance-off-02`（无调试器）
完整交互均正常退出 0，应用 SHA256 为 `56fbec6a3110e6a1cb87f9f7ff2e8366f53a0c73bf237f70c2c44140831a1a2f`。
这两个通过报告不能证明早先问题已修复。复现场景与限制见[窗口接口](../../../../docs/subsystems/window-interface.md)。

## 建议与下一步

复核现有 `out/windows/b2-acceptance-auto.py`、`b2-repro-auto.py` 和正式控制器，
整理可重复的文件取消/确认、截图和菜单操作序列。
每次等驱动完成与脚本回调，再提交下一动作，分别记录附加调试器与直接运行的结果。
使用同一应用和匹配 PDB 捕获访问冲突的异常栈及 dump。
原生消息框仍需实际 Windows 输入，组件驱动不能伪造其返回。

## 完成条件

保留可解释的失败证据，定位根因并实施修正；针对该原因验证失败变通过，补跑完整交互及正常退出。
只有连续成功但没有根因时，记录复现范围和次数，保持本问题待调查。

## 假设、备选与风险

模态嵌套绘制、截图回调和组件输入时序是待检查方向，均未证实。
不要从新构建的同 RVA 符号推断旧构建的故障函数，也不要先改定时器或销毁逻辑再寻找解释。
CLion debugger 或已有 CDB 均可使用；调试工具的时序影响应单独记录。
