# 桌面命令行启动与引擎设置

Status: implemented

桌面启动器和全局、单游戏设置页在 Windows、Linux 和 macOS 构建中均由命令行代替。
`--game`、`--xp3` 或位置参数选择目标。`--list-settings` 显示可用设置，重复 `--set KEY=VALUE`
时同名设置取最后一次。Windows 通过终端应用入口接收 UTF-16 命令行并转换为 UTF-8。
[桌面命令行](../../../../docs/subsystems/desktop-cli.md)

配置覆盖在验证游戏目标后安装，优先于全局及单游戏 XML，并与持久化数据分开。引擎菜单的虚拟鼠标模式
通过 `--set virtual_mouse=1` 开启。Cocos 游玩菜单、脚本文件选择和对话框继续走原接口。
Android/iOS 构建按移动平台打包原有选择器和设置页面。

## 验证

- 生产改动 `c9277c6`。本地 `launch_tests` 10 项及 `tools/game-smoke` Python 49 项全部通过。
  foundation 依赖检查、actionlint、clang-format 和 diff 检查通过。
- [Linux 完整应用](https://github.com/2468785842/krkr2/actions/runs/45026362299)使用
  `sdl3_host_contract=true` 及 `imageops_contracts=true`，通过 15 项正式 CLI 检查、SDL3 自制场景、17 项 Cocos 回归、自制视频及 8 套图像配置。
  `opengl-half` 用 CLI 的 `renderer=opengl` 和 `ogl_max_texsize=512` 覆盖游戏 XML 的 `software` 和 `1024`，
  实际渲染报告得到 OpenGL 和 512，XML 的 SHA-256 保持一致。
  可执行文件 SHA-256 为 `303589e6a53194c0f20f4302e3a5288721176f92ffbb50da9b0d016e1b69641d`。
- [Windows 完整应用](https://github.com/2468785842/krkr2/actions/runs/45026909442)使用 `pointer_contracts=true`，
  通过 15 项正式 CLI 检查、17 项 Cocos 回归、输入及模态交互场景和同一组图像配置覆盖。可执行文件 SHA-256 为
  `1b560fe132a4ff1985c8d9eac01039f0918799bda92f938729af080dd7edf49e`。
  [首次 Windows CI](https://github.com/2468785842/krkr2/actions/runs/45026366711)的双窗口夹具在替换命令文件时读取失败。
  `b0fcb34` 让夹具在下一次定时器回调中重新读取，控制器仍检查当前命令的确认和完整事件序列。
- [三平台独立 CI](https://github.com/2468785842/krkr2/actions/runs/45026834039)通过全部 13 个任务。
  首次运行的独立依赖检查仍只允许原先的解析源文件。`915bfe6` 登记新增的设置校验源，继续检查相同的依赖闭包。

macOS 验证覆盖独立解析及平台契约。下一步扩大 SDL3 实际窗口验收，先测缩放与黑边坐标、失焦释放键鼠和焦点恢复，再接入 IME 文本提交。
