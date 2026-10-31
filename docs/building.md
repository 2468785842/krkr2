# 构建、验证与测量

依赖准备见根 README，实际预设见 `CMakePresets.json`，CI 命令见 `.github/workflows/`。
本页命令用于仓库根目录，不依赖个人绝对路径。

## 桌面构建

需要 CMake/Ninja、C++ 工具链、Python、Bison 和 `VCPKG_ROOT`；Windows 在 MSVC x64 开发环境运行，并把 win_bison 加入 PATH。vcpkg 基线与 overlay 已记录在配置文件中，不为修复局部问题随意升级全仓依赖。

| 平台 | 配置预设 | 构建预设 | 测试目录 |
|---|---|---|---|
| Windows | `Windows Debug Config` | `Windows Debug Build` | `out/windows/debug` |
| Linux | `Linux Debug Config` | `Linux Debug Build` | `out/linux/debug` |
| macOS | `MacOS Debug Config` | `MacOS Debug Build` | `out/macos/debug` |

```sh
cmake --list-presets
cmake --preset "Windows Debug Config" -DENABLE_TESTS=ON -DBUILD_TOOLS=ON
cmake --build --preset "Windows Debug Build"
ctest --test-dir out/windows/debug -N
ctest --test-dir out/windows/debug --output-on-failure --no-tests=error
```

其他桌面平台替换表中的名字和目录。Release 预设将 Debug 替换为 Release。
现有根开关是 `ENABLE_TESTS`、`BUILD_TOOLS`；关闭它们不会自动停止构建主程序、插件或安装根清单的全部依赖。
选择目标可用 `cmake --build --preset "Windows Debug Build" --target xp3`；它仍需要根配置成功。

Android 使用 README 所列 JDK/SDK/NDK，在对应平台运行 `platforms/android/gradlew`，例如：

```sh
./platforms/android/gradlew -p ./platforms/android assembleDebug
```

Windows 使用 `platforms/android/gradlew.bat`。Gradle/NDK 验证不能用桌面 CMake 通过代替。

## 按改动选择验证

| 改动 | 最低相关验证 |
|---|---|
| 文档 | 本地链接/路径、代码块中的预设与开关、`git diff --check` |
| 基础库/工具 | 受影响目标构建，相关 Catch2；改依赖再运行 `python scripts/check_core_module_deps.py` |
| TJS/插件 | 相关测试、最小脚本及宿主回调；记录异常与对象释放行为 |
| 图像算法 | 合成像素用例、旧实现对照、裁剪/非紧密 pitch/alpha/边界；约定精确或容差比较 |
| 平台/渲染 | 目标平台窗口、DPI、输入、IME、缩放、全屏、暂停恢复及关闭；截图和事件序列对照 |
| 音视频 | 播放/暂停/seek/结束、音画同步、异常文件、线程及资源释放 |

新增测试要在对应子目录 CMake 的 SOURCES 中登记。先 `ctest -N` 确认注册，再运行；当前 visual 测试清单为空，需为图像替换建立实际用例。CI 部分构建关闭测试，构建徽章不代表全量测试通过。
后端改动还需实际游戏或可分发的自制 TJS/KAG fixture。报告资源版本/哈希、输入步骤、平台和结果；不把仅能启动当作兼容完成。

## 编译提速的基线

分开记录：依赖首次安装、配置、冷编译、缓存编译、无改动构建、修改公共头后重建和链接。
每次记录提交、操作系统/CPU、编译器、构建类型、并发数、vcpkg 基线、缓存是否命中、耗时及产物大小。
同机相同配置比较；不要把缓存命中与换库收益混为一谈，不为测冷构建清除用户全局缓存。

当前 TJS CMake 同时存在配置期 Bison 调用和构建期生成规则；日期词表也在配置期生成。这是可独立评估的构建任务，修复时须确认输出、依赖和无改动构建行为。

## 无法完成验证时

保留命令、退出码和最早的有效错误，区分依赖安装/配置/编译/链接/测试/游戏回归阶段。
不要把“环境缺库”写成源码测试失败，也不要把未运行写成通过。本文建立时只核对命令与源码，未重新执行全平台完整构建。
