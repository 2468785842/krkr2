# 自制窗口交互回归场景

该目录完全自制，不含游戏素材。通过散装 startup 启动真实引擎；无需 XP3 或 KAG。
测试目标为菜单 checked 切换、System.inform 返回、文件选择确认/取消、close veto/accept 回调。

```powershell
python tools/game-smoke/interaction_smoke.py prepare out/ui-game-smoke/interaction-01
python tools/game-smoke/interaction_smoke.py run out/windows/debug/bin/krkr2/krkr2.exe out/ui-game-smoke/interaction-01 --seconds 300
```

在另一个终端向副本的 `interaction-command.txt` 写入以下单行请求，每次等待事件日志确认再继续：

1. `inform`：在真实消息框点击确认，日志应有 `inform-return:0`。
2. `select-cancel`：在真实文件选择器点击取消，得到 `select-cancel-return:0`。
3. `select-accept`：选择副本中的 `fixture.txt` 并确认，得到返回 1 与规范化文件名。
4. 写入 `menu-open` 打开脚本菜单，点击 Contract toggle；再写入 `menu-open-again` 并点击一次，
   应依次记录 `menu-click:1` 与 `menu-click:0`。
5. `veto`：脚本执行 window.close，onCloseQuery 调用 super.onCloseQuery(false)，验证窗口仍可见。
6. `accept`：通过同一事件回调允许关闭；记录 `close-query:1` 和 `close-returned`。
7. 若宿主仍运行，写入 `finish` 结束运行；控制器要求进程正常退出，超时强停始终判失败。

例如：

```powershell
Set-Content -Encoding utf8 out/ui-game-smoke/interaction-01/interaction-command.txt inform
```

事件写入副本 `interaction-events.txt`，完整进程输出在 `interaction-process.log`，
最终报告在 `interaction-report.json`，包括 exe/脚本 SHA256、退出码、强停标志、缺失证据。
控制器拒绝已有状态/请求的目录，每次测试应新建副本。
UI 操作可由人工或系统 UI 自动化完成。请求文件不会直接调用 MenuItem.onClick，也不会替对话框生成返回值。
本场景不验证像素、IME、多窗口、音视频或存读档。

## 可选组件输入驱动

Windows 截图/坐标自动化不可用时，可为本地测试构建增加：

```sh
cmake -S . -B out/windows/debug -DKRKR2_ENABLE_UI_TEST_DRIVER=ON
cmake --build out/windows/debug --target krkr2
python tools/game-smoke/interaction_smoke.py run out/windows/debug/bin/krkr2/krkr2.exe out/ui-game-smoke/interaction-01 --component-input
```

该选项默认关闭；不支持移动端。驱动仅在显式传入 `-ui-contract-driver=yes` 且直启目录包含
`interaction-fixture.json` 时启用。控制器的 `--component-input` 添加该参数。
正常构建不编译 `UIContractDriver.cpp`，应用中也没有驱动入口调用。

向副本 `ui-action.txt` 写入“唯一序号 操作 目标”：

```text
1 tree
2 capture
3 click-name cancel
4 click-name ok
5 click-name handler
6 click-name btn_gamemenu
7 click-text Contract toggle
```

每次仅提交一行，等待 `ui-driver-events.txt` 确认，再提交新序号。
`tree` 输出当前可见节点树到 `ui-tree.txt`；`capture` 通过 Cocos 保存 `ui-frame.png`。
点击要求唯一可见节点（同一按钮与子标签的同名匹配合并为子标签），且节点中心位于窗口内。以节点中心通过 `GLView::handleTouchesBegin/End` 投递输入，
由实际命中测试、事件分发和控件回调产生结果；不会直接调用 MenuItem.onClick 或文件选择回调。
目标未找到/不唯一会写 error，组件模式的最终报告会据此失败。
这验证场景内组件交互，不能替代操作系统鼠标、触摸设备、DPI 或输入法验收。
原生消息框仍需人工或系统 UI 自动化确认。

## 跨帧触摸回调探针

在开启测试驱动的应用上使用全新目录：

```powershell
python tools/game-smoke/pointer_repro.py out/windows/engine-config-check/bin/krkr2/krkr2.exe out/ui-game-smoke/pointer-01
```

探针通过 GLView 的 begin/move/end/cancel 路径输入，等待真实 TJS Window 的鼠标回调，
检查拖动取消不点击、双指短点传递右键、隐藏窗口前取消抬起、显示后旧触点不点击，最后验证新手势。
报告严格比较完整回调顺序，多余点击/抬起、错误按钮、脚本错误、驱动超时或强停均失败。
保存启动 exe/PDB、哈希、逐步轨迹与两张截图；需实际查看图片确认场景。
它独立于文件/菜单回归，不要求原生消息框，也不能替代完整交互回归。

