# Windows 原生最小化与恢复验收

Status: implemented

## 范围与实现

基于 `e763c05`，本轮 dirty 仅修改探针、测试、工作流与文档，没有修改生产 C++。
`tools/game-smoke/focus_repro.py --minimize` 使用独立场景，不创建普通切焦用的辅助窗口。
`win32_input.py` 通过 ShowWindowAsync 请求 OS 最小化/恢复，并等待 IsIconic 状态变化；
标题栏操作前临时置顶自有引擎窗口（NOACTIVATE），仍要求实际命中与 SendInput 后的前台 HWND 确认。

先同时保持原生 A 键与 GLView 拖动，再最小化。窗口确实最小化后发布 `unfocused` 请求，
连续一秒检查脚本 Timer 没有处理请求、窗口仍最小化且进程存活。
恢复后等待脚本确认与 System 激活通知；取消回调查询 A/左键均已释放，且不产生 click。
随后检查迟到的原生 A-up、组件 A-up 与旧触点结束不会产生多余回调，新的原生 A 键和 GLView 拖动正常。
普通切焦期间的输入抑制仍由默认模式单独检查，不能用本场景替代。

`base/impl/SystemImpl.cpp` 以 REMOVE_POST 替换同类激活事件并在分发时检查当前状态，
因此接受仅一次 activate，或依次 deactivate、activate；缺失恢复通知、重复、逆序通知均失败。
生产队列、动画泵及音频行为没有调整。

`frame_pixels.py` 用标准库读取 Cocos 保存的非交错 8-bit RGB/RGBA PNG，检查 CRC、尺寸和五种滤波。
最小化报告要求恢复前后中心像素都是自制填充层的 `(48,64,80)`；缺图、损坏或黑色中心像素不能通过。
这只是固定填充层检查，不代表完整场景、视频或所有像素验收。

显式 `--us-keyboard-layout` 仅向自有引擎线程请求已安装的 US English 布局并等待确认；
不安装布局或调用系统默认输入设置 API。原生发键轨迹记录实际窗口布局。
此选项用于建立非 IME 基线，不构成输入法兼容验收。

## 本地验证

Windows / MSVC 14.44 / Ninja / Debug。缓存 `out/windows/engine-config-check/CMakeCache.txt`：
app=ON、tests=ON、scope=full、管理 UI=ON、测试驱动=ON，源目录匹配本仓库。
使用已有 vcvars64 环境脚本 `cmd /c out/windows/b2-build-driver.cmd`，实际执行：

```powershell
cmake --build out/windows/engine-config-check --target krkr2
python tools/game-smoke/focus_repro.py out/windows/engine-config-check/bin/krkr2/krkr2.exe out/ui-game-smoke/native-minimize-08 --minimize --us-keyboard-layout
python -m unittest discover -s tools/game-smoke -p 'test_*.py' -v
python -m py_compile tools/game-smoke/focus_repro.py tools/game-smoke/win32_input.py tools/game-smoke/frame_pixels.py
python scripts/check_core_module_deps.py
git diff --check
```

应用实际重新编译 TVPWindow.cpp 并成功链接，退出 0。
最终 Python 36 项通过，约 8.31 秒；包含三项 PNG 滤波/损坏/黑色回归检查。
依赖守卫检查 11 个 foundation 目标通过，不代表 domain 无环。
本轮没有以旧 CTest 二进制代替新验证，也没有运行全部 full 用例或其他平台构建。

最终 `native-minimize-08` passed=true、退出 0、forced_stop=false、无 automation/脚本/驱动错误。
engine HWND `8596044`，helper=null；实际收到 deactivate、activate，两条取消回调的 A/左键均为 0；
旧抬起后无回调，新键盘与拖动的四条回调完整，最后 fixture-finished。
报告、逐步轨迹、截图与 exe/PDB 副本在 `out/ui-game-smoke/native-minimize-08/`。

