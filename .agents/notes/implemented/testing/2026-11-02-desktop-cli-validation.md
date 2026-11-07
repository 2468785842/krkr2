# Desktop CLI 的 Windows 验证记录

Status: implemented

本记录保留当时的构建与验证事实，结论只适用于下述版本和范围；当前使用方式见对应 docs 指南。

## Windows 本轮验证

本批同时修正 `sysinit/impl/SysInitImpl.cpp` 的两处旧行为：初始化清空参数库存时，
重放初始化前设置的参数；区分本地项目目录与存在的归档文件，避免给散装目录追加归档分隔符。
`TVPGetCommandLine` 查询接口、更新后项优先及参数代数更新仍保留。

| 验证 | 结果 |
|---|---|
| launch-only 配置、编译、CTest | 5/5；编译清单仅 LaunchOptions.cpp / LaunchOptionsTest.cpp |
| 应用与 launch_tests | 链接通过；复用本机现有依赖，不代表全新依赖安装 |
| 应用构建目录的启动 CTest | 6/6，含 CLI 集成检查 |
| CLI help/错误进程检查 | 10 项通过；缺参、损坏 XP3、缺 startup 的真实 XP3、未知选项等按契约处理 |
| 控制器单元检查 | 3/3，含冻结心跳和缺失参数探针拒绝 |
| 无启动目标 | 5 秒运行，日志确认创建选择器且未直接启动游戏；随后测试进程被终止 |
| tslm 目录自动选 data.xp3、--xp3、位置参数 | 各 25 秒通过；含中文/空格路径、持续心跳、参数断言及无选择器日志 |
| 散装 startup 引导、归档中保留素材 | 25 秒通过，实际 KAG 到 title.ks:33，内置参数探针通过 |

原始状态和日志在 `out/ui-game-smoke/tslm-cli-final-*`，内置探针报告在
`tslm-cli-probe/smoke-report.json`；散装引导验证在 `tslm-cli-loose-bootstrap-v2/`。
该散装引导只用自制 startup 注册 data.xp3 的搜索路径，再执行原启动脚本，没有改游戏主体。

**完整散装的商业游戏尚未通过**：解包 data.xp3 的全部文件后，原启动脚本能够执行，
但在 custom.ks 标题界面遇到 `title@背景%base` 图片查找/加载异常，测试明确失败。
不能将目录入口通过等同于全部解包素材兼容；这需要另行检查合成资源的路径映射。

为保留用户正在运行的调试游戏，使用独立 `out/windows/engine-config-check` 验证。
该目录原 zlib 缓存引用已失效的库名，已仅在此目录重新查找现有库；未改全局工具链或安装依赖。
源码使用 Debug 配置，最终验证 exe 通过该目录的临时 CMake hook 关闭 PDB/增量链接，
避免调试符号链接阻塞；不把它用于体积或耗时收益比较，也不替代带符号的调试构建验证。

clang-format 20、模块依赖检查和 diff 检查通过。新增无窗口 CI launch job 覆盖三桌面平台，
已检查 YAML 与本地构建，但未在本轮触发远程 CI；Linux/macOS 的应用入口及 Android/iOS 的
共享初始化改动尚未实机验证。存档恢复、音视频、正常退出和完整游戏兼容不在本轮通过清单中。
