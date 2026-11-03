---
name: krkr-verify
description: 为 KrKr2 的 C++、CMake 或后端改动选择并执行构建与测试验收，核对实际注册用例、依赖和构建身份；汇报验证结果或调查构建失败时使用。
---

# KrKr2 构建与验收

先读本次变更所涉及的 [构建指南](../../../docs/guides/building.md) 和模块 CMake，
结合 git diff 选择目标。已有构建目录是本机状态，运行前核对 CMakeCache 的 scope、
应用开关、管理 UI 与测试驱动开关；不要假设 debug 目录就是完整验收配置。

## 选择范围

- TJS 字符串、归档、图像或启动参数可按根 `KRKR2_TEST_SCOPE` 的实际选项选择独立范围。
  只有 `--target` 不会缩小配置期依赖；独立闭包结论需要独立配置依据。
- 改窗口适配或 UI，构建应用，并查阅[窗口接口](../../../docs/subsystems/window-interface.md)的交互验收范围。
  `cocos_window_tests` 只在 full 范围注册，需要实际构建才会发现 Catch2 用例。
- 文档或技能调整只做相应路径、元数据与 diff 检查；新脚本验证实际行为。
- 基础模块依赖改动执行 `python scripts/check_core_module_deps.py`，其成功范围以工具输出为准。

## 结果判定

先用 `ctest --test-dir <构建目录> -N` 核对预期用例，再用有针对性的 `-R`
和 `--output-on-failure --no-tests=error` 执行。
新测试需登记到相应 CMake SOURCES；仅存在文件、成功链接或 CTest 找不到测试都不足以通过。
有 unresolved symbol 时取得实际链接行和符号实现归属，区分被测逻辑与宿主依赖，
不得以空生产实现或删掉失败测试替代修复。

## 交付证据

记录实际命令、退出码、运行数量和筛选范围，说明未执行目标。
运行游戏时另记 exe/hash、场景身份、关键开关和是否强停；构建通过与游戏兼容分别说明。
日志放独立 `out/` 目录，仍未解决的原因和具体下一步更新到对应 note。
检查通过后，只有新改动、失败或未解疑点才重复或扩大测试。