- exe SHA256：`ffd26131ed926ea52220adaffe2240b511ea64e2657cb3e680fd2c063614c5e2`
- PDB SHA256：`a5a5c025dce15ef0fca944b2f7df8aa7888551d403a3b3c1217ef8a622fd7ae8`
- fixture SHA256：`5269d52194561f02c1d973aec17f513edf08c0e0523ba03c1d23ae0bc1d702ba`
- 请求的英文布局：`67699721`（`0x04090409`）。

正式入口前的 `native-minimize-06/07` 也完成输入及正常退出；07 尚未把像素检查接入最终判定。
07 两张 PNG 的中心像素由独立标准库解码与 Pillow 均确认 `(48,64,80)`，单独查看恢复图为蓝灰填充层。
批量图片查看曾把后续图片显示为黑色；原 PNG 数据不黑，不能据此宣称生产恢复黑屏。
无最小化的 `capture-repeat-01` 三张连续 PNG 中心也相同；该诊断不作为最小化通过证据。

## 失败轮次与边界

首轮 Python 32 项中，既有 `test_frozen_child_is_not_a_pass` 在构建期间失败：期望退出 1，实际 0。
其四秒期限贴近三秒心跳阈值；单独三项复跑及后续整套 32/33/36 项均通过。
没有调整该测试期限或改变生产心跳判定；首次失败不能被后续结果覆盖。

`native-minimize-01/02` 在原生 A-down 回调前超时，退出 1、forced_stop=true，未进入最小化。
02 补扫描码后仍失败，已撤回该无效尝试。
03 临时实验记录原布局 `134481924`（`0x08040804`），向自有引擎线程请求已安装英文布局后 A-down 到达。
本轮未采集 WM_KEYDOWN/VK_PROCESSKEY 消息栈，不能把输入法过滤认定为完整根因结论。
本地 GLFW 3.4 源 `src/win32_window.c` 对 VK_PROCESSKEY 的跳过只是支持调查方向的实现事实。

早期把普通切焦接在最小化恢复后：03/05 在 helper-unfocused 前台确认超时；
04 开始前标题栏被其他窗口遮挡，实际命中 HWND 不符。这三轮均失败并强停，不能计为普通切焦通过。
最终最小化模式只验证自己的生命周期契约，默认切焦检查及 CI 中的独立调用仍保留。
本地恢复后辅助窗口的前台失败仍未定因；历史 CI 的普通切焦通过证据见
[原生焦点记录](2026-11-04-cocos-input-focus.md#windows-原生焦点探针补验)。

Windows 手动 `pointer_contracts=true` 已增加独立 `native-minimize` 调用和两类 artifact 路径；
本轮没有提交、推送或触发远端 CI，不能将本地结果宣称为远端成功。

## 同步后补充的远端验证

上述本机批次随后以 `16f8468` 入库。同步后手动执行
[Windows 运行](https://github.com/2468785842/krkr2/actions/runs/41930503911)，
job `126925892312` 全部通过：完整 Release 应用构建、9 项 Cocos CPU 用例、
36 项 Python、单/双窗口指针、普通焦点及独立最小化恢复场景均成功。
四个报告均退出 0、未强停；最小化场景收到一次 activate，符合既有合并规则，不能要求必有 deactivate。
exe SHA256 `23f13fedfdf64950b0370e368b3b16937547ae58a745f8d17571b616b5435d7d`，
PDB `0b38703bbcabacaf1e32f10ae7e856a6fb0546aa245c981638c937da5f7d41a4`，
focus fixture `21ebba88583564de9926218dda51582ba07b9e37bea6a272fca3460797a6f960`。
该脚本身份与早期本机 08 的哈希不同，两轮分别保留，不把旧本机结果冒认为远端产物。
同提交的 [Headless 运行](https://github.com/2468785842/krkr2/actions/runs/41831684168) 也已全部成功。

仍未验证其他平台、IME、物理触摸/手柄、多游戏窗口与最小化组合、
音视频恢复、GL context 丢失/重建或移动端 surface 生命周期。
下一步按[平台契约](../../proposed/architecture/2026-11-03-platform-contracts.md)推进这些独立边界，
并在需要普通切焦本机验收时调查辅助窗口前台失败，保留上述失败轨迹。
