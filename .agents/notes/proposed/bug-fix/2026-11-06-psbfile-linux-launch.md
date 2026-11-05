# PSBFile 补全与 Linux 游戏启动崩溃

Status: proposed

## 问题与来源

基于 dev `3dcfb29aa8a7ed47981e1037db4b549dc310216a`。
用户引用 #20 的 2025-08-21 回复；当前 dev 已有 PSB 解析器，但 `load(Octet)` 仍为空分支，
文件加载引用了没有初始化的局部 `PSBMedia*`，而真正的介质在 `PSBMediaRegistry.cpp` 中。

对用户授权的独立游戏副本调试，未把游戏资源或 Windows 插件加入提交。
ARM64 Release、Ubuntu 24.04 构建，在 Android 的 DroidDeck Linux 会话执行。

## 已验证证据

- 空字符串/自动路径：`GetLength` 的 null-this 在优化下被删除；首次修复长度后，
  空字符串索引和数值转换又命中相同约定。显式 GetLen 判空并把
  `-fno-delete-null-pointer-checks` 作为 tjs2 的 PUBLIC 编译选项传递给内联消费者。
- 创建存档目录返回值取反，fstat 删除不存在文件继续调用本地路径转换。
  合成 XP3 回归覆盖创建目录、Dictionary 二进制保存/读回与重复删除。
- PSB 资源注册崩溃：原 PC `0x123a518`，合成 PIMG 和独立游戏副本命中同一位置。
  改用共享 registry 后，副本成功加载 ezsave.PIMG、bgmtitle.PIMG 并执行完启动脚本。
- 提交 `cdf0c0a` 的完整 ARM64 构建通过；`psbfile-runtime` CTest 实际注册并通过 1/1。
  https://github.com/realRurichan/DroidDeck/actions/runs/42084774489
- 提交 `ff167f3` 增加 v2/v3 名称 trie、嵌套资源与自动路径枚举，构建和 CTest 通过。
  https://github.com/realRurichan/DroidDeck/actions/runs/42086212932
- 同一引擎在 DroidDeck 显示路径出现第二处崩溃。二进制 SHA-256
  `a7d0efdbea2f654f2c95ac3dd3ccd0aa9515ab0dd523743e5bda042e851840d2`，
  匹配符号解码 PC `0x12b64a4` 为 Blend2D `memGetTablesImpl`，调用链经
  `blFontFaceCreateFromData` 到 `LayerExDraw.cpp:54 initGdiPlus`。
  原实现在 createFromData(raw pointer) 后立即 delete[] 字体数据，是释放后访问。
  `76ab60d` 改用 BLArray 重载持有数据，两个字体入口共用 RAII helper。

## 参考与边界

- Blend2D 固定依赖 `d2027ebfd6aaf53b190b6b3b497425fc85f14251` 的
  `src/blend2d/fontdata.h`：数组重载持有共享存储，raw-pointer 重载借用外部数据。
  仅据接口契约修复调用方，未复制其实现。
- 查看 zeas2/Kirikiroid2 `d1c2b1259423542c893e0b65eaeb46c848848f2b` 的
  readme、LICENSE、文件树及 `src/plugins/InternalPlugins.cpp`。
  该公开版本不含 psbfile/Blend2D 实现；参考其 ncbind 内建插件接口，未复制代码。
- 仅补全现有 PSB v1–v3 路径；不声明闭源插件完全兼容、v4 支持或完整安全审计。
  LZ4/解密路径尚无本次回归覆盖，游戏音视频、全部场景与存档兼容仍需独立验证。
- Xvfb/softpipe 中的启动完成不能代替 DroidDeck 实际 GPU 显示路径验收。

## 字体修复验证与剩余游戏现象

`76ab60d` 原生 ARM64 Release 构建及包含 layerExDraw 加载的 CTest 1/1 通过：
https://github.com/realRurichan/DroidDeck/actions/runs/42089412736
二进制 SHA-256 `193321e8bf366a1a1b658acebf67031911d44ccdd31a6471d6a8d975d63d5198`。
同一手机的 DroidDeck/Gamescope 路径已越过字体与 PSB 崩溃，实际加载两份 PIMG，
AfterStartup 心跳持续递增，KAG 到达 title.ks 第 35 行（标题脚本等待输入处）。
该轮仍未显示标题内容；后续定位和修复结果见下文，勿与已证实的崩溃混为一谈。

