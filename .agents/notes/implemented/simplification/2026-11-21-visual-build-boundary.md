# 移除 Visual 的直接 Cocos 构建依赖

Status: implemented

基于 dev `6b3c51b` 接续渲染器宿主服务隔离。本批让 `core_visual_module` 显式声明字体、
图像编解码、FFmpeg、xxHash 与原生 GL 依赖。生产代码验证版本为 `a1e264e`。

## 实现

- 内置字体由 Cocos 宿主沿用现有资源搜索。首次枚举同步借用字节，重新打开时复制到独立的可定位流。
- PVRv3 继续使用引擎已有文件头。CPU 的 PVRTC 解码经宿主调用现有解码器，输入尺寸、输出指针与位深传递保持一致。
- 虚拟屏幕维持 TJS 可见的 2048×2048 和桌面原点 (0, 0)。Linux GL 选择使用编译器的原生平台宏。
- 软件半行纹理继续使用相同种子和字节范围的 XXH32。独立端口固定于现有基线的 0.8.3。Cocos 的旧头文件安装到自己的子目录。
- 依赖守卫拒绝 Visual 源码及其 CMake 直接使用 Cocos。完整应用包含现有 Cocos 窗口、输入和呈现。

## 验证

- 本机 `platform` 42 项、`imageops` 7 项和 Python 5 项通过，clang-format 20.1.8、依赖守卫、文档链接和 diff 检查通过。
- Linux 用真实开发依赖对两套渲染器、PVR 加载器、字体和屏幕做无 Cocos 头的编译检查，两个宿主适配器另使用真实 Cocos 头检查。
- 对比原 Cocos 和独立 xxHash 的 327,760 个 XXH32 样本。用实际 Cocos 子目标的导出规则验证两份头可共存并由不同目标消费。
- 用真实 Cocos 解码器和合成块验证宿主适配器的 2 位和 4 位 PVRTC 输出。自制场景还检查 Noto Sans CJK 绘字、虚拟屏幕及两种 PVRv3 文件加载。
- 无窗口 CI [44853165246](https://github.com/2468785842/krkr2/actions/runs/44853165246) 共 13 个任务通过。该运行源码为 `8625bf9`，之后的改动只涉及完整应用路径和 Cocos 端口。

| 范围 | 结果与证据 |
|---|---|
| Linux 完整应用 | [44854511221](https://github.com/2468785842/krkr2/actions/runs/44854511221) 通过 15 项 Cocos 契约与八配置渲染测试。八份报告均为退出 0、无强停，二进制 SHA-256 为 `ecc37663c59dcddd8d0e45c5bee66c90e58bd98d8377d025fc6605ae1b8c7f2b`，产物中的 xxHash 许可与仓库文件一致。 |
| Linux 自制场景 | 用前一批 [44839400282](https://github.com/2468785842/krkr2/actions/runs/44839400282) 和本批的测试程序，在独立 Xvfb 会话启动同一场景。两者均退出 0，均报告 2048×2048，Noto Sans CJK JP 覆盖 932 个像素，PVRTC 2/4 位样本均为不透明白色。两次输出的字体 BMP 和脚本报告逐字节一致。BMP SHA-256 为 `713927baa8060f8893b39f35e65e42bb9e877a21698dae40496f2a118cd72b3f`。 |
| Windows 完整应用 | [44854570463](https://github.com/2468785842/krkr2/actions/runs/44854570463) 通过 15 项 Cocos 契约、八配置渲染测试和五组原生窗口交互。场景均退出 0 且无强停。编译清单中 54 个 Visual 编译单元均无 Cocos 头文件搜索路径，两份宿主适配源码各编译一次。 |
| Android ARM64 | [44855138588](https://github.com/2468785842/krkr2/actions/runs/44855138588) 通过 352 个项目源码的构建和 Debug APK 打包。下载的 APK 与构建清单 SHA-256 同为 `bd2c2e3401319d3656fb905567e8e1a5021ee9113c3596cbd66108c16488406d`，内有单一 ABI、四份 AArch64 ELF 和 xxHash 许可。54 个 Visual 编译单元均无 Cocos 头文件搜索路径。 |

## 失败与修正

Windows [44853164871](https://github.com/2468785842/krkr2/actions/runs/44853164871) 和 Linux [44853165508](https://github.com/2468785842/krkr2/actions/runs/44853165508)
在 vcpkg 安装时发现两个端口都安装 `include/xxhash.h`。将 Cocos 自带头移入其子目录并更新导出路径后，两平台均进入应用编译。
Linux [44853402264](https://github.com/2468785842/krkr2/actions/runs/44853402264)、Windows [44853402054](https://github.com/2468785842/krkr2/actions/runs/44853402054)
与 Android [44853402194](https://github.com/2468785842/krkr2/actions/runs/44853402194)
在去掉 PVR 的 Cocos 头后编译发现 PVRTC 解码声明缺失，改由显式宿主接口提供。
本机使用与 Visual 目标一致的平台定义编译时，还发现 Linux GL 依赖 Cocos 导出的平台宏，改用编译器宏并复测。

## 剩余范围

完整应用仍包含 Cocos 宿主和 PVRTC 解码器。完整 GL context 恢复、SDL3 迁移和性能测量待单独验收。
未运行商业游戏、Android 真机和 macOS 完整应用。
