# 窗口接口拆分与 UI 修正的验证记录

Status: implemented

本记录保存 B2 首批窗口拆分的测量、修正依据和本地验证，未解决问题仍由 proposed notes 跟踪。

本地 MSVC 19.44/C++17 使用同一 WindowImpl 编译参数，对旧 `TVPWindow.h` 与
新 `WindowLayer.h` 的独立翻译单元分别执行 `/showIncludes /Zs`：

| 入口头 | 语法检查 | 包含记录 | 其中 Cocos 路径 |
|---|---|---|---|
| HEAD 基线的 TVPWindow.h | 通过 | 566 | 96 |
| WindowLayer.h | 通过 | 153 | 0 |

基线为 `b3765f7`，日志及响应文件在 `out/windows/b2-header-audit/`。
这是独立头文件解析范围对照，不能当作完整消费者的编译耗时、包体或运行性能收益。
Cocos 类型注入通用头、消费者重新引入实现头两项负向依赖检查均被拒绝。
Python 控制器与资源等共 10 项测试通过，交互本身由下述实际运行记录另行确认。


## 截图发现的兼容问题与修正

本轮使用测试驱动的 `capture` 从 Cocos 帧缓冲保存 PNG，避免 Windows 窗口采集超时。
文件选择器曾只在画面两侧可见，中间被游戏 Sprite 覆盖；
`CCNode::sortNodes` 在 MSVC x64 使用了错误字节序分支下的打包键，排序实际优先比较加入顺序。
`vcpkg/ports/cocos2dx/patch/fix-node-order.patch` 改为直接比较 localZOrder 和 orderOfArrival，
保留字段布局/ABI，port-version 升至 11；更新后需要重建 cocos 依赖，旧安装不能代表修正后结果。
该补丁仅调整已有比较方式，基于 cocos2d-x tag cocos2d-x-3.17.2
（提交 `1528ea01d2749b4ef65e97f79cffa6135fe13c4d`）的
`cocos/2d/CCNode.h`，保留上游 MIT 许可/版权。

`tests/unit-tests/core/environ/NodeOrderTest.cpp` 在 full 测试范围验证
UI 先加入但应在游戏之上、负 z、同层顺序及重新排序，不创建 GL 窗口；不进入独立 launch 测试闭包。

截图还暴露出共享 UI 的已有布局缺陷：偏好/菜单行的内部根节点按父坐标放置却使用了中心锚点；
浮动菜单也有相同边界问题；文件输入仍保留演示用 10 字符限制，文件行字号只按宽度缩放而超出行高。
本批对齐这些嵌入节点的原点，文件名上限改为 255，标题、文件行与底栏按容器实际高度限制字号，底栏放在内容之后。
无底栏时，主体使用头栏之外的剩余空间；避免菜单底部透出游戏。
测试驱动拒绝窗口外的点击，避免隐藏按钮也能产生成功回调的假阳性。

排序修正前后截图保存在 `out/windows/b2-cocos-order/file-dialog-before.png` 与
`file-dialog-after-sort.png`；后者证明覆盖问题修复，也暴露了本批继续处理的底栏文字溢出。
最终截图和交互结果另按最终构建记录，不能将这些中间截图当作最终 UI 验收。

## Windows 本地运行记录（2026-11-03）

`out/windows/engine-config-check` 使用实际 port-version 11 的 Cocos 库，
`KRKR2_USE_COCOS_UI=OFF`、`KRKR2_ENABLE_UI_TEST_DRIVER=ON`。
应用 SHA256 为 `56fbec6a3110e6a1cb87f9f7ff2e8366f53a0c73bf237f70c2c44140831a1a2f`，
场景 SHA256 为 `17077d2aadb45d3df6dcb8d72414a159667ef04716c0336bfaa939064eea1391`。

