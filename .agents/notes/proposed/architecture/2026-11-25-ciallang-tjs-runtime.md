# Ciallang 可切换脚本后端接入与 TJS2 替换

Status: proposed

## 任务与边界

2026-11-25 用户要求先登记任务；2026-11-26 已明确授权实施，以 Ciallang 运行 `D:/krkr/tslm` 为目标，允许修补 Ciallang 源码，正式依赖通过自定义 vcpkg port 引入。
本任务独立于 Cocos 后端精简、依赖审计 PR #63；平台拆分基线已提交为 `69368ab`。

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

正在执行 P0/P1 和最小桥原型；只有 Ciallang 后端的实际游戏证据才算替换验收，不能用旧 TJS2 回退结果代替。


## 2026-11-26 实施与 Windows CRT 问题

平台拆分已提交 `69368ab`。已拉取上游 Ciallang `1dfe555ef2ea81f066047b26e93e0aefad58b3eb`，
自定义 `vcpkg/ports/ciallang` 固定该提交及 SHA512，通过独立 vcpkg manifest 安装 Debug/Release SDK，
当前未修改上游。宿主使用目标级 C++20 适配，`KRKR2_SCRIPT_BACKEND=ciallang` 显式启用；
默认仍为 TJS2，现有桥接实现尚未完成游戏、原生继承、容器/分派全协议和字节码验收。

Windows 桥接测试初次 3/4 通过；导出回调释放触发 CRT 报错窗口。
CDB 第一机会异常栈定位：`cial::Object::hasProp` → `ScriptObject::FuncCall` →
`tTJSCustomObject::Finalize` → Variant 析构；析构中继续传播异常，最后
`terminate` → `abort` → `_CrtDbgReportW`。这是代理终结协议错误，未发现 CRT 缺失证据。

修复为在代理构造时通过 `TJS_CII_SET_FINALIZE` 清空代理的脚本终结入口；
VM 保持脚本值的终结所有权，宿主临时引用释放不执行第二套脚本 finalize。
TJS 原生实例的失效与成员释放仍沿用基类。
Windows TJS 测试入口把 CRT 报告转到 stderr，并保留断言、abort 和失败退出，避免弹窗阻塞。

重建命令见 `out/rebuild-ciallang-bridge.cmd`：
`cmake --build out/windows/ciallang-bridge-tests --target CiallangRuntimeTest --parallel 2` 成功；
`ctest --test-dir out/windows/ciallang-bridge-tests -N -R "^Ciallang"` 注册 5 项；
`ctest --test-dir out/windows/ciallang-bridge-tests -R "^Ciallang" --timeout 30 --output-on-failure --no-tests=error`
5/5 通过，0.13 秒。覆盖全局、宿主闭包接收者、GC 后回调、关闭后的回调失效及临时代理释放不提前终结存活脚本对象。
日志 `out/ciallang-bridge-*.log`；CRT 栈/原始 dump 为 `out/ciallang-crt-throws.log`、`out/ciallang-crt.dmp`。
这只证明最小桥接，不构成 TJS2 核心或 tslm 已替换的结论。

旧 TJS2 的 tslm 初轮基线只给 15 秒，启动脚本尚未执行到心跳；`passed=false`、`forced_stop=true`，
不能作为游戏通过证据。素材隔离于 `out/game-smoke/ciallang-tslm-tjs2-baseline-01`，原始目录未改。
下一步增加观测窗口重建有效基线，并接通真实原生类继承后运行同一资源的 Ciallang 场景。


### 后续桥接与包验证（2026-11-26）

- 上游工作副本 `out/ciallang-src` 的 `codex/krkr2-native-inheritance` 已本地提交
  `5f16565`，增加 GC 可见的原生基类成员面、无脚本字段初始化器的继承入口和基类身份查询。
  SDK 范围 6 个用例、38 个断言通过（复用已登记 `tests/sdk_test.cpp`，CTest 1/1）。
  未推送远端；补丁已导出到 `vcpkg/ports/ciallang/0001-native-base-facets.patch`。
- overlay port 版本为 `0.1.0-preview.2#1`，上游基线仍固定 `1dfe555`；
  独立 vcpkg 安装已实际应用补丁、编译 Debug/Release 并完成包校验；随后通过二进制缓存
  安装到完整宿主 SDK。日志分别为 `out/ciallang-port-patched-install.log` 与
  `out/ciallang-host-patched-install.log`。
- 首轮真实原生继承测试触发访问违规：异常桥用空 `tTJSScriptBlock*` 构造
  `eTJSScriptException`，在 `tTJSScriptBlock::AddRef` 空指针崩溃。
  CDB 栈保存在 `out/ciallang-native-bridge-stack.log`。补充 `CreateDiagnostic` 持有源码、
  文件名和行表，不解析或执行旧 VM，也不改变异常/源码块的对象布局。
- 修复诊断后，原始错误暴露为 `TJS_E_INVALIDPARAM (-1003)`：`NativeFacet::hasProp`
  给 `PropGet` 传空输出指针。栈见 `out/ciallang-native-first-throw.log`。
  两处属性存在性检查均改用临时 Variant，保留 `TJS_IGNOREPROP`。
- 修复后开发 SDK 的桥接验收实际注册 7 项，7/7 通过，0.87 秒；
  包含真实 NativeInstanceSupport 指针、继承属性读写、GC 后访问，以及抛出值 123 的
  文件名、偏移后第 8 行和源码文本。日志为 `out/ciallang-bridge-native-*.log`。
  此处使用工作副本安装的开发 SDK，正式包消费仍需重跑这 7 项。
- 编译清单确认 `tjs.cpp` 与 `SdlHost.cpp` 使用 C++17，适配实现使用 C++20。
  依赖检查首次拒绝未登记的适配目标；现将该对象目标纳入 foundation 审计并继续禁止
  domain/平台反向依赖，12 个目标的检查通过。
- 旧 TJS2 的第二轮 tslm 基线：90 秒到达 `title.ks:33`、144 个状态采样，
  心跳通过但按期限强停，不证明正常退出。证据在
  `out/game-smoke/ciallang-tslm-tjs2-baseline-02/smoke-report.json`。
- 已在忽略目录检查 data.xp3：startup 和当前抽出的 73 个 TJS 文件为 UTF-16 文本，
  startup 引用 `system/Initialize.tjs`。没有因此认定其他归档/插件不含字节码。

应用第一次链接属于较早适配修订，已停止该过时构建；正在使用正式补丁包和修正后的
异常/分派实现重建普通应用。Ciallang Window/Layer/Timer 场景与 tslm 尚无通过证据。
原生对象的完整失效/终结、跨堆循环、全部分派 flags/Operation/枚举/容器与存储、字节码仍需继续验收。

## 2026-11-26 真实 KAG 语料驱动的解析修复

正式链接完成后首次跑通最小宿主场景：`out/game-smoke/ciallang-host-bootstrap-03/startup.tjs`
创建 Window/Layer、Timer 回调触发、`System.exit()` 后进程退出码 0。日志
`out/ciallang-host-bootstrap-03.log`。这只证明最小原生宿主路径，不含 KAG。

