# Windows/Linux 收敛与 Cocos 删除

Status: implemented

## 任务与基线

用户明确要求新建分支，暂停 Android/macOS，并删除 Cocos ports、vcpkg、CMake 等全部集成。
基线 `c494ea0`，分支 `codex/remove-cocos-desktop-only`，开始时工作区干净。
历史平台实现保留于 Git；本次不实现移动或 Apple 替代宿主，不混入 TJS2/Ciallang 工作。

## 本次实现

- 根 CMake 在 `project()` / vcpkg 安装前拒绝 Android、macOS/iOS 与对应 triplet/toolchain；
  支持目标只允许 Windows/Linux，正式应用唯一宿主为 SDL3。
- 删除 Cocos overlay port、`cocos` / `crash-upload` features、Cocos CMake helpers 与旧宿主分支；
  删除 Cocos 窗口、管理 UI、移动文件后端、网络上报和旧平台入口，删除旧 minizip 写入器。
- 删除专属 Cocos 适配测试、UI 资源及 Android CI、Apple 预设/测试矩阵；
  CI 统一接入 `native-desktop.yml`，保留通用测试和原生 Windows/Linux 交互探针。
- 保留独立 PVRTC 解码与许可、软件 Layer、存储、插件及音视频消费者。
  旧 GL 工厂不编译/注册，motionplayer 私有 GL 消费者及其依赖继续保留。
- CLI 默认 SDL3，拒绝其他宿主；设置列表只声明软件渲染。
- 增加 `scripts/check_desktop_sources.py` 与平台配置拒绝回归，更新当前操作文档。

本地旧平台构建缓存及签名文件不删除；根 `.gitignore` 忽略已暂停平台的遗留目录。
历史 notes 和保留算法的版权来源不作为活跃依赖删除。

## 验证

原始日志位于 `out/desktop-*.log`；构建目录 `out/windows/desktop-only`。

- `python scripts/check_core_module_deps.py`：11 个 foundation 的显式依赖与接口约束通过。
- `python scripts/check_desktop_sources.py`：生产源码、旧宿主路径与 vcpkg features 检查通过。
- `python -m unittest discover -s scripts/tests -v`：9 项通过；新增用例覆盖 8 种不支持平台参数，均在安装依赖前拒绝。
- `cmake --list-presets`：Windows 预设可读取，Apple 预设已删除。
- Windows 普通应用：新构建目录、Debug、full、脚本 smoke 开启、UI 驱动关闭；
  vcpkg 安装开启，复用此前不含 Cocos 的 SDL3 SDK，更新当前基线所需依赖。
  这不等同于从空 SDK 安装的闭包证据。
- `cmake --preset "Windows SDL3 Debug Config" -B out/windows/desktop-only`
  附加 `-DKRKR2_TEST_SCOPE=full -DKRKR2_ENABLE_SCRIPT_SMOKE=ON`，SDK 指定为
  `out/windows/sdl3-debug/vcpkg_installed`；`cmake --build out/windows/desktop-only --parallel 12` 退出 0。
- `ctest --test-dir out/windows/desktop-only -N` 实际注册 119 项；
  `ctest --test-dir out/windows/desktop-only --output-on-failure --no-tests=error`：119/119 通过，51.90 秒。
  包含配置、输入/窗口生命周期、启动、资源、归档、FFmpeg、ImageOps、TJS、PSB 和三项脚本 smoke。
- `python scripts/check_native_runtime.py out/windows/desktop-only` 退出 0：369 个编译单元、108 个 SDK ports，
  实际编译/链接命令、动态依赖及资源不含旧宿主；拒绝 `--host=cocos`。
- 定向 launch-only 使用新目录和现有 Catch2 SDK，安装关闭；11 项注册并通过。
  最终完整构建重新编译并执行同一启动模块，覆盖最终源码。
- `python tools/game-smoke/imageops_repro.py out/windows/desktop-only/bin/krkr2/krkr2.exe out/desktop-evidence/imageops-01 --renderer software`
  退出 0，缩放/模糊/仿射/透视事件和像素通过，未强停、引擎退出 0；已查看实际 BMP。
  图像 SHA256 `395f5bb131813c513681734ed29859ea0682c6640e0fe80bebfc47e1375d711d`。
