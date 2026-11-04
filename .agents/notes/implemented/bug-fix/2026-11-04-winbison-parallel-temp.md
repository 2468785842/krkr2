# WinFlexBison 并行生成的 m4 临时文件冲突

Status: implemented

## 失败证据与调查

提交 `4a341e6` 的[完整 Windows 构建](https://github.com/2468785842/krkr2/actions/runs/41770221479)
job `126377925966` 在三份 Bison 语法并行生成时失败：
`tjspp.y`、`tjsdate.y` 都报 `lalr1.cc:756: undefined macro b4_symbol(32, has_printer)`，
随后报告 `field has_printer of 32 is not a Boolean`；`tjs.y` 同批成功。
失败发生在 C++ 输入适配器编译之前，不将它归因于本批取消/字符输入修复。
同一提交 Headless 的 13 个 job 全部通过；此前完整 Windows 构建也曾通过。
早期另一次未复现的 Bison 失败仍保留在[历史记录](../testing/2026-11-03-sdl3-focus-validation.md)，不以这次调查覆盖旧证据。

仓库安装 WinFlexBison v2.5.25。上游该 tag 的 `bison/src/output.c` 在 `output_skeleton`
以 `_tempnam(NULL, "~m4_in_")` / `_tempnam(NULL, "~m4_out_")` 选名，然后直接 `fopen`。
多个进程之间没有原子创建保障；[上游问题 #100](https://github.com/lexxmark/winflexbison/issues/100)
及其最后一条分析明确说明同名覆盖也影响 win_bison，修复 #91 尚未包含在 v2.5.25 中。
本次没有捕获临时文件覆盖的系统跟踪；上述共享临时文件机制与并行宏损坏现象一致。

## 本仓库修正与验证

修正提交 `1539229`，`cpp/core/tjs2/CMakeLists.txt` 在 Windows **宿主**上为每份语法建立构建目录内的私有临时目录，
只对该命令通过 `cmake -E env` 设置 TMP/TEMP。保留三个生成任务并行、现有输出路径与语法内容，
不改变全局工具链，也不要求升级到未发布工具版本。
不同语法和不同构建目录各自隔离；同一个构建目录不支持多个独立构建进程同时写同一输出。

Linux SDL3/TJS 原型重新配置、构建无新增工作，11 个 foundation 约束与 diff 检查通过。
本地没有执行 Windows 分支。随后同一提交的[完整 Windows 构建](https://github.com/2468785842/krkr2/actions/runs/41770810072)
job `126379808412` 成功：三份语法实际重新生成，日志步骤 2/389、3/389、4/389 分别为 date、pp、主解析器，
继而实际编译本批 `TVPWindow.cpp`、`MainScene.cpp`，成功链接应用和 cocos_window_tests。
实际发现并通过 5 项 Cocos 节点/坐标用例（约 0.35 秒），两个渲染器编译范围及二进制上传通过。
Windows 宿主 CMake 分支按 FILE_NAME 为三条命令设置不同 TMP/TEMP；没有关闭并行或修改语法。
同一提交的[Headless CI](https://github.com/2468785842/krkr2/actions/runs/41770801542)全部 13 个 job 通过。

## 风险与后续

私有目录防止本项目生成器互相覆盖，不是对工具所有并行缺陷的全面保证。
升级工具版本时复查上游修复及实际发布内容；若仍失败，应保存本轮私有临时文件与具体诊断，
不能通过重复重跑直到偶然成功关闭问题。
