# ImageOps 接口与验证

CPU 图像操作统一通过 `cpp/core/imageops/ImageOps.h`。`core_imageops` 仅依赖 C++17 与系统线程库，
生产代码和 vcpkg engine/imageops features 均不再依赖 OpenCV；默认测试仅额外需要 Catch2。

`ImageOps.cpp` 实现视图校验、输出所有权、Nearest/Linear resize；`ImageOps_native.cpp`
实现 Area/Cubic resize、box blur、仿射/透视、矩阵求解及同步行调度。
后者提取改写自 OpenCV 4.7.0 的相关算法，已移除 Mat、HAL、IPP、OpenCL 与 OpenMP 框架。
来源提交、修改范围及 BSD/Apache-2.0 许可见 [来源说明](../../cpp/core/imageops/UPSTREAM.md)。
原版权/许可同时保存在源码旁和 `assets/licenses/imageops/`，随现有桌面/Android 资源规则分发。

## 消费者与内存契约

| 调用方 | 操作 |
|---|---|
| `visual/RenderManager.cpp` | blur、四种 resize、仿射与透视输出 |
| `visual/ogl/RenderManager_ogl.cpp` | 1/4 通道 CPU resize、透视矩阵求解 |
| `plugins/layerex_draw/blend2d/LayerExDraw.cpp` | 使用原 blend2d 绘制，无 OpenCV 引用 |

视图借用调用方内存，支持 Gray8/RGBA8、裁剪起点、未对齐地址和非紧密正向 pitch；
pitch=0 表示紧密行。四通道按原始字节独立处理，不转换颜色顺序，不预乘或反预乘 alpha。
Resize 输入输出应独立，仅相同视图的恒等请求支持原位；BoxBlur 支持原位操作。
Warp 返回独立持有紧密像素的可移动、不可复制缓冲区，零边界也写满；软件临时纹理只借用其生命周期内的数据。
非法尺寸、格式、pitch、采样模式和非有限变换顶点抛 `std::invalid_argument`。
内存和线程资源错误按标准 C++ 异常传播。

## 算法语义

- 采样模式顺序仍为 Nearest/Area/Linear/Cubic。Nearest 保留旧 double 两次除法的索引舍入，
  如 6→34 的位置 17 仍取源位置 2；恒等请求只复制有效行字节。
- Linear 使用中心坐标、Q11 系数及原 8 位垂直截断公式；2x2 整数缩小使用 `(和+2)>>2`。
  x86 保留行缓存和 SSE2；ARM 也使用相同独立标量公式。
- Area 同时缩小时使用面积权重，整数缩小使用整数累加；放大或混合方向缩放保留原 Area 的两点坐标公式。
  scale 保留先算目标/源比例再取倒数的顺序，避免整数边界处选错系数。
- Cubic 使用 A=-0.75、Q11 系数；8 字节组采用原基线 SIMD 的浮点垂直合成，尾部保留 Q22 整数公式。
  系数运算关闭浮点收缩，固定运算顺序。
- BoxBlur 归一化、中心 anchor（偶数核向下取整）、REFLECT_101；边界属于当前裁剪视图。
  小核使用 16 位行缓存和 Q23 归一化，保留旧 16 字节区段饱和与标量尾部行为；
  最多保留核高+1 行，原位请求串行滑动，其他重叠视图使用完整中间结果。
  大核保留两遍宽累加。
- Warp 保留正向矩阵、逆向采样、零常量边界。Nearest 采用旧坐标舍入，Linear/Area 采用 1/32 子像素，
  Cubic 使用 Q15 表与系数和修正；Affine 只使用前三个顶点。Perspective 顶点乘积先以 float 求值，再进入 double LU。
  奇异求解沿用零系数行为，透视矩阵末项为 1。

早期 macOS/ARM64 的 OpenCV 自动向量化/FMA 曾相对源码标量公式产生 ±LSB 差异。
现在统一采用固定公式，无 OpenCV 回退；不能据 Linux 对照宣称旧 ARM 后端逐字节一致。
当前平台验证范围见[移除记录](../../.agents/notes/implemented/simplification/2026-11-20-opencv-removal.md)。

## 线程行为