用 `tools/game-smoke/game_smoke.py` 准备隔离副本 `out/game-smoke/ciallang-tslm-*`，
原始 `D:/krkr/tslm` 未改。逐轮把 Ciallang 后端推进到 `initialize.tjs` 更深位置，
每次失败点都换成上游回归用例并通过正式 vcpkg 包复核。上游本地提交（未推送）：

| 提交 | 修复 |
|---|---|
| `f1ab8d4` | `Function` 无命名成员时返回"未找到"而非 `Object::hasProp` 的 "Not support"，使宿主以函数对象为接收者回调时裸标识符能回退 global；补前缀成员 `++/--` 降糖 |
| `1ce517a` | 词法合并相邻同引号字符串（`'A' 'B'` → `'AB'`），与 `tjsLex.cpp` 一致 |
| `a4e5762` | `delete identifier` 降糖为 `this`/`global` 加 `DelPropC`，`GlobalObject` 支持删除槽位 |
| `0ff6aa5` | 字典下标未命中返回 void（`archive_exists[name]` 之类探测依赖） |
| `67e5814` | 全局成员未命中返回 void（`typeof global.x` 探测依赖） |
| `e3a943f` | 点号后的关键字按裸标识符处理（`uiinfo.class`） |
| `3fd09d5` | 三元两分支不吸收逗号表达式（`%[ a: c ? x : y, b: z ]`） |
| `f2119eb` | 后缀 `isvalid` 接受任意表达式（`this isvalid`） |
| `beb3ab3` | `lookAhead` 丢弃已缓冲的注释 token |
| `bb79f03` | 深前视时丢弃新读到的注释而非队首 token（codex CLI 定位；此前会误删已缓冲实参） |
| `18121ee` | 任意 `priority_expr` 作调用目标、下标 `++/--`、`default` 之后的 `case` |
| `509644f` | 守卫式 `invalidate x if ...` 与 `var x = void` 类成员的回归覆盖 |

port 版本随之推进到 `#14`，每轮均经 `cmake --preset "Windows Debug Config"` 重装并重建
`out/windows/debug/bin/krkr2/krkr2.exe`。上游 SDK 测试从 6 用例增至 25 用例，全部通过。

`out/codex-batch-run.log` 是全量语法调查结果：递归扫描 `out/ciallang-tslm-data/data`
下 73 个 `.tjs`（`system/` 49、`sysscn/` 22、顶层 2），48 通过、25 失败，
去重后 5 类解析根因：
P1 类体仅接受 var/function/property（15 个文件）、P2 调用目标白名单（14 个，含 startup）、
P3 下标 `++/--`（2 个）、P4 限定名 `extends`（1 个）、P5 `default` 后仍有 `case`（1 个）。
P2/P3/P5 已在 `18121ee` 修复；P1 与 P4 仍未实现。

P1 的根因由 codex CLI 定位为 `HistoryLayer.tjs:101` 的类体语句
`wwFollowing += wwFollowingWeak;`：类体被拒绝后解析恢复位置错乱，才产生后面那些
`expected token '('` 级联诊断。修复把类体语句收进 `ClassDeclNode::statements`，
与字段初始化同序在实例上执行（`7766ab3`）。P4 的 `extends global.Name` 已支持
（`b4cf755`，`ClassMeta::extends` 仍为 Atom，按最右侧成员名解析）。

随后 tslm 依次通过 `SelectLayer`/`MapSelectLayer`/`DialogLayer`/`HistoryLayer`/
`Conductor`/`AffineLayer` 等 17 个 KAG 系统脚本。期间修掉两处宿主桥接缺陷：
宿主成员读取未命中返回 void（不再抛 `-1001`），以及 `VMState::propClassMember` 在
无对象接收者时对 `thisObj.asObject().value()` 的失败访问（`Ret.hpp` 断言来源）。

`initialize.tjs:37` 的失败最终定位在宿主桥接：`ScriptObject::PropSet` 对
“替换已有成员”的默认 setter 探测返回了 `TJS_E_INVALIDPARAM`，按 TJS2
`tjsObject.cpp` 的回退契约应返回 `TJS_E_INVALIDTYPE`，否则 TJS 不再走普通替换。
修正后该行通过。

后续又修掉三处语言/生成缺口（都在 port 补丁里）：
- `delete dict[name]`（运行期键删除）降糖为 `DelProp`（`0020`）；
- `a <-> b, c <-> d;` 中 `swap` 在需要值时给出结果寄存器（`0021`）；
- 类体内的嵌套类登记为外层**类对象**的成员（`0019`，codex 实现）。

`MessageLayer`/`MainWindow` 等脚本随后通过，KAG 系统脚本加载数达 48 个。
当前阻塞在 `sysscn/Override.tjs:1` 的 `Plugins.link("csvParser.dll")`：
插件 `InitPlugin_CSVParser` 执行 `TVPExecuteExpression("Array")` 后对返回的对象
`PropGet("clear")`，桥接返回 -1001。Ciallang 的 `Array` 方法在原生成员表
（`Context::findNativeMember`）中，而 `ScriptObject::PropGet` 只查
`_prop_`/`ClassMeta`，因此宿主看不到内建类型的方法。
宿主单测 `Ciallang exposes Array class members to the host` 已稳定复现。

宿主弹窗问题同时处理：`ShowException` 与 `TVPShowSimpleMessageBox` 在
stderr 被重定向时改为写日志并直接返回/退出，不再弹模态框阻塞自动化运行。

内建类型方法可见性修好后，`Override.tjs` 继续暴露两处 Array 标准库缺口：
`Array.add(item)`（返回新元素索引，与 `push` 不同）与 `Array.find(item, start)`；
另补 `Array.clear` 作为 `erase` 的别名（`0022` 起，合并进 `0023`）。

随后 KAG 系统脚本加载数达 50，`sysscn/Override.tjs` 通过，当前阻塞在
`system/YesNoDialog.tjs`：`class YesNoDialogWindow extends Window` 的构造器里某个赋值
命中只读 `Property`，报 `property no have setter`（`Property.hpp:49`）。
需要判定是子类字段与原生 Window 成员同名遮蔽，还是 `super.Window()` 把脚本 property
装到了实例上；判定依据是 TJS2 `tjsObject.cpp` 的 `TJSDefaultPropSet` 回退规则。
该定位已交给 codex CLI。

只读 property 的赋值按 TJS2 `tjsObject.cpp:1397-1414` 改为回退到普通成员替换，
`YesNoDialog.tjs` 通过；`&identifier = v` 与 `&obj[key]` 两种 SPDS 形态补齐后
`sysscn/system.tjs` 的编译期错误全部清除。

当前进度：56 个 KAG 脚本加载，`sysscn/system.tjs` 已进入运行期。阻塞点是
`system.tjs:347` 的 `new _AliasItem_(...)`：`_AliasItem_` 是 `SystemRegistory`
的嵌套类，方法内按裸名实例化时抛 `TypeError("Invalid type get as Object")`。
已确认 `typeof this.I` 能读到实例上的嵌套类槽位，但 `new I()` 的实例化路径未打通。
SDK 用例 `SDK instantiates a nested class by its bare name` 以 `[!mayfail]`
标记记录该缺口；其余 39 个用例通过。

