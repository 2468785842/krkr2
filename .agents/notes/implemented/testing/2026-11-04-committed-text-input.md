# 提交文本解码隔离与 Unicode 输入验收

Status: implemented

## 实现与保持的契约

基于 `16f8468`，实现 `e855ac4`。`core_input` 增加仅依赖 C++17 的
`TextInput.h/.cpp`，`MainScene::onTextInput` 不再使用 Cocos StringUtils 解码。
窗口有效性与焦点门禁、排队方式、逐 UTF-16 单元的 onKeyPress 分发保持原入口。
非法 UTF-8 整段拒绝，补充平面字符保持高/低代理项顺序，不规范化组合字符；
解码保留嵌入 NUL，窗口适配器保持旧规则忽略零单元并继续分发后续字符。

独立平台范围新增 5 项，合计 31 项、15 个项目源，vcpkg 闭包仍只允许 Catch2。
full 范围另新增 2 项真实 Cocos 对照：旧转换函数生成全部合法 Unicode 标量输入并对照 UTF-16，
以及 10,000 组确定性的任意字节输入，合计 11 项 Cocos CPU 测试。
旧实现依据为仓库 pin 的 cocos2d-x 3.17.2 `cocos/base/ccUTF8.cpp` 的
UTF8ToUTF16 / utfConvert，调用 ConvertUTF 的 strictConversion；生产算法为本仓库独立实现，未复制外部源码。

`tools/game-smoke/text_repro.py` 使用独立 TJS fixture：Layer 的 imOpen 和逻辑焦点启用文本输入，
以 TJS `#key` 记录 UTF-16 单元。Win32 VK_PACKET 提交 ASCII、中日文、重音、组合字符与 emoji；
新增测试驱动 `text-hex` 经真实 Cocos IMEDispatcher 进入适配器，检查 NUL、错误编码、截断及无前缀泄漏。
真实原生辅助窗口切焦后提交应忽略，恢复后的新文本正常，最终要求完整回调、检查点、退出 0、未强停及填充层像素。
这不是 IME preedit/候选窗、物理设备或 Android JNI 的验收。

## 本地验证

本会话是新的 Linux 环境；旧 out 构建目录引用的临时工具已不存在。
在 `/tmp/krkr-dev-tools` 安装本次 CMake/Ninja/clang-format 20，
Catch2 v3.7.1 从官方 tag 源码构建到 `/tmp/krkr-native-text-deps/prefix`，未修改全局工具链。
第一次配置将 Ninja 路径指向不存在的 data/bin，退出 1；改为实际 bin/ninja 后重新配置并构建成功。

```sh
cmake -S . -B out/linux/text-input -G Ninja -DCMAKE_PREFIX_PATH=/tmp/krkr-native-text-deps/prefix -DKRKR2_BUILD_APP=OFF -DBUILD_TOOLS=OFF -DENABLE_TESTS=ON -DKRKR2_TEST_SCOPE=platform -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build out/linux/text-input --parallel 4
ctest --test-dir out/linux/text-input -N
ctest --test-dir out/linux/text-input --output-on-failure --no-tests=error
python -m unittest discover -s tools/game-smoke -p 'test_*.py' -v
python scripts/check_core_module_deps.py
```

实际发现并通过 31 项 C++（约 0.06 秒），41 项 Python（约 8.32 秒）全通过；
11 个基础目标依赖守卫、Python 编译、改动范围 clang-format 20、两个工作流 actionlint、diff 检查通过。
本地 Catch2 安装直接用于最小配置，不据此宣称 vcpkg 安装闭包通过；该闭包由三平台 CI 独立检查。
没有本地完整 Cocos 应用；真实旧转换对照与原生交互由下述 Windows CI 独立核对。

## 首轮远端与场景修正

`e855ac4` 的 Headless `41931834058` 已 13/13 job 全通过，三平台各 31 项与 15 源闭包通过。
Windows `41931835667` / job `126930393835` 应用编译、11 项 Cocos CPU（含两项真实旧转换对照）、
41 项 Python 和原有四个场景通过；新文本场景在原生字符回调前超时，退出 1、forced_stop=true。
报告只有 ready、command:native、phase:native，没有字符回调，不能计为原生 Unicode 通过。
exe SHA256 `8b46aaaf62c4c5e60eede9ea179abcd94a03eca5ef960cac53da8e6822963620`，
PDB `4fff3c19c3fecb2271f0b70151ea3fc1745b127302959fe7240b2ff785404137`，
fixture `5b9dc7e29861335e1013378e19341b4ae36debae448c41e3d47069202b874d96`。
已下载轻量 artifact `13372447188` 并核对到 `out/windows-ci/41931835667/`，
ZIP SHA256 `30b5ebe5adfdc16bd51b45b6835e3f03b77f8a5eea95968e45047002e1694c09`。

