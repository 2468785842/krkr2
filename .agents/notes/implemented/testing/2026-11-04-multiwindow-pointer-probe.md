# 双游戏窗口触摸路由探针

Status: implemented

## 实现与边界

接续 `2f41757` 的隐藏窗口取消抬起修复；该提交的 Windows 本机单窗口证据见
[触摸按钮记录](2026-11-04-touch-button-ownership.md)。

`tools/game-smoke/pointer_windows_repro.py` 通过现有 opt-in GLView 驱动，
创建真实 TJS Window A/B 并按窗口名记录回调。检查：

- A 拖动期间 `B.bringToFront()`，取消抬起到达 A。
- 在 B 开始拖动后结束 A 的旧触点，不得释放 B；B 取消后仍能新点击。
- B 拖动期间 `invalidate B`，失效窗口不再接收回调；路由回到 A。
- A 新拖动与 B 旧触点交错，旧触点不得点击或释放 A；A 新点击仍正常。

控制器等待本轮命令/驱动/脚本确认，结束后严格比较全部指针回调、必要标记与正常退出。
结束旧触点后另由 TJS 检查目标窗口的 `pointerHeld` 和 `System.getKeyState(1, true)`，
记录新的 held 确认后再等待抬起；不允许提前释放的旧事件冒充后续取消。
异常、额外点击/抬起、错误窗口、缺失回调或强停均不能通过。
五项新增 Python 用例只检查证据判定器，不能替代应用交互。

`build-windows.yml` 增加默认 false 的手动 `pointer_contracts` 输入。
开启时编译原有测试驱动，以独立 runtime 副本运行两个探针，普通构建仍关闭驱动。
软件 OpenGL 来自 pal1000/mesa-dist-win `26.2.6` 的 x64 MinGW 包，
URL 固定，压缩包 SHA256
`da0f71d9181b672c1918f3c342f11118c687f6059a46b16502845160733cc388`，
已本机下载核对并检查所需 `opengl32.dll` / `libgallium_wgl.dll` 确实存在。
首次选择的同版 MSVC 包缺少 OpenGL；上游 release notes 的已知回归建议使用 MinGW x64。
首次手动运行 `41809305422` 在构建前主动取消，不算失败/通过证据。
不安装系统驱动，不把 Mesa DLL 加到原应用 artifact；带驱动应用使用
`windows-pointer-test-binaries`，回调报告与截图另存 `windows-pointer-contracts`。

## 本机验证

```sh
python -m unittest discover -s tools/game-smoke -p 'test_*.py' -v
python -m py_compile tools/game-smoke/pointer_windows_repro.py tools/game-smoke/test_pointer_windows_repro.py
/tmp/krkr-dev-tools/bin/actionlint -shellcheck= .github/workflows/build-windows.yml
python scripts/check_core_module_deps.py
git diff --check
```