驱动新增 `touch-begin|touch-move|touch-end|touch-cancel ID x y`，
ID 为 1..10，x/y 是窗口帧左上角起的归一化坐标 `[0,1]`。
开始必须使用未活动 ID，其他动作必须使用活动 ID；每次仍以唯一序号等待完成。
这些驱动操作自身只证明组件分发；窗口切换须使用下面的双窗口场景单独验收。
它们不证明物理触摸屏、多设备仲裁或 OS 切焦。

`pointer_windows_repro.py` 使用同一驱动创建两个真实 TJS Window，分别标记 A/B 回调。
先在 A 拖动后通过 `bringToFront` 切到 B，验证 A 的取消抬起；保持 A 旧触点活动时在 B
开始新拖动，再结束 A 的旧触点。随后在 B 按住期间执行 `invalidate`，验证回到 A 后
B 的旧触点不会产生回调或释放 A 的新拖动，最后检查新点击与正常退出。
每次结束旧触点后，脚本同时核对目标 Window 仍处于按住状态和共享左键状态，
再等待新触点的抬起，避免把旧触点提前释放算作随后取消成功。

```powershell
python tools/game-smoke/pointer_windows_repro.py out/windows/engine-config-check/bin/krkr2/krkr2.exe out/ui-game-smoke/pointer-windows-01
gh workflow run build-windows.yml --ref dev -f pointer_contracts=true
```

可手动开启 Windows CI 的 `pointer_contracts` 输入；默认关闭。
该模式构建显式测试驱动，在独立运行目录放置固定版本并校验 SHA256 的 Mesa 软件 OpenGL，
执行单窗口和双窗口探针。测试二进制使用独立 artifact 名称，不安装系统显卡驱动，
不修改系统 OpenGL。报告、真实脚本轨迹、exe/PDB 与截图在 `windows-pointer-contracts` artifact。
软件 OpenGL 回调通过也不代表画面、原生 OS 焦点或物理设备验收通过。

## 连续复现与取证

测试驱动构建可用下列命令自动准备新场景、执行三轮文件/菜单操作，再测试关闭否决与接受：

```powershell
python tools/game-smoke/interaction_repro.py out/windows/debug/bin/krkr2/krkr2.exe out/game-smoke/repro-01 --rounds 3 --seconds 300
```

原生消息框弹出后仍需实际确认；工具等待其真实返回，超时失败。
每轮以事件计数和唯一请求序号确认新结果，不接受上一轮的同名事件；截图也等待保存确认。
第二次菜单点击后通过实际返回按钮关闭该菜单，再进入下一轮，避免累积同名可见控件。
操作轨迹在 `repro-trace.json`，每轮图片独立命名；最终判定仍由 `interaction_smoke.py` 写入原格式报告。
自动操作超时或异常也计入失败，完整脚本事件不能覆盖操作驱动失败。

若本机已有 CDB，可附加到控制器启动的进程：

```powershell
python tools/game-smoke/interaction_repro.py out/windows/debug/bin/krkr2/krkr2.exe out/game-smoke/repro-debug-01 --cdb "D:/Windows Kits/10/Debuggers/x64/cdb.exe"
```

`evidence/` 保留启动时的 exe 和可用 PDB 副本及 SHA256，CDB 模式要求 PDB 存在；
复制前后哈希不一致会失败。建议使用完整 PDB；FASTLINK PDB 还依赖原对象文件，复制本身不能保证符号完整。
`cdb.log` 包含符号加载、模块信息及访问冲突的异常栈，dump 写入本次目录。
根据实际符号加载结果判断是否匹配，不将旧 RVA 套用到新构建。
控制器仍读取原进程退出码；观察到访问冲突或调试器异常退出时报告失败。
成功重复运行不能关闭没有根因的偶发崩溃调查。

增加 `--keyboard-probe` 可在打开消息框前检查 A 键按下/抬起的脚本回调及
`System.getKeyState` 的旧查询消费行为。驱动仅接受 `key-down A`、`key-up A`，
经 Cocos EventDispatcher 和原输入队列分发；每步等待脚本确认，不代表系统设备输入验收。
对应证据为 `interaction-events.txt`、`keyboard-probe.png` 和 `repro-trace.json`。

`--lifecycle-probe` 检查共享宿主通知入口经原事件队列到达 `System.onDeactivate`、
`System.onActivate`，分别等到脚本确认后再继续。驱动不停止 Cocos 动画泵，
所以该探针不证明 OS 挂起、后台计时、GL context 恢复或移动端 surface 生命周期。
