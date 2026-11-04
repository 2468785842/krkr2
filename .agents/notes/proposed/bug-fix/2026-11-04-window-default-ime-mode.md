# Window 默认输入模式 setter 未实现

Status: proposed

## 已知事实

基于 `16f8468`：WindowIntf.cpp 的 Window.imeMode setter 调用 SetDefaultImeMode，
WindowImpl.cpp 的对应方法只有注释，不保存模式、不更新平台输入状态。
TVPWindowLayer::GetDefaultImeMode 返回初始化的 imDisable；Layer.imeMode + focus 则经过
LayerManager、DrawDevice、Window::SetImeMode 到真实 Cocos IMEDelegate。

文本模块提取首轮 `41931835667` 的场景仅设置 Window.imeMode=imOpen，没有字符回调而超时。
该失败不是新旧解码差异的证据，Cocos 转换对照在同轮已全部通过。
本批场景改用游戏编辑控件现有的 Layer 输入路径；没有把窗口默认模式能力宣称为已完成。

## 下一步

先核对原 TJS2 的窗口默认模式、聚焦 Layer 模式、imDontCare 与失焦重置的关系，
用最小脚本分别记录属性读回、无 Layer 焦点时的提交、Layer 覆盖及取消焦点后的恢复。
实现时同时处理通用窗口接口、Cocos 适配器和调用方，保留各平台键盘显示/隐藏差异。
需要真实脚本与原生文本路径验收，不能仅填一条保存字段的空壳或由测试驱动强制 attach 来通过。
