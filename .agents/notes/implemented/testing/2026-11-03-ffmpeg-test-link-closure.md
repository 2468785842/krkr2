# FFmpeg 初始化测试的链接边界

Status: implemented

## 原问题

基线 `0f163a2` 的 full 范围中，`ffmpeg` 测试只调用 `TVPInitLibAVCodec`，
该函数却与视频工厂和文件探测同在 `krffmpeg.cpp`。
静态库带入整个对象后引用播放器、窗口和设备宿主；测试链接 movie/sound/visual/utils，
没有 environ，实际重链报告 LNK1120 的 81 个未解析符号。
基线命令和日志在 `out/windows/ffmpeg-link-closure/baseline-commands.txt`、
`baseline-link-rule.txt`、`baseline-build.log`。

## 决定

- 原初始化函数与 FFmpeg 锁管理回调移到 `cpp/core/movie/ffmpeg/FFmpegInit.cpp`，
  由现有 `core_movie_module` 编译。调用顺序、异常和重复调用行为保持不变。
  工厂及文件探测继续使用原生产实现，没有删除媒体功能或窗口借用适配器。
- `krffmpeg.cpp` 显式包含初始化声明，初始化测试不再带入该工厂对象。
  dumpbin 显示新初始化对象的引擎引用只有原 `TVPThrowExceptionMessage`，没有窗口或播放器引用。
- 拆分后仅剩 TJS 的 `TVPGetMessageByLocale` 未解析引用，来自 `tTJSBinaryStream::ReadBuffer`。
  初始化测试的 main 与既有独立 TJS 宿主一样返回消息键，不创建 locale 管理器或 GUI。
  这只提供测试宿主的文本回退，生产异常函数和初始化函数均真实链接。
- 原 `init ffmpeg` 用例增加 codec、demuxer、filter 注册结果及重复初始化不破坏注册的 6 个断言。

## 验证（2026-11-03）

在 `out/windows/engine-config-check` 的 Debug/full 配置中使用本机现有依赖：

```sh
cmake --build out/windows/engine-config-check --target ffmpeg
ctest --test-dir out/windows/engine-config-check -N -R "^init ffmpeg$"
ctest --test-dir out/windows/engine-config-check -R "^init ffmpeg$" --output-on-failure --no-tests=error
out/windows/engine-config-check/tests/unit-tests/core/movie/ffmpeg.exe "init ffmpeg" --reporter console
python scripts/check_core_module_deps.py
```

目标构建成功，CTest 实际注册并运行 1/1，直接运行得到 6 个断言通过，依赖守卫 10 目标通过。
测试 exe SHA256 为 `4ce60fba6b81f25932e6b3205d794e9739f03246ad024b18efc32a6aa8950411`，27,057,664 字节。
修复基于上述提交加当前未提交修改。构建日志为 `headless-locale-build.log`，对象符号在 `init-symbols.txt`。
期间 CLion 正在重装 Cocos，一次链接因安装目录暂缺库报 LNK1104；等待安装完成后重新验证，
不把这一环境状态算作源码测试结果。

## 取舍与边界

没有通过把整个 environ 链入测试或允许未解析符号来掩盖问题。
当前 full 配置和声明的静态依赖仍含 Cocos 等域库；本次没有建立独立的最小 FFmpeg 配置，
也不宣称消除了 movie 的全部宿主依赖。
通过结果仅覆盖注册和重复初始化，不代表解码、网络、音画同步、真实播放或完整测试套件通过。
窗口和媒体整体边界见[当前架构](../../../../docs/architecture.md)，
后续[访问冲突调查](../../proposed/bug-fix/2026-11-03-ui-contract-access-violation.md)仍未关闭。

后续输入/帧钩子接入后，完整配置再次实际链接 FFmpeg 测试并运行 10 项定向 CTest，全部通过；
本次新产物、冷链接日志及排除旧产物结果的说明见[接续验证](2026-11-03-platform-contracts-and-sdl3-probe.md)。
