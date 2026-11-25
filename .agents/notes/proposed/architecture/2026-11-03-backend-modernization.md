# 后端精简与 AI 可执行任务

Status: proposed

方案材料：本页包含待实施的接口或目录设计，不能作为当前构建与可用功能说明；事实以源码及 docs 为准。

状态：迁移提案。目标是降低 cocos2d/OpenCV 等依赖和构建范围，保持吉里吉里脚本、图层、存储及插件行为。
Windows 已有 SDL3 独立构建，Linux 有可选 SDL3/Cocos 双宿主；其他平台继续使用 Cocos。平台壳选型及早期现状见
[平台壳选型](2026-11-03-ui-platform-shell.md)。Windows 当前实现与验证见
[独立宿主记录](../../implemented/testing/2026-11-24-windows-sdl3-independent.md)。ANGLE 为 GLES 翻译层而非 cocos 替代，
与窗口库正交，延后到 macOS 阶段决策。
最小工具、字符串、归档测试及 ImageOps 接口/像素测试的构建隔离已有实现，进展与验证边界见[依赖瘦身实施记录](../../implemented/simplification/2026-11-01-dependency-slimming.md)。
2026-11-20：OpenCV 生产依赖已剔除，B4 算法/线程与默认独立测试已迁移；跨平台完整消费者和游戏验收仍按范围单独核实，见[移除记录](../../implemented/simplification/2026-11-20-opencv-removal.md)。
2026-11-20：纹理呈现首批已隔离公共画面描述、CPU上传与宿主截图，保留Cocos后端；GL状态/context边界仍待拆分，见[实施记录](../../implemented/simplification/2026-11-20-render-presentation-boundary.md)。
此轮优先精简平台/图形后端，TJS2 与 Ciallang 替换另设任务、另行验收。
独立待办、最小验证批次及初步工作量见 [Ciallang 接入任务](2026-11-25-ciallang-tjs-runtime.md)，当前仅登记，尚未实施。

2026-11-25 后续：本次分支暂停 Android/Apple，仅保留 Windows/Linux SDL3，删除 Cocos port、宿主、管理 UI 与旧构建路径。
本次改动和验证见[桌面收敛记录](../../implemented/architecture/2026-11-25-desktop-only-cocos-removal.md)；下文 Cocos 过渡方案与旧平台顺序属于历史背景。

## 目标边界

```text
游戏逻辑：TJS2 / KAG / Layer / 存储 / 音视频 / 插件
                        ↓
窗口与输入接口 / 画面呈现接口 / 文件与平台服务
                        ↓
cocos 适配（过渡） / 新平台适配（候选）

模拟器管理 UI → 可选应用壳，不能反向成为基础存储依赖
```

先复用现有软件/OpenGL 渲染行为，再拆最终呈现；不能把所有 Layer 混合和转场直接映射到普通纹理 blit。
OpenCV 替换独立为图像算法模块，保留插值、border、pitch、alpha、舍入及异常输入契约。
脚本请求的菜单、对话框和窗口功能与启动器/偏好 UI 分开处理；可选 UI 不等于可以静默丢弃游戏功能。

## 可独立交付的批次

| 批次 | 任务 | 完成证据 |
|---|---|---|
| B0 基线 | 记录构建阶段耗时/依赖与最小自制游戏场景 | 可复测的环境、日志、截图及事件/音视频检查项 |
| B1 构建隔离 | 使工具/核心验证可独立配置；按实际引用拆 vcpkg features，修正生成规则 | 不安装 cocos/OpenCV/FFmpeg 等无关库即可配置并构建明确的最小目标；完整构建仍可用 |
| B2 平台契约 | 清除公共接口中的 cocos 类型，拆窗口、输入、主循环、文件及生命周期 | cocos 适配仍通过基线；核心公共头不再暴露 cocos 类型 |
| B3 后端原型 | 按已定 SDL3 实现最小运行器，桌面先行、Android 随 B5；IME、手柄、生命周期为验收重点 | 实际脚本驱动窗口、输入和画面，不仅是清屏示例；记录不支持能力 |
| B4 图像算法 | 收敛 OpenCV 调用，分算法替换 | 像素对照覆盖缩放/模糊/仿射/透视；CPU 读回与插件消费者验证 |
| B5 移动与媒体 | 拆 YUV 呈现、Android 资源/JNI、暂停/恢复及设备处理 | 真机图像、音视频、IME、权限/资源访问和关闭回归 |
| B6 移除与测量 | 删除已无消费者的依赖、UI 资源和打包规则 | 完整依赖闭包检查、同机前后构建报告、游戏兼容报告 |

