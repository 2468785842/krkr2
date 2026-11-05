# 完整 Android 运行时的原生构建与 APK 验证

Status: implemented

## 目标

接续 Android 输入桥子批次，实际构建完整默认功能的 Android Cocos/TJS APK，
验证之前只通过桌面/独立 JNI 库时未覆盖的原生依赖、应用链接及打包闭包。
本批分别完成 x86_64 和 ARM64 Debug；真实游戏回调仍须后续独立验收。

## 改动

`12699b9` 添加可选 krkr2.androidAbis，默认保留两个生产 ABI；
按 ABI 复用同一配置选择构建/拆包，单 ABI 不再生成相同内容的通用副本。
Android 显式关闭桌面测试/工具并导出 compile_commands。
新 android-runtime-tests.yml 使用完整应用，保留 PSD/管理 UI，不使用 lean 或宿主空实现。
验收脚本检查真实 CMake、JNI/文本/两种渲染器源、完整 ports、APK 架构和 ELF 动态依赖。

新 workflow_dispatch 文件尚未被 GitHub 默认分支注册，直接 dispatch 返回 404。
`3d2f9a6` 经已注册 Build Android 的 runtime_contracts=true 调用该可复用工作流；
原 Release job 在此模式跳过，不触发临时签名或旧 SDK/Mono 流程。
修改旧入口时 actionlint 发现过期 setup-java@v3、前向 cache step 引用和不存在的 matrix.os，
`43dba17` 同时修正，两个工作流 actionlint 随后通过。

## 首轮失败（不可计为原生构建通过）

