# ImageOps 的 Linux 像素与 sanitizer 验证

Status: implemented

本记录描述原生最近邻续批的本机结果，不代表所有图像后端或游戏画面已验收。

本机 Linux/GCC 14.2 Release 为 13 项用例、41,377 个断言通过；模块和测试启用 ASan/UBSan 的 Debug 构建同样通过，
启用泄漏检测。参考 OpenCV 库使用 Release、未启用 sanitizer，因此不声称检查了后端内部所有访问。
归档+XP3 的原 9 项 CTest 回归通过。跨平台进展见[实施记录](../simplification/2026-11-01-dependency-slimming.md)。