## 下一步与完成条件

标题初始化修复已加入 PR；完整剧情、音视频和存读档仍需独立验收。
本记录保留 proposed，以免把当前启动回归的范围扩大为完整游戏兼容结论。

## Kirakira 设计参考

查看 liulifox233/Kirakira `c451918023477620ff9055a2eb8a6714b947d9c1` 的 README、许可证、
PSB 模块设计说明和测试名称。其 AGPL-3.0-or-later 代码没有被移植、翻译或加入本仓库。
参考的通用检查点是“解析成功应产生实际对象树”和“脚本端应保留字典/数组/Octet 类型”。
据此复查本仓库，发现 root getter 用 TJSCreateCustomObject，而嵌套字典已有
PSBDictionary::toTJSVal 原生转换；改为复用本仓库既有转换，并新增 Dictionary 类型身份断言。
PSB 校验和、加密标志诊断与 PSBValueClass 接口仍需依据格式和独立样本单独验证，未照搬参考实现。

## 标题初始化定位

AfterStartup 的 KAG snapshot 也是全黑，说明不是单纯显示输出问题。
createCallConfigFile("macro.ks") 实际返回 `[call storage=macro.ks]`，
游戏通过 `[emb escape=false ...]` 插入该命令；现有 core/KAGParser 总是转义左方括号，
所以宏和 custom 配置的加载命令变成文字，随后 title.ks 在等待状态保持空画面。
仓库原有 `cpp/plugins/KAGParser/readme.txt` 已明确规定 escape=false 应执行生成标签，
而插件入口只是“已内置”占位。`c938d5d` 据此给既有展开路径增加开关，保留默认转义。
合成测试覆盖 false、&false、true 和省略参数，以及展开后的后续标签位置。

`d937ebe` 原生 ARM64 Release CI 通过，包含上述 KAG 展开和 PSB 回归：
https://github.com/realRurichan/DroidDeck/actions/runs/42093925083 。
二进制 SHA-256 `35799800f7fdfad04887a01bd834a61950fdb949401dabaf4fbf03318f8eb6b2`。
同一手机的独立副本实际显示品牌 Logo，随后显示带“新的开始”等按钮的完整标题菜单；
KAG 心跳进入 title.ks 第 35 行。未发生先前的字体/PSB 崩溃。
实际截图只保存在忽略的调试目录，未将商业素材加入仓库。
手柄未响应属于 DroidDeck 会话输入适配问题，另在前端实现键鼠映射，不作为 PSB 修复的一部分。

## 桌面 Enter 丢失

DroidDeck 完成手柄键鼠映射后，独立蓝色图层场景收到 Down=40、Escape=27、Control=17
及鼠标回调，但普通 Enter 没有回调；Android 系统直接注入 Enter 同样失败。
Cocos 3.17.2 的 desktop GLView 将 GLFW_KEY_ENTER 转成 KEY_ENTER，
而本仓库 CCKeyCodeConv.cpp 将该枚举置于 return 0 分支，仅 KEY_RETURN/KEY_KP_ENTER 生效。
`63c6e33` 将 KEY_ENTER 合并到 VK_RETURN，并增加 opt-in X11 真按键 CTest。
前端映射不需要改成模拟点击或小键盘回车绕过此问题。

`43bab23` 的 ARM64 CI 通过，实际注册并运行 `psbfile-runtime` 和
`x11-keyboard-runtime` 两项测试，以及普通 TJS / XP3 启动和存档回归：
https://github.com/realRurichan/DroidDeck/actions/runs/42100670826 。
新键盘测试逐项验证 Return、KP_Enter、Escape、Down、Control_L 的 down/up，并正常退出。
前一轮已通过按键断言，但测试脚本错误地赋值 Timer.onTimer 导致退出等待超时；
改为现有 Timer(action, "") 约定后重跑完整 CI 通过，没有放宽断言或接受强停。
二进制 SHA-256 `9ed77f2c8e9d1950602474316a29ef7ea9615550cdeba0f6a7678feffe5f7f25`。

该哈希引擎安装到同一手机后，独立输入场景实测 DroidDeck 屏幕 A 键收到 down:13 / up:13，
四个方向键分别收到 37/38/39/40 的按下和抬起；鼠标点击与 Esc 同样有脚本回调。
实体手柄尚未在本轮实机验收；前端映射实现和单元测试在 DroidDeck 功能分支。
