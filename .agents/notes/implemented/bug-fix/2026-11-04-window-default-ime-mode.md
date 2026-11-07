# Window 默认输入模式与聚焦 Layer 优先级

Status: implemented

## 历史问题与复现

基于 `16f8468`：WindowIntf.cpp 的 Window.imeMode setter 调用 SetDefaultImeMode，
WindowImpl.cpp 的对应方法只有注释，不保存模式、不更新平台输入状态。
TVPWindowLayer::GetDefaultImeMode 返回初始化的 imDisable；Layer.imeMode + focus 则经过
LayerManager、DrawDevice、Window::SetImeMode 到真实 Cocos IMEDelegate。

文本模块提取首轮 `41931835667` 的场景仅设置 Window.imeMode=imOpen，没有字符回调而超时。
该失败不是新旧解码差异的证据，Cocos 转换对照在同轮已全部通过。
本批场景改用游戏编辑控件现有的 Layer 输入路径；没有把窗口默认模式能力宣称为已完成。

## 验收目标

本次按原 note 要求核对窗口默认模式、聚焦 Layer 模式、imDontCare 与失焦重置的关系，
以最小脚本记录属性读回、无 Layer 焦点时的提交、Layer 覆盖及取消焦点后的恢复，
同时处理通用窗口接口、Cocos 适配器和调用方，保留各平台键盘显示/隐藏分支。
以真实脚本与原生文本路径验收，不由测试驱动强制 attach。

## 2026-11-05 实施

源提交 `380e4ef`：通用 `iWindowLayer` 增加 SetDefaultImeMode(mode, reset)，
WindowImpl 经实际 DrawDevice 的 GetFocusedLayer 决定是否立即应用。
无聚焦 Layer 时默认模式立即生效；有焦点时只更新默认属性，焦点清除再经既有 ResetImeMode 应用。
Cocos 适配器缓存各窗口最近的具体模式，imDontCare 不再强制开启；
非活动窗口只保存模式，切窗、隐藏回退及失效回退时恢复目标模式，无活动窗口则关闭。
初始 imDisable 保持不变，Android 保留既有显示/隐藏键盘分支，本轮不修复 JNI 编码。

参考核对为 krkrz/krkrz `fd5c4baa6a2ef5978db1bd043634351f48667daf`：
`environ/win32/WindowFormUnit.cpp` 的 SetDefaultImeMode(mode, reset) 保存默认值并按 reset 应用；
`environ/win32/ImeControl.h` 明确 ModeDontCare 延续先前状态。
但该版本 `visual/win32/WindowImpl.cpp` 的脚本默认模式 setter 也仍有注释掉的转发，
不能把上游整个脚本链路宣称为已实现或以它作旧二进制行为对照。
本仓库独立实现接线和缓存，没有复制外部代码；只对照表单接口和已写明的模式含义。

扩展 `text/startup.tjs` 和 `text_repro.py`，在原 Unicode/无效输入/失焦场景前增加 13 阶段，
核对默认属性读回、无 Layer 焦点时的开启/关闭、两种 dontCare 延续、Layer 覆盖、
清除焦点恢复默认值，以及非活动窗口配置、切窗、隐藏和失效回退。
正向阶段等待实际 TJS onKeyPress；关闭阶段经过原生发键和实际绘制帧，最终严格比较完整回调。
测试驱动不强制 attach、不直接调用脚本事件。

本地 31 项 platform CTest、44 项 Python、11 个 foundation 依赖守卫、Python 编译与 diff 检查通过。
首次运行本机 clang-format-diff 包装脚本缺少 PYTHONPATH，退出 1；补上本次临时工具目录后成功，
仅格式化上述四个 C++ 文件的改动行。
## 远端验收

源提交 `380e4ef2d675db1aad0f210e85c4a0559bf6e310`：

- Windows [41941560054](https://github.com/2468785842/krkr2/actions/runs/41941560054)，
  job `126963390206`，completed/success。完整 Release 应用，测试驱动 ON，Mesa 软件 OpenGL，
  cocos2dx 3.17.2#12；11 项 Cocos CPU、44 项 Python 及五个真实交互场景全部成功。
- Headless [41941551333](https://github.com/2468785842/krkr2/actions/runs/41941551333)，
  13/13 job 成功，三平台 platform 各 31 项与 15 源闭包通过。

已下载轻量 artifact `13376853321`（71,831 字节）到 `out/windows-ci/41941560054/`，
ZIP SHA256 `02ceb717ba7fe014d4d41175e5fe84ee1653b1287771b356c7a090709c1305ce`。
五个场景均 passed=true、exit_code=0、forced_stop=false、failures=[]，exe/PDB 哈希相同：

- exe：`ee08dd666f30c9c56006bcf566b83ed27bbf5031a7f2173e69845ee72dd8cff7`。
- PDB：`d6659ea4162a2f52369fd04f640a9fbd00c0a1bb6a719762cfed3eebaf95c197`。
- 文本 fixture（Windows CRLF）：`5ce4440476274f5bf860084a431c9ac66fb0e9aa57e0dd88ff54034a4e6f1b1b`。
  本地 LF 源为 `af48b9ea1d72d2d82e545cfd40cf5f7dcb03c40afed4deb94fd688e711306433`；
  下载的实际脚本经行尾规范化与当前源逐字节相同，保留两种原始身份，不以 LF 哈希代替运行脚本。

13 阶段属性与焦点标记逐项匹配：默认开启 D、Layer 开启覆盖默认关闭 L、清焦恢复默认开启 R、
默认 dontCare 保留开启 C、非活动窗口关闭不干扰当前 H、隐藏次窗口恢复 B、
次窗口开启且后台主窗口设为禁用仍收到 S。默认关闭、关闭后的 dontCare、Layer 关闭、
Layer dontCare 延续关闭、次窗口关闭、失效回退到禁用主窗口均未出现 Q 回调。
之后旧 Unicode/NUL/无效输入/失焦/恢复场景全部保留并通过。
恢复前后两个主场景 PNG 均为 960×640，中心 (480,320) RGB (48,64,80)，
SHA256 均为 `ab612b7831f86c0d918ead9a9206c8371dd5bcfea8427a4701184b3def84403d`，
已查看恢复截图；关闭阶段的额外 capture 用作消息/绘制屏障，不作为所有切窗画面的像素验收。
完整共享 artifact `13376698756`（127,130,544 字节），服务端 SHA256
`78518f89f9f8c0e03f1ea86181cd017ba76116abfef373838eca8af6065505a1`，公共 runtime 保存一份二进制。

## 后续边界

只验收当前 Cocos 提交入口的粗粒度开关与脚本优先级。
当前后端把 imDisable/imClose 都映射为关闭提交，其他语言模式沿用开启分支，
并未新增 Windows IMM 语言/转换模式控制；IME 预编辑、候选窗及用户切换状态仍是独立范围。

本机 `command -v adb` 和 `command -v javac` 未找到可执行程序，本批没有 Android 构建或设备回归；
Android 编码问题继续见[独立调查](../../proposed/bug-fix/2026-11-04-android-text-encoding.md)。
