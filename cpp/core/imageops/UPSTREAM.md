# ImageOps 算法来源与许可

`ImageOps_native.cpp` 按 OpenCV 4.7.0 的以下实现提取并改写，来源提交
[`725e440d278aca07d35a5e8963ef990572b07316`](https://github.com/opencv/opencv/tree/725e440d278aca07d35a5e8963ef990572b07316)。

| 来源文件 | 使用范围 | 许可文件 |
|---|---|---|
| `modules/imgproc/src/resize.cpp` | Area 权重、Cubic 系数与定点/浮点舍入 | `LICENSES/OpenCV-resize.txt` |
| `modules/imgproc/src/imgwarp.cpp` | 变换方程、坐标量化、Q15 采样表与矩阵逆 | `LICENSES/OpenCV-imgwarp.txt` |
| `modules/imgproc/src/box_filter.simd.hpp` | REFLECT_101 滑动累加及小核定点归一化 | `LICENSES/OpenCV-box-filter.txt` |
| `modules/core/src/matrix_decomp.cpp` | 小矩阵的部分主元 LU 与计算顺序 | `LICENSES/Apache-2.0.txt` |
| `modules/core/src/lapack.cpp` | 3x3 矩阵求逆 | `LICENSES/OpenCV-lapack.txt` |

图像算法与 lapack 来源文件保留 BSD 三条款式文件头，版权与条款逐字保存在上述文件。
OpenCV 顶层及矩阵求解源使用 Apache-2.0；原版顶层 LICENSE 完整保留。
该提交根目录没有 NOTICE 文件。分发包含这些实现的二进制时，须一并提供此目录的 LICENSES 文件。
这些移植部分保留各自许可，仓库其他代码的许可不因此更改。

2026-11-20 修改：仅支持现有 Gray8/RGBA8 借用视图；移除 cv::Mat、HAL、IPP、OpenCL、
插件分派、FilterEngine 与平台线程框架；以标准 C++17 内存、数组和同步行任务代替。
保留现有中心坐标、float 顶点乘积、固定点系数、复制/零边界及退化变换语义。
原先 x86 Linear 的独立实现继续使用，ARM 也使用相同确定性公式。
Cubic 明确定义 8 字节组的浮点合成和标量尾部，禁止浮点收缩，避免编译器改变系数求值。

固定输出样本由原版 OpenCV 4.7.0 独立生成，生成器与可选实时对照仅属于测试工具。
生产目标不编译、链接或安装 OpenCV。
