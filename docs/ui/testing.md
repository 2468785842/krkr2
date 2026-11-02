# UI 迁移前的游戏验证

状态：首批测试工具，尚未完成 UI 或 Launch 迁移。源码事实核对于 2026-11-02。

## 方案审阅与实施顺序

`docs/ui/` 的 Launcher 与游戏进程分离、Android 设置与游戏视口分离，可以作为目标。
先交付可直接启动游戏的入口，再去除管理 UI，最后迁移视口。当前
`platforms/windows/main.cpp` 仍把第一个位置参数交给文件选择器的静态 `filePath`，
`MainScene::startupFrom` 启动现有 cocos 游戏场景；`--xp3`、`--config`、
LaunchOptionsAdapter、krkr2-render、RN Activity 和回滚开关仍是提案。

实施前需要收紧草案的几项约束：

- 启动目标必须同时支持游戏目录与 XP3；多归档游戏不能只测试单个 XP3。
- 管理界面与脚本可观察的菜单、消息框、输入法、多窗口分别验收，不能一并删除。
- `rendering.md` 不能通过 PUBLIC 链接当前 `krkr2core` 而保证无 cocos：
  它仍聚合 environ 等实现。先建立窄接口及目标依赖，再考虑移除框架。
- `architecture.md` 的“KAG 合成必须在 GL 完成”需要服从现有软件图像与读回契约；
  GL 最终呈现与 CPU Layer 运算分开。窗口后端尚未按本仓库完成选型验收。
- 双 Activity 本身不隔离进程崩溃；需要另行定义进程边界及恢复行为。
- 原甘特图中的日期属于历史计划，后续按通过的检查项推进。

## 本地商业游戏 smoke

工具需要 Python 3.11 或更新版本，仅使用标准库。

`D:/krkr/tslm` 是用户提供的本机样本，不能作为公共 CI 资源提交。
本次用仓库 XP3 工具读取 main、patch、data，确认能解包脚本；不推断所有素材或插件已兼容。
解包文件仅位于被忽略的 `out/ui-game-audit/`，仓库只提交自编测试工具。

```powershell
python tools/game-smoke/game_smoke.py prepare D:/krkr/tslm out/ui-game-smoke/tslm-01
python tools/game-smoke/game_smoke.py run out/windows/engine-config-check/bin/krkr2/krkr2.exe out/ui-game-smoke/tslm-01 --seconds 45
```

prepare 要求全新、位于源游戏之外的目录，复制 XP3 和兼容 patch，记录 SHA256；
不复制 savedata。当前引擎 `ApplicationSpecialPath::GetDataPathDirectory` 把存档放在
项目目录的 savedata，`System.exePath` 在本引擎同样指向项目路径；不是可执行文件目录。
游戏自定义绝对存档路径仍需单独审计。原游戏文件不修改。

现有 `ScriptMgnIntf.cpp` 在 startup 返回后执行 `AfterStartup.tjs`；测试在副本安装此钩子，
不改引擎，不覆写原 startup，不解包合并各归档，也不改变 patch 优先级。
钩子记录启动返回、KAG 对象、脚本位置和每 500ms 的 Timer 心跳，外部工具等待有限时长，
拒绝旧证据，记录引擎哈希、PID、状态序列、退出码和是否强制结束。
源目录若已有 AfterStartup 钩子则拒绝，需先显式设计组合顺序。

通过只表示 KAG 已创建、主循环持续运行至期限且插装未报错；不表示标题画面正确、
剧情可推进、输入正确、音画同步、保存恢复或正常退出通过。
工具在期限后终止自己启动的进程，报告 `forced_stop`，不能作为正常关闭验收。
当前通过位置参数启动的旧二进制只作为旧后端基线，不代表工作区最新源码构建成功。
当前文件选择器只接受有散装 `startup.tjs` 的目录；本工具对此游戏自动选择副本的
`data.xp3` 作为启动目标。未来 Launch 入口应明确目录探测与失败退出码。

## 画面与操作证据

需要交互时使用 Computer Use 检查实际窗口，再执行一次输入并重新观察；
不以“进程未崩溃”代替画面验收。GUI 自动化能力已存在，无需先搭建另一套鼠标驱动。

对该 KAG 样本，运行期间可在副本创建空 `smoke-snapshot.request`。
测试 Timer 调用游戏自身 `lockSnapshot` / `snapshotLayer.saveLayerImage` / `unlockSnapshot`，
输出一次 `smoke-frame.bmp`。这是 KAG 合成快照，不包含 cocos 管理 overlay、
窗口边框或独立视频视口；最终窗口画面仍需另外观察。请求不会执行任意 TJS。

迁移验收应分为：启动返回、标题可见、真实输入进入新游戏、相同剧情位置画面对照、
菜单与脚本对话框、保存读档、关闭及退出、音视频、DPI/IME/焦点、Android 生命周期。
每项保留脚本位置、图片与对应操作，允许动画区域掩码；避免按等待秒数直接比较整帧。
公共 CI 仍使用自制可分发 TJS/KAG fixture，商业样本只在本机运行。

## 本机首轮结果（2026-11-02）

使用现有 `out/windows/engine-config-check/bin/krkr2/krkr2.exe`，SHA256 为
`43616b24e2e73aa80292192d376de495bc7dd30fe71d2071fd9a248abae50296`，
文件时间为 2026-11-01，不能代表当前工作区。

- XP3 工具成功解包 main（35 项）、patch（13 项）、data（1194 项）。
- Python 的副本隔离、拒绝已有目标目录及 UTF-16 状态读取检查通过。
- 直接传游戏目录（包括末尾斜杠）没有进入 startup，有限时长测试正确失败。
- 改传 data.xp3 后日志到达 `Startup script ended.`；修正插装中 TJS 不支持的
  finally 语法后，得到 `startup-returned`、KAG 存在、`first.ks:0` 及副本 savedata 路径。
