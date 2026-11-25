# Ciallang 可切换脚本后端接入与 TJS2 替换

Status: proposed

## 任务与边界

2026-11-25 用户要求先登记任务，尚未授权启动本方案实施。
本任务独立于 Cocos 后端精简、依赖审计 PR #63；当前不改默认脚本后端或运行时。

第一阶段目标：由 Ciallang 执行游戏脚本，保持既有 TJS 原生接口、游戏和插件可观察行为，
保留原 TJS2 后端用于按进程选择和差分。完全删除 TJS 类型、绑定层或旧插件 ABI 属于后续独立范围。

## 已知事实与证据

宿主调查基线为 dev `b6cb999`；调查时工作区分支 `chore/dependency-audit`，HEAD `4c51186`。
以下为源码/文档核对，没有构建 Ciallang，也没有做双后端 KAG/游戏验证。

- `cpp/core/base/ScriptMgnIntf.*` 直接初始化 tTJS，暴露 script/expression/storage/bytecode、
  context、源码名和行偏移入口；`TVPGetScriptEngine()` 返回具体 tTJS 指针，不能只替换一个 eval 调用。
- `tjs2/tjsInterface.h` 是属性、调用、创建、枚举、Operation、原生实例、失效和错误码协议。
  `tjsVariant.h` 的对象闭包分别保存 Object 与 ObjThis，二者身份和持有必须保留。
- 扫描 cpp 下 C/C++/Objective-C++ 源/头（排除 tjs2、external）发现约 170 个文件直接使用
  tTJSVariant、165 个使用 iTJSDispatch2、64 个使用 tTJSNativeClass/Instance；这是文本扫描范围，
  不等于当前平台实际编译数量，也不能直接换算工期。
- `base/impl/SystemImpl.cpp`、`plugins/dirlist.cpp` 等会获取具体 tTJSArrayNI，
  普通 Dispatch 代理不能冒充其布局。数组/字典逐次复制会破坏身份、别名和共享修改语义。
