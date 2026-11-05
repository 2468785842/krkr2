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
当前仍未显示标题内容，不能声明游戏已完整可玩；此现象继续调查，勿与已证实的崩溃混为一谈。

## 下一步与完成条件

继续定位标题内容未显示的原因；当前 PR 限定为已验证的 PSB/启动崩溃修复。
通过后本 note 可标记 implemented，但保留未验证游戏场景的边界。