源 `43dba17afde658419b924af6e80343645ede2ab4`，运行
[41965753235](https://github.com/2468785842/krkr2/actions/runs/41965753235)，
原生 job `127042253995`，在 Gradle 配置即失败：
Conflicting configuration: x86_64 in ndk abiFilters cannot be present when splits abi filters are set。
核对 AGP 8.7.3 的 ApplicationVariantFactory.kt：原双 ABI 启用 universalApk 时允许两者共存。
本次新增单 ABI 模式关闭通用包，却保留 ndk 过滤，才引入此无效组合，不能归咎于原双 ABI 路径。
现由 splits 统一选择 ABI；AbiConfigurator.kt 会将原生选择集合与 splitsFilterAbis 取交集。
依据来自固定版本 gradle-8.7.3-sources.jar，未复制外部实现。
未进入 vcpkg/NDK 构建，没有 APK。

本轮日志管道默认 shell 没有 pipefail，Gradle BUILD FAILED in 46s 被 tee 的成功退出覆盖，
下一步校验报 Expected one application CMake cache ... got []，所以整体 job 仍失败。
`95d40bd` 改为拆包单一 ABI 来源，并显式 shell=bash，确保管道传回构建失败。
`3dcfae8` 将依赖配置与 APK 组装分步；运行 41966419898 / job 127044367042
进入完整依赖配置后，为修正缓存策略由本会话取消，不宣称通过或新构建错误。

首轮已经保存过空的 vcpkg cache 固定键；仅用 cache-hit 判断保存会令后续完成的依赖无法入缓存。
`05d61b2` 改为按 run_id/run_attempt 另存并按 ABI/依赖哈希恢复，每次保留已完成部分；
避免后续某个 port 失败时重复编译前面的库。新同 ABI 运行使用 concurrency 避免重复消耗。

## 首次完整配置与应用编译失败

源 `05d61b2`，运行 [41966701548](https://github.com/2468785842/krkr2/actions/runs/41966701548)，
原生 job `127045254274`。完整配置耗时 29 分 15 秒并退出成功；
实际 compile_commands 含 345 个项目源，vcpkg status 含 133 条安装记录。
CMake 为 x86_64、Debug、android-29、x64-android；APP/PSD 开启，测试/工具/上传/桌面驱动关闭。
应用编译在 115/384 停于 `cpp/core/base/impl/StorageImpl.cpp`：
unlink/read/write/close/lseek64 未声明，尚未到应用链接，没有 APK。
`5035631` 为非 Windows 显式包含 unistd.h，并移除只读 open 调用中无效的 mode 参数。
依赖检查和修改范围格式检查通过；以实际 Android 复编验收，不能以头文件修改代替构建结果。
`66263b5` 在 Gradle 配置后增加正常 krkr2 目标的 Ninja `-k 0` 编译，
一次收集独立源文件的失败，仍保留非零退出码；成功后仍由 assembleDebug 完成 Java/资源/原生打包。

该轮成功保存完整依赖缓存，键尾为 `41966701548-1`。
artifact `13388384047`，ZIP SHA256：
`08fce9cc741c4c7bbbab5e30974333f07da611314d020678347667db6d4a580b`。
本机证据位于 `out/android-ci/41966701548/`，含 CMakeCache、编译清单、安装记录和两阶段日志。

复验 x86_64：运行 `41970543177`，源 `50356318cd20906a3c8383bf46018b45af2816c1`，
job `127057505235`。配置成功（17 分 29 秒），StorageImpl 已通过；
随后 `XP3ArchiveRepack.cpp` 的 close/write/lseek64 缺少声明，应用编译仍失败，没有 APK。
`6e7b9d8` 在该文件的非 Windows 分支补齐 unistd.h，保留 Windows io.h。
artifact `13390347129`，ZIP SHA256：
`9ce8193f47f8f0b56fffdd176a32a30f825342f3a37da59cf455b932dcf55f12`。

ARM64 冷构建：运行 `41969323245`，源 `bc7955f`，job `127053620727`。
配置成功（29 分 22 秒），随后复现早于修复的 StorageImpl 声明错误，应用编译失败，没有 APK。
artifact `13390431466`，ZIP SHA256：
`1b98407f758405b71fded8aafc37bfe633b5a8e07c9a041d7cec2957b296f008`。
两个失败轮次的诊断包均已下载并校验到 `out/android-ci/<run-id>/`。

源 `6e7b9d8008b7d3db05cdd83108361c2128a474f1` 已启动批量编译复验：
x86_64 `41972846609` / job `127064824499`；ARM64 `41972850310` / job `127064835175`。
x86_64 本轮配置 70 秒成功。两架构均实际恢复 108 个 vcpkg 包，
批量编译都只报告 `cpp/plugins/layerex_draw/blend2d/LayerExDraw.cpp:2173`
的 assert 缺少声明；未进入最终应用链接，无 APK。
`d3a3692` 为此文件显式包含 cassert。
x86_64 artifact `13389913625`，ZIP SHA256：
`793ce1f5500f57cbba6ca802df3dfe77e469f1d5ff3f89a035e80dcf8c752dd5`；
ARM64 artifact `13390921863`，ZIP SHA256：
`37ee7c6991b927e95a5b05fa2d1ea5c4b7d6d7edf2d88cf3193c68694a3a6df3`。
相同源的 headless CI `41972731870` 已整体成功；基础回归不能替代 Android 原生目标结果。

源 `d3a3692f731d55d75cef33f687009d10980a8997` 继续复验：
x86_64 `41973600795` / job `127067248262`；ARM64 `41973605513` / job `127067254306`。
两轮均成功完成原生编译/链接、assembleDebug 和 APK/ELF 验收。
相同源的 headless CI `41973600115` 全部 13 项成功。

## 完整 APK 成功产物

构建源均为 `d3a3692f731d55d75cef33f687009d10980a8997`，NDK 28.0.13004108、
API 29、Debug；APP/PSD/原 Cocos UI 开启，测试/工具/上传/桌面驱动关闭。
两 ABI 各有 345 条项目编译记录；未用裁剪功能或空实现绕过链接。

| ABI | 成功运行 | artifact | APK 大小（字节） |
|---|---|---|---|
| x86_64 | [41973600795](https://github.com/2468785842/krkr2/actions/runs/41973600795) | 13390389887 | 60202950 |
| arm64-v8a | [41973605513](https://github.com/2468785842/krkr2/actions/runs/41973605513) | 13391056949 | 58524854 |

x86_64：

- artifact ZIP SHA256：`c2ca58de2363029f966437a26e838f022b9bcebdf93fb64c6f79f4bfd60890cf`
- APK SHA256：`df3ec60e2d04a34d6d80f2e270f940d03594dbfb7b33fab73ab1fde1667d4ec8`
- libkrkr2.so SHA256：`9cbdebbb620f665b8439574999b3501d1eed28687816dded2b37e6a28c7f201d`

ARM64：

- artifact ZIP SHA256：`9ddbd8477ee9889c0096518ece24c5a2f2659c090d1e4eaadaa6ef7e9b323410`
- APK SHA256：`8b8aacf2172c299fa219bed0650c3a5020fddfb5ec68bf7dfa4fa64551817181`
- libkrkr2.so SHA256：`949d0c504cc3d25f437825ac55e4a216c6be3f1b7c2037e03de9cd63000d7c2a`

两包均包含且仅包含对应 ABI 的 libkrkr2.so、libc++_shared.so、liboboe.so 和 libomp.so；
所有 ELF 的 DT_NEEDED 均由包内库或 NDK API 29 公共系统 stub 覆盖。
下载到 `out/android-ci/<run-id>/` 后再次校验 artifact ZIP、APK 和每个包内库的 SHA256，均匹配。
另用本机 readelf --dyn-syms 核对两包 JNI_OnLoad、Cocos Renderer nativeInit/nativeRender、
Cocos Helper nativeSetContext、KR2Activity initDump/nativeCommitText/nativeKeyAction 共 7 个必要导出，均存在。
完整清单及所有库的哈希/动态依赖在 artifact 的 out/android-runtime/build-manifest.json。

此状态只完成所声明的完整原生构建与打包范围；未安装或运行游戏，
不表示最小 TJS 输入回调、厂商 IME、ARM64 设备或 surface 生命周期通过。

## 缓存工具身份漂移

`41970543177` 确实恢复约 463 MB 的上轮 GitHub cache，但 vcpkg 日志显示 Restored 0 packages。
原因证据：`41966701548` 和 `41969323245` 的 vcpkg 要求 CMake 4.4.4，
`41970543177` 要求 4.4.3；后两轮保存的检测日志表明宿主编译器哈希相同
（GNU 13.3.0，compiler hash `9b6f0c0a4d070db4395c24ca86081e5d9124b163`）。
连宿主空工具包 vcpkg-cmake-config 的 ABI 都由 ed6a7c60... 变为 9602f1e4...，
因此仅固定 ports baseline、NDK 和恢复归档不能保证命中。
当前保留累积缓存；后续应将工作流专用 vcpkg 工具源码/引导版本固定，
并把实际工具身份及包 ABI 信息纳入证据，不修改 runner 的全局工具链或关闭 ABI 检查。

## 验收边界

成功需构建退出 0、CMake 闭包正确、APK 实含本轮 libkrkr2.so/STL、ELF 与依赖闭包通过。
保留源码/ABI/NDK/开关、APK/原生库 SHA256、日志及 artifact 身份。
APK 链接或静态依赖检查不替代安装、启动、TJS onKeyPress/onKeyDown、IME/焦点及正常退出。

## 后续真实 TJS 接线调查

现有 UIContractDriver 被 CMake 明确限制为桌面，读取 DesktopTarget；不得为移动验证直接放开并伪装为已支持。
Android 的 TVPCheckStartupArg 当前只注册 Android_PushEvents 的引擎线程消费，然后进入选择器，
没有桌面 --game 的目录入口。MainScene::startupFrom 仍是可复用的正常启动路径。

后续可使用显式 Debug-only 测试 APK/入口：将带 marker 的自制 TJS fixture 放在独立应用私有目录，
经 Android_PushEvents 调用 startupFrom，随后由真实 DummyEdit/InputConnection 进入现有 JNI 链。
严格比较脚本的 UTF-16 onKeyPress、退格 VK_BACK（8）按下/抬起、ready/checkpoint，
不能让测试入口直接调用脚本回调或强制 attach IME。
Android System.exit 会结束包含 instrumentation 的目标进程，最终退出验证需主机 adb 控制器独立观察，
不能以 instrumentation 结束时强停进程替代游戏正常退出。
API 34 模拟器还可调查 ApplicationExitInfo（adb dumpsys activity exit-info）的自退出原因与状态码；
须匹配本轮进程和时间，不能只看到 PID 消失或脚本写出结束标记就判为正常退出。
这一步须以完整运行时 APK 首先链接/打包成功为前提，本批未新增未验证的移动测试入口。
