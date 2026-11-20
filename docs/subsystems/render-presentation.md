# 纹理呈现与宿主截图边界

`visual/RenderManager.h` 的 `iTVPTexture2D::GetPresentationFrame()` 返回
`visual/TexturePresentation.h` 中不依赖 Cocos/TJS 的画面描述。
CPU 渲染器不再创建 Cocos 纹理；宿主通过 `environ/cocos2d/CocosTexturePresentation.*`
消费描述，窗口和图层调试 UI 使用同一入口。

## 画面描述与所有权

- `RGBAFrame` 借用 RGBA 字节、物理宽高与带符号行距；可提供行读取回调，供半高去重行直接上传。
  数据只在同步上传期间有效，不允许保留行指针。连续布局整块上传，非连续布局逐行上传。
- `OpenGLFrame` 借用当前共享 context 中的 GL 名字与物理宽高，禁止宿主删除该 GL 名字。
  Cocos 包装器持有源 `iTVPTexture2D` 引用，同尺寸换源时先保留新源，再释放旧源；
  销毁包装器先清空其 GL 名字，随后释放源引用，实际 GL 删除继续由渲染器承担。
- CPU 上传纹理与借用 GL 纹理用不同类型识别，不能把借用的 GL 名字当作 CPU 上传目标。
  CPU 纹理按实际上传尺寸复用，避免原先比较逻辑宽度、分配 pitch/4 宽度导致的反复分配。
- 描述中的宽高是物理存储范围；可见裁剪与缩放仍由 `GetWidth/GetHeight/GetInternalWidth/GetInternalHeight/GetScale`
  和现有窗口 Sprite 处理。宿主每次交帧重新核对逻辑裁剪和缩放，不能以包装纹理地址未变为由跳过。静态 CPU 路径保留 pitch/4 的物理宽度；半高路径保留实际压缩行数。
  本接口延续已有 RGBA 呈现，不新增 Gray/RGB 呈现或颜色空间转换。

这些操作继续在原渲染线程/context 上执行；这里没有新增跨线程上传队列。

## 最终窗口截图

`environ/FrameCapture.h` 提供 `TVPCaptureHostFrame(path, completion)`，头文件仅使用标准 C++。
Cocos 实现在 `environ/cocos2d/CocosFrameCapture.cpp`，沿用后端原有帧边界和异步保存完成通知。
自制场景驱动必须等待本轮唯一序号的保存回执，截图包含窗口/UI，不能以 Layer BMP 代替。
模态绘制仍由既有 `RenderUtils.cpp` 执行 hook → drawScene → swap。

## 验证与剩余边界

独立 `platform` 测试范围新增 `texture_presentation_tests`，覆盖带 padding/负 pitch 的行访问、
压缩行回调、先保留替换源后释放原源、相同源复用与析构释放。与输入/帧钩子共 38 项，仅依赖 Catch2。
真实 Window/Layer 验证使用[自制图像场景](../../tools/game-smoke/imageops/README.md)，
同时检查算法图与五轮换图的实际管理器、像素、窗口截图及正常退出。

`scripts/check_core_module_deps.py` 拒绝公共呈现头重新暴露 Cocos 类型及 CPU 渲染器重新直接包含 Cocos 呈现 API。
这只约束直接边界，不代表整个 visual 模块已无传递 Cocos 依赖。
OpenGL 后端通过 `visual/GLHost.h` 访问宿主状态缓存、视口和 context 重建订阅；
具体 Cocos 调用集中在 `environ/cocos2d/CocosGLHost.cpp`，公共头仅使用标准 C++。
28处缓存调用的顺序不变；GL错误检查同样经宿主，保留Debug查询/消费错误、Release不查询的既有行为。绘制后恢复仍为 unpack 行长 → blend cache → framebuffer → viewport。
manager 以 unique_ptr 持有重建订阅；Cocos 订阅同时保留 dispatcher 与 listener，析构只注销自己的监听，
宿主主动清理该事件后也可安全销毁。重建回调继续只清 shader 缓存并 Rebuild，未新增纹理/FBO 恢复。
视频、窗口节点、输入和管理 UI 也仍使用 Cocos。
本轮不删除 Cocos port、不切换 SDL3、不宣称包体或编译速度改善；后续需先定义真正 context 丢失时的资源恢复顺序。

纹理呈现首批的三平台独立测试、Windows/Linux完整应用与双后端自制场景、Android ARM64完整APK均通过。
测量、早期失败/对照与具体范围见[实施记录](../../.agents/notes/implemented/simplification/2026-11-20-render-presentation-boundary.md)
及[验证报告](../../.agents/reports/render-presentation-boundary-2026-11-20.json)。
macOS未构建完整应用，Android未在设备运行，真实游戏按用户要求暂不使用。

原生 `ogl_common.h` 直接包含各平台系统GL头；Apple TargetConditionals/扩展头与 GLES packed-depth-stencil
枚举不再依赖 Cocos 头的传递定义。macOS CI 单独编译原生头，不代表完整应用已验证。
Cocos context 订阅的四项真实 EventDispatcher 用例与原窗口/输入契约共15项；
事件分发测试不创建或销毁真实GPU context，不能作为设备丢失恢复证明。

GL宿主边界的最终验证见[实施记录](../../.agents/notes/implemented/simplification/2026-11-20-gl-host-boundary.md)
与[报告](../../.agents/reports/gl-host-boundary-2026-11-20.json)：Windows/Linux各15项Cocos契约与双后端场景通过，
Android ARM64完整APK、三平台独立测试和macOS原生GL头编译通过；真实context丢失恢复仍未验证。
