# OpenCV 仅构建 ImageOps 所需模块

Status: implemented

## 决定与来源

2026-11-14 从 `519f113` 接续依赖精简。生产 OpenCV 调用已集中于
`cpp/core/imageops/ImageOps_opencv.cpp`，`core_imageops` 只链接 core/imgproc，
但原 vcpkg port 仍默认编译多个未使用模块。本批通过 opt-in `imageops` feature
设置 `BUILD_LIST=core,imgproc`，engine 与独立图像测试共同选择它；不修改图像算法。

实现提交 `1a899bb`；Windows 补丁字节修复 `6542a12`。完整 port 保留固定的
OpenCV 4.7.0#6、原补丁和上游可选功能。精简 feature 与额外模块 feature 混用时明确失败，
未选择该 feature 的消费者保留上游行为。原始 port tree、blob、许可见
[vcpkg 来源](../../../../vcpkg/ports/opencv4/UPSTREAM.md)。已逐文件比对 15 个补丁、usage，
portfile 仅增加 include，manifest 除新增 feature 外语义相同。

Windows 首轮 `43550163194` 的 imageops job 与完整构建 `43550171248` 在应用
`0009-fix-protobuf.patch` 时失败：checkout 将补丁转为 CRLF，`git apply` 报第 9 行损坏。
使用局部 `.gitattributes` 的 `*.patch -text` 沿用上游 vcpkg 规则；
本地复现 CRLF 失败，并以 `core.autocrlf=true checkout-index` 确认全部 15 个补丁字节不变。
未改补丁内容，也未删掉失败用例。

## 验证

`python scripts/check_core_module_deps.py` 通过，范围仍限工具声明的显式依赖和直接包含。
`python -m unittest discover -s scripts/tests -v` 7 项通过：平台库文件名、Android ABI 子目录、
缺失库、残留额外库、导出/头不一致、未完成安装和错误 port 版本。CMake 独立检查确认
core/imageops/openmp 接受，imageops+dnn 和 imageops+default-features 拒绝，无 imageops 时不增加 BUILD_LIST。

Headless workflow `43552112375`，源码 `a824bd4`：全部 13 个任务通过，
其中三个平台各 15 项图像 CTest 和 7 项检查器测试通过，
实际依赖均为 Catch2/OpenCV/zlib 与三个 vcpkg host helper ports，实际导出、头文件和库均只有 core/imgproc。

| 平台 / job | 安装的 OpenCV 库字节数（Debug + Release，不是最终包体） |
|---|---:|
| Windows x64 / `132316695455` | 509,968,366 |
| Linux x64 / `132316695360` | 242,708,204 |
| macOS ARM64 / `132316695468` | 101,773,912 |

Windows 完整应用 workflow `43550427013` / job `132311399509`（源码 `6542a12`）通过：
构建 krkr2 与 cocos_window_tests，确认软件/OpenGL 渲染器及两个 ImageOps 源各编译一次，
最终链接成功，11 项节点/坐标/焦点/文本契约全部通过；实际 OpenCV 仍只安装 core/imgproc。

### 同机前后对照

本机 Debian 13 x86_64 / GCC 14.2.0；使用项目 out 下独立 bootstrap 的 vcpkg 工具
`2026-04-08-e0612b42ce44e55a0e630f2ee9d3c533a63d8bc1`，同一固定 registry baseline。
两个独立安装根仅在 opencv4 的 features 上不同：baseline 为 core/openmp，精简为 core/imageops/openmp。
实际导出模块由 13 个降为 2 个；Debug + Release OpenCV 静态库字节数由
503,659,266 降为 243,021,534（减少 51.75%）。不含其他依赖、头文件或最终应用，不能用于推算包体。