- 已查阅个人仓库 [2468785842/Ciallang](https://github.com/2468785842/Ciallang)，
  不是组织仓库同名旧版本。其当前资料声明已有 C++20 嵌入 SDK、源码执行、部分 TJS2 字节码转译、
  UTF-16 码元语义及 TextStorageReader；接入文档明确双向对象桥与真实 KAG/游戏验收尚未完成。
  本轮没有独立重跑这些验证，不能将 README 的能力声明当作本宿主可直接替换的证据。

参考资料（2026-11-25 查阅；实施前固定 Ciallang 提交和 SDK API 版本）：

- [接入与验收](https://github.com/2468785842/Ciallang/blob/master/doc/krkr2-integration.md)
  的宿主参考还是旧 main `dca7264`，需要先更新到本项目实际 dev 基线。
- [嵌入 API](https://github.com/2468785842/Ciallang/blob/master/doc/embedding.md)
- [兼容性与已知差异](https://github.com/2468785842/Ciallang/blob/master/doc/compatibility.md)
- [对侧接入任务](https://github.com/2468785842/Ciallang/blob/master/.agents/notes/proposed/architecture/2026-11-04-krkr2-integration.md)

## 拟议结构

TVP 脚本入口/存储 → 统一脚本运行接口 → 原 TJS2 后端或 Ciallang 后端。
Ciallang 后端通过双向 TJS 分派兼容层连接 Window、Layer、Timer、KAG 和原生插件。
这里的接口与开关均为设计，当前宿主尚未实现。

- 运行接口覆盖初始化/销毁、执行/求值、global/context、存储执行、字节码、诊断和异常；
  逐步迁移直接依赖具体 TVPGetScriptEngine 的消费者。
- 使用目标级 C++20 适配库和实现隐藏，将 Ciallang 头与 VM 内部类型留在适配实现中，
  不全局升级宿主 C++17；复用同一工具链/CRT 与 fmt、Oniguruma 版本，父 manifest 准备所需依赖。
- 保留 tTJSVariant/iTJSDispatch2/ncbind 兼容表面，设计宿主对象→脚本包装与脚本对象→宿主代理。
  桥接缓存保持身份但不能构成永久根；明确 AddRef/Release、GC Handle、跨堆循环和关闭时的断开顺序。
- 分派逐项处理 flags、数字成员、getter/setter、一次 Operation、枚举、原生实例、Invalidate/IsValid。
  不把多次读改写当作一次宿主 Operation，不伪造 tTJSArrayNI 类型身份。
- 单独决定容器策略：可插拔兼容容器或迁移直接原生实例消费者，先用最小原型确定可行路径。
- 所有读取/序列化接 TVP/XP3 的文本与二进制流、路径、编码和模式串；
  TextStorageReader 不能替代所有容器文件 API，CLI 字节码转译也需要合适的公开嵌入入口。
- 整个游戏进程选定同一后端，不在执行失败后混合切换两套对象/全局状态；首次接入关闭 JIT。
  若同进程共存做开发对拍，协调 Oniguruma 的初始化与 onig_end 生命周期。

## 分阶段待办与验收门槛

- [ ] P0：固定双侧提交/工具链，建立源码、表达式、字节码、异常和序列化差分语料。
- [ ] P1：SDK 与最小运行接口，原 TJS2 默认行为不变；无窗口脚本可按后端选择运行。
- [ ] P2：最小对象桥及容器可行性；验证对象/接收者身份、原生实例、属性、强制 GC、重入、循环引用和销毁。
- [ ] P3：TVP/XP3 文本与二进制存储、模式、编码、行偏移、存档序列化与字节码缺口。
- [ ] P4：Window/Layer/Timer、完整 KAG、内建插件、异常回调、重启与正常退出。
- [ ] P5：固定实际游戏语料，在相同资源和操作序列下做桌面差分；覆盖存读档、音视频、内存与性能。
- [ ] P6：桌面达到门槛后，再做 Android ARM64/x86_64 构建、生命周期、输入与真机验收。

建议先做约两周可行性验证：Ciallang 创建真实 Window/Layer，Timer 回调修改画面，
宿主持有脚本回调并触发强制 GC，最后正常关闭、释放资源。
同时覆盖纯脚本对拍与一处真实原生容器消费者，避免只证明简单标量绑定可用。
两周是验证预算，不是承诺两周完成游戏替换；原型后重新估算。

## 初步工作量（估算，非排期承诺）

假设一名熟悉两边代码的全职 C++ 开发者、Ciallang 无需大规模 VM 重构、合法测试素材可用：

| 工作包 | 人周 |
|---|---:|
| 基线、SDK 与最小双后端入口 | 1–2 |
| 值/分派/原生实例与 GC 桥 | 4–8 |
| 存储、序列化、字节码缺口 | 3–6 |
| KAG、原生类、回调与插件 | 4–8 |
| 指定桌面游戏回归与稳定化 | 4–8 |

合计 16–32 人周，加入兼容性返工预算后，桌面成熟替换按约 **5–10 人月**规划。
Android 接入与真机验收初估另加 **1–3 人月**；增加人手不能线性缩短对象桥这一关键路径。
全量删除 TJS 类型/旧插件 ABI 不在上述估算内。目标游戏、插件范围及兼容标准变化后必须重估。

## 完成条件与主要风险

完成条件是相同游戏资源/脚本在新后端达到既定兼容门槛，且原后端仍能作为差分基线。
SDK 链接成功、语言单测通过、字节码 fixture 匹配分别只证明对应范围，不构成“核心已替换”。

主要风险是 GC 与引用计数跨堆循环、原生数组/字典实例假设、Object/ObjThis、原子 Operation、
嵌套异常、UTF-16/WTF-8 转换、字节码动态类与 typeof/proxy 差异，以及存档/第三方插件兼容。
性能须测实际游戏帧、宿主调用、GC 停顿和内存，不能用单一 fib/JIT 基准作为替换依据。

下一步仅在用户要求启动后执行 P0/P1 和最小桥原型；本次仅登记任务。