至此 Ciallang 后端仍无 KAG 心跳证据（未到 title.ks）。语言与标准库缺口已从
最初 25 个失败文件收敛到"方法内实例化嵌套类"这一处；原生类继承、宿主属性语义、
存储与字节码仍未系统验收。


## 2026-11-26 Ret 临时诊断与真实 KAG 首个失败点

本轮遵循 `out/ciallang-fail-fast-task.md`，仅诊断，不修复该 VM 语义。
保留开始时宿主接入改动及 Ciallang 工作副本已有的 `tests/sdk_test.cpp` 修改。

- Ciallang 工作副本 HEAD 为 `b4cf755499e459671a7de7f7b572776d03f56091`，
  port 上游仍固定 `1dfe555ef2ea81f066047b26e93e0aefad58b3eb`（MIT，
  Copyright (c) 2024 2468785842）；在既有 0001–0016 补丁上增加
  `vcpkg/ports/ciallang/0017-ret-fail-fast-diagnostics.patch`。
  使用 Python 的 `subprocess.check_output(["git","diff","--","CMakeLists.txt","src/common/Ret.hpp"], cwd="out/ciallang-src")`
  原样写出补丁，没有包含预先存在的 SDK 测试修改。
- 新 CMake 选项 `CIAL_RET_DIAGNOSTICS` 默认 OFF；开启后以 PUBLIC 宏统一库/SDK
  内联定义，`Ret<T>::value()` 使用默认参数 `std::source_location::current()` 捕获调用点，
  失败时保存 ErrCode、消息及 file:line/function，记录于加锁的 32 项静态环形缓冲区后
  抛出 `std::runtime_error`。`Ret<T>/Ret<void>::getErr()` 的非法访问同样抛出；
  宏关闭时仍保留原断言。访问器为 `cial::RetDebug::dumpFailures()`。
- overlay port 显式开启该临时选项，port-version 16 → 17。安装记录确认
  `0.1.0-preview.2#17`，ABI
  `9c665afc82db9182a1f47b5edca6e0dca82e60637b4ebd84e68c814da8d21df1`。
  安装日志明确应用 0017，编译并安装 Debug/Release；导出 target 含
  `INTERFACE_COMPILE_DEFINITIONS "CIAL_RET_DIAGNOSTICS=1"`。
- 按原工具链执行 `cmd /c out\reconfigure-ciallang-host.cmd`、
  `cmd /c out\reinstall-ciallang-host.cmd`。
  配置 full、ENABLE_TESTS=ON、UI_TEST_DRIVER=OFF、SCRIPT_BACKEND=ciallang。
  日志 `out/ciallang-host-reconfigure2.log`、`out/ciallang-host-rebuild2.log`。
- 首轮 fresh 副本 `ciallang-assert-20` 已将 abort 变为异常，但 KAG 的异常处理器又触发
  `TJS dispatch failed: -1003 in HostObject::setProp @line(37) initialize.tjs`，
  遮蔽原始异常。宿主 `CiallangRuntime.cpp` 的两类异常出口增加宏控制的 stderr
  缓冲区输出，重建后改用 fresh 副本 `ciallang-assert-21`。
- 两轮均从原始 `D:/krkr/tslm` 经
  `python tools/game-smoke/game_smoke.py prepare D:/krkr/tslm out/game-smoke/ciallang-assert-<20或21>`
  建立，不修改原始游戏；运行
  `python tools/game-smoke/game_smoke.py run out/windows/debug/bin/krkr2/krkr2.exe out/game-smoke/ciallang-assert-<20或21> --seconds 150 --launch-style game`。

实际第一个诊断（21 的 smoke-process.log）：

```text
[CIAL_RET_DIAGNOSTICS] animationlayer.tjs
Ret::value at D:\cpp_tool\vcpkg\buildtrees\ciallang\src\efad58b3eb-e5b1e21394.clean\src\vm\VMState.cpp:367
cial::vm::propGetMember
ErrCode=1 message=Invalid type get as Object
```

具体出错调用为 `fun->thisObj = cur->thisObj.asObject().value();`。
读取 ClassObject 上的 Function 时无条件将当前执行接收者当作 Object；
失败 Ret 来自 `cur->thisObj.asObject()`，ErrCode 1 即 TypeError。
这是 VM 类成员读取路径，不是宿主 bridge，也不是某个 stdlib 函数。

触发 KAG 代码是 `data.xp3>system/AnimationLayer.tjs:1227–1231` 的
`AnimationLayer.Anim_reconstructHandlers = %[pimage: AnimationLayer.loadPartialImage, ptext: AnimationLayer.drawReconstructibleText];`。
第一个方法读取位于 1229 行。正式 SDK 独立复现程序以三个空基类替代
BaseConductor/AffineLayer/KAGLayer，然后执行抽出的完整 AnimationLayer.tjs，
复现同一 C++ 位置；VM 故障映射指向整条赋值起点 `animationlayer.tjs:1227:1`，
不是方法子表达式的 1229 行。无需实例化这些 Layer，也不依赖 stdlib 方法调用。
最初测试误预期 VM 会映射到 1229，实际为 1227；原始失败记录保留
`out/ret-probe-first-kag-fault.log`，校正语句起点断言后实际注册/执行 2/2 通过。

验证程序与命令：
`out/ret-diagnostics-test/{main.cpp,kag_fault.cpp,CMakeLists.txt}`，
`cmd /c out\run-ret-probe.cmd`（同一 VsDevCmd/MSVC、Ninja、Debug、
CMAKE_PREFIX_PATH 指向宿主正式安装 SDK，没有新工具链）。
`ctest --test-dir out/ret-diagnostics-test/build -N` 注册 2 项；
`ctest --test-dir out/ret-diagnostics-test/build -V --output-on-failure --no-tests=error`
2/2 通过。覆盖调用行号/错误消息、成功访问、两类 getErr 保护、32 项缓冲回绕，
以及上述完整脚本故障定位。日志 `out/ret-probe-{configure,build,list,test}.log`。

21 轮 exe SHA256：
`2b144267487ea30af152dcd828f17f195a9e48b7042ef5117c2565cdf034bce4`。
游戏 42.91 秒退出，exit_code=0、forced_stop=false，但 passed=false、sample_count=0，
没有 KAG 心跳；退出码 0 是引擎错误处理后的退出，不能记为兼容通过。
证据为 `out/game-smoke/ciallang-assert-21/smoke-process.log`、
`smoke-report.json`。20 轮报告和日志独立保留。

后续应独立修复类方法提取时对顶层非对象接收者的处理，补方法身份/绑定接收者回归，
再推进真实 KAG。此次诊断不改变该语义；跨平台、完整宿主单测、剧情、音视频、
存读档仍未验证，整体任务保持 proposed。


## 2026-11-27 接续：嵌套类实例槽与 typeof 成员探测

接续时保留宿主桥接、port #26 等全部 dirty 改动，PowerShell 可正常执行。
按 `krkr-game-debug` / `krkr-verify` 使用隔离副本，不修改 `D:/krkr/tslm`。

