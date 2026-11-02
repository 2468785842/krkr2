# 依赖瘦身评估与实施方案

评估日期：2026-11-01。源码基线：合并后的 `853ec7d`，包含远端 dev `0a80f3f`。
本文保留初始源码审计及迁移提案，实际实施见下方记录；未测得可归因于本轮的体积或构建耗时收益。
后续实施：已加入 XP3/字符串/归档测试独立配置、依赖功能拆分、可选 PSD/上报、插件源去重与注册对象保留、CocoStudio 残留清理、公共头与生成规则优化；ZIP 读取器与共享句柄缓存也已脱离 cocos/XP3 的反向依赖。见 [实施记录](dependency-slimming-implementation.md)。下文审计表保留评估时的源码现状；ImageOps 接口及像素对照已落地，最近邻 resize 采样已用原生 C++ 实现，其余 OpenCV 算法、调度与平台后端替换仍待实施。
原有路线见 [后端精简](backend-modernization.md)；本文补充具体依赖决策和构建边界。

## 建议结论

先让最小目标真正独立配置、安装依赖和编译，再裁剪 cocos/FFmpeg 功能，随后替换 OpenCV 和 PSD 模板解析器，最后迁移平台后端。
推荐完整跨平台运行器采用 **SDL3 + 现有 Layer/软件与 OpenGL 渲染 + 现有字体/解码库**；桌面极简运行器可研究 GLFW。Sokol 作为另一个原型候选，暂不同时引入多个新图形框架。
SDL3 不替代 Layer 混合、CPU 图像算法、字体排版和媒体解码；无需为了窗口迁移再加入 SDL_image、SDL_ttf、SDL_mixer。

目标分别衡量：依赖闭包数量、首次依赖构建时间、业务源码干净构建时间、增量时间、发行包大小和运行内存。单头文件库与较少直接依赖均不自动代表更快。

## 初始审计证据与决策

以下路径相对仓库根；优先级按实施收益与风险综合排序，不是实测耗时排名。

| 优先级 | 对象与证据 | 建议 | 收益与风险 |
|---|---|---|---|
| P0 | 根 `CMakeLists.txt` 无条件添加 external/core/plugins 并创建应用；`vcpkg.json` 把 Catch2、argparse 及全部重库放入基础依赖 | 分开应用、工具、测试与插件构建；同步拆 manifest features | 最小构建不再安装或配置无关依赖；对完整功能影响最小 |
| P0 | `cpp/plugins/CMakeLists.txt`、`psdfile/CMakeLists.txt` 将实现 `.cpp` 放入 `target_sources(... PUBLIC ...)` | 实现源改为 PRIVATE，检查生成图中消费者是否重复编译同一源文件 | 可减少重复编译和 ODR 风险；先核查所有目标及链接注册方式 |
| P1 | cocos overlay 已有功能开关，但 manifest 仍启用 network、cocostudio、desktop 等 | 优先拆网络、编辑器残留、桌面平台服务；保留现有 cocos 后端 | 在不重写窗口的情况下缩小依赖闭包 |
| P1 | `visual/RenderManager.cpp`、`visual/ogl/RenderManager_ogl.cpp` 使用 resize、boxFilter、仿射、透视及矩阵求解 | 建立小型 ImageOps 接口，逐项替换 OpenCV | 可移除 core/imgproc 和相关构建；像素兼容风险较高 |
| P1 | `psdfile/psdparse/psdparse.h` 使用 Spirit Qi/Phoenix，`psdfile.h` 使用 mapped_file | 插件可选化；需要完整 PSD 时用有界字节读取器替换模板语法解析 | 减少 Boost 模板实例化；需保留图层/资源/描述符语义 |
| P1 | `vcpkg/ports/ffmpeg` 固定 3.3.9，已有 small/关闭程序与 avdevice；movie 实际使用 avfilter；`cpp/external/libbpg` 也依赖 avcodec/avutil | 先裁剪 FFmpeg 构建，再考虑格式白名单 | 不推荐整库换为单格式解码器；关闭视频仍可能因 BPG 引入 FFmpeg |
| P2 | `sound/win32/WaveMixer.cpp` 有 Oboe 与 OpenAL 输出；Android 的 `WaveMixerMiniaudio.cpp` 用 miniaudio 转换 | 明确解码、混音、格式转换、设备输出四层，再统一设备接口 | 若选 SDL3，优先研究 SDL 音频输出；若独立音频则研究 miniaudio，不同时新增两套设备层 |
| P2 | `tjs2/tjsString.h` 公共头包含 `boost/locale.hpp`；TextStream 有旧编码转换；PSB 有大小写转换 | 先把重头移入实现；再区分 UTF 转换、CP932 等旧编码、大小写折叠进行替换 | 可降低广泛重编译；不能用 ASCII 或平台默认 locale 替代所有语义 |
| P2 | `core/log/CMakeLists.txt`、SpdlogSink、TVPLogLevelConfig 和部分插件仍用 spdlog | 保留 fmt；可选替换 spdlog sink，并清理所有消费者 | 日志提交标题不能作为已移除依赖的证据；收益通常应排在重型依赖之后 |
| P3 | 存储依赖 7zip、libarchive、unrar、zlib 等 | 首先按归档能力拆 feature，后续才比较合并实现 | libarchive 不保证兼容所有 RAR、7z、加密及随机访问需求 |

