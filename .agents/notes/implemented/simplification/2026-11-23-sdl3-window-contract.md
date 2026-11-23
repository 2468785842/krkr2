# SDL3 正式游戏窗口契约

Status: implemented

基于 `6412549` 的桌面命令行入口。SDL3 为每个 TJS Window 创建原生窗口并持有独立呈现纹理。
窗口尺寸和主图层尺寸分别维护，SDL 使用同一逻辑坐标变换进行像素呈现、指针定位、光标设置和 IME 锚点。
程序关闭通过同步查询，窗口管理器请求进入引擎输入队列。模态期间其他游戏窗口的输入和关闭请求被拦截，
接受关闭后返回脚本。销毁时先撤销焦点和输入状态，再清理窗口资源。
[窗口接口](../../../../docs/subsystems/window-interface.md#linux-sdl3-窗口契约)

## 实际验证

- Linux Release 应用测试开启 `sdl3_host_contract=true` 和 `imageops_contracts=true`，正式启动命令为
  `krkr2 --host=sdl3 --game PATH`。自制场景由 Xvfb/X11、XTest 和 WM_DELETE_WINDOW 驱动，
  外部读回实际 X11 窗口像素。[构建与探针命令](../../../../docs/subsystems/desktop-cli.md#linux-sdl3-游戏宿主)
- `bbea9e6` 通过窗口从 320×240 调整为 640×360、两侧黑边读回、黑边拒绝新按下、窗口外拖动和释放、
  滚轮和右键、失焦释放及重新聚焦、脚本 3:2 缩放到 480×360，以及隐藏后重映射同一个窗口。
  [缩放与输入 CI](https://github.com/2468785842/krkr2/actions/runs/45029601428)
- X11 输入法通过 Compose 序列提交“中文𠀀”，TJS 收到 UTF-16 单元
  `20013`、`25991`、`55360`、`56320`。关闭 IME 时按 C 键无文本提交，重新开启后序列可用。
- `99c3069` 的双窗口场景读回模态画布，向主窗投递按键和关闭请求并确认被拦截。
  模态第一次关闭否决后仍接收按键，第二次允许关闭后主窗重新接收按键并以 0 退出。
  [模态 CI](https://github.com/2468785842/krkr2/actions/runs/45094871972)
- `5b5ed01` 在同一场景核对了左键双击和模态返回后对隐藏窗口的同步关闭查询。
  `modal:returned:0`、`modal-close:3`、`modal:program-close-returned` 依次出现，
  主窗口最终退出码为 0，无强制停止。可执行文件 SHA-256 为
  `20ee55fe0eb12a983af04e972e0313d1746fb481d948084a7f7221998cc7bbcf`，
  场景 SHA-256 为 `00392aa45d957da35d19c4b7194db2dca56c7190c887b53804c3a8cf0a94bc5b`。
  [Linux CI](https://github.com/2468785842/krkr2/actions/runs/45095018517) · [验收报告](https://github.com/2468785842/krkr2/actions/runs/45095018517/artifacts/14783540634)
- 同一提交的 Linux CI 通过 15 项 CLI 检查、17 项 Cocos 窗口契约、视频和图像操作场景。
  Windows 构建以 `pointer_contracts=true` 通过完整 Cocos 应用的输入、模态和 CLI 回归。
  [Windows 回归](https://github.com/2468785842/krkr2/actions/runs/45095045316)
- 本地 52 项平台用例、私有 Xvfb 的 4 项辅助窗口用例及 foundation 依赖检查通过。
  [独立三平台 CI](https://github.com/2468785842/krkr2/actions/runs/45029952311)通过 13 个任务。

首个模态构建失败于 `TVPPostEvent` 所需的非 const 事件名引用。复用持久的 TJS 事件名后编译及运行通过。
[失败构建](https://github.com/2468785842/krkr2/actions/runs/45029949861)

本次实测范围为 Linux/X11 软件呈现与键鼠。Wayland、物理高 DPI、全屏和最小化的窗口管理器行为、系统输入法候选窗、
触摸手势、键盘模拟鼠标、文件拖放、KAG 与媒体需要独立验收。