两个全新场景 `out/ui-game-smoke/b2-acceptance-off-01`、`b2-acceptance-off-02`
均由 `interaction_smoke.py run ... --component-input` 生成通过报告，
退出码 0、未强停、无脚本或驱动错误。第一轮附加 CDB，第二轮不带调试器。
实际事件覆盖原生消息框确认、文件取消/确认与规范化文件名、菜单 checked 的 1→0、
close veto 保留窗口及随后接受关闭。原生消息框通过 Windows 输入确认；其余点击经过 Cocos 命中及分发。
每轮目录的 `file-selector.png`、`menu.png`、`menu-checked.png` 是对应构建实际截图。
这只验收本场景的可见布局与脚本回调，不代表 DPI、IME、设备输入或所有菜单状态的像素一致性。

相关 CTest 12/12 通过：

```sh
ctest --test-dir out/windows/engine-config-check -R "cocos window|desktop|runtime|Adreno|launch_cli_smoke" --output-on-failure --no-tests=error
python -m unittest discover -s tools/game-smoke -p "test_*.py"
python scripts/check_core_module_deps.py
```

Python 共 10 项通过。`tslm-b2-off` 私有副本运行 25 秒通过，36 个状态样本，
中文、空值、重复覆盖与标志参数探针通过；到期由控制器终止，不能作为游戏正常退出验收。
仅从已有私有副本链接 XP3，测试钩子及存档独立，未写入 `D:/krkr/tslm`。

默认管理界面开启的 `out/windows/debug` 另以 `KRKR2_ENABLE_UI_TEST_DRIVER=OFF` 构建应用成功，
编译清单没有驱动翻译单元或 MainScene 的驱动宏，应用中也没有驱动的标记检查诊断字串。
该应用 SHA256 为 `81023c0d3de78493a0606f87a6ed20b92837e6831ecb92f476afa5879e516361`。
同样筛选的 CTest 为 10/10（该目录未构建新增的 cocos_window_tests，不能计为 12 项）。
`tslm-b2-on` 副本 25 秒通过，34 个状态样本及参数探针通过，仍在 `title.ks:33`，
到期由控制器终止。通过 KAG snapshotLayer 导出的标题画面为该副本的 `smoke-frame.bmp`，
`smoke-frame.png` 只转换格式以供查看；该图不包含宿主 UI，也不是操作系统窗口采集。
两个模式的通过结果只涵盖标题场景、心跳和参数探针。

```sh
cmake -S . -B out/windows/debug -DKRKR2_ENABLE_UI_TEST_DRIVER=OFF
cmake --build out/windows/debug --target krkr2
python tools/game-smoke/game_smoke.py run out/windows/debug/bin/krkr2/krkr2.exe out/ui-game-smoke/tslm-b2-on --seconds 25 --launch-style game --probe-engine-args
```

以上命令使用已准备的本地私有副本和本机 MSVC 开发环境；报告目录不可直接复用于下一次运行。
构建仍有原有插件的格式化参数等警告，不视作零警告构建，也不据此比较发布包体或链接耗时。

早先中间构建发生过一次访问冲突，根因尚未定位，后续通过结果不关闭该问题；
失败构建身份、证据和下一步集中维护在[访问冲突 note](../../proposed/bug-fix/2026-11-03-ui-contract-access-violation.md)。
重复交互调试的第三次菜单点击因两个可见同名节点被驱动拒绝，也不作为完整通过报告。

本记录阶段完整 Debug 的 ffmpeg 测试目标曾链接失败；随后初始化测试的链接和实际用例已通过，见
[测试闭包修复](2026-11-03-ffmpeg-test-link-closure.md)。
本记录中的定向 CTest 本身不能代表全套测试通过。
Linux/macOS/Android、视频节点借用的实际媒体播放、多窗口、IME、存读档及完整剧情兼容尚未验收。
B2 的输入、主循环及生命周期契约仍是[后续批次](../../proposed/architecture/2026-11-03-platform-contracts.md)，SDL3 未在本批实现。