- 嵌套类故障的直接原因：`IRGenerator::generateClass` 定义期的 `nestedReg`
  被直接写入另一个 `gen`（实例字段初始化器）的字节码。寄存器编号只在所属 chunk
  有效，实例帧里同编号读到的不是 ClassObject。修复让初始化器分配自己的寄存器，
  并从自己的常量表加载 nestedMeta 后写入实例。正式补丁 `0027`，来源工作副本
  本地提交 `b75b465`（基于原固定上游及既有补丁，保留 MIT/Copyright 来源）。
- 移除裸名嵌套类用例的 `[!mayfail]`；SDK 40 个用例 / 181 断言通过。
- 对照 `cpp/core/tjs2/tjsInterCodeExec.cpp` 的 TypeOfMemberDirect/Indirect 和 TypeOf，
  发现 Ciallang 将缺失成员与 void 槽位混为一谈，且对象返回类名。TJS2 的成员缺失
  为 `undefined`、已有 void 为 `void`、对象统一为 `Object`。真实 system.tjs:353
  的 `isDefined` 因此误判，导致 add() 跳过初始化数组，在 493 行访问 void。
- `0028` 增加 TypeOfMember 指令（AST/SSA/编码/VM），保留属性 getter 执行与异常传播，
  并修正对象 typeof。补充缺失/void/对象/getter 与 SystemRegistory/派生 Hooks/嵌套 Alias
  的组合回归。当前本地 SDK 42 个用例 / 185 断言全通过。
  `cmd /c out/build-ciallang-sdk-tests.cmd` 构建和 CTest 退出码 0；
  `out/ciallang-native-probe/build/ciallang_sdk_tests.exe "[sdk]" --reporter compact`
  报告上述实际数量。正式应用尚在重建，本节不宣称游戏通过。
- #27 首次重装失败是 SDK 测试补丁上下文与 port #26 内容不一致（本地测试比补丁包多），
  不是生产源码编译错误。已基于正式 #26 源文件重新生成完整测试 diff；修正后的
  #27 已完成补丁应用并进入 Debug/Release 编译。#28 在该正式源树上 `git apply --check`
  退出码 0。原始失败与后续日志见 `out/ciallang-host-reconfigure2.log`（后续轮会更新）。

下一步：完成 #28 正式包、普通 Ciallang 应用及桥接测试构建，运行 fresh 副本
`out/game-smoke/ciallang-resume-38`，逐个处理真实首错，直到标题及剧情实际输入证据成立。


正式 #28 SDK 已由 CLion 自动配置完成安装（status 为 `0.1.0-preview.2#28`）。
本地源提交 `54e437e` 对应 `0028`。独立程序以 CMAKE_PREFIX_PATH 指向普通应用的
`vcpkg_installed/x64-windows-static-md`，通过 find_package(Ciallang) 链接正式包，
没有 add_subdirectory 本地源。`cmd /c out/ciallang-resume-package-tests.cmd` 完成
配置/编译/实际 CTest 注册，`CiallangSDKPackage` 1 项执行 42 个 Catch2 用例、185 断言全通过。
证据 `out/ciallang-resume-package-{configure,build,list,test}.log`。
先前独立 port 安装尝试因全局 vcpkg lock 被 CLion 自动配置持有而退出 1，未停止其他进程；
后续直接复核 CLion 安装结果。普通应用的迭代构建目前显式使用
`out/ciallang-native-sdk/lib/cmake/Ciallang`（同一 #28 本地源），Ciallang 后端、full 范围、
UI_TEST_DRIVER=OFF，等待宿主及桥接测试链接后开展真实游戏验证。


## 2026-11-27 宿主文本流、原生回调与默认构造参数栈损坏

#28 应用首轮 `out/game-smoke/ciallang-resume-38` 的 exe SHA256 为
`51b3f573bd9f41efe986837c5ebfbac78676cc11bfe184052fe035f2c19ddaa5`。
脚本加载结束、Startup 返回，但无可见窗口、无心跳，150 秒左右由控制器结束。
`passed=false`，不能视为游戏启动通过。

- #39/#40 钩子诊断：`global.kag` 为 undefined；原始 AfterStartup 在 Array.save 时
  抛 `cannot open file`。Ciallang SerializeLib 直接访问文件系统，不支持 TVP `file://`
  路径和虚拟存储。宿主新增 Array.load/save 注册，委托原 TJS 文本流工厂处理路径、mode、
  编码和释放，拆行/返回当前数组行为对照 `cpp/core/tjs2/tjsArray.cpp`。内存流回归与
  真实 TJS Array.save 对照输出，含 Unicode、空行、mode 原样透传及写入失败关闭流。
- 自制 `ciallang-window-inherit-01` 完成 super.Window 和 new，但退出回调报
  `value is not callable`。原因是 ScriptObject::FuncCall 将原生 TJS 方法的 HostObject
  闭包交给仅支持 SDK Function/NativeFunction 的 VM::callValue。改为按原 TJS 闭包
  分派原生方法，保留 ObjThis/返回码；用派生 NativeCounter 的 C++ FuncCall 回归。
- 宿主测试原有两处 typeof 缺失成员预期仍为 void，改为 TJS2 的 undefined。
  第二轮实际注册并运行 `ctest --test-dir out/windows/debug -R '^Ciallang'
  --output-on-failure --no-tests=error`，15/15 全通过。证据
  `out/ciallang-resume-host-{list2,tests2}.log`；其中存储用例 16 断言。
- 首次增量链接失败 LNK1168 是诊断游戏持有 exe；已结束本任务启动的 #43/#44 进程后
  重试成功。没有运行失败构建的旧产物冒充修复验证。

调查记录：#41/#42 将归档 Initialize 散装执行后改变了相对搜索上下文，提前报
`Cannot find storage LayerEx.tjs`，这两轮只属于诊断失败。用旧 TJS2 exe 仅作为私有归档
文本读取工具，确认本样本启动/Initialize/MainWindow 实际由 TenshinChs.xp3 覆盖，Config
来自 main.xp3。抽出文件保存在 `out/game-smoke/ciallang-script-extract-0{1,2,3}/`，未提交素材。
#43/#44 用显式归档搜索路径和插装文件缩小到 KAGWindow_config 处 Invalid type get as Object，
没有把修改资源搜索后的诊断场景作为兼容通过证据。#40 起 XP3 在隔离副本之间使用硬链接，
源为已复制的 #38（不是原始游戏），引擎只读归档；脚本/存档/日志各自独立。

CLion 使用 `krkr2` 的一次性 programArguments/workingDirectory 覆盖启动全新 #45，原配置
`D:/krkr/tslm/data.xp3` 未更改。在 `out/ciallang-src/src/common/Ret.hpp:147` 的 unwrap throw
断下：首先暴露 initialize.tjs 的 KAGLoadScript 在 `getTickCount()-start` 处 Sub 类型错误，
start 本应为整数却变为带 crossfade 字段的 DataObject。栈与值保存于
`out/ciallang-default-ctor-debug-stack.json`。无参数/普通构造器 throw 的 SDK 对照用例通过，
因此撤回了未获证实的构造器异常边界改写。

