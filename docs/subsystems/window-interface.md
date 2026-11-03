# 窗口接口拆分（B2 首批）

状态：从 Cocos 窗口实现中拆出通用接口，保留现有后端。
本批不替换窗口、输入、渲染、媒体后端或平台库。

## 边界

- `cpp/core/visual/WindowLayer.h` 承载 `iWindowLayer`、修饰键别名和方向常量，
  包含已有 `WindowIntf.h`，不包含 Cocos 或控制台 UI。
  消息接收器注册枚举移至 `WindowIntf.h`，枚举值保持不变。
- `environ/impl/TVPWindow.h` 保留 `TVPWindowLayer`、Cocos ScrollView、输入状态及窗口管理实现。
  `visual/impl/WindowImpl.cpp` 和 `BasicDrawDevice.cpp` 改为包含通用接口。
- 通用接口不再提供返回 Cocos Node 的 `GetPrimaryArea`。
  仍依赖 Cocos 的视频呈现通过 `environ/cocos2d/CocosWindowAccess.h` 的
  `TVPGetCocosWindowPrimaryArea` 借用节点，所有权保持属于窗口。
  实现在 `TVPWindow.cpp`，不匹配的后端抛出 `std::bad_cast`，不会静默跳过视频。
- 原接口中的空实现及既有限制原样迁移；本批不宣称补齐全屏、位置、触摸查询等功能。
  当前接口仍依赖 TJS/WindowIntf，并非完全独立的宿主 API。

`scripts/check_core_module_deps.py` 额外拒绝上述消费者重新包含 `TVPWindow.h`，
并检查通用头不直接暴露 Cocos 类型；该静态检查不能代替传递包含检查或全模块无环证明。

## 验证方法

真实交互使用自制 [窗口场景](../../tools/game-smoke/interaction/README.md)，
通过原脚本入口、原游戏菜单/文件选择与消息框、原关闭事件运行。
脚本请求文件仅负责打开对话框和发起 close；不能伪造用户点击或成功返回值。
控制器要求实际菜单事件、确认/取消返回值、选中文件名及关闭回调，空运行不能通过。

测量、排序补丁来源、截图与本机运行结果见 [窗口验证记录](../../.agents/notes/implemented/testing/2026-11-03-window-interface-validation.md)。

## Cocos UI 约束

`vcpkg/ports/cocos2dx/patch/fix-node-order.patch` 直接比较 localZOrder 与 orderOfArrival，避免打包排序键的字节序依赖；更新 port 后需要重建依赖。
共享 UI 按实际分配的行高限制文件名、标题与底栏字号，嵌入节点使用父级局部原点。
`tests/unit-tests/core/environ/NodeOrderTest.cpp` 在 full 范围验证层级与同层顺序，不创建 GL 窗口。
