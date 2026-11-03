# 平台壳选型（SDL3）与子系统现状

Status: proposed

方案材料：本页包含待实施的接口或目录设计，不能作为当前构建与可用功能说明；事实以源码及 docs 为准。

状态：选型决策（2026-11-02）。平台壳（窗口 / 输入 / 主循环）选定 **SDL3**，
作为[后端精简路线](2026-11-03-backend-modernization.md) B3 批次的实现目标。
本文件只记录决策与现状调查，**尚未实现任何 SDL3 代码**；当前后端仍是 cocos。
现状核对基于 dev 工作区（基线 `b3765f7` 及未提交的 B2 拆分）。

## 决策摘要

- **平台壳选 SDL3**：桌面（Windows/Linux/macOS）与 Android 共用同一库承担窗口、
  GL context、输入（键盘/鼠标/触摸/手柄）、主循环与生命周期事件。
- **ANGLE 不是 cocos 的替代品**：它是 GLES→D3D11/Vulkan/Metal 翻译层（EGL 实现），
  不提供窗口、输入或 UI，与 cocos2d-x 不在同一层。它与窗口库正交，可与 SDL3 组合；
  引入决策延后到 macOS 阶段，不与壳替换同批验收。
- **音频与渲染本体不在替换范围**：sound 模块与 cocos 零耦合（OpenAL/Oboe 输出），
  GL 渲染器为自研（`visual/ogl/RenderManager_ogl.cpp`），替换的只是 cocos 承担的“应用壳”。

## 现状：四个子系统怎么工作

### 窗口

- OS 窗口与 GL context 由 cocos 创建：`environ/cocos2d/AppDelegate.cpp:58,63`
  `GLViewImpl::createWithRect/create`（cocos 桌面内部为 GLFW）。入口
  `platforms/windows/main.cpp:51` 等调用 `TVPAppDelegate->run()`，主循环归 cocos `Director`。
- 引擎“窗口”不是 OS 窗口，而是场景内节点：`environ/impl/TVPWindow.h:15`
  `TVPWindowLayer : cocos2d::extension::ScrollView, iWindowLayer`，由
  `visual/impl/WindowImpl.cpp:1130` 经 `TVPCreateAndAddWindow` 创建（实现在
  `environ/cocos2d/MainScene.cpp`）。多窗口 = 同一 OS 窗口内的节点切换；
  `visual/WindowLayer.h` 中位置、全屏、置顶均为哑实现。

### 输入

- 全部输入经 cocos EventDispatcher 进入：键盘/触摸/手柄监听注册于
  `environ/cocos2d/MainScene.cpp:460,466,479`，鼠标监听在 `TVPWindowLayer::init`
  （`environ/impl/TVPWindow.cpp`）。
- 键码经 `CCKeyCodeConv` 转 Windows VK；触摸合成鼠标（单指左键/双指右键/三指中键，
  `TVPWindow.cpp` 的 `onTouch*`）；手柄走同一 VK 路径。
- 统一入队 `TVPPostInputEvent`（`cpp/core/event/EventIntf.cpp:642`），经
  `TVPDeliverAllEvents` 派发到 TJS 回调；**该队列及派发管线与 cocos 无关，可整体复用**。
- IME：`TVPMainScene` 实现 `IMEDelegate`；Android 走 JNI（`environ/android/AndroidUtils.cpp`
  → `KR2Activity.showTextInput`），Java 侧 `DummyEdit.java`、`SDLInputConnection.java`、
  `ShowTextInputTask.java`（`platforms/android/app/java/org/tvp/kirikiri2/`）源自 SDL。
- Android 输入：Java 触摸/按键 → JNI（`platforms/android/cpp/krkr2_android.cpp`）→
  **重新注入 cocos 管线** → 与桌面同路。换 SDL3 后可直接进引擎队列，路径反而变短。

### 音频（替换壳不涉及）

- 输出后端：桌面 OpenAL（`sound/win32/WaveMixer.cpp:682` `alcOpenDevice`，
  `:728` `CreateAudioRenderer`）；Android 先 Oboe（`:731`），失败回退 OpenAL。
- 解码：FFmpeg + libvorbis + libopusfile + 内建 RIFF（`sound/WaveIntf.cpp` 注册顺序
  FFmpeg→Opus→RIFF→Vorbis）；自有解码线程（`sound/win32/WaveImpl.cpp` 的
  `tTVPWaveSoundBufferDecodeThread`）。
- MIDI/CDDA 仅 Windows（`sound/win32/MIDIImpl.cpp`、`CDDAImpl.cpp`，条件编译）。
- sound 目录不含任何 cocos 头文件与链接目标（`cpp/core/sound/CMakeLists.txt`）。

### 图形渲染

- GL 渲染器为引擎自研：`visual/ogl/RenderManager_ogl.cpp:2436`
  `TVPRenderManager_OpenGL`（注册名 "opengl"），自编 shader/FBO/数十种混合与转场方法；
  shader 为 GLES2 级（`visual/ogl/ogl_common.h:24`），桌面走兼容 profile + GLEW。
- 软件渲染器（`visual/RenderManager.cpp`，注册名 "software"）是配置默认
  （`environ/ConfigManager/PreferenceDefaults.inc:5`）。