真正最小复现：`C(a=1,b=2,c=3,d=4)` 默认构造器在函数内连续 new 两次，将局部 kept
从 sentinel 改为空；四次在 FastRegisterPool.hpp:37 `_sp >= n` 断言失败。命令为
`out/ciallang-resume-package-tests/build/ciallang_probe.exe out/ciallang-default-ctor-{1,2,4}.tjs`。
四次版本的 CRT 断言探针已结束，没有用于健康套件。源码证明 Class.cpp newClass 与 super
构造分支在 run/freeCallFrame 已回收参数后又 pop(cnt)，重复扣除缺省参数槽。当前移除两处
额外 pop，增加普通/派生构造的调用方寄存器保留回归，等待 SDK 重新构建及真实游戏复核。


## 2026-11-27 接入方式改为 git submodule 与 KAG 启动链推进

接入方式变更（用户决定）：Ciallang 不再通过 vcpkg 安装或源码目录 `out/ciallang-src`，
改为 `.gitmodules` 中的 `external/Ciallang` 子模块；`cpp/core/tjs2/CMakeLists.txt` 用
`add_subdirectory` 构建，默认 `CIAL_BUILD_*`/`JIT`/`SDK` 全部关闭、仅 `C++20` 适配层可见。
`vcpkg/ports/ciallang` 保留（端口文件与 vcpkg 清单仍在仓库内）但当前构建不走到它。
Ciallang 侧不再是 `out/` 下的本地补丁，而是上游提交：本地 25 个历史提交 + 本轮修复已
合入 `master` 并推送，HEAD `de4ee5c`（提交前用 clang-format 20 对改动行范围格式化）。
重装/更新依赖只需 `git submodule update --remote external/Ciallang` 后重新配置。

本轮真实游戏（`D:/krkr/tslm`，隔离副本 `out/game-smoke/ciallang-resume-*`）逐轮推进记录：

| 轮次 | 首错 | 修复 |
|---|---|---|
| 62 | `affinelayer.tjs:1072` `super.setPos` 报 `in class object Not found property` | `Super`/类成员查找增加 native facet 回退；接收者为多级祖先时取实例 facet 而不是实例自身（避免 `property type` 的 getter 自递归） |
| 63–64 | 栈溢出 `0xC00000FD` | 同上；确认 `super.type` 从实例读会取回同一个 Property |
| 65–66 | `bgm.tjs:195` 裸名调用原生基类构造器 | `HostObject::call` 支持「构造器直接原生基类」就地初始化 |
| 67–68 | `menus.tjs:267` `super.enabled`、`mainwindow:2111` `typeof x == "undefined"` | 原生属性读改用实例代理作 objthis；`||`/`&&` 改为真短路且返回操作数；非数字字符串真值不再 abort |
| 69–70 | `mainwindow.tjs:1603/1607` | 类对象成员读/调用的实例接收者与闭包重绑；仅在替换实例时 `ChangeClosureObjThis` |
| 71–73 | `0xC0000005` | receiver 代理统一 RAII（`ReceiverGuard`），修正 Release 顺序 |
| 74–79 | `menu[s].tjs:254` `object to int`、`world.tjs:425` | 前缀 `++`/`--` 与赋值表达式值寄存器不再提前 `freeRegister`；`extends` 定义期读取触发惰性类加载 |
| 80 | `world.tjs:425` `Not support` | 同上 + `DataObject::getProp` 未命中返回 void |
| 83–84 | `menus.tjs:254` | 实例需要看到类对象上的可调用成员覆盖（`with(global.Layer){ .loadImages=... }`）；先无条件优先导致 6 个 SDK 回归，改为只匹配 `Function/NativeFunction/Property` 且直查 `_props`（不走会抛异常的 `ClassObject::getProp`） |
| 85–95 | `diffimage.tjs` 首次通过，`custom.tjs` 失败 | `ChgThis` 改为克隆换绑、`Runtime::hostRebindHook`、`fromHost` 保留外部闭包 ObjThis、`ScriptObject::FuncCall` 也克隆脚本函数 |
| 96–99 | `custom.tjs` `f(...)` 被调者不可调用 | `resolveGlobalBase`：基类解析统一走 `loadGlobal`，触发惰性 `property` 的 getter（`Super`/`findClassMethod`/`initClassFields`/`ClassObject::call` 全部改用它） |
| 100 | 仍在 `custom.tjs` | 剩余缺口：`class KAGEnvCharacterForYuzu extends KAGEnvCharacterEx` 的定义期读取惰性基类仍抛异常 |

诊断改进（保留）：`Value::requireObject` 透传 `std::source_location`，失败时直接报调用点，
不再一律显示 `Value.cpp:26`；`CallOmit` 对非可调用被调者给出明确消息。

验证：Ciallang SDK 62 用例 / 247 断言，`ctest`/直接运行退出码 0，其中 1 例
`[!mayfail]` 记录上述惰性基类缺口；宿主 `CiallangRuntimeTest` 18/18 通过。
真实游戏仍未到 KAG 心跳（`sample_count=0`、`passed=false`），不构成替换验收；
`custom.tjs` 之后还有标题、剧情、存读档、音视频未验证。


## 2026-11-28 macOS nested SetIndex 首错与静态方法同名键

本轮基线：宿主 HEAD `1d61099e1f16df3e9444d262c26d30597c7475ef`、子模块 HEAD
`de4ee5c0c94c9eac0c4499916efcaebd8b44b015`，两侧均有既有 dirty 改动；保留全部原改动，
本轮未 commit/push。SDK 构建目录 `out/macos/ciallang-sdk` 的 source 为
`external/Ciallang`、Debug、`CIAL_BUILD_TESTS=ON`、JIT OFF；宿主
`out/macos/ciallang` 为 Debug、`KRKR2_SCRIPT_BACKEND=ciallang`、full scope、
`ENABLE_TESTS=ON`、普通 app、UI driver OFF、JIT OFF。

### 实际定位与修复

原首错为 `override.tjs:75`：`value is not indexable: void (void)`，已确认是
nested **SetIndex receiver** 为 void，目标属性 `enabled`、value 为 0；不是最终
GetIndex 自己抛缺键异常，也没有证据指向 TJS 原生 ObjThis。

先收敛 `external/Ciallang/tests/sdk_test.cpp` 的既有 nested postfix 用例：
`_fore[getName()][getProp()] = getValue() if guard()` 在全局容器和类字段两种形态下，
true 分支结果 42、三个操作数各一次、guard 一次；false 分支结果仍为 7、
三个操作数零次、仅 guard 一次。未修生产代码时即通过 1 用例 / 12 断言。
补入原脚本方法参数形态及非同名缺失键也通过，故没有依据改写
`IRGenerator.cpp` 的 AssignExprNode/IndexExprNode/TernaryExprNode 生命周期或 postfix-if 语义。

真实轨迹（仅 out 中保留日志，生产临时输出已删除）发现失败键实际为 `save`。
同一 Dictionary 地址在 guard 的 `typeof _fore[name]` 得到 `Object`，后续
GetIndex 却报告 missing `save` 并返回 void。原因是 `VMState.cpp::TypeOfMember`
在字典键不存在时继续匹配 Dictionary 的原生静态方法 `save`，把缺失键误判为存在。
这是**静态方法同名键的存在性探测错误**，不是一般 Dictionary 缺键读取错误或寄存器复用。

