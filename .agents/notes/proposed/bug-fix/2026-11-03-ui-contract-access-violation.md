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

已有正式复现工具 `tools/game-smoke/interaction_repro.py`，用法见
[交互场景说明](../../../../tools/game-smoke/interaction/README.md#连续复现与取证)。
继续用当前带符号应用分别执行直接运行和 CDB 附加的连续操作序列，实际观察每轮截图。
每次等驱动完成与脚本回调，再提交下一动作，分别记录附加调试器与直接运行的结果。
使用同一应用和匹配 PDB 捕获访问冲突的异常栈及 dump。
原生消息框仍需实际 Windows 输入，组件驱动不能伪造其返回。

## 取证工具验证进展

工具复用原控制器判定，按事件计数和独立动作序号确认本轮结果，截图等保存回调后复制。
每轮第二次菜单点击后实际点击返回按钮，避免上一轮仍打开的菜单导致同名控件累积。
启动时保留 exe/PDB 副本与哈希，原进程退出码、操作轨迹、CDB 日志和 dump 进入本次私有目录。
13 项 Python 测试通过，包括旧事件、错误序号及操作超时不能被完整脚本事件覆盖的负向检查。

使用自编原生空指针故障样本实测 CDB，已验证初始化就绪标记、首次及二次异常处理、
带空格的 dump 路径、符号化 main 栈，以及原始 `0xc0000005` 退出码。
报告在 `out/ui-repro-diagnostic/dump with spaces 03/selftest-report.json`，两份 dump 已写出。
最初只设置首次异常命令时，二次异常停在调试提示符并导致调试器退出，退出码变成 `0xc0000354`；
现已同时设置首次和二次异常的命令，并等待明确就绪标记再发起交互。
该故障样本只验证取证机制，不是原游戏崩溃的复现或修复。

## 正式连续操作结果（2026-11-03）

`post-b2-repro-direct-01` 与 `post-b2-repro-cdb-01` 两个新副本各完成 3 轮，
均 `passed=true`、进程退出 0、未强停，文件/菜单事件、关闭否决与接受齐全；
CDB 轮的调试器退出 0、没有访问冲突标记或 dump，模块信息确认加载 `evidence/krkr2.pdb` 的 private symbols。
实际检查了文件选择器与第三轮菜单截图，未宣称所有控件状态的像素一致性。

应用 SHA256 为 `f37b492cc220aecbff789b09684bd357d5de4a7a80c9bd1463001d4f4aa29b4d`，
PDB SHA256 为 `6c066f8dfd702ea0e222be8a73e17211e4b558f6f2be20e7a5d70432cea0232d`，
两轮均保存了副本与哈希。各自 `repro-trace.json` 包含请求、完成确认、截图名及时间。
目录均在 `out/ui-game-smoke/`，启动日志在 `out/windows/ffmpeg-link-closure/repro-direct.log`、`repro-cdb.log`。
本轮增强了复现和取证能力，仍没有原故障的根因，问题保持 proposed。

## 完成条件

保留可解释的失败证据，定位根因并实施修正；针对该原因验证失败变通过，补跑完整交互及正常退出。
只有连续成功但没有根因时，记录复现范围和次数，保持本问题待调查。

## 假设、备选与风险

模态嵌套绘制、截图回调和组件输入时序是待检查方向，均未证实。
不要从新构建的同 RVA 符号推断旧构建的故障函数，也不要先改定时器或销毁逻辑再寻找解释。
CLion debugger 或已有 CDB 均可使用；调试工具的时序影响应单独记录。
