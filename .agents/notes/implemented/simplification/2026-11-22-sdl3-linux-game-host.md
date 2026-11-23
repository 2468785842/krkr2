# Linux SDL3 正式游戏入口与自制场景验收

Status: implemented

本页记录首个单窗口场景。[后续 SDL3 窗口契约](2026-11-23-sdl3-window-contract.md)记录扩展后的接口与验收。

Linux 完整应用启用 `KRKR2_ENABLE_SDL3_HOST` 后，可用 `krkr2 --host=sdl3 --game PATH`
加载正式 TJS 启动脚本。SDL3 宿主接入生产 `Window`、`Layer`、`System` 与引擎帧循环，使用软件渲染的
RGBA 帧和 SDL3 原生窗口表面。Cocos 仍是默认宿主，并提供部分共享服务。

## CI 故障与修复

- [初始诊断](https://github.com/2468785842/krkr2/actions/runs/45022292262)中，正式应用启动成功。
  引擎的 RGBA 帧与 SDL 渲染器读回都得到 `204060ff`，X11 窗口读回为 `000000`，探针超时后强制停止。
- 单独构建 SDL 3.4.8 并在 Xvfb 重现了两份读回值的差异。关闭 SDL 软件表面的隐式 GPU 加速后，
  X11 窗口同一点回到 `204060`。`eb49c61` 在生产宿主采用原生软件表面。
- `69bd313` 禁用 SDL 在最后一个窗口收到关闭请求时附带的退出事件，由引擎脚本处理关闭查询。

## 验证

CI 启用 `sdl3_host_contract=true` 和 `imageops_contracts=true`，用 Linux Release 运行以下探针。

```sh
xvfb-run -a -s '-screen 0 1024x768x24' python tools/game-smoke/sdl_host_repro.py \
  out/linux/release/bin/krkr2/krkr2 out/linux/sdl-host-runtime
```

最终生产提交 `69bd313` 的报告记录了 `ready`、`close:1`、
`key-down:65:1`、`key-up:65:0`、`mouse-down:240:60:1`、`mouse-up:240:60:0` 和 `close:2`。
三个 X11 采样点先为 `204060 / 204060 / 204060`，首次关闭否决后的键盘输入把第一个点改为 `d03020`，
鼠标左键把第二个点改为 `30c050`。第二次关闭后进程退出 0，无强制停止。
见 [Linux CI](https://github.com/2468785842/krkr2/actions/runs/45022997981)及产物
[`linux-sdl-host-runtime`](https://github.com/2468785842/krkr2/actions/runs/45022997981/artifacts/14754523295)。
可执行文件 SHA-256 为 `27a50ab23aabca674bed6ea9af585a54e4a7827f54d3782cb13976f229d52088`。
场景 SHA-256 为 `bea32c1bacb89e71c27e80c58bc1df6c9543f755e8964bb33c4df2933b360cf0`。

同一次 Linux CI 也通过了 17 项 Cocos 窗口契约、8 组图像配置、自制视频和 OpenCV 依赖检查。
同一提交的[无窗口 CI](https://github.com/2468785842/krkr2/actions/runs/45023087641)通过全部 13 个任务。
本地通过 9 项启动配置用例、foundation 依赖检查及 actionlint。

当前验收范围为 Linux/X11、软件呈现、单窗口及自制场景。其他平台、Wayland、高 DPI、IME、KAG 与媒体属于后续验收范围。