每批可按单接口或算法继续拆小。早期保留旧后端用于对照，是否切默认由能力与回归结果决定；后端原型完成不意味着可删除旧实现。

## 兼容验收重点

- 输入：物理键与文本输入分开、中文/日文 IME、鼠标/触摸坐标、DPI 与 letterbox、焦点丢失和按键释放、手柄与事件顺序。
- 窗口：脚本尺寸、缩放、全屏、多窗口/模态需求、退出回调、移动端后台恢复。
- 渲染：alpha/混合模式、裁剪、纹理寿命、软件和 GPU 读回、转场、帧同步；GPU 插值不能自动替代可读回的 CPU 图像结果。
- 服务：移动端打包资源、存档路径、文件选择和权限、剪贴板、视频纹理及音频设备所有权。

“更轻”需分别比较依赖安装、编译耗时、二进制大小、运行内存和平台维护量，不能用头文件数量代替测量。

## 给下一位协作者的任务格式

```text
目标：解决哪个具体场景，允许改哪些模块。
基线：提交、平台、复现命令/输入、现有行为与耗时。
契约：必须保持的脚本/像素/事件/资源所有权行为。
实现：最小改动和涉及的 CMake/vcpkg/资源项。
验收：相关构建、测试、手动步骤及成功条件。
交付：变更、测量、未验证平台、已知限制和下一个独立任务。
```

外部经验与适用限制见 [参考资料](../../../../docs/reference/upstream.md)，本地结构见 [当前架构](../../../../docs/architecture.md)。

2026-11-20续批：GL状态缓存/视口/诊断与重建订阅已迁入宿主接口，三端完整构建和桌面自制场景通过，
见[GL宿主边界](../../implemented/simplification/2026-11-20-gl-host-boundary.md)。完整context资源恢复仍待定义与验证。

2026-11-20续批：软件/OpenGL 配置、内存压力、日志、提示框与本地化访问已迁入宿主适配；
Windows/Linux 八配置矩阵、三平台42项独立契约与 ARM64 完整 APK 通过。
见[渲染器宿主服务隔离](../../implemented/simplification/2026-11-20-render-host-services.md)。
2026-11-21 续批已移除 `core_visual_module` 对 Cocos 的直接构建依赖。字体资源经宿主接口读取，
窗口、截图与 GL 状态仍由 Cocos 宿主提供，完整 context 资源恢复待处理。见[实施记录](../../implemented/simplification/2026-11-21-visual-build-boundary.md)。

2026-11-23 后续以 [Linux 纯 SDL3 依赖闭包](../../implemented/architecture/2026-11-23-cocos-free-runtime.md) 为目标推进。
首批拆分配置/语言读取与 Cocos 控件绑定；续批完成通用资源入口、本地读取器和独立 LibXml2 支持，
Windows/WSL 独立测试、Windows 完整应用及双渲染器自制场景通过。
再续批拆出全局/游戏/语言配置管理器及宿主设置存储，Windows/WSL 各 20 项独立测试通过；
路径包装已从 MainScene 迁出，Cocos 移动端权限写入仍由适配器保留。
剩余平台服务、脚本 UI 和最终纯 SDL3 构建闭包尚未完成。

2026-11-24：Windows 独立宿主已完成真实游戏短剧情/存读档/正常退出，Linux 独立应用及 KAG/媒体自制场景也已完成。
当前合并使用 `KRKR2_SDL3_ONLY`、共享宿主服务与兼容 PVRTC 解码；详见上述 Linux 实施记录和
[Windows 实施记录](../../implemented/testing/2026-11-24-windows-sdl3-independent.md)。
