# OpenCV 生产依赖剔除

Status: implemented

## 范围与结果

基于 `8cc26c6`（dev），本次工作树将剩余图像操作迁入独立 C++17 实现。
Nearest/Linear 保留原有公式与 x86 SSE2；Area、Cubic、blur、Affine/Perspective、LU 及行调度
不再调用 OpenCV。ARM Linear 也采用固定标量公式，删除原 OpenCV 回退。

- 删除 `ImageOps_opencv.cpp`，生产 `core_imageops` 只链接 `Threads::Threads`。
- 删除 vcpkg 的 opencv4 override、engine/imageops 中的 OpenCV 依赖及整个 overlay port。
  空 imageops feature 作为现有配置兼容入口保留。
- 默认测试只依赖 Catch2；原 16 项实时对照保存在默认关闭的 reference 开关中。
  可选 generator/benchmark 也只用于开发验证，不由默认清单安装 OpenCV。
- 三平台独立 CI、Windows 完整构建和 Android APK 检查改为拒绝 OpenCV；
  Android 包检查同时拒绝包内 OpenCV 共享库。这些工作流已修改并解析，尚未在远端运行。
- 移植来源为 OpenCV 4.7.0 提交 `725e440d278aca07d35a5e8963ef990572b07316`。
  图像与 lapack 文件保留 BSD 三条款式声明，matrix_decomp 使用 Apache-2.0。
  来源与修改说明见 [UPSTREAM](../../../../cpp/core/imageops/UPSTREAM.md)。
  五份原许可已保存并复制到 `assets/licenses/imageops/`；资源选择验证确认关闭管理 UI 时仍包含所有许可。

当前 API、命令和消费者说明见 [ImageOps](../../../../docs/subsystems/imageops.md)。

## 同机证据

环境为本次 Linux x86_64 云工作区，GCC Release，CMake/Ninja；未复用先前 Windows 构建产物。
Catch2 3.8.1 从固定 tag 构建。OpenCV 参考从上述原版源码单独构建 core/imgproc，
启用 OpenMP、默认 CPU dispatch，关闭 IPP/OpenCL/ITT/LAPACK/Eigen；不是重新运行旧 vcpkg port。
配置与原始日志在忽略的 `out/`，环境和产物/源哈希入库到[测量报告](../../../reports/imageops-removal-2026-11-20.json)。

### 默认独立目标

```sh
cmake -S . -B out/imageops-native -G Ninja -DCMAKE_PREFIX_PATH=$PWD/out/reference/install -DCMAKE_DISABLE_FIND_PACKAGE_OpenCV=ON -DKRKR2_BUILD_APP=OFF -DBUILD_TOOLS=OFF -DENABLE_TESTS=ON -DKRKR2_TEST_SCOPE=imageops -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build out/imageops-native --parallel 4
ctest --test-dir out/imageops-native -N
ctest --test-dir out/imageops-native --output-on-failure --no-tests=error
out/imageops-native/tests/unit-tests/core/visual/imageops_tests --reporter compact
python scripts/check_no_opencv.py out/imageops-native
```

上述配置、构建、测试退出 0：6 项，21,390 断言通过。
`compile_commands.json` 仅有 ImageOps.cpp、ImageOps_native.cpp、ImageOpsTest.cpp；
闭包检查无 OpenCV 编译参数或链接项；`nm -C libcore_imageops.a` 无 `cv::` 符号。
测试 exe 的 `ldd` 只有标准 C/C++/数学/系统动态库，无 OpenCV 或 OpenMP。
本机未运行 vcpkg 安装，检查器明确报告 `installed_status_checked: false`；
不能把空 ports 数组解释为已验证 vcpkg 安装闭包。清单/overlay 的删除另有源码检查，CI 将检查真实 status。

### 标量与 sanitizer

- `out/imageops-scalar` 使用单独仅含 Catch2 的 `out/catch-only` 前缀、同样禁用 OpenCV 查找，
  加 `-DCMAKE_CXX_FLAGS=-U__SSE2__`；构建与 CTest 均退出 0，6 项通过。
  这关闭手写 SSE2 路径，不代表禁用了编译器所有自动向量化。
- `out/imageops-sanitize` 使用 Debug、`-fsanitize=address,undefined -fno-omit-frame-pointer`，
  可执行链接同加 sanitizers。`ASAN_OPTIONS=detect_leaks=1 ctest ... --no-tests=error` 退出 0，6 项通过。
  两个生产算法源都插装；预构建 Catch2 未插装。
- 大图 601×419 blur 显式对比串行、并行和原位结果；行调度测试覆盖每行仅执行一次、
  调用线程/工作线程异常和同时调用。

### 原版对照与固定样本

`out/native-reference` 显式开启 `KRKR2_IMAGEOPS_OPENCV_REFERENCE=ON` 与 benchmark，
只在此开发构建中链接 OpenCV。CTest 注册并通过 22 项（默认 6 项 + 原有实时 16 项）。
未删除或放宽旧对照中的字节相等和矩阵精确相等要求。