22 项 Python 用例全通过；actionlint 与 diff 检查退出 0；11 个基础目标依赖检查通过。
actionlint 同时发现工作流原有未定义 `matrix.os`，已改成实际 `runner.os`。
本机只有独立范围依赖，没有完整 Cocos 应用，未声称本地运行 Window 场景。
`39924c2` 的 [Windows 运行](https://github.com/2468785842/krkr2/actions/runs/41809419898)
job `126504627839` 已成功：Release/full/app/tests=ON、测试驱动=ON、管理 UI 默认 ON，
应用链接成功，实际发现并运行 9 项 Cocos CPU 用例全通过（约 0.66 秒），渲染器编译范围检查通过。
22 项 Python 用例通过；两个真实 GLView/TJS 探针均 passed=true、退出 0、forced_stop=false。
该轮尚未包含随后补充的 held 检查点，不把它当作加强后判定器的验收。
exe SHA256 `ce6a0ca04851c0bedd7d3137133a838f3e131cf4c079839a7732f2d812454f25`，
PDB `e25f2b893574eef893ce4f3a9594b3a4833e4efb99ec419705fed7c9fee1b217`，
fixture `7693b3b9037df63519fbceee87b8c1e0363c71dc7b003d814810bf88da33d853`。
两份报告、日志、匹配二进制与截图在该运行的 `windows-pointer-contracts` artifact。

针对加强 held 检查后的 `9ff3c06`，Windows 复验为 `41810540535`；首次 dispatch 返回 502，
确认未创建运行后重试成功。job `126508387474` 已成功，9 项 Cocos 用例全通过（约 0.62 秒），
22 项 Python 通过，两个 GLView/TJS 探针 passed=true、退出 0、未强停，
实际记录了 `pointer-window-b-held` / `pointer-window-a-held`。
exe SHA256 `53087bfd3e46341d900173468553c13407fa3e4b7ec4cc7c3fa23e0ad394fa44`，
PDB `396c8775491d3d69c001cb1b35ba471e55d408a494623afdf1d63853160ea002`，
fixture `e66d9075ea6df47436685e293fabb4581542d8ee998583fcda78f0bac6917971`。
`3e5072a` 的 [Headless 运行](https://github.com/2468785842/krkr2/actions/runs/41809302309)
已完成，13/13 job 全通过，包含三平台各 26 项平台契约、SDL3 和 game-smoke Python 检查。
`9ff3c06` 的 [Headless 复验](https://github.com/2468785842/krkr2/actions/runs/41810267501)
同样 13/13 job 全通过。

## 切换窗口后节点未复位

下载并核对首轮 artifact `13312106973`，ZIP SHA256
`10812b4cce68c67423e2bf9801483b98cdf38806792cda859800ec11d7494e67`。
解压报告和截图至 `out/windows-ci/41809419898/` 并实际查看四张图；RGB 数据补充核对：
单窗口起始/结束、双窗口结束的中心 `(480,320)` 均为 `(48,64,80)`；
双窗口起始为黑色 `(0,0,0)`，宿主菜单图标仍可见。
不能根据图片展示器的差异把相同哈希的文件算作不同画面；本轮单窗口结束与双窗口结束 PNG
实际相同，SHA256 `ab612b7831f86c0d918ead9a9206c8371dd5bcfea8427a4701184b3def84403d`。
双窗口起始 SHA256 `4b368fb0c83d0b8ac5c35e4e3b255db4dcc96cce4df55b13a3c19ac1a569fb70`。

`TVPWindowLayer::BringToFront` 把旧窗口放在 `viewSize.width`，却不把新目标恢复到原点；
`SetVisible(false)` 选回旧窗口时同样没有恢复位置。失效回退的 `leaveCurrentWindow` 则已有复位。
`d8f9a6d` 给前两处补上原点恢复，不改变管理 overlay 的动画切换入口。
探针增加隐藏 B 后 A 的新点击，以及每条 down/up 的真实局部坐标，要求位于 640×480 场景内；
同时保留切 B 后截图，以区分 A 的蓝灰层与 B 的绿层。Python 现为 24 项，全通过。
新增轻量报告 artifact，避免只查看图片也必须下载两份匹配 exe/PDB 副本。
前两轮结果不计入新增坐标检查。
`d8f9a6d` 的 [Headless 验收](https://github.com/2468785842/krkr2/actions/runs/41811489260)
13/13 job 已全通过；新增坐标判定的 Python 24 项由 launch 范围实际执行。
修复后的 [Windows 运行](https://github.com/2468785842/krkr2/actions/runs/41811492400)，
job `126511583199` 已全部成功：完整应用构建、9 项 Cocos CPU 用例（约 0.63 秒）、
渲染器范围检查、24 项 Python 和两个真实场景通过；报告均退出 0、未强停。
双窗口实际坐标包括按下 `(319,239)` 与拖动抬起 `(391,239)`，全部在场景内；
两个 held 检查点、隐藏 B 后 A 的新点击，以及 B 失效后的旧触点隔离全部通过。
exe SHA256 `76c612441cd76b1128990e5bc13a57a21cafdefe88fd44df11ba2f4bb669ff2f`，
PDB `19f3005524cdb4a09e78cacd76047512826aee99ce08ffaf3f1eb0cdfe09a9ef`，
fixture `e29071f03cf3d608180b71842f9c4130bbd5ee926c95485766923c0fd2b09adc`。

轻量 artifact `13312193820` 约 40 KB，SHA256
`7b88de05a2c81f25ce62a244ae021d03009db2febf8740569bd7cf4e211a9e79`，
已下载核对并查看图片，保留在 `out/windows-ci/41811492400/`。
双窗口 ready 的中心从黑色恢复 `(48,64,80)`；切 B 后为 `(80,96,48)`，
失效回到 A 后又为 `(48,64,80)`。单窗口起始/结束的数据也均为预期蓝灰。
双窗口 ready PNG SHA256 `93c3c18324c8bd2aba1d49dcfa4c30f8ebeff91ecb097804aaa22dffcb8f6e84`。
这些颜色核对只确认自制固定填充层，不声称完整渲染器、UI 动画或游戏画面回归通过。

## 未验证

这套工具检查组件输入到真实 TJS 回调，不证明物理触摸屏、混合设备仲裁、OS 切焦、DPI/IME，
图像只核对自制固定填充层。此前单窗口截图黑底差异仍待查，不能以回调通过替代画面稳定性。
应用编译/9 项 Cocos CPU 测试与实际双窗口轨迹必须分别记录结果。