`SetThreadCount(0)` 自动选择最多 8 个线程；1 为串行，其他正数为上限。
小于约 128K 像素的任务串行，大图按 64K 像素与高度限制分片；线程数不会改变输出。
大图任务使用最多 7 个常驻工作线程和调用线程，按请求上限分片，避免每次创建线程。
并发调用各自等待自己的批次；嵌套行任务在当前线程串行执行，避免池内等待死锁。
调用同步完成，不保留回调或借用视图；工作线程异常在调用线程重抛，退出前等待所有任务结束。
不再读取 `cv::setNumThreads` 或 OpenMP 的线程设置。

## 独立构建与默认测试

设置 `VCPKG_ROOT`，选择平台 triplet，使用新目录验证闭包：

```sh
cmake -S . -B out/imageops -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DKRKR2_BUILD_APP=OFF -DBUILD_TOOLS=OFF -DENABLE_TESTS=ON -DKRKR2_TEST_SCOPE=imageops -DCMAKE_DISABLE_FIND_PACKAGE_OpenCV=ON
cmake --build out/imageops --target imageops_tests
ctest --test-dir out/imageops -N
ctest --test-dir out/imageops --output-on-failure --no-tests=error
python scripts/check_no_opencv.py out/imageops
python scripts/check_core_module_deps.py
```

未使用 vcpkg 时可通过 `CMAKE_PREFIX_PATH` 提供 Catch2 3。默认注册 7 项用例；
3,096 组独立生成的 OpenCV 4.7.0 固定输出摘要分别以 1/3 线程验证，包含所有操作、随机数据/尺寸、
短行尾部、单像素轴、非紧密 pitch、奇偶/大核、奇异矩阵和零边界。
另有手算像素、alpha、ROI/前后哨兵、原位/恒等、输出所有权、参数拒绝、线程异常与并发调用检查。
固定摘要只描述有效像素；保护区与源缓冲区单独断言。

项目只编译 `ImageOps.cpp`、`ImageOps_native.cpp` 和 `ImageOpsTest.cpp`。
闭包检查拒绝实际安装的 opencv ports、编译参数和生成链接元数据中的 OpenCV；
它不代替目标平台完整应用构建或包内动态依赖检查。

## 可选实时对照、样本生成与微基准

仅开发验证时，在独立目录通过 `OpenCV_DIR` 提供原版 4.7.0，默认 vcpkg 清单不安装它：

```sh
cmake -S . -B out/imageops-reference -G Ninja -DCMAKE_BUILD_TYPE=Release -DKRKR2_BUILD_APP=OFF -DBUILD_TOOLS=OFF -DENABLE_TESTS=ON -DKRKR2_TEST_SCOPE=imageops -DKRKR2_IMAGEOPS_OPENCV_REFERENCE=ON -DKRKR2_BUILD_IMAGEOPS_BENCHMARK=ON -DOpenCV_DIR=/path/to/opencv/lib/cmake/opencv4
cmake --build out/imageops-reference
ctest --test-dir out/imageops-reference -N
ctest --test-dir out/imageops-reference --output-on-failure --no-tests=error
```

额外注册 `ImageOpsReferenceTest.cpp` 中原有 16 项逐字节对照，默认 7 项仍全部运行。
`imageops_generate_golden` 只链接 OpenCV，不链接 ImageOps；其 stdout 经 clang-format 20 格式化后
对应 `ImageOpsGolden.h`。勿使用新实现生成自己的期望结果。

`imageops_benchmark 1` / `imageops_benchmark 3` 同进程交替比较原 OpenCV 调用和 ImageOps，
预热后每项三轮、每轮 100 次，输出原始 JSON；包括分配和矩阵求解，并非整帧性能。
历史报告保留原配置，新报告见[移除测量](../../.agents/reports/imageops-removal-2026-11-20.json)。
独立实现部分操作仍慢于 OpenCV，不能把依赖移除解释为游戏帧率提升；38 组前后配对测量见[性能收敛报告](../../.agents/reports/imageops-native-performance-2026-11-20.json)。
Windows/macOS/Android 完整应用和真实游戏需分别验证，旧版构建记录不代表本次已通过。
