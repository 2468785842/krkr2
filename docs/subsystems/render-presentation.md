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
压缩行回调、先保留替换源后释放原源、相同源复用与析构释放。与输入/帧钩子及渲染配置共 42 项，仅依赖 Catch2。
真实 Window/Layer 验证使用[自制图像场景](../../tools/game-smoke/imageops/README.md)，
同时检查算法图与五轮换图的实际管理器、像素、窗口截图及正常退出。

`scripts/check_core_module_deps.py` 检查通用呈现接口和 `visual` 源码的直接 Cocos 引用，
也检查 `core_visual_module` 的 CMake 声明。完整引擎仍通过宿主实现窗口、呈现和资源读取。
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

## 渲染配置与宿主服务

`visual/RenderSettings.h` 定义标准 C++ 配置读取接口、纹理存储枚举与旧纹理上限策略；
`visual/RenderHost.h` 声明配置、内存压力、日志、提示框与本地化服务。
具体适配在 `environ/RenderHost.cpp`，软件与 GL 渲染器不再直接包含配置管理器、Platform 或 WindowImpl。
`RenderCapabilities.h` 单独声明 GL 能力查询，查询信息仍要求原渲染线程和有效 context。

配置管理器读取缺省值时会写回配置，因此各键仍在原调用时机按需读取，不生成全量快照。
renderer 的任意注册名称、游戏/全局配置回退、未知压缩值的普通纹理回退均保留；
存储工厂继续在管理器构造时选定，没有新增热更新。
纹理上限保留设备值小于 1024 时允许配置覆盖的旧规则，MSVC 仍读取但忽略 GL 扩展开关。
11 个扩展名改用有序数组遍历，避免跨结构体成员作指针迭代。

显式测试驱动记录实际管理器、选中存储工厂、纹理上限与 copy_image 能力，不额外读取配置。
`imageops_repro.py --capture --profiles` 为八组配置启动全新进程，先从 BMP 重载静态源检查像素，
再执行原算法与五轮呈现；GL half 还检查 512 上限和 Linux 禁用 copy_image 的回退。
诊断表示选中的工厂，不能证明每张纹理均采用某种内部编码；ETC2/PVRTC 未作设备运行验收。

存储探针同时重载 64×64 和 128×128 非纯色图；后者越过 GL half 的 64 阈值，实际执行 128→64 缩减，并检查四个象限内点。

本批八配置矩阵在 Windows/Linux 均通过，各含 15 项 Cocos 契约；三平台独立范围各 42 项通过，
Android ARM64 完整 APK 编译/打包及 ABI/依赖核验通过。
具体源码、早期编译失败与修正、产物哈希见[实施记录](../../.agents/notes/implemented/simplification/2026-11-20-render-host-services.md)
和[报告](../../.agents/reports/render-host-services-2026-11-20.json)。

## Visual 构建依赖

`core_visual_module` 直接声明 FreeType、PNG、WebP、原生 GL 等依赖，源码和目标配置均移除了
Cocos 的直接引用。内置字体通过 `environ/PackagedAsset.h` 同步访问宿主资源。
Cocos 适配继续使用原有搜索路径，字体枚举在回调内借用字节，后续字体读取复制到独立内存流。
PVRv3 加载继续使用引擎已有的文件头，CPU 的 PVRTC 解码通过宿主调用原有解码器。
TJS 的虚拟屏幕继续报告 2048×2048，桌面原点为 (0, 0)。

软件半行纹理使用独立 xxHash 的 `XXH32`，原输入字节范围、种子及去重行为保持一致。
Cocos 自带的旧头文件现在安装到自己的子目录，独立 xxHash 的 BSD 许可随应用资源打包。

运行验证与构建产物见[实施记录](../../.agents/notes/implemented/simplification/2026-11-21-visual-build-boundary.md)。

## 视频覆盖层

`movie/VideoPresentation.h` 定义同步借用的紧密排列 YUV420P 三平面和以左上为原点的目标矩形。
FFmpeg 播放器负责帧队列、PTS、跳帧与脚本回调。Cocos 适配负责视频节点、帧回调注册、
YUV 纹理上传、缩放、坐标换算和显隐。播放和停止控制画面 Sprite 的显隐，脚本 `visible`
控制其根节点。销毁呈现对象时解绑节点并注销帧回调，已从父节点拆除的节点也会清理回调。

游戏内 `VideoOverlay` 的 overlay、mixer 和 MFEVR 路径共用窗口呈现工厂。
管理界面媒体预览使用同一个 Cocos 适配器，由预览自身的更新循环主动交帧。
Layer 模式继续把画面送往引擎位图。依赖守卫检查 `core_movie_module` 中的源码和构建描述。

Linux 测试构建可用 FFmpeg 生成 YUV420P 双彩图，驱动 TJS 覆盖层播放、关闭、重开和退出。
探针通过显式启用的宿主截图驱动等待每次保存回执，核对左右颜色及覆盖区外的背景像素。

```sh
LIBGL_ALWAYS_SOFTWARE=1 ALSOFT_DRIVERS=null xvfb-run -a \
  python tools/game-smoke/video_repro.py out/linux/release/bin/krkr2/krkr2 out/linux/video-runtime
```

跨平台构建、截图采样和复测结果见[视频呈现实施记录](../../.agents/notes/implemented/simplification/2026-11-22-video-presentation-boundary.md)。