旧 TJS2 Release app 对自制 `out/game-smoke/ciallang-nested-reference/startup.tjs`
的实测输出为 `NESTED-REFERENCE:undefined:undefined:Object`（依次为
`typeof d["save"]`、`typeof d.save`、`typeof Dictionary.save`），守卫写入的 hits=0，
进程退出码 0；证据 `out/ciallang-nested-reference.log`。
源码也对应 `cpp/core/tjs2/tjsDictionary.cpp` 的静态方法登记与 `tjsNative.h` 的
`TJS_STATICMEMBER`。仅将 SDK 缺失键从 `absent` 改成 `save`，修复前即可稳定失败：
1 用例失败、14 断言中 2 失败，日志 `out/ciallang-nested-sdk-before-fix.log`。

实际改动：
- `external/Ciallang/src/types/Dictionary.hpp` 区分标准库类对象与普通字典；
  `src/stdlib/DictLib.cpp` 仅把注册的 Dictionary 类对象标为类对象。
- `src/vm/VMState.cpp::TypeOfMember` 对普通字典直接按键槽返回类型/undefined，
  与 GetIndex 一致，不再把静态方法误作实例键；保留类对象静态方法的类型探测。
- 同一个 SDK 正式回归还断言 missing `save` 的 guard 安全跳过，以及
  `undefined|undefined|Object` 的双实例/类对象探测结果。原有独立 missing/void
  回归保留；删除了旧 nested 测试中重复的 present-void 探针。
- 保留 SetIndex/GetIndex 的 receiver-rich 错误诊断、Application/ScriptMgnIntf 的
  Debug stderr 与 startup 首错日志；不吞运行期异常。全部临时 TYPE/INDEX/寄存器轨迹输出已移除。
- 标准库实例方法直接调用这一既有扩展未改；存在性探测以键槽为准。边界说明已同步
  `external/Ciallang/doc/compatibility.md`。IRGenerator 无本轮改动。

### 构建与测试证据

以下构建/测试均退出 0，日志前缀 `out/ciallang-nested-`：

```sh
cmake --build out/macos/ciallang-sdk --target CiallangTests --parallel 4
out/macos/ciallang-sdk/tests/CiallangTests 'SDK guards nested dictionary writes with postfix if'
out/macos/ciallang-sdk/tests/CiallangTests '[sdk]'
ctest --test-dir out/macos/ciallang-sdk -N
cmake --build out/macos/ciallang --target CiallangRuntimeTest krkr2 --parallel 4
ctest --test-dir out/macos/ciallang -N -R '^Ciallang'
ctest --test-dir out/macos/ciallang -R '^Ciallang' --output-on-failure --no-tests=error
```

最终新单例 **1 用例 / 18 断言**；SDK 全套 **67 用例 / 288 断言**，全部通过。
SDK CTest 实际总注册 **277 项**，新回归登记为 #54，已有 `sdk_test.cpp` 在
`external/Ciallang/tests/CMakeLists.txt` SOURCES 中，无需新增清单；277 项并非全跑通过。
宿主 `^Ciallang` 实际注册并运行 **19/19 通过**。
C++ 本轮修改范围经 `/usr/local/bin/clang-format` **20.1.0** 格式化；
父仓库与子模块 `git diff --check`、新增 note 引用路径检查通过。
无新增基础模块依赖，未触发全依赖重装；Windows/Linux 未验证。

### 全新真实游戏运行

Python 使用 `/opt/homebrew/bin/python3.13`（系统 python3 缺 `hashlib.file_digest`，
#20 只留下准备失败目录，未作为运行证据）。#21–25 均由 `game_smoke.py prepare`
从 `/Users/lidong/Documents/tslm` 准备全新副本；未改原始游戏、XP3、游戏脚本或存档。
#22–24 为临时 VM 轨迹版，只用于定位；#25 已删除全部临时轨迹，用最终 Debug app：

```sh
/opt/homebrew/bin/python3.13 tools/game-smoke/game_smoke.py prepare /Users/lidong/Documents/tslm out/game-smoke/macos-ciallang-25
/opt/homebrew/bin/python3.13 tools/game-smoke/game_smoke.py run out/macos/ciallang/bin/krkr2/krkr2.app/Contents/MacOS/krkr2 out/game-smoke/macos-ciallang-25 --seconds 30 --launch-style game
```

各轮 exe 均为 `out/macos/ciallang/bin/krkr2/krkr2.app/Contents/MacOS/krkr2`；
manifest SHA256 均为 `8cb1320daac1a5ae66d2a1ac1dff4c659a8b308e5c5824e9df247fa5b1ef3972`，
hook SHA256 为 `adcf88ff497804289bebeadc96dc6a7bbc6cc447efe3f9461a45c899ea2d2c09`。
每轮报告/原始日志为 `out/game-smoke/macos-ciallang-<轮次>/smoke-{report.json,process.log}`，
运行命令和 PID 在报告内，smoke 命令均因 passed=false 返回 1。

| 轮次 | exe SHA256 | 实测秒数 | 进程退出码 | forced_stop | heartbeat samples |
|---|---|---:|---|---|---:|
| 21 | `b77a7774691e12e320ccaaf9503938b478b3561a5f9db7f64531dec39ad7b40e` | 3.56 | `0` | false | 0 |
| 22 | `dd053719d1a8525463d2024214d4510fb8ba00a19c9ecf76b195bcd511fbb2f6` | 4.31 | `0` | false | 0 |
| 23 | `ccf3e15793402ff43e9f6360f4aecc32f6e8144204b500fa85b751956750e7c4` | 4.33 | `0` | false | 0 |
| 24 | `9300b6fde820e7b4e2fc0bade3ce77441c0e1704b76f7f1412e61c5318844327` | 4.31 | `0` | false | 0 |
| 25 | `d3390cbb0455efb106202649413d447667e0cb92428dbcb3a8fbec814dc4a100` | 4.92 | `-11` | false | 0 |

#21–24 首错仍为 override 的 nested SetIndex；#25 已越过原首错，加载到
`standview.tjs`、`particle.tjs`、`UILoader.tjs` 和 `AfterInit.tjs`。
新的首错为 `TJS dispatch failed: -1008 in HostObject::call`，栈首
`conductor.tjs:657` → `mainwindow.tjs:4270` → `initialize.tjs:425` → `startup.tjs:29`。
之后 startup fallback 重新初始化，最终 SIGSEGV（-11）；本轮没有把该崩溃归因于新首错，
也没有吞异常或改 fallback。无 heartbeat、passed=false、未强停，不构成启动兼容通过，
更不代表正常退出、剧情、音视频或存读档验收。

下一步：围绕 conductor 首错建立最小 HostObject::call 用例，核对实际被调者、参数与
TJS 分派返回码；另保留首次异常后 fallback/销毁期 SIGSEGV 的独立调查边界。
迁移目标仍未完成，note 保持 proposed。


## 2026-11-28 接续：Conductor 原生构造与首个 heartbeat（调查中）

用户要求持续推进直到游戏成功运行。保留全部既有 dirty 改动，未 commit/push。
已增加 `Ciallang exposes ancestor names before dynamic native construction` 宿主正式回归，
复现动态祖先类名查找、宿主返回的 TJS Array 经过 Ciallang Array.assign、缓存原生构造器
跨实例调用，以及 `{class,class}` 形式注册的原生类。普通显式 super 多层继承本来可通过；
加入实际动态路径后失败，证据 `out/ciallang-conductor-{before,array-before,cached-before}.log`。