源码确认 Window.imeMode setter 调用的 SetDefaultImeMode 为空，初版场景没有真正启用文本输入。
`b284d43` 改用现有 Layer.imeMode + focus 路径，额外断言 focused；没有注入脚本回调或放宽字符序列。
窗口默认模式接口缺口见[独立调查](../../proposed/bug-fix/2026-11-04-window-default-ime-mode.md)。
同批将轻量报告改成优先上传，完整 artifact 只保存一份公共 runtime exe/PDB，
五份场景报告仍各自保留启动/证据哈希；首轮完整 artifact 大小 635,648,867 字节，修正后大小待核对。
`b284d43` 的 Windows `41932936280` / job `126934140261` 应用、11 项 Cocos 对照、41 项 Python
及原有四场景通过，但原生 emoji 失败：前六个 ASCII/CJK/重音/组合单元正确，
U+1F642 被记录为单个 `63042`（U+F642），没有预期 D83D/DE42。
超时后退出 1、forced_stop=true；未计为 Unicode 交互成功。
exe `97a770da4b306f7ed4d42edc4cf2e70feb72c90b862a2551833c7090e8472611`，
PDB `904904c847fea852e5b123c796374838fd2f7acd86801948f6e41382ce016857`，
fixture `f9f0b83fc84de680dd04fa2d03979f16168edf610322046957bb7a716fe659a6`。
轻量 artifact `13373022337` SHA256
`36e0adc8d5d282c9308268199e86e3b3e6f9f4bf2ae341a291361982479ba068`，
已下载到 `out/windows-ci/41932936280/` 并核对原始回调。
该轮完整共享二进制包为 127,073,619 字节，较前一轮 635,648,867 字节减少约 80%；
这是 artifact 优化数据，不作为失败场景通过的证据。

## Cocos 原生字符截断修复

上游固定来源为 cocos2d-x 3.17.2，commit
`1528ea01d2749b4ef65e97f79cffa6135fe13c4d`，
`cocos/platform/desktop/CCGLViewImpl-desktop.cpp::onGLFWCharCallback`。
原实现将 GLFW unsigned int 码点强转为 char16_t，再把该单元编码为 UTF-8，
恰好解释 0x1F642 → 0xF642；不是共享 UTF-8 解码的差异。
原文件 SHA256 `974560ede3cde31a68dbf78af3bb676b7d4fdc0ccb00af665239afe0172b51e7`。

`1022ae7` 增加 overlay patch `fix-desktop-unicode-input.patch`，
以 UTF32ToUTF8 转换完整码点，保留原生 NUL 忽略及私用区功能键过滤。
无效码点不分发；cocos2dx port-version 从 11 提至 12，必须重新配置并重建依赖，不能只运行旧应用。
patch 已对上述原文件通过 git apply --check；没有替换 GLFW 回调或直接调用脚本来绕过原路径。
文件原 MIT 许可及版权头保留：2010–2012 cocos2d-x.org、2013–2016 Chukong Technologies Inc.、
2017–2018 Xiamen Yaji Software Co., Ltd.；修改通过仓库 overlay 管理。
Windows `41934504722` / job `126939459445` 日志确认重新构建 cocos2dx #12、应用 overlay patch，
完整应用、11 项 Cocos CPU、41 项 Python 和原有四场景通过。
新文本场景已完整收到 Native ASCII/CJK/组合字符、D83D/DE42，以及恢复后的 Z 和代理项；
无效提交未泄漏前缀、失焦文本忽略，退出 0、未强停，恢复前后中心像素均为蓝灰。
但整体报告仍失败，因为初版断言错误地要求 NUL 产生空字符串回调。

