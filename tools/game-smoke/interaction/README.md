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
