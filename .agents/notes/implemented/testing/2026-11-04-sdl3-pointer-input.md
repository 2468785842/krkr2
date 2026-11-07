# SDL3 原型鼠标、视口与原生拖动

Status: implemented

## 范围与实现

实现提交 `d0537c5`（基于 `881554c`），仅扩展默认关闭的 `tools/sdl3-prototype`，
没有替换生产 Window/Layer、Cocos 输入或渲染器。
`SdlPointerInput` 将 SDL 鼠标事件转换为共享 VK 状态、修饰键快照与整数坐标；
位置通过 SDL 的窗口到渲染坐标 API，再使用共享向零截断，不裁剪黑边或窗口外位置。
SDL 顶部原点不翻转 Y，滚轮方向归一但保留分数与未缩放增量。
失焦先清空全部共享状态，分别补发键盘/鼠标抬起，再通知脚本。

`canvasWidth/Height` 可独立于窗口尺寸，缺省沿用窗口尺寸。
新增可调整大小及高像素密度窗口标记，但没有因此宣称高 DPI 验收。
鼠标回调单独写入 JSONL，原有键盘事件序列不混入鼠标日志。
呈现日志仅记录初始和变化后的实际窗口、像素、逻辑尺寸及视口，供原生探针等待就绪。
稳定使用说明与回调参数见[原型 README](../../../../tools/sdl3-prototype/README.md)。

## 本地证据

Linux GCC Debug，`out/linux/sdl3-focus`：应用/工具/测试关闭，
`KRKR2_BUILD_SDL3_PROTOTYPE=ON`，实际重新构建 `krkr2_sdl3_prototype`。
SDL 官方 3.4.8 归档与仓库 pin port 的 SHA512 相同，依赖准备同[前批记录](2026-11-03-sdl3-focus-validation.md)。
最终可执行文件 SHA256：
`213b44c88b6c5b8cbd40cd38df42eaf03cf89c7cf5a85de240a6f00f12599842`；
鼠标脚本 SHA256：`530561e7c375f71cdaff9b9835ebcdbb552f27c5274827673b9aaf5939e57f3b`。

```sh
cmake --build out/linux/sdl3-focus --target krkr2_sdl3_prototype -j 4
xvfb-run -a python tools/sdl3-prototype/verify.py out/linux/sdl3-focus/tools/sdl3-prototype/krkr2_sdl3_prototype out/linux/final-verify-pointer-01 --pointer
xvfb-run -a python tools/sdl3-prototype/verify_x11.py out/linux/sdl3-focus/tools/sdl3-prototype/krkr2_sdl3_prototype out/linux/final-verify_x11-pointer-01 --pointer
```

上述两项 passed=true、退出 0、未强停。另以相同最终 exe 执行未加 `--pointer` 的两个验证器，
保存在 `out/linux/final-verify-keyboard-01`、`final-verify_x11-keyboard-01`，均通过。
队列鼠标检查八帧逻辑画布 BMP 的 13 个像素、完整按下/抬起/滚轮序列及有序拖动位置；
还检查外窗口、失焦输入和冗余抬起被忽略。
X11 窗口就绪 unittest 实际运行 1 项、3 个断言，通过。

原生探针经 XTest/X server，不直接投递 SDL 事件：

- 800×480 窗口、640×480 画布，视口 `[80,0,640,480]`；窗口 `(180,120)` 对应逻辑 `(100,120)`。
- 左键按住移动到黑边 `(40,120)`，逻辑 x=-40；再移到窗口外 `(850,120)`，逻辑 x=770，
  仍收到移动和抬起，验证 SDL 默认自动捕获。
- 调整到 1200×720，等待视口 `[120,0,960,720]`；窗口 `(270,180)` 仍对应逻辑 `(100,120)`。
- 右键按住切焦，收到且只收到一次无残留标记的抬起；恢复后再次按下/抬起正确。
- 观察到 3 次按下、3 次抬起、1 次滚轮、1 次失焦、1 次恢复；本机失焦右键由共享状态补发。
  SDL 若先补发鼠标抬起也可接受，但不能重复。第一次 OS close 否决仍存活，第二次正常退出。

正常读回图像已人工检查。负向场景故意保留错误失焦背景、将脚本矩形位置右移 16，
进程均退出 0，验证器退出 1，分别定位 `unfocused.bmp` 和 `pointer-down.bmp` 的像素错误。
`clang-format 20 --dry-run --Werror` 通过；11 个 foundation 显式依赖/直接包含检查通过。
没有修改 core_input，因此未重复其已通过的 18 项测试；远端 CI 仍运行三平台该范围。

## CI 与剩余范围

Headless 工作流加入鼠标队列、原生鼠标/调整大小、反向像素检查并上传产物；
依赖闭包限制继续排除 Cocos/OpenCV/FFmpeg/Catch2/libarchive/unrar。
提交 `d0537c5` 的[Headless CI](https://github.com/2468785842/krkr2/actions/runs/41749387360)
中 SDL3 job `126310388516` 已成功：实际安装依赖、构建、键盘/鼠标队列、
X11 就绪回归 1 项、原生键盘/鼠标、两项反向像素、依赖闭包和产物上传全部通过。
四项正向验证均 passed=true、退出 0、未强停；鼠标队列计数 2 down/2 up，
原生计数 3 down/3 up，窗口与像素宽度从 800 变为 1200，逻辑宽度保持 640。
该运行的全部 13 个 job 已通过，包含三平台各 18 项平台契约及归档、图像、启动范围。
CI exe SHA256：`e5237f31ff736b300e095a75385cf63a213e31519ffde343a607e705017750db`。
本批没有修改生产 Cocos，未重复完整 Windows 应用构建；上一批同路径验证见[指针契约记录](2026-11-04-pointer-coordinates.md)。

本批不验证物理鼠标、高 DPI/多屏、Windows/macOS 鼠标、组合多设备、触摸、IME、
捕获期间销毁或生产游戏兼容。窗口 1.5 倍调整只证明视口缩放，不能冒充高 DPI 设备证据。
下一步仍按[平台契约](../../proposed/architecture/2026-11-03-platform-contracts.md)建立现有后端
真实失焦/后台基线，再推进生产接入，不扩张本原型 API 为未经验证的完整窗口接口。
