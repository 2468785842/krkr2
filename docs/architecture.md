# 当前架构与代码导航

核对日期：2026-11-25。当前仅支持 Windows/Linux；Android、macOS/iOS 暂停构建。
Cocos port、宿主、管理 UI、平台入口及打包资源已经移除，恢复其他平台需接入新的宿主实现。
历史实现可查 Git 基线 `c494ea0`，不能作为当前构建说明。

## 运行链路

```text
Windows/Linux 入口 → SDL3 宿主 / 原生菜单与对话框
                         ↓
          base 脚本管理 / 事件 / KAG → TJS2
                         ↓
     visual 软件图层 / sound / movie / 原生插件
                         ↓
          io 文件后端 / storage / archive
```

`platforms/windows/main.cpp` 和 `platforms/linux/sdl_main.cpp` 经
`environ/launch/DesktopLaunch.cpp` 解析命令行，在归档校验前通过
`environ/sdl3/NativeServices.cpp` 安装本地资源与配置服务，再进入 `SdlHost.cpp`。
无目标时显示错误；`--help` 和 `--list-settings` 不创建游戏窗口。

## 模块与依赖

- 根 CMake 使用 C++17；`cpp/core/CMakeLists.txt` 的 `krkr2core` 是 INTERFACE 聚合目标，
  domain 静态库仍有互依赖，不表示已经完成单向分层。
- `tjs2/` 提供编译与执行；`base/ScriptMgnIntf.*` 处理源码、表达式、字节码及存储加载。
- `input/` 保存共享虚拟键、修饰键、坐标和文本状态；OS 输入、IME 与事件排队由 SDL3 宿主负责。
- `visual/WindowLayer.h` 是窗口接口；`environ/sdl3/SdlHost.cpp` 实现生产窗口、输入、帧循环和视频呈现。
- `visual/RenderManager.cpp` 只注册软件渲染器；`visual/TexturePresentation.h` 描述借用的像素和行布局。
  旧 `ogl/RenderManager_ogl.cpp` 保留算法源码供后续 GPU 后端工作，但不编译或注册。
  motionplayer 的私有 GL 消费者仍在，不能据此删除全部 OpenGL/GLEW 依赖。
- `imageops/` 使用独立 CPU 图像算法与标准 C++17 行任务，生产依赖不含 OpenCV。
- `event/FrameStep.*` 与 `MainThreadMessageQueue.*` 处理共享帧推进和消息；队列由宿主持有并驱动。
- `environ/resources/` 提供本地资源读取；`ConfigManager/` 使用 JSON，保留 pugixml 旧 XML 只读迁移。
- Windows 使用 Win32 原生菜单/文件对话框与 SDL3 消息框，Linux 使用 GTK3；没有旧游戏管理器 UI。
- `sound/` 和 `movie/ffmpeg/` 保留正式音视频实现；`plugin/`、`cpp/plugins/` 保留原生插件注册与行为。
- `io/`、`storage/` 和 `archive/` 保持文件后端、存储介质和归档职责；XP3 工具只配置必要基础层。

## 构建与验证边界

普通 Windows/Linux 预设默认构建 SDL3；原 SDL3 预设名继续可用。
不再存在 `KRKR2_SDL3_ONLY`、`KRKR2_COCOS_FREE`、`cocos` 或 `crash-upload` feature。
基础模块检查只覆盖显式依赖和指定源码约束，不证明 domain 无环。
构建、脚本与游戏验收分别报告，见[构建指南](guides/building.md)、
[桌面运行时](subsystems/native-runtime.md)和[Windows 宿主](subsystems/windows-sdl3.md)。