- 插装运行约 11 秒后以 `0xC0000005` 退出，没有 Timer 心跳；临时关闭整个
  AfterStartup 钩子对照运行约 14 秒，也以相同异常退出码结束。
  这说明崩溃可以在无测试钩子时复现，不足以判断具体根因，需符号化堆栈继续定位。
- 记录中有部分插件加载失败，但不能仅据此归因；脚本可能已有可选插件回退。
- Computer Use 窗口采集及重试超时，未获得可核验截图，未继续盲点操作。
  KAG 合成快照分支尚未执行成功；标题、输入、音视频、保存恢复和正常关闭未通过验收。

副本及原始记录在 `out/ui-game-smoke/tslm-01/`：
`attempt-instrumented-report.json`、`attempt-instrumented-state.txt`、
`attempt-uninstrumented-report.json` 和对应日志。游戏素材、解包脚本、存档和日志不提交。
本轮只新增测试工具和文档，没有修改游戏功能或实施 UI/渲染迁移。

## 新构建与 CLion 交互验证续批

同日完成 `Windows Debug Build` 的 `krkr2` 目标链接，产物为
`out/windows/debug/bin/krkr2/krkr2.exe`，86,081,536 字节，SHA256：
`bc65b08b5af2bc94816fe48f9e5ed516e0a9b10908cd7b55393a40b1cc6201a4`。
这与前述旧二进制不同，旧二进制的启动崩溃不能直接归因于当前源码。

CLion 使用一次性参数覆盖启动测试副本的 data.xp3，保留用户原运行参数。
关闭测试钩子的情况下，入口 `platforms/windows/main.cpp:24` 与
主循环 `environ/cocos2d/MainScene.cpp:702` 均实际命中，游戏菜单由用户确认可见。
用户点击新游戏后，日志新增游戏内菜单、日期 0408 和 clickglyph；
后续确认键输入后，新增 ev0102a 与 diff_ev0102f 图像加载。
`KAGParser::GetNextTag` 断点取得 Timer → TJS 执行 → 原生解析器调用栈；
该次解析对象是 clickglyph.asd 的动画，不能将其行号当作剧情位置。
新构建在该段交互中未复现旧二进制的 0xC0000005。

尝试在调试器调用 TVPExecuteScript 动态安装钩子，因调试表达式无法构造
tTJSString 而失败，未实际执行注入。副本的 AfterStartup 文件已恢复，
当前这次游戏并未加载它；Timer 心跳与 KAG 快照仍需下一次启动验证。
窗口截图仍采集超时，未宣称通过像素对照、保存读档、音画同步或正常退出验收。

记录位于副本的 `debug-session-report.json`、`debug-session.log` 和
`debug-parser-stack.json`。用户断点保留，测试创建的断点在结束时清理，游戏恢复运行。

测试控制器补充“最后状态更新必须在 3 秒内”的条件，避免活进程冻结后误判通过。
`python -m unittest discover -s tools/game-smoke -p test_controller.py -v`
两项通过：持续心跳与进程回收、活进程心跳冻结拒绝。
这里用 Python 子进程检查控制器，不代替真实引擎 Timer 或游戏兼容验证。

### 真实钩子的自动运行结果

随后保留当前调试游戏，在独立 `out/ui-game-smoke/tslm-02/` 使用相同新 exe
运行 35 秒，`game_smoke.py` 返回 0，报告 `passed=true`。
该目录的 XP3 通过硬链接引用已经复制到 tslm-01 的归档，不指向原游戏；
patch 和钩子独立复制，未复制存档，两个游戏实例的 savedata 分开。

实际取得启动返回、56 次 Timer 心跳及 KAG 状态：`first.ks:0` → `custom.ks`
→ `title.ks:33`（这里为钩子记录的原始行号）。截止时最后状态约 0.5 秒前更新，
无 smoke-error；输出 `smoke-frame.bmp`，1280×720、3,686,454 字节，
视觉检查内容为启动注意事项，验证了 lockSnapshot / BMP 保存 / unlockSnapshot 路径。
该快照在第一次 Timer 时捕获，不能当作最后标题画面，不能作为迁移前后像素对照。

报告为 `out/ui-game-smoke/tslm-02/smoke-report.json`，原始完整状态序列保存在文件中，
终端只打印摘要。到期后控制器终止其启动的第二个进程，`forced_stop=true`，
所以不代表正常退出通过；第一个 CLion 游戏会话仍保持运行。
此结果补齐上文首批尚未验证的真实 Timer 与快照分支；存档读写恢复、音视频、
移动端、最终窗口呈现及整个 UI 迁移仍未完成验收。

## L1 独立入口续批

后续已实现 `--game`、`--xp3`、位置参数与目录入口识别，详见
[Desktop CLI 的实现和本轮验证](../launch/desktop-cli.md)。
测试工具增加启动样式选择及 `--probe-engine-args`，通过真实 TJS 查询参数，
避免只检查 C++ 解析结果而遗漏初始化清空参数的问题。
目录自动选 data.xp3、显式归档、旧位置参数均在实际游戏中通过。
散装 startup 配合归档素材的引导也通过；完整解包目录的标题合成资源加载仍失败，已分别记录。

## U1 管理界面开关续批

默认开启与关闭两种构建均完成本地真实游戏启动验证，配置默认值不再依赖页面初始化。
源码/资源数量、命令与未验证交互见 [U1 验证记录](optional-management-ui.md)。
