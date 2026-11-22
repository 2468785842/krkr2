# GL 宿主状态与 context 订阅边界

Status: implemented

## 结果与实现

接续纹理呈现隔离，基于 dev `d4af4eb`，验证候选 `2329ee5`。用户授权继续并在验证后同步dev，
保留Cocos后端，用自制场景验收，真实游戏暂不使用。
来源、原始结果和产物哈希见[验证报告](../../../reports/gl-host-boundary-2026-11-20.json)，
当前API见[纹理呈现与宿主边界](../../../../docs/subsystems/render-presentation.md)。

- `visual/GLHost.h` 只使用标准C++，承载纹理激活/绑定/删除、程序与顶点状态、blend/viewport恢复、日志和GL错误检查。
  实现在 `environ/cocos2d/CocosGLHost.cpp`；RenderManager_ogl.cpp 不再直接包含或调用Cocos类型/头/缓存/事件。
- 原有28处GL缓存调用的对应关系与顺序逐一核对不变，仍使用宿主的同一份缓存。
  帧末恢复顺序仍为unpack行长 → blend cache → 原framebuffer → 宿主viewport。
  Debug保留glGetError查询/消费，Release保留不查询的既有行为；日志记录调用处的文件/函数/行。
- manager以unique_ptr持有context订阅；Cocos实现保留dispatcher和listener，析构只移除自己的监听。
  宿主已移除该事件并清空autorelease池时，订阅仍可安全销毁；构造后续步骤抛异常时也按成员生命周期注销。
  重建回调继续只清shader缓存并逐方法Rebuild，不新增纹理/FBO/context资源的完整恢复。
- 原生GL头直接包含Apple TargetConditionals/OpenGL扩展、Win32加载函数声明和标准packed-depth-stencil枚举。
  可变参数显式包含cstdarg，不再借用Cocos头的传递声明；GLHost公共头可独立用标准C++编译。

## 实际验证

| 范围 | 结果 |
|---|---|
| Headless | [44738611060](https://github.com/2468785842/krkr2/actions/runs/44738611060) 全13个job通过；三平台独立platform各38项保持通过；macOS额外在不含Cocos的条件下编译原生GL头及framebuffer API |
| Windows完整应用 | [44738286127](https://github.com/2468785842/krkr2/actions/runs/44738286127) 编译/链接、15项真实Cocos对象契约、5组既有窗口交互、软件/OpenGL场景均通过，正常退出0、无强停 |
| Linux完整应用 | [44738290012](https://github.com/2468785842/krkr2/actions/runs/44738290012) 编译/链接、15项真实Cocos对象契约、软件/OpenGL场景均通过，正常退出0、无强停 |
| Android ARM64 | [44739726553](https://github.com/2468785842/krkr2/actions/runs/44739726553) 完整Debug APK成功；349个项目源，宿主适配与GL渲染器各编译一次；下载实际APK核对库哈希/ELF架构/动态依赖与五份许可 |
| 本机静态验证 | GLHost.h以C++17、Wall/Wextra/Werror单独语法编译；11个foundation和呈现/GL边界守卫、修改C++格式、YAML与diff检查通过 |

新增的4项真实EventDispatcher测试分别检查重复通知及优先级、只注销自身、dispatcher/listener寿命、
宿主移除全部重建监听后销毁；与既有11项窗口/指针/焦点/文本契约合计15项。
Linux测试构建现在也构建并执行这15项，独立platform范围仍是38项，不引入Cocos闭包。

自制场景保持原7项图像操作、BMP28个样本与保护点，以及五轮换图、65→66边缘覆盖检查。
两端软件/OpenGL均完成全部阶段；右边缘每次检测960个合格样本。
已实际查看两端软件/OpenGL初始图、OpenGL边缘标记和最后替换帧，共8张截图。
它们验证正常绘制与状态交接，没有制造或销毁真实GL context。

Windows运行引擎SHA256：3562264065ba2c561733d93dc7e2b1118617ea2c47da64a9ec800e4c1dea575f。
Linux运行引擎SHA256：db56a2ae1d2eb57b390844d85d123a480461be63aa612e2df6db4bd2dc54184a。
ARM64 APK：56,775,822字节，SHA256
3187bd2dc22943995184c6450af3b50359adef5bea2a5b2c1224f91f09563d17。
原始日志、截图及中间证据在忽略的out/gl-host-boundary，报告保留run/artifact/源哈希。

## 失败与语义核对

首轮测试设计中，核对Cocos 3.17.2 `cocos/base/CCEventDispatcher.cpp` 后确认
removeAllEventListeners会保留EVENT_RENDERER_RECREATED等内部事件，因此销毁测试明确使用
removeCustomEventListeners清除重建监听，不把普通监听清理误当成context监听已注销。

`165d3f5` 的Windows [44737452600](https://github.com/2468785842/krkr2/actions/runs/44737452600) 和
Linux [44737456611](https://github.com/2468785842/krkr2/actions/runs/44737456611) 在编译失败：
移除Cocos头后暴露va_start/va_end、Win32函数加载类型和CHECK_GL_ERROR_DEBUG的传递依赖。
`27c50c0` 的Android [44736903284](https://github.com/2468785842/krkr2/actions/runs/44736903284)
同样在原生编译阶段报告CHECK_GL_ERROR_DEBUG未声明，未生成可作为通过证据的最终APK。
2329ee5显式补齐标准/原生声明，并将错误检查迁入宿主，随后三端完整构建重新通过。
最终通过以重新构建后的2329ee5及报告中的产物哈希为准，早期失败仅保留为过程证据。

## 边界与下一步

本批隔离的是宿主状态和订阅所有权，Cocos仍链接且负责窗口/UI/视频等，visual也仍有传递依赖。
不宣称已经移除Cocos、缩小包体或提高FPS。
既有context通知仅重建shader缓存/方法；纹理、FBO、缓冲、参数/属性位置缓存和宿主资源恢复顺序，
需要下一批基于真实context切换/丢失场景定义并验证。事件分发测试不能替代硬件context丢失恢复。
macOS完整应用、Android设备和真实游戏未运行。
