# ImageOps 接口与验证

图像算法集中到 `cpp/core/imageops/` 的 `core_imageops` 静态库。公共头只使用 C++17 类型。
`ImageOps.cpp` 实现公共校验、输出缓冲区与最近邻 resize 的像素采样；`ImageOps_opencv.cpp`
保留其他算法和并行行任务调度。最近邻 warp 仍由 OpenCV 执行，不能把 resize 的替换扩展为所有最近邻路径已替换。
静态库的 PRIVATE 链接仍会进入最终链接闭包，不能据此宣称移除了 OpenCV。

## 调用边界

| 调用方 | 通过 ImageOps 执行 |
|---|---|
| `visual/RenderManager.cpp` | box blur、四种缩放、仿射与透视输出 |
| `visual/ogl/RenderManager_ogl.cpp` | 1/4 通道 CPU 缩放、透视矩阵求解 |
| `plugins/layerex_draw/blend2d/LayerExDraw.cpp` | 删除未使用的 OpenCV 总头；仍使用原 blend2d 绘制 |

视图借用调用方内存，支持 Gray8/四通道字节、非紧密正向 pitch 和裁剪起点；pitch=0 表示紧密排列。
四通道不转换颜色顺序或 alpha，不保存视图指针。Warp 输出独立持有像素，可移动、不可复制；
临时软件纹理只在输出仍存活期间使用它。尺寸、pitch、通道、边界、坐标、矩阵布局及异常契约见公共头注释。

与旧调用相比，非法参数现在明确拒绝：例如 blur 的输入输出尺寸不一致会抛异常，
避免旧 cv::Mat 在内部重新分配后没有写入调用方目标的行为。
软件采样参数钳制为 0..3，修复原上界允许 4 及负值造成的数组越界。
合法模式仍依次映射 Nearest/Area/Linear/Cubic。最近邻 resize 保留旧版两次 double 除法的舍入，
例如 6→34 的输出索引 17 仍取源索引 2，不能直接改为整数比例。四通道通过小块 memcpy 搬运，
支持未对齐的 ROI/pitch；恒等复制只写有效行像素，相同数据地址的恒等操作保持旧行为。
resize 的输入输出应使用独立缓冲区（相同视图恒等操作除外），不承诺部分重叠区域行为。

原生采样仍通过内部同步行任务接口使用 OpenCV 调度，保留旧分片与线程设置；不保留回调及借用像素。
纯串行大图版本曾测得回退，因此未采用。后续移除 OpenCV 前仍须迁移该调度接口，并完成其他算法替换。

## 独立构建

设置 `VCPKG_ROOT`，在新的构建目录运行；Windows 需 MSVC 开发环境，可添加 `-DWINDOWS=ON`
与 `-DVCPKG_TARGET_TRIPLET=x64-windows-static-md`，其他平台选择对应 triplet。

```sh
cmake -S . -B out/imageops -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DKRKR2_BUILD_APP=OFF -DBUILD_TOOLS=OFF -DENABLE_TESTS=ON -DKRKR2_TEST_SCOPE=imageops
cmake --build out/imageops --target imageops_tests
ctest --test-dir out/imageops -N
ctest --test-dir out/imageops --output-on-failure --no-tests=error
python scripts/check_core_module_deps.py
```

此组合在 project() 之前只选择 tests/imageops 两个 vcpkg features，不选择 foundation/engine。
Boost.Locale、fmt、spdlog、Oniguruma 已归入默认 foundation feature；原工具/字符串/归档和完整引擎
由 CMake 显式选择它，直接执行 vcpkg 时默认行为也保留。复用旧全量构建或安装目录不能证明最小闭包。
imageops 与工具同时开启仍会加入工具的基础依赖。

