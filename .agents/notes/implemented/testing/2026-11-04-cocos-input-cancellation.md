# Cocos 取消触摸与窗口销毁后的字符输入

Status: implemented

## 问题与修正

基于 `7d8bf5a` 加本批改动，审查生产输入的取消与失焦路径时发现两处代码缺口：

- `TVPWindowLayer::onTouchMoved` 在开始拖动时按下共享鼠标键；`onTouchEnded` 会释放，
  但 `onTouchCancelled` 原先只清理拖动标记并排队抬起，未释放共享键。
  现在最后一个触摸取消时先释放对应键，再采样修饰位和排队抬起，保持原坐标与不生成点击的行为。
  释放保留未读按下标记，仍遵守 `System.getKeyState` 的旧查询契约。
- `TVPMainScene::onCharInput/onTextInput` 原先直接解引用 `_currentWindowLayer`。
  原生/IME 回调可在窗口建立前或最后一个窗口销毁后到达；现在无当前窗口时忽略字符，
  存在窗口时保持原 UTF-8 → UTF-16 与逐 UTF-16 单元的 OnKeyPress 分发。

这不是[既往访问冲突](../../proposed/bug-fix/2026-11-03-ui-contract-access-violation.md)的根因结论；
没有原异常栈，不关联这两处防护与历史故障。

## 验证

本机仅有独立 Linux 平台配置，应用关闭、测试开启、scope=platform：

```sh
cmake --build out/linux/platform-focus --target input_state_tests frame_hook_tests -j 4
ctest --test-dir out/linux/platform-focus -N
ctest --test-dir out/linux/platform-focus --output-on-failure --no-tests=error
python scripts/check_core_module_deps.py
```

实际注册并通过 18 项，包含单键释放保留未读按下标记、修饰位快照及批量释放。
该范围没有编译生产 Cocos，不能将这 18 项描述为触摸取消或 IME 交互回归。
仅对两个修改源执行 clang-format 20；diff 与 11 个 foundation 约束检查通过。
完整 Windows 应用构建与 5 项 Cocos 节点/坐标测试在推送后补充。

## 未验证与后续

尚未实际执行系统触摸取消、窗口关闭同时收到 IME 文本、普通失焦与最小化交互。
本批不改变字符编码规则、键码映射、事件排队或 Cocos 焦点接线。
桌面 Cocos 焦点 custom event 与 iconify 的 AppDelegate 通知不同；
具体剩余接线与验收继续由[平台契约](../../proposed/architecture/2026-11-03-platform-contracts.md)跟踪。