当前已修：ScriptObject::ClassInstanceInfo(GET) 提供派生到基类的脚本/原生类名链；
Runtime.hostArrayAssignHook 在显式 Array.assign 时复制真实宿主 TJS Array 的元素，
不复制或替换宿主数组对象身份；HostObject::memberReceiver 允许原生类自绑定闭包
切换到当前脚本实例；原生 super 构造器回调在调用时取当前实例，避免全局缓存一直
初始化第一次读构造器的 AnimationConductor。实现位于
`cpp/core/tjs2/CiallangRuntime.cpp`、`external/Ciallang/src/runtime/Runtime.hpp`、
`external/Ciallang/src/stdlib/ArrayLib.cpp`。探针显示修复前 KAGParser 构造反复初始化
同一个 AnimationConductor 地址，loadScenario 方法则以原生方法自身为 receiver。
临时日志已删除，原诊断保留。

截至 #29：宿主 registered tests 20/20 通过；SDK 67 用例 / 288 断言通过。
#29 的正式 game_smoke 报告首次 passed=true，32 个 heartbeat 样本，场景 custom.ks:2；
exe SHA256 `143c11a8bf09bd70133c8ad2279e34bdc6448ef4560cdfaa6d2fd93343d1c056`，
30 秒期限强停，SIGTERM 未退出后 SIGKILL（-9），不代表正常退出。
#30 的 CUA 实际窗口与 AfterStartup snapshot 都是白屏，仍停 custom.ks:2；
不能因 heartbeat 通过就声称游戏运行兼容通过。CUA 第一次按 app 名获取超时，随后
用完整 app 路径成功；快照 `out/game-smoke/macos-ciallang-30/smoke-frame.{bmp,png}`。
#30 结束附近磁盘耗尽，报告缺失；#31 prepare 失败未运行，不能算 smoke 通过。
只对本任务新建 #20–30 中的完整只读 XP3 做哈希核对后改为硬链接到 #25，回收约
6.27 GB，保留全部各轮脚本/存档/日志；未改原始游戏。后续用忽略目录脚本
`out/prepare_shared_ciallang_game.py` 调用原 game_smoke.prepare，逐档核对原始素材、
#25 与 manifest 的 SHA256 再链接，其他文件照常复制。#32 为附加只读状态 Timer 的诊断副本。
下一步查 custom.ks:2 白屏时的 Conductor status/timer/队列，继续推进标题与实际交互。


### 后续推进检查点：数值转换、宿主字典与 ncbind（2026-11-28）

#32 只读 Timer 确认白屏时 Conductor `status=0`、`timerEnabled=0`、
`inProcessing=1`，停 custom.ks:2。临时 VM 异常轨迹（#33–35）暴露
SelectLayer.setOptions 的 `+elm.shadow` 对字符串 `"true"` 抛 `cannot convert string to number`。
旧 TJS2 自制对照输出 `NUMBER-REFERENCE:0|0|1|0|12|0.5`，对应空串、空白、true、abc、12foo、.5。
`external/Ciallang/src/types/TypeConverter.cpp::stringToNumber` 现接受词法数字/布尔关键字、
小数与符号后空白，失败文本按 TJS2 归零，保持整数/实数类型。只改一元 + 的数值规范化路径，
未扩大到 asInteger/asReal 的其他转换。新增 `tests/tjs/expression/unary/03.tjs`，
修复前失败，修复后 1 文件 / 10 断言通过；unary 目录注册 3 项，3/3 通过。
为实际运行语言回归，SDK 目录显式启用 `CIAL_BUILD_CLI=ON`，使用已有 argparse，
没有重装工具链；此后 CTest 总注册数须重新核对，不能沿用早先 277。

#36 passed=true，持续 heartbeat，但仍白屏，推进到 custom.ks:15。
#37 轨迹首错为 `Dictionary.assign expects a dictionary`（override.tjs:1453）：
原生 KAGParser 返回的 TJS Dictionary 不能被 Ciallang Dictionary.assign 复制。
已增 Runtime.hostEnumMembersHook，通过宿主 EnumMembers(TJS_IGNOREPROP) 忽略隐藏成员，
在显式 Dictionary.assign / Array.assign 时逐项转换；字典 `clear=false` 合并语义补齐，
嵌套对象仍为原宿主身份。正式宿主回归
`Ciallang assigns host dictionary members without losing aliases` 修复前抛该异常，
修复后 7 断言通过，含共享嵌套对象写回、GC 后访问、clear=false 保留与默认清除。
此时 registered host tests 21/21 通过。

#38 继而在启动后的 AsyncTrigger 事件里 SIGSEGV（-11），startup 已返回、仅一个样本，
未强停。LLDB #39/#40 匹配 exe SHA256
`e8a2c6410b591316478c8fe1c0a83aaf96656c46a178529820f06bdef48d2391`：
`ncbAttachTJS2Class<DictAdd>::RegistVariant` (`cpp/core/plugin/ncbind.hpp:2033`)
→ saveStruct.cpp:365 → Plugins.link → AsyncTrigger 回调。完整 35 层栈
`out/ciallang-event-crash-stack.log`。旧 ncbind GetGlobalObject 返回临时 Variant 的
AsObjectNoAddRef，旧全局槽通常持有同一 TJS 对象；Ciallang 全局只持有 VM 值，
不会持有这次导出的临时 dispatch proxy，因此挂接阶段访问悬垂代理。
新增 `Ciallang class attachment retains its exported dispatch` 单例在修复前稳定 SIGSEGV，
证据 `out/ciallang-ncbind-before.log`。修复为 ncbAttachTJS2Class 用 Variant 持有
整个 Regist/Unregist 批次的类代理，并在 End 释放，不把代理全局永久保活。
正在重建全部 ncbind 消费者及继续游戏验证。迁移仍 proposed，白屏不算游戏运行通过。


### 2026-11-28 标题黑屏：KAG 裸属性字符串真值（接续中）

TJS2 `macos-tjs2-current-98` 的同一份历史钩子记录 `custom.ks:107` 时
message0 子层由 1 变 7；Ciallang #97 到 `title.ks:33` 始终为 1。
直接核对 `cpp/core/kag/KAGParser.cpp:2067`，省略值的属性（`[syspage uiload]`）
存为字符串 `"true"`；`data/sysscn/system.tjs` 的 syspage 处理器用 `if(elm.uiload)`。
`Value::asBool` 调用严格的 stringToInteger，stripSign 拒绝非数字首字符，导致跳过加载。
现在字符串真值复用 stringToNumber，再取整数真值，保留小数字符串截断的 TJS2 行为。
修改仅在 `external/Ciallang/src/types/Value.cpp`；没有用游戏特例强制创建图层。

新增语言回归 `tests/tjs/expression/conversion/string-condition.tjs`：
修复前第 1 个断言失败（`out/ciallang-string-condition-before.log`）；修复后
13 个断言通过，包含 Dictionary.assign 后的真实 syspage 分支形态。
TJS2 同脚本自制 startup 对照 `ciallang-string-condition-reference-03` 输出
`__TJS_TEST_PASS__:13` 且退出 0。前两次对照因测试写法（前导空白假设、
无绑定字典回调里的裸 Dictionary 名）失败，保留原日志，不记为通过。