### cocos 的短期可裁剪点

- `cpp/core/environ/DumpSend.cpp` 使用 HttpClient 上报。将上报设为可选服务；关闭时同时移除源文件/调用及 cocos network feature。overlay 的 network 会引入 curl/OpenSSL，但其他消费者仍可能保留它们，须检查最终闭包。
- CSD 已部分由 `environ/ui/csd/CsdUIFactory.cpp` 用 C++ 创建；但 `ui/extension/UIExtension.cpp` 仍继承 `cocostudio::NodeReader` 并注册 FlatBuffers reader。先移除这一实际残留及其调用，再关闭 cocostudio；不能仅凭没有 CSLoader 搜索结果删除模块。
- `vcpkg/ports/cocos2dx/vcpkg.json` 的 desktop 同时引入 GLFW/GLEW、Linux GTK3/fontconfig、SQLite 及部分音频依赖。拆为窗口、文件对话框、字体发现、音频、数据库子功能，并审计 overlay 的 `CocosConfigDepend.cmake` 与 external CMake。
- overlay `portfile.cmake` 仍会在 Windows desktop 强制打开 SQLite，在 Linux desktop 打开 TIFF。只改 manifest 默认功能不足以消除这些依赖。
- `archive/zip/CMakeLists.txt` 仍链接 cocos。将 ZIP 访问改为独立 minizip/zlib 适配后，基础归档才能脱离窗口框架；仓库已有 `cpp/external/minizip`，优先评估复用。
- 保留 JPEG/PNG/WebP/FreeType 等实际消费者；物理、Spine、WebSocket 等当前未启用的功能不应列为本轮新增瘦身收益。

### OpenCV 替换方案

第一阶段收敛头文件为 core/imgproc，避免 `opencv.hpp` 总头；这主要减少解析量，不等于删除库。
第二阶段实现保持 pitch/ROI 的 ImageView，将现有调用集中到 ImageOps。以旧实现生成可重复的参考输出。
第三阶段逐项替换：

| 算法 | 首选评估 | 必须验证 |
|---|---|---|
| 最近邻、线性缩放 | 小型自有实现；高质量缩放可比较 stb_image_resize2 | 半像素坐标、1/4 通道、缩小面积滤波、alpha 和舍入 |
| boxFilter | 可分离滑动窗口，按现有 anchor/border/归一化语义实现 | 偶数核、边缘、极小图、溢出和原位操作 |
| 仿射/透视 | 小矩阵求解 + 逆映射采样，复用裁剪与插值内核 | 奇异矩阵、透明边界、透视除法、CPU 可读回结果 |

