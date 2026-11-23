# SDL3 桌面与 KAG 验收

Status: implemented

Linux SDL3 正式入口现在可以直接连接 Wayland。X11 继续使用软件窗口表面。Wayland 通过 EGL
呈现引擎生成的 RGBA 纹理，启用 libdecor 窗口控件，并在调整原生缓冲区尺寸后补呈现一帧。
SDL3 启动时跳过 Cocos 的 GTK 初始化。游戏调用原生消息框时按需初始化 GTK，必要时使用 SDL 消息框。
[构建与入口](../../../../docs/subsystems/desktop-cli.md#linux-sdl3-游戏宿主)

## 桌面与游戏验收

- Openbox 和嵌套 Weston 均通过正式的 `krkr2 --host=sdl3 --game PATH` 运行。
  通过外部窗口像素、键鼠事件和焦点回调验证全屏、退出全屏、最小化和恢复。
  Weston 使用 libdecor 最小化按钮和 Alt+Tab 恢复。测试在 2× 虚拟输出上读回 640×480 的
  窗口画布，确认恢复后按键已释放，并核对点击映射为游戏坐标 `(65, 165)` 及对应重绘。
- X11 上的 Fcitx5 拼音候选窗随图层输入锚点移动 140 像素、50 像素。通过空格选中候选，
  两次提交均收到“你好”的 TJS 字符码点。候选窗截图和回调记录随测试上传。
- 自制 KAG 场景使用固定的 KAG3 框架提交 `1f3ab309106d210e3169bbbe0fb4e066ae463b42`。
  通过场景菜单进入剧情并存档，退出后启动新进程，通过菜单读档恢复 `f.lamp=42` 和相同文字像素。
  两个进程均以 0 正常退出。读档前后的存档文件 SHA-256 相同。
  [KAG3 框架](https://github.com/krkrz/kag3/tree/1f3ab309106d210e3169bbbe0fb4e066ae463b42)
- 鼠标键、文件拖放和触摸归入后续兼容性工作。当前 KAG 场景的验收流程由键盘和鼠标完成。
  缩放覆盖范围为 Weston 虚拟显示的 2× 整数缩放，输入法候选窗覆盖范围为 X11 上的 Fcitx5。

## CI 与测试身份

Build Linux 工作流启用 `sdl3_desktop_contract=true` 和 `imageops_contracts=true`。
各桌面探针通过 `xvfb-run` 和私有 D-Bus 会话运行。旧 X11 基础探针继续覆盖模态关闭否决。
桌面测试显式安装提供 `xprop` 的 `x11-utils`。Wayland 重绘像素采样位于鼠标热点左侧，
因为 Weston 将可见光标合成到截图中。

- `d663b4e` 的 Linux Release CI 通过四种桌面探针，全部游戏进程正常退出，无强制停止。
  CLI 检查、17 项 Cocos 窗口契约、视频呈现和双后端图像操作回归一并通过。
  [Linux CI](https://github.com/2468785842/krkr2/actions/runs/45099486419) · [截图和报告](https://github.com/2468785842/krkr2/actions/runs/45099486419/artifacts/14785205710)
- 可执行文件 SHA-256 为 `1006ece4849cfe36b4dbabc94bde51e87968738dd95110762a3ea9f581b95874`。
  窗口与输入法场景为 `30d5ff4d4765144781024ef8d6aaa8ae3071577512751bbf2ad31bcbe1a0be5b`。
  KAG 剧本为 `5bb67463e8bb57c86ae41f6008cd37bae8fae558e9fa2fb7add11d2c7d295767`，
  启动钩子为 `a95f4f818ab53f8c69e9e340f24646920c5f1e52cc84b85204afb4ecbd1af2b3`。
  CI 的 KAG 存档为 22 596 字节，SHA-256 为
  `4d7c43a167126df234ca4a5c4f2efcacaea7ace8f9cddb5920f8e4f1b7f466a8`。
- `c0fef90` 的三平台基础 CI 通过 13 个任务，包含 Linux SDL3 独立原型。
  本地游戏工具的 49 项单元测试和 foundation 依赖检查通过。
  [三平台基础 CI](https://github.com/2468785842/krkr2/actions/runs/45098938278)