构建 SDK 的 cial/CiallangTests 及宿主 krkr2/CiallangRuntimeTest 成功。
SDK `[sdk]` 73 用例 / 300 断言通过；宿主 `^Ciallang` 26/26 通过；
CTest 实际登记并运行 conversion/judgment 14/14 通过，包含新回归 #349。
日志为 `out/ciallang-string-condition-{sdk-build,host-build,sdk-tests,host-tests,list,regressions}.log`。
macOS 本地 Debug/JIT OFF、工作区与子模块 dirty，Windows/Linux 未验证。

`ciallang-resume-002` 从原始素材准备全新副本，仅沿用 #97 既有历史/截图钩子。
修复后自然路径在 `title.ks:33` 记录 message0=7、message1=2；CUA 实际窗口
已从全黑变为神社标题背景，但 logo/按钮仍未显示，点击未推进。
该运行后来附加 LLDB，父控制器提前记录的退出 0 不能视为正常退出证据。
`macos-ciallang-98` 是本轮旧构建黑屏运行，为隔离同路径多进程主动强停；
`ciallang-resume-001` 是中断前的无效诊断副本，不作为验收。
后续用 `ciallang-resume-003` 的原脚本和 LLDB 检查显示回调与 fore/back/current，
不增加游戏插装。仍未达到游戏正常运行门槛。


### 2026-11-28 标题显示第二处缺口：Array.erase 错误清空命令队列

LLDB 对原游戏脚本（`ciallang-resume-003/004`，无新脚本插装）确认：

### 2026-11-28 标题按钮无响应与开场 logo：String.escape / sprintf %s 两处 TJS2 差异

接续任务：Ciallang 后端能启动到标题，但 SYSTEM/EXIT（以及 START/LOAD）点击无反应，
开场 logo 动画异常。保留全部既有 dirty 改动，本轮未 commit/push。
macOS Debug、JIT OFF；TJS2 基线为 `out/macos/tjs2-current`（同一宿主、同资源副本）。

**按钮点击链路：事件确实到达脚本。** 在隔离副本 `out/game-smoke/ciallang-resume-009..012`
给标题按钮和 `DialogLayer` 加日志后，真实点击 SYSTEM 记录：
`BTN-EXEC system enabled=1 button=0 mbLeft=0 eq=1 down=1` →
`ON-BUTTON-CLICK n=4` → `PROCESS-LINK n=4 storage=custom.ks target=*tsel_option`，
随后 processLink 在 `Scripts.eval(ln.exp)` 抛 `Invalid type get as String`（trace:
`se.tjs:51 → mainwindow.tjs:10864 errorSound → mainwindow.tjs:10831 errorLine`），
所以 `kag.process` 从未执行，画面停 `title.ks:33`。直接调用 `kag.process("custom.ks","*tsel_option")`
可立即进入 `option.ks:5`，证明下游正常。

**根因一：`String.escape()` 把非 ASCII 按 UTF-8 字节转义。**
TJS2 对照：`"【システム】切替２".escape()` 返回原 9 字符；Ciallang 返回 108 字符的
`\xe3\x80\x90...`。KAG 用 `escape()` 生成按钮 SE 表达式再 `Scripts.eval`，日文 SE 名因此变乱码，
`Storages.isExistentStorage` 未命中 `.wav/.ogg/.tcw`，进入“效果音文件不存在”错误路径，
processLink 在 `window.process` 之前中断。宿主探针 `Storages.isExistentStorage("【システム】決
定２.ogg")=1`（两后端一致），只是 eval 后的名字被破坏。
修复 `external/Ciallang/src/types/String.cpp::escapeBackSlash`：按 UTF-16 码元遍历，
只转义控制字符/引号/反斜线，保留非 ASCII，并保留 TJS2 的 hexflag（`\xHH` 后十六进制字符
继续转义，避免词法器最多 4 位 hex 吞并）。回归 `tests/tjs/native-function/string/escape.tjs`。

**根因二：`sprintf` 的 `%s` 只接受 String。**
错误路径里 `"%s:%s: %s".sprintf(filename, curLine, msg)` 的 curLine 是整数，Ciallang
`formatString` 直接 `asString().unwrap()` → 再次抛 `Invalid type get as String`，
把普通的图像/声音告警升级成致命错误，开场 `custom.ks:103 [begintrans]` 处整个 logo/ATTENTION
序列被中止（`out/inspect/run-025.log`）。TJS2 `TJSFormatString` 对 `%s/%c` 使用
`tTJSVariant::AsString`，整数/实数/void 均转字符串。
修复同一文件 `formatString` 的 `%s`：复制参数并 `Value::toString()` 后取字符串；
octet 等不可转换值不输出。回归 `tests/tjs/native-function/string/sprintf-conversion.tjs`
（含 `"%5s" / "%-5s" / 1.5 / void` 形态和 KAG 错误行形态）。

**验证（全部实际执行）：**
- `cmake --build out/macos/ciallang-sdk --target cial CiallangTests --parallel 4` 成功；
  `out/macos/ciallang-sdk/tests/CiallangTests '[sdk]'`：73 用例 / 300 断言通过。
- `python3.13 external/Ciallang/tests/run_tjs_tests.py --engine out/macos/ciallang-sdk/cial
  external/Ciallang/tests/tjs/native-function/string`：13 文件 / 62 断言通过（含新增 2 文件）。
- `cmake --build out/macos/ciallang --target krkr2 --parallel 4` 成功；
  `ctest --test-dir out/macos/ciallang -R '^Ciallang'`：26/26 通过。
- 全新副本 `out/game-smoke/ciallang-resume-023`，实机点击 SYSTEM：
  点击前截图标题、点击后 smoke-state 从 `title.ks:33` 变为 `option.ks:5`，
  证明 SYSTEM 菜单按钮已响应。截图 `out/inspect/anim-ciallang/before-system.webp`、
  `after-system.webp`；构建 `7d172b9e...` 之后修复版未重算 exe SHA（以命令/状态为准）。
- TJS2 基线同一操作：`out/game-smoke/ciallang-resume-024` 点击 SYSTEM 后进入
  `custom.ks:158` 的选项菜单流程（`out/inspect/anim-tjs2/`）。

**开场 logo 仍未完全对齐（继续调查）：** 两处修复后 `custom.ks:103` 的致命错误不再出现
（`out/inspect/run-030.log` 无“致命的”），但 `out/inspect/anim-fixed/f000..f025`
仍只见白/灰/黑过渡，没有 TJS2 基线的 ATTENTION 文字与 YuzuSoft logo；
同期 TJS2 基线 `out/inspect/anim-tjs2/f000..f029` 完整显示两者。
资源存在性两后端一致（`諸注意.png`、`yuzu.png` 均为 1，探针日志
`out/inspect/open-probe-{ciallang,tjs2}.txt`），下一步从 KAG `[ev]/[syscover]/[begintrans]`
图层提升与前景更新回调查，而不是继续改存储路径。未把标题可点击当作完整启动/剧情验收。