固定样本由只链接 OpenCV 的 `GenerateImageOpsGolden.cpp` 生成，2,700 组覆盖四种 resize、
blur、两种 warp、1/4 通道、随机像素、短行尾部、极端轴形状、奇偶/超视图核、奇异变换及零边界。
每组在默认测试里分别请求 1/3 线程；有效输出以 FNV-1a 摘要对比，另检查源不变、padding 和前后保护区。
重新运行生成器并经 clang-format 20 后，结果与入库头文件逐字节一致。

首次扩大样本时，Area 的放大/混合方向有 42 次摘要检查失败（包含两种线程请求）。
定位到 `source/destination` 与旧 `1/(destination/source)` 的浮点临界差异；改为旧倒数顺序后全部通过。
没有修改旧参考输出或容差来绕过该差异。

### 其他检查

- `g++ -std=c++17 -O2 -Wall -Wextra -Werror -ffp-contract=off -c ImageOps_native.cpp` 通过。
- `python scripts/check_core_module_deps.py` 通过 11 个 foundation 目标约束；不代表 domain 图无环。
- 新闭包检查器的 4 项测试通过，包含安装/仅头/仅链接残留、缺失证据与目录名误报。
- 三个修改的 workflow YAML 解析通过；源码 clang-format 20、文档链接和 `git diff --check HEAD` 通过。

## 性能与取舍

报告保存最终同进程直接 OpenCV/独立实现的全部三轮数据，各 100 次，预热后交替顺序。
仅在所有构建与 sanitizer 结束后串行采样；早期与编译/测试重叠的探索结果不进入报告。
共享云机器仍可能有调度和频率波动，因此不据此宣称游戏性能或最终包体收益。

独立实现的模糊/warp 仍有明显运行时回退。已去掉首版每像素反射求余、重复格式/stride 校验，
预计算仿射列偏移、采用连续垂直累加及小核 32 位通道特化，继续保持固定输出。
本批完成依赖删除，不声称这些操作优于 OpenCV。blur 的完整中间缓冲为 O(宽×高×通道)，
也应在实际游戏大图场景继续评估内存。

最终样本（320×240 RGBA，μs/次，括号为新/旧耗时比）：

| 操作 | 1 线程 OpenCV→独立 | 3 线程 OpenCV→独立 |
|---|---|---|
| box_9x9_rgba | 129→776（6.03×） | 123→690（5.60×） |
| warp_affine_rgba | 396→1228（3.10×） | 423→1170（2.76×） |
| warp_perspective_rgba | 603→1929（3.20×） | 643→2120（3.29×） |

## 未验证边界与下一步

Windows/macOS/Android 完整应用、APK/设备运行、CPU/GPU 消费者画面和真实游戏本次未运行。
前轮完整 Windows/Android 构建只属于旧后端，不能当作本批证据。
ARM 统一公式相对旧 OpenCV 自动向量化/FMA 可能存在历史已知的 ±LSB 差异；
删除回退后不能宣称旧 ARM 输出全量逐字节保持。三平台 CI 会对同一固定样本核对确定性。

后续优先运行修改后的三平台独立 CI 与 Windows/Android 完整消费者构建，
再按自制脚本/真实游戏场景检查软件和 OpenGL 输出；性能继续针对 blur/warp 与行任务开销优化。
这些尚缺的验证与性能工作不改变本批已完成的生产 OpenCV 依赖删除事实。

## 后续性能与跨平台验收（进行中）

用户随后要求先完成性能、跨平台构建、自制软件/OpenGL 场景并提交同步；真实游戏回归按用户要求暂缓。
候选提交 `3fb5fd1` 已在独立分支运行 CI：三平台 ImageOps 均成功；完整 Windows 首次在 NuGet 工具路径解析处失败，尚未进入配置。
Headless 的 macOS archives 首次在滚动 CMake 对 Boost.Locale 的 source_group 检查处失败；其 ImageOps job 成功。
已调整 NuGet 获取方式，固定完整 Windows 的 MSVC 2022/CMake 3.31.1，archives 使用 CMake 3.31.1；等待新提交复验。

当前独立实现增加小核行环形缓存、SSE2 归一化/采样和分块透视坐标，并以常驻有界工作池替换 std::async。
额外 50,688 组白色小核对照发现旧向量区段与尾部差异，已按原 16 字节区段固定，扫描零差异；
396 组对应样本加入默认固定样本，总计 3,096 组。新增嵌套池任务检查，原 22 项对照、标量与 ASan/UBSan 再次通过。
旧报告仍描述首版剔除实现，不作为这轮性能/完整消费者的最终证据；待实际 CI/运行结束后补充新报告。

候选 `46da99a` 的 Headless 13 个 job 全绿，包含三平台 ImageOps（3,096 组样本）。
后续补充了 Cubic 的内部 SSE2 采样，Linux 原有 16 项实时对照和默认 7 项合计 23 项通过；
标量与 ASan/UBSan 各 7 项通过。自制场景同时检查图层 BMP 的 28 个样本、保护像素与实际 Cocos 帧颜色，
仅在显式测试驱动构建中记录实际渲染管理器。
Android 两种 ABI 的 `3fb5fd1` 首跑都完成全部源码及链接，后置检查误把 Gradle tools 的 compile_commands.json 当作第二个应用构建而失败；
现在按源码根、ABI 和 Debug 缓存身份筛选，与已通过的编译步骤保持一致。首轮未产出 APK，不据此声称 APK 通过。
