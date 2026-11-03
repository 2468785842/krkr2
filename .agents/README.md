# AI 协作

先读根 [AGENTS.md](../AGENTS.md)，再按当前任务选择相关资料。
当前实现与操作指南以 [docs](../docs/README.md)、源码和 CMake 为准。

## 组织边界

| 位置 | 内容 |
|---|---|
| 根 `AGENTS.md` | 项目约束与任务路由 |
| [notes](notes/README.md) | 问题调查、设计提案、决策依据与历史验证 |
| [krkr-verify](skills/krkr-verify/SKILL.md) | 构建、测试范围、注册与结果证据 |
| [krkr-game-debug](skills/krkr-game-debug/SKILL.md) | 隔离游戏、截图交互及异常定位 |
| `reports/` | 入库的小型测量报告，保留环境和样本数据 |
| 仓库 `out/` | Git 忽略的原始日志、截图、dump、临时调试脚本 |

这里维护的是项目工作方法和任务记忆，执行仍使用现有终端、IDE、调试器和测试工具。
技能已经被当前宿主发现；其他宿主可通过 AGENTS 中的路径直接读取，不要求安装全局技能副本。

## 接续工作

1. 查看 git status/diff，保留未提交改动；按主题搜索活动 notes，只加载相关内容。
2. 核对 note 的版本、路径、构建开关与产物身份，选用匹配技能。
3. 定义要保持的可观察行为，执行最小修改与相关验证。
4. 将新证据、仍缺的证据和具体下一步更新到原 note；实现事实同步到 docs。

用户只说“继续”时延续当前主题，不把所有活动 notes 自动扩展为本轮任务。
验证结论应注明范围，未复现不关闭问题；通过条件以对应记录和真实行为为准。
可重复的方法逐步提炼进技能，可执行逻辑放 tools/scripts，避免在技能中复制整套工具实现。

## 当前后端工作的顺序

- [FFmpeg 测试链接闭包](notes/proposed/testing/2026-11-03-ffmpeg-test-link-closure.md)：取得链接行和符号归属，恢复真实测试目标。
- [文件交互访问冲突](notes/proposed/bug-fix/2026-11-03-ui-contract-access-violation.md)：整理可重复操作与匹配符号，保留异常栈和 dump。
- [B2 后续契约](notes/proposed/architecture/2026-11-03-platform-contracts.md)：逐项处理输入、主循环和生命周期，再推进平台壳原型。

其他方案按需浏览 `notes/proposed/architecture/`，不因为文件已存在就视为已实现。
历史测量与验证放 `notes/implemented/`，早期失效分析放 `notes/archived/`。