项目自身的独立目标只编译 `ImageOps.cpp`、`ImageOps_opencv.cpp`、`ImageOpsTest.cpp`；OpenCV 及其 port 所需系统库/host 工具仍存在。
只链接 core/imgproc 不会自动裁剪 vcpkg port 内的其他 OpenCV 模块构建，本批尚未调整该 port。
CI 检查实际 vcpkg status 没有 Boost、日志、存储、cocos 与 FFmpeg，并检查这三个编译源。
默认 full 测试也登记相同用例；这不代表 full 应用已构建。

## 像素对照

`tests/unit-tests/core/visual/ImageOpsTest.cpp` 有 13 个有效注册用例。测试中直接运行旧 OpenCV 调用作为参考，
在同一后端/平台逐字节比较输出及 padding/保护区，不跨平台承诺不同 SIMD 实现的输出相同。
macOS 的固定 vcpkg port 会禁用 OpenMP，测试仍使用相同本机后端对照，不将三平台视为同一并行配置。
除旧实现对照外，还检查手工确定的最近邻像素、alpha 中间值、独立输出所有权和非法输入无写入。

覆盖 1/4 通道、四种采样模式、放大/缩小/单像素/细长图、ROI/非紧密 pitch、奇偶与超大 blur 核、
REFLECT_101 与原位 blur、仿射/透视/退化点/零边界，以及 float 顶点到行优先 double 矩阵。
这组测试为后续小型算法替换提供可重复基线，尚不覆盖所有游戏尺寸、字体、混合模式、GPU 读回或插件画面。

最近邻新增 4 万组尺寸/通道/方向对照，覆盖浮点临界位置、未对齐裁剪、恒等复制、较大图与请求 1/3 线程的结果。
本机 Linux/GCC 14.2 Release 为 13 项用例、41,377 个断言通过；模块和测试启用 ASan/UBSan 的 Debug 构建同样通过，
启用泄漏检测。参考 OpenCV 库使用 Release、未启用 sanitizer，因此不声称检查了后端内部所有访问。
归档+XP3 的原 9 项 CTest 回归通过。跨平台进展见[实施记录](dependency-slimming-implementation.md)。

完整软件/OpenGL 消费者构建及脚本画面对照尚待完成；Android 和真实游戏未验证。
Windows 完整构建的具体进展见实施记录，不把独立图像测试作为完整引擎通过。后续继续按单算法替换；只有所有消费者和依赖声明都移除后，才能删除 OpenCV。

## 可选微基准

```sh
cmake -S . -B out/imageops -DKRKR2_BUILD_IMAGEOPS_BENCHMARK=ON
cmake --build out/imageops --target imageops_benchmark
# 单配置 Ninja 示例；多配置生成器需选择 Release 子目录
./out/imageops/tests/unit-tests/core/visual/imageops_benchmark 1 > imageops-benchmark-1.json
./out/imageops/tests/unit-tests/core/visual/imageops_benchmark 3 > imageops-benchmark-3.json
```

同进程比较与原渲染器等价的直接 OpenCV 调用和 ImageOps 接口，默认请求单线程，可用首个参数请求线程数；
预热后交替先后顺序；
每项 3 轮、每轮 100 次，报告每次操作的微秒。包含 warp 的矩阵求解和输出分配，非整帧性能。
[首批包装报告](reports/imageops-2026-11-02.json) 与[最近邻报告](reports/imageops-nearest-2026-11-02.json)
记录环境、后端构建及全部样本。最近邻增加 4K→1080p 大图，不以小图单线程结果替代大图并行测试。
本机 OpenCV 使用 SSE2、禁用 CPU dispatch，不代表发行版 AVX2/GCD 等后端性能；容器样本也有明显波动。
这些数据不能推断完整游戏帧率、构建时间或包体收益。

最近邻行为对照来源为 [OpenCV 4.7.0 resize.cpp](https://github.com/opencv/opencv/blob/725e440d278aca07d35a5e8963ef990572b07316/modules/imgproc/src/resize.cpp)
的 resize/resizeNN 路径；原生采样独立编写，未复制该文件的 SIMD 或循环实现。旧后端及其许可仍随 OpenCV 分发。