- cocos 在渲染链只剩：GL 状态缓存工具、`AdapterTexture2D` 纹理包装、最终
  `UpdateDrawBuffer`（`environ/impl/TVPWindow.cpp:472`）把合成纹理贴到一个 Sprite 上屏；
  视频走独立 `environ/cocos2d/YUVSprite` 节点，不经过 Layer 树。

## cocos 实际承担面（即替换清单）

窗口与 GL context、主循环（`Director`）、输入分发、最终 blit、UI 控件
（`environ/ui/` 菜单/文件对话框/管理界面）、视频 `YUVSprite`、
Android `Cocos2dxActivity` 壳与 JNI 输入桥，以及[当前架构](../../../../docs/architecture.md)
“依赖事实”所列的 UI、配置、文件后端等辅助路径。

## 选择 SDL3 的理由

1. **IME 是最大工作量与风险项**：krkr 游戏为日/中文视觉小说，文本输入离不开 CJK IME。
   GLFW 无 preedit/候选窗定位支持；而现有 Android IME Java 文件本就抄自 SDL
   （`SDLInputConnection.java` 等命名即为证据），选 SDL3 可回归上游维护。
2. **单库覆盖桌面三平台 + Android**：窗口、输入、手柄、电源/生命周期事件统一，
   避免桌面 GLFW + Android 自拼 GLSurfaceView/JNI 两套壳。
3. **与 ANGLE 可组合**：SDL3 支持以 ANGLE 作为 GLES 后端，macOS 阶段决策不受本批约束。
4. **现有引擎侧逻辑可平移**：触摸合成鼠标、虚拟鼠标、VK 派发管线均在引擎代码内，
   不依赖 cocos 特有机制。

手柄是附带收益而非主因：GLFW 3.3 起也有基础 gamepad API（映射表本身源自 SDL 的
gamecontroller DB），krkr 的手柄接口（`TVPGetJoyPadAsyncState`、`_scancode` 位图）
在引擎侧，接哪个库都行；SDL3 的差异在于更完整（震动、扳机、触摸板、热插拔、
Steam Input 集成）且桌面/移动行为一致。

## 备选方案对比

| 方案 | 优点 | 不选原因 |
|---|---|---|
| 全 SDL3（已选） | 一套窗口/输入/事件循环抽象，一个 `iWindowLayer` 实现、一张键码→VK 表；IME 全平台完整；Android 壳/生命周期/JNI/IME 全承担 | — |
| 桌面 GLFW + 移动 SDL3 | 桌面二进制略小（GLFW 静态几百 KB vs SDL3 约 1–2MB，差距 <2MB，相对 FFmpeg 等占比可忽略）；GLFW API 极简 | 两套窗口/输入/键码/IME 路径，B3 表面积约翻倍；GLFW 无 IME preedit 与候选窗定位，恰在最需要 CJK 输入的桌面端最弱；手柄两端行为不一致 |
| 纯 GLFW | — | GLFW 不支持 Android，方案不成立 |

体积补充：SDL3 构建期可关闭无关子系统（本项目音频已用 OpenAL/Oboe，SDL audio 可关），
进一步缩小差距；体积差不是本选型的决策因素。

## 工作量评估（基于上述现状，非实施结论）

| 任务 | 评估 | 说明 |
|---|---|---|
| 音频 | 零改动 | 已无 cocos 耦合 |
| 渲染本体 | 零改动 | 自研；`iTVPRenderManager` 抽象已在 |
| 最终 blit + YUV 呈现 | 小 | 全屏 quad + viewport 策略；`YUVSprite` 逻辑平移为 [krkr2-render](2026-11-03-ui-rendering.md) 的 Compositor/YUVRenderer |
| 桌面窗口 + 主循环 | 小-中 | SDL3 数天级；DPI、全屏、letterbox 中等 |
| 鼠标/键盘/手柄接入 | 小-中 | `TVPPostInputEvent` 之后管线已自有；新增 SDL 键码→VK 转表 |
| CJK IME/文本输入 | 中-大 | SDL3 覆盖大半；仍需逐平台验收 preedit、候选窗、组合键 |
| Android 壳替换 | 中-大 | SDL3 Activity/EGL、surface 重建（渲染器已有 `EVENT_RENDERER_RECREATED` 重建逻辑）、JNI 输入/IME、暂停恢复；现有 JNI 桥“注回 cocos”可改为直入引擎队列 |
| 游戏 UI 对话框/菜单 | 中-大 | 脚本请求的功能不能静默丢弃；原生对话框或自绘，独立批次 |
| 多 OS 窗口 | 无需对齐 | 当前实现已是单 OS 窗口内节点切换，维持现状 |

总体：替换 cocos 是“平台壳”工程而非渲染器工程；风险集中在 IME 与 Android 生命周期。

## 与批次衔接

- B2（接口拆分）进行中：`visual/WindowLayer.h` 已不暴露 cocos 类型，SDL3 后端实现
  `iWindowLayer` 即可接入。
- B3 按本选型执行：先做桌面最小运行器（实际脚本驱动窗口、输入、画面，非清屏示例），
  Android 随 B5 处理；验收沿用[后端精简路线](2026-11-03-backend-modernization.md)的兼容验收重点
  （IME、DPI、letterbox、焦点丢失按键释放、后台恢复等）。
- vcpkg 清单当前未声明 SDL3，但 `vcpkg-configuration.json` 所 pin baseline
  （`aa40adda`）已含 `sdl3` port（3.4.8），B3 动工时加入 `vcpkg.json` 即可，无需 bump baseline。