核对两次配置的 CPU baseline=SSE3、dispatch=SSE4_1/SSE4_2/AVX/FP16/AVX2/AVX512_SKX、
WITH_OPENMP=ON、Release `-O3 -DNDEBUG` 和 Debug `-g` 一致。未调整 CPU 或浮点求值选项。
两个独立项目构建均只编译 ImageOps.cpp、ImageOps_opencv.cpp、ImageOpsTest.cpp，
各实际注册并通过 15 项 CTest（`ctest -N` 后 `ctest --output-on-failure --no-tests=error`）。

临时 probe 使用固定 mt19937 seed 8675309，1/4 通道、请求 1/3 线程各 80 个随机尺寸，
源 offset=3、pitch=width*channels+13，覆盖四种 resize、恒等复制、仿射/透视、奇偶 blur 和原位 blur。
320 组源视图共产生 7,680 张输出图，逐字节相同，共 122,736,336 字节；
SHA256 `cd0aaa1f3f8d2a1fb52d31348bdbfe1dbe995fd118acc8bbb120c2d9176e9d9c`。
probe 及本机原始日志位于忽略的 `out/opencv-profile/`，不是新增的长期测试入口；
稳定回归入口仍为 ImageOpsTest.cpp 和 CI。环境重启后 out 可能消失，必要数值及 probe 源哈希保存在
[测量报告](../../../reports/opencv-imageops-profile-2026-11-14.json)。
本地早期编译进程曾中断，重新完整编译后才执行本次验收，不报告编译耗时收益。

### Android

首轮 ARM64 `43550178520` / x86_64 `43550174954` 的原生源码和最终链接成功，
随后检查器错误地只扫描桌面 lib/bin 路径，拒绝了实际位于
`[debug/]sdk/native/staticlibs/<ABI>/libopencv_{core,imgproc}4[d].a` 的库。
从 port install 日志定位后，`a824bd4` 改为遍历安装根的实际 OpenCV 库并加入 Android 路径回归；
保留相同的导出/头/版本/安装状态校验。首轮没有完成 APK 打包，不能据此宣称 APK 通过。

重跑源码均为 `a824bd4`，两个 workflow 全部成功：

| ABI | workflow / job | Debug APK 字节数 |
|---|---|---:|
| arm64-v8a | `43552134707` / `132316766970` | 58,550,086 |
| x86_64 | `43552222732` / `132317034109` | 60,242,046 |

实际检查 346 个项目源码，完整应用、PSD 开启，测试/工具/崩溃上传/UI driver 关闭。
两种 ABI 都只安装 OpenCV core/imgproc；其库合计分别为 208,267,442 / 286,287,708 字节（Debug + Release）。
APK 由 check_android_runtime_apk.py 校验项目配置、依赖闭包、包内 ELF 架构与 NEEDED 依赖，
均包含 libc++_shared.so、libkrkr2.so、liboboe.so、libomp.so，无未打包的非系统依赖。
APK SHA256（不是前后包体对照）：

- ARM64：`e943e99f091ab107b6fe98fe2231cc056b6163eb0fafa17b30d679a43b7bb096`
- x86_64：`4905669f9c643f491c9aec93e2c53e592a047514092bc2ab6985d49bba9ff597`

具体配置、库哈希及动态依赖保存在测量报告的 ci.android 中；产物可从对应 workflow 下载。
没有在模拟器或真机上运行游戏，不将 APK 构建通过作为 Android 游戏兼容结论。

## 边界与下一步

此批仅减少依赖模块的构建、安装范围。OpenCV 并未移除：Area/Cubic、blur、warp、
矩阵求解及并行调度仍使用它，ARM Linear 保留原回退。静态链接原本只引用两个模块，
不能将安装库体积减少直接解释为 APK/exe 同比例变小。

测试不是游戏画面验收，也不证明字体、GPU 读回或全部插件场景兼容。
下一批可独立推进原生 box blur；先固定 REFLECT_101、偶数 anchor、原位与 pitch 行为，
同机像素和性能通过后再替换，仍保留其他 OpenCV 后端。
