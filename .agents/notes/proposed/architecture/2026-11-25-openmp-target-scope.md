# OpenMP 选项没有传到实际任务执行目标

Status: proposed

## 现象与证据

依赖审计基线 dev `b6cb999`，Linux/GCC 14.2/Release/SDL3-only，PSD ON、UI test driver OFF。
`cpp/core/CMakeLists.txt` 在 krkr2core INTERFACE 上查找/传播 OpenMP，
但含 `#pragma omp parallel for` 的 `utils/impl/ThreadImpl.cpp` 由 core_utils_module 编译，
没有反向依赖该聚合接口。实际 compile_commands 中 87 个消费者含 -fopenmp，ThreadImpl.cpp 不含。
全仓直接 pragma 仅发现此处；RenderManager.cpp 多处调用 TVPExecThreadTask。

这能证明本次 Linux 构建的选项归属错误，不能凭目录结构推断每个平台/外部工具链都相同。
SDK/源码审计证据由 `scripts/audit_dependencies.py` 保存于 `out/dependency-audit/*.json`。
本轮依赖精简没有修改任务调度或假称 OpenMP 已被替换。

## 建议下一步

先验证 TVPExecThreadTask 的任务次数/索引、零或单线程、异常、嵌套调用、并行度和资源失败契约，
记录同机像素基线与 CPU 场景耗时。然后选择：

- 将 OpenMP 的查找、选项与链接移到真正编译循环的 core_utils_module，保留其他平台行为；或
- 抽出与 TJS/宿主无关的标准线程调度，供 ImageOps 与共享渲染任务使用，避免新建反向依赖环。

不能仅删除 find_package 让 pragma 静默变串行，也不能仅开启并行后把线程内异常/数据竞争留给游戏。
完成条件为对应契约测试、实际编译/链接闭包、软件图像像素、异常路径及同机性能证据。
