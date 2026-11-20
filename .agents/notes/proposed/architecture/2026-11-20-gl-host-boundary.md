# GL 宿主状态与 context 订阅边界

Status: proposed

接续已完成的纹理呈现隔离，基于 dev d4af4eb。用户授权继续，保留 Cocos 后端与自制场景，
真实游戏暂不使用；验证后同步 dev。

## 现状与本批实现

RenderManager_ogl.cpp 原先直接调用 Cocos GL 缓存、Director 视口和 context 重建事件。
重建监听器捕获 this，却没有保存可注销的所有权；现有重建仅清着色器缓存并逐方法 Rebuild，
不包含所有纹理/FBO/context 丢失后的完整恢复，不能据此宣称支持真正的设备丢失。

本批引入只用标准 C++ 的 visual/GLHost.h，具体 Cocos 调用移到 environ/cocos2d/CocosGLHost.cpp。
纹理激活/绑定/删除、程序和顶点状态继续走原缓存，不改成绕过缓存的 raw GL。
恢复顺序仍为 unpack row length → blend cache → 原 framebuffer → 原宿主 viewport。
context 监听器由 manager 的 unique_ptr 持有，Cocos 订阅保留 dispatcher/listener，析构注销自己的监听。
重建回调的 shader cache/Rebuild 次序不变，不新增假的完整 context 恢复。

新增四项真实 Cocos EventDispatcher 测试：通知和优先级、只移除自身、dispatcher/listener寿命、
宿主主动移除全部重建监听后安全销毁。与现有11项Cocos窗口/输入契约合计15项。
独立 platform 范围仍为38项，不把需要Cocos的测试塞入独立闭包。

## 验证状态与完成条件

公共头已用标准 g++ C++17 单独语法编译，依赖守卫与 YAML/diff 检查通过。
尚需 Windows/Linux 完整应用和15项实际Cocos契约、自制软件/OpenGL算法图+五轮呈现、
Android ARM64 完整APK与三平台独立测试。不得将假发重建事件等同于真实GL context销毁重建。
全部通过后保存来源/产物哈希和运行证据，迁为 implemented 并同步 dev。

原始日志与后续报告放忽略的 out/gl-host-boundary。既有基线与截图方法见
[上一批记录](../../implemented/simplification/2026-11-20-render-presentation-boundary.md)。
下一批应先梳理真实 context 丢失时 shader 参数缓存、FBO/纹理/缓冲和宿主资源重建顺序，再决定恢复实现。

核对 Cocos 3.17.2 EventDispatcher.cpp：removeAllEventListeners 保留内部的
EVENT_RENDERER_RECREATED，故销毁测试明确使用 removeCustomEventListeners 清除该事件。
这保留真实 Cocos 事件语义，不把普通监听清理误当作 context 监听已经注销。

移除 Cocos GL 头时发现原本依赖传递定义的 Apple TargetConditionals/OpenGL 扩展头和
GLES packed-depth-stencil 枚举。ogl_common.h 已直接包含原生系统头并固定标准枚举，
Headless macOS 额外在不含 Cocos 的条件下编译此头与 framebuffer API；仍不等同完整 macOS 应用验证。