- Windows 原生探针最终命令：
  `python tools/game-smoke/sdl_windows_repro.py out/windows/desktop-only/bin/krkr2/krkr2.exe out/desktop-evidence/windows-04`。
  退出 0，`component_input=false`、`forced_stop=false`、引擎退出 0、全部事件通过；
  选中路径精确对应本轮 `windows-04/fixture.txt`。
  已查看实际消息框、文件选择及恢复后窗口截图。
- `python -m unittest discover -s tools/game-smoke -p test_sdl_windows_repro.py -v`：2 项通过，
  旧目录同名 fixture 必须拒绝，当前轮存储路径/本地路径可接受；CI 已登记该验证。
- YAML 语法与可复用 workflow 引用、JSON、修改文档的相对链接和 `git diff --check` 通过；
  C++ 修改文件使用 clang-format 20.1.8 格式化。

本轮 exe SHA256：`fc3a45e69f57f6bdcbb9640462ead40ecbe248a87e61639c76d9b44239e21058`。
软件图像和最终原生报告匹配此 exe。

## 失败记录与控制器修正

- `windows-01` 在首次标题栏点击前被其他窗口遮挡，`Native caption obscured`，强停；没有进入原生交互验收。
- 控制器复用已有 `raise_without_activation`，只抬高自己启动的目标，再经真实 SendInput 点击并检查前台归属。
  `windows-02` 首次键盘往返成功，随后焦点变化导致第二次 A 按下事件超时；保留失败日志与截图，未判为引擎修复。
- `windows-03` 无并发桌面查询时完成全部交互，但实际查看截图发现系统对话框记住旧目录，
  返回的是删除前目录的同名 fixture。旧判定仅核对 basename，因此这轮不计为完整本轮证据。
- 修正控制器：Alt+N、Ctrl+A 与 UTF-16 SendInput 输入本轮绝对路径，确认编辑控件就绪，
  再通过真实 Enter 确认；判定核对完整返回路径。没有注入脚本结果或跳过失败步骤。
  新鲜 `windows-04` 全部通过，先前失败/不充分记录继续保留。


## 删除后的真实游戏验收

按用户后续要求，使用同一最终 exe 验证 `D:/krkr/tslm` 的《天神乱漫 LUCKY or UNLUCKY!?》。
`game_smoke.prepare` 复制归档和兼容 patch 至全新 `out/desktop-evidence/tslm-02/game`，
不复制原 savedata，仅安装仓库自编 AfterStartup 心跳钩子；没有修改游戏逻辑或注入按钮结果。
临时控制器 `out/desktop_real_game.py`、`out/desktop_real_game_continue.py` 使用 Win32 SendInput。

- 首进程：`title.ks:33` → START → `１.ks`；真实鼠标打开 SAVE、写入 File01，
  新建 `savedata/data0.bmp` 及相关存档；返回剧情后打开 LOAD，确认读取并恢复 `１.ks:24`，
  继续推进，然后真实点击游戏退出确认。`first-report.json`：退出 0、未强停。
- 第二进程：同一私有 savedata，从标题 LOAD 读取 File01，再次恢复 `１.ks:24`，
  推进至 `１.ks:26`，确认退出。`restart-report.json`：退出 0、未强停。
- 第三进程：再次从私有 File01 恢复 `１.ks:24`，真实 Enter 推进到 `１.ks:38`，
  实际截图可见 CG、文字和游戏按钮，再确认退出。
  `continued-report.json`：退出 0、未强停。
- 三轮报告的 exe 哈希均为本页最终 `fc3a45e6…e21058`；
  hook SHA256 `adcf88ff497804289bebeadc96dc6a7bbc6cc447efe3f9461a45c899ea2d2c09`。
  已实际查看新建存档、重启后剧情、后续 CG 与退出确认截图。
- 完成后逐项重算原始 13 个资源文件哈希，全部与准备时一致；
  `source-verification.json` 的 `data.xp3` SHA256 为
  `6791e688118c95924281a51e9e182f6e4bc512c6ef29447f994a803364f93282`。
  商业资源、存档和截图仅留在忽略的 `out/`。