源码 `TVPWindowLayer::OnKeyPress` 的 `if(tjsNativeInstance && vk)` 在基线 `16f8468` 中已存在，
旧适配器本来就过滤零单元；实际只记录 A 与 NUL 后的中文是正确的既有行为。
`01f0e1b` 修正错误预期，仍保留含 NUL 的提交并要求其后字符到达，新增断言拒绝意外空字符串回调；
没有修改生产零值处理或删除 NUL 场景。41 项 Python 重新通过。
本轮是预期写错导致报告失败，不能以退出 0 或 emoji 已修正将整轮写成通过。
exe `2bffc615c2daeb544928c6935115046e156ffd2e4b56be169678359930140e7a`，
PDB `c324dc606ecadbbea5aedcce7c4d2ae3fc296f4366d0df90299e2d2abed9ba2a`，fixture 与 b284d43 相同。
轻量 artifact `13372788929` SHA256
`895aabc5b8dc2582711ad69992e295a6928f4cc895b02daf9ab1e7b96a5260b3`，
已下载核对到 `out/windows-ci/41934504722/`；完整包降至 126,968,474 字节。
## 最终远端验收

源提交 `01f0e1b8142c543b8c415bcbada2250877c66120`：

- [Headless 41936126666](https://github.com/2468785842/krkr2/actions/runs/41936126666)：13/13 job 成功；
  Windows/Linux/macOS 的 platform 范围各 31 项、15 个项目源及 Catch2-only 安装闭包通过，41 项 Python 通过。
- [Windows 41936130860](https://github.com/2468785842/krkr2/actions/runs/41936130860)，
  job `126944909686` 最终 completed/success：完整 Release 应用与测试驱动构建成功，
  11 项 Cocos CPU 对照、41 项 Python，以及单窗口、多窗口、原生切焦、最小化恢复、原生文本五个交互场景全部成功。
  五份报告均 passed=true、exit_code=0、forced_stop=false、failures=[]。

文本报告实际记录原生提交 `A中あéé🙂` 的 UTF-16 单元
`65, 20013, 12354, 233, 101, 769, 55357, 56898`；
组件提交 A/NUL/中仅产生 `65, 20013`，保留既有零值过滤并确认 NUL 后文本未丢失。
四组无效/截断 UTF-8 没有有效前缀回调；原生失焦期间组件提交未分发；
恢复后 `Z🙂` 收到 `90, 55357, 56898`，最终 fixture-finished 后正常退出。

五场景 engine/evidence 的 exe 与 PDB 哈希一致：

- exe SHA256：`767b9f5c2d68d119f94cf6c9942685dcf9f7c88b8f20411b79746cbb13c86494`。
- PDB SHA256：`3129d7bf8736f44e722b3650f74191b8fbd534aec48a6383c366793ec6052309`。
- 文本 fixture SHA256：`f9f0b83fc84de680dd04fa2d03979f16168edf610322046957bb7a716fe659a6`。

已下载轻量 artifact `13372694895`（52,022 字节），ZIP SHA256
`34734a7e3fb4ae135fa4dc4bc08a99da79a589d8bf3683016700bfdd7c4631e0`，
解包在 `out/windows-ci/41936130860/`。文本恢复前后截图均为 960×640，
中心 (480,320) RGB 为 (48,64,80)，人工检查为填充层的蓝灰画面；本场景不检查字体字形渲染。
`native-focus-ready.png` SHA256
`d894929698b87a8d75cd9c3b1fa547c1997148e1770569a433e207934be988e1`；
`native-focus-recovered.png` SHA256
`ab612b7831f86c0d918ead9a9206c8371dd5bcfea8427a4701184b3def84403d`。

最终完整共享包 artifact `13373792645` 为 126,803,199 字节，服务端 SHA256
`aede7a7a9ce5923a0199c75f43d184accc15af5a92afbe29c9351fa8d4b1bd8a`，
较首轮重复保存 exe/PDB 的 635,648,867 字节减少约 80%。这是 CI 证据包去重，
不代表发布程序体积下降；完整包只保存一份 runtime，报告仍保留各场景的运行身份。
没有据此宣称真实 IME 组合/候选窗、物理键盘、硬件 DPI 或 Android 已验收。

## 后续边界

OS 高 DPI、IME 组合/候选 UI、移动生命周期、Android 编码桥与原生鼠标捕获仍分别验收。
Android 发现的键码/Modified UTF-8 边界见[独立调查](../../proposed/bug-fix/2026-11-04-android-text-encoding.md)。
