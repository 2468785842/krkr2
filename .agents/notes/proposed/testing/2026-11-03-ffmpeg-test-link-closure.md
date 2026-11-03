# FFmpeg 单元测试目标的宿主链接闭包

Status: proposed

## 问题与事实

2026-11-03 的 Windows Debug full 测试构建在 `tests/unit-tests/core/movie/ffmpeg.exe`
链接失败，日志为 `out/windows/b2-continue-debug-build2.log`，最后报告 81 个未解析符号。
包含 `tTVPApplication::RegisterActiveEvent`、`TVPGetRoughTickCount32`、
`TVPShowPopMenu`、`TVPGetCocosWindowPrimaryArea` 和 `TVPYUVSprite::create`。
其中窗口节点适配函数由本批 B2 新增，不能假设全部问题都与本批无关。
应用目标已构建成功，定向 CTest 通过不覆盖这个失败目标。
工作基线为 `b3765f7` 加未提交 B2 改动；验证详情见[窗口接口](../../../../docs/subsystems/window-interface.md)。

## 建议与下一步

先读 `tests/unit-tests/core/movie/CMakeLists.txt`、测试源码及 movie/environ/visual 的 CMake，
取得 ffmpeg 目标的实际链接命令，逐项定位符号实现归属。
区分测试本身必须使用的宿主实现与因静态库对象粒度被带入的其他功能，
再选择补齐真实依赖或缩小被测对象边界。需要基线对照时使用隔离构建，保留当前未提交改动。

## 完成条件

真实 ffmpeg 测试目标链接成功；`ctest -N` 能看到对应注册用例并实际运行通过。
报告说明用了哪些生产实现、仍缺哪些集成验证，并检查修改后的模块依赖。

## 备选与风险

直接链入完整应用壳可能扩大测试依赖甚至产生循环；先依据链接证据判断。
不以空实现吞掉媒体、窗口或生命周期功能，也不通过取消注册把失败目标变成通过。
若原本需要集成测试，应明确该边界并保留等价行为覆盖。