stb_image_resize2 只作为缩放候选，不能单独覆盖模糊、仿射和透视，插值也不保证与 OpenCV 逐像素相同。GPU 路径仍有 CPU 缩放/矩阵计算，不能只修改软件渲染器。
当前已登记 `ImageOpsTest.cpp` 并提供独立 imageops 测试范围，具体覆盖与未验证项见 [ImageOps](imageops.md)。第三阶段已实现最近邻 resize 采样，保留旧 double 舍入及后端并行调度；其他算法尚未替换。

### FFmpeg 与格式库

先审计 FFmpeg 实际符号及 vcpkg 锁定版本的 port features/configure 参数，优先关闭命令行程序、文档、编码器、设备和无消费者的网络/过滤功能。
复核纠正：`movie/ffmpeg/VideoCodecFFmpeg.cpp` 实际使用 avfilter 创建和运行滤镜图，因此保留 avfilter。源码还使用 `avformat_*`，已在 CMake 显式声明 avformat。实际 `vcpkg/ports/ffmpeg` 锁定 FFmpeg 3.3.9，并已使用 enable-small、关闭文档/命令行程序/avdevice；不能把上游 port 默认功能当作本仓库现状，也不能重复计入既有裁剪收益。
随后根据实际游戏素材统计容器、视频和音频编码，建立“完整兼容”和“已知素材最小”配置。编码白名单同时覆盖 demuxer、parser、decoder、protocol、bitstream filter 及 BPG 所需能力；不能只启用几个 decoder。
JPEG/PNG/WebP/JXR/BPG、Vorbis/Opus 是格式能力，不建议统一换成 stb_image 或 dr_* 后宣称兼容不变。JXR/BPG、PSD、PSB、motionplayer、blend2d 等应作为可选能力，但精简包遇到缺失功能必须明确报错。

## 平台替代比较

| 方案 | 适合范围 | 与本项目相关的代价 | 决策 |
|---|---|---|---|
| 裁剪 cocos | 短期完整运行器 | 保留场景树/UI 耦合；继续维护 overlay | 最先实施，提供旧行为对照 |
| SDL3 + 现有渲染器 | 桌面与 Android 共用平台层 | 需要适配 IME、生命周期、资源、窗口及视频纹理；SDL 本身也有平台依赖 | 推荐长期主线，先做脚本驱动原型 |
| GLFW + 现有 OpenGL | 桌面极简运行器 | 音频、移动端、平台服务需另做；不能当作 Android 统一方案 | 桌面专项可选，不与 SDL3 主线并行铺开 |
| Sokol app/gfx/audio | 愿意投入平台服务与绘制适配的小型运行器 | C 头文件组织紧凑，但已有 GL 管线仍需适配；输入法、多窗口需求要逐项验证 | 原型备选，不凭源码规模直接定胜负 |

推荐拆分窗口/输入、最终画面呈现、平台文件、生命周期、模拟器管理 UI。移除 `TVPWindow.h` 等公共接口中的 cocos 类型；旧后端继续验证脚本尺寸、多窗口、事件顺序和销毁回调。SDL Renderer 或 Sokol gfx 的引入不自动复现现有 Layer 混合与转场。

## 分批实施与验收

以下保留初始分批计划；已完成项见实施记录，可执行开关以构建指南为准。media/legacy-images 等尚未实现。