早期 `tslm-01` 首进程失败保留：临时控制器对已在前台的游戏重复点击标题栏，
双击使窗口最大化，后续坐标在窗口尺寸变化期间误点到 LOAD，等待 `１.ks` 超时后强停。
实际截图与心跳证实停在 `load.ks:30`，没有判定为引擎兼容故障。
控制器改为仅在非前台时执行真实标题栏点击，新隔离副本 `tslm-02` 按上述完整步骤通过。

## 标准预设统一与旧构建目录升级

用户随后指出 `Windows Debug Config` 缺少 `nlohmann_json`，并明确要求只有一个宿主时不再区分 SDL3 预设。
此前验收使用了 `Windows SDL3 Debug Config`，覆盖输出目录和 SDK；没有覆盖原样标准预设的旧缓存入口。
实际读取 `out/windows/debug/CMakeCache.txt`：`VCPKG_MANIFEST_INSTALL:BOOL=OFF`、
`nlohmann_json_DIR:PATH=nlohmann_json_DIR-NOTFOUND`。当前 manifest 已声明 nlohmann-json，错误来自缓存关闭安装。

- 删除 Windows/Linux 的 SDL3 专用应用预设，仅保留标准 Debug/Release；XP3、strings、lean 是功能范围，继续保留。
- Windows/Linux 标准预设显式开启 `VCPKG_MANIFEST_INSTALL`；用户显式 `-D...=OFF` 仍可覆盖。
- CI、当前操作文档、技能和输出路径统一到标准预设；发布包名称去掉宿主后缀。
  上文旧预设命令与目录只保留为当时实际执行的历史证据。
- 新 `test_cmake_presets.py` 将真实预设用于无依赖的最小 CMake 工程，先种入安装 OFF 缓存，
  再原样执行 Debug/Release 预设，确认 CMake 实际将该缓存恢复为 ON。

本机实际命令不再覆盖构建目录或 SDK：

```powershell
cmake --preset "Windows Debug Config"
cmake --build --preset "Windows Debug Build" --parallel 2
ctest --test-dir out/windows/debug -N
ctest --test-dir out/windows/debug --output-on-failure --no-tests=error
```

- 在原 `out/windows/debug` / `vcpkg_installed` 目录成功配置并找到 nlohmann_json 3.12.0，缓存安装开关为 TRUE。
- 标准构建成功；首次 12 并行链接内存占用较高，主动终止该次验证构建后以并行 2 续跑同一预设成功。
  没有修改全局工具链或删减目标；中止日志保留，未将中止后的旧程序作为构建通过证据。
- 实际注册并运行 119 项测试，全部通过，40.32 秒；本机缓存为 full、工具 ON、脚本 smoke ON、UI 驱动 OFF。
- 首次依赖检查发现旧输出中残留 65 个管理 UI 文件，均属于 `c494ea0` 的 70 项已删除源资源。
  新 `cmake/retired-runtime-assets.txt` 提供明确清单；同步器先验证完整操作再仅删除这些文件及空父目录。
  新增 3 项资源迁移回归，验证源文件已删除、保留自定义文件、拒绝现用资产冲突及目录占位。
- 迁移后再次原样配置、构建；资源/CLI 两项定向 CTest 通过；
  `check_native_runtime.py out/windows/debug` 通过：374 个编译单元、109 个 SDK ports，无旧宿主依赖或 UI 残留。
- `python -m unittest discover -s scripts/tests -v`：10 项通过；
  `python -m unittest discover -s tools/game-smoke -p test_resources.py -v`：7 项通过。
  当前预设引用、YAML、Python 语法、修改文档链接及 diff 检查通过。

标准预设 exe SHA256：`2aec6552d8e517e9e95ca5860a68f979c9e75212a8f484b86fc893977fa21e2d`。
原始记录为 `out/windows-debug-preset-*.log`；本节没有重新声称 Linux 本机运行或 Release 完整构建已验证。

## 未验证范围

Windows 删除和上述范围已经验收；没有声称所有平台或所有游戏已验证。
Linux 构建和真实交互需新 CI，不把此前基线记录作为本分支验证。
商业游戏完整剧情、物理 DPI/IME/手柄和未来移动端生命周期均不包含在本轮验收。
