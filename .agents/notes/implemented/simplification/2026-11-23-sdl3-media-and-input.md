# SDL3 游戏音视频和桌面输入

Status: implemented

Linux SDL3 宿主将 FFmpeg 的 YUV420P 视频提交到各自游戏窗口的 SDL 纹理，沿用脚本的矩形、
显隐、结束及停止契约。播放结束或关闭后清理画面。SDL3 的 X11 和 Wayland 保留原有窗口呈现路径。
[视频覆盖层](../../../../docs/subsystems/render-presentation.md#视频覆盖层)

`Window.onFileDrop` 每次收到一次拖放的存储路径数组。`Window.useMouseKey` 复用普通指针事件和
共享按键状态。SDL 在失焦通知前投递人工键释放，宿主用取消释放结束模拟按钮，避免失焦产生额外单击。
`Window.enableTouch` 接收 SDL 触点，保留设备和手指身份，并向脚本提供稳定整数 ID、触点查询和速度。
双指缩放按距离阈值开始，旋转参数使用弧度并沿用已有移动阈值和开始标记。实现参考了上游的脚本契约。
[窗口接口](../../../../docs/subsystems/window-interface.md#linux-sdl3-窗口契约) ·
[上游触摸契约](https://github.com/krkrz/krkrz/blob/fd5c4baa6a2ef5978db1bd043634351f48667daf/environ/win32/TouchPoint.h)

原始音视频复现能进入正常结束和点击跳过后的剧情，但 `System.exit()` 在
`tTVPWaveSoundBufferThread::~tTVPWaveSoundBufferThread` 等待未停止的线程。GDB 的主线程停在
`std::thread::join`，音频工作线程仍在等待下一轮混音。销毁时现在先发出停止信号，再唤醒并等待线程。
线程启动等待使用谓词，停止状态通过原子变量共享。

## 验收范围

- KAG 使用固定提交 `1f3ab309106d210e3169bbbe0fb4e066ae463b42` 的框架与自制场景。
  用正式命令 `krkr2 --host=sdl3 --game PATH` 启动，读取电影双彩图像和结束后恢复的画面。
  从 OpenAL 文件输出检测背景音乐、音效和电影的三个测试音。
- X11 用 GTK 发起系统文件拖放，用 XTest 验证方向键移动、左右键和失焦取消。
  触摸用显式测试构建的 `SDL_PushEvent` 验证双指、黑边映射、手势与取消。
  触摸的证据范围为 SDL 事件到游戏脚本。
- 本地另在嵌套 Weston 的 2× 虚拟输出验证了视频开始、清除、再次播放和退出，以及键盘指针像素及点击。
  Wayland 以 SDL 的 EGL 纹理路径呈现。

## 实际结果

- `9dfcf4b` 的 Linux Release CI 使用 `sdl3_desktop_contract=true` 和
  `imageops_contracts=true`，通过 KAG 音视频和桌面输入探针，以及窗口、Fcitx5、KAG 存读档、CLI、
  Cocos 窗口、视频和图像操作的既有回归。两项新探针均以 0 退出，无强制停止。
  [Linux CI](https://github.com/2468785842/krkr2/actions/runs/45127268470) ·
  [截图、音频与报告](https://github.com/2468785842/krkr2/actions/runs/45127268470/artifacts/14795838976)
- 同一 Linux 可执行文件 SHA-256 为
  `2525b90e922a9cec7b74ac1ab38bd8223b5fd3c90ddca5bc4ab6b9516ba53d8b`，
  KAG 音视频脚本为 `c90fa990f300634d4d4c8d113532b5cea22935d3bc8d004873000e8dd75fa042`，
  桌面输入脚本为 `9f82cf82b8e404f87362691dfb15b11d4129dbebdf61da0409144932e26ca5ac`。
  CI 捕获 440、660、880 Hz 三种声音，点击跳过后在 0.101 秒推进剧情。
- 本地的 52 项平台契约、49 项游戏工具测试及 foundation 依赖检查通过。
- 同一提交的 Windows Release CI 启用 `pointer_contracts=true`，通过 CLI、Cocos 窗口契约和
  真实窗口的输入、文本及生命周期回归。
  [Windows CI](https://github.com/2468785842/krkr2/actions/runs/45127328004)