1. **构建隔离**：增加 `KRKR2_BUILD_APP`、工具/测试/插件选项，在 `project()` 触发 vcpkg 前确定 features；基础 manifest 只留真正公共依赖。显式关闭 default features 后分别选择 app、tools、tests、psd、media、legacy-images、crash-upload 等能力。`ENABLE_TESTS=OFF` 本身不会阻止基础 manifest 安装 Catch2。
2. **最小工具**：拆出 XP3 所需存储子集；当前 `tools/xp3` 链接完整 `core_storage`，仍要求 libarchive/unrar。还需移除其间接 cocos/ZIP 依赖和根级全量配置，不能只改最终 link 行。验收：空安装目录下只配置 XP3，不出现 cocos/OpenCV/FFmpeg 查找与安装，执行打包/解包及损坏输入验证。
3. **构建图和 cocos 裁剪**：修正 PUBLIC 实现源、精确链接 foundation 消费者；拆上报和编辑器残留，关闭相应 port 功能。验收完整应用和平台打包，比较实际依赖闭包。
4. **OpenCV 与 PSD**：两个独立批次，先回归再替换。PSD 覆盖混合模式、图层、通道、资源块、截断与超大长度，保持插件接口。
5. **媒体裁剪与设备统一**：校验 BPG、音频 seek/循环/淡入淡出、暂停恢复、采样位置和设备切换；删除无消费者库。选择 SDL3 音频后再评估移除 OpenAL/Oboe，避免提前删除仍在使用的输出。
6. **平台迁移**：先桌面实际 TJS/Layer 场景，再 Android 真机 IME、触摸、多窗口需求、后台恢复、资源与视频；确认无消费者后删除 cocos 源、CMake、ports 和打包资源。

CMake 的 PRIVATE 不隔离静态库最终链接；foundation 当前一次 PUBLIC 链接多个归档及存储目标，也需按真实符号拆分。不要通过伪实现消除缺失符号。
编译优化优先重头移出公共头、精确源文件可见性、Ninja 并行和编译缓存；PCH/Unity 仅对独立目标测量后启用，Unity 需检查宏泄漏和静态名称冲突。LTO/裁符号主要优化发行物，可能增加链接耗时，不属于开发构建加速的默认措施。

## 测量与交付要求

固定机器、提交、工具链、triplet、配置和并发数。分别记录：无二进制缓存的依赖构建、缓存命中安装、已有依赖的全量业务构建、单 cpp 修改、公共头修改；重复至少三次记录中位数。不要混用 Debug 与 Release、strip 前后体积。
记录 vcpkg 安装计划/实际 status、CMake File API 或 Graphviz 目标图、Ninja 编译任务、依赖日志和发行包明细；比较库数时分别列 host 工具、目标库和系统依赖。
候选验收目标：最小工具零图形/媒体依赖；完整包兼容回归无退步；构建时间和包体目标在同机基线后设定，不预填未经测量的百分比。
相关构建测试命令见 [构建与验证](building.md)。先运行 `ctest -N` 确认实际测试数量，再使用 `--no-tests=error`；本次仅分析和文档修改，没有运行应用编译、游戏或跨平台验证。

## 外部资料

外部文档仅说明候选接口，不代表本仓库已验证迁移，也不用于替代锁定版本 port 审计。本次未复制外部实现代码；未来引用代码需记录提交、文件及原许可。

- [SDL3 文本输入](https://wiki.libsdl.org/SDL3/SDL_StartTextInput)：文本/编辑事件与 IME 启用接口。
- [SDL3 Android](https://wiki.libsdl.org/SDL3/README-android)：Activity/JNI 集成说明。
- [Sokol 官方仓库](https://github.com/floooh/sokol)：独立 C headers 与支持平台。
- [GLFW 官方 API 指南](https://www.glfw.org/docs/latest/intro_guide.html)：窗口平台、生命周期与线程约束。
- [stb_image_resize2 源码](https://github.com/nothings/stb/blob/master/stb_image_resize2.h)：缩放候选，非通用图像变换替代。
- [miniaudio 官方仓库](https://github.com/mackron/miniaudio)：单文件音频库，仍需按功能配置和实测。
- [FFmpeg configure](https://github.com/FFmpeg/FFmpeg/blob/master/configure)：组件裁剪选项；实际参数须以项目锁定版本为准。
