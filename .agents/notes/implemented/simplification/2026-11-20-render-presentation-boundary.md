# Cocos 纹理呈现边界拆分

Status: implemented

## 结果与范围

用户授权继续 Cocos 渲染边界隔离，记录基线，先拆上传/呈现/截图，保留现有后端，
用自制场景验收后同步 dev。真实游戏按用户要求暂不使用。
基于 `9d7063a`；最终验证候选 `4988d02`，生产代码最后变更在 `de1c046`。
完整 run、产物/源码哈希与测量数据见[验证报告](../../../reports/render-presentation-boundary-2026-11-20.json)。
当前 API 和所有权约定见[纹理呈现](../../../../docs/subsystems/render-presentation.md)。

- `RenderManager.h` 不再暴露 cocos2d::Texture2D，改为 `GetPresentationFrame()`。
  `TexturePresentation.h` 仅依赖标准 C++，描述借用的 RGBA 行或当前共享 context 中的 GL 名字。
- CPU RenderManager.cpp 不再创建 Cocos 纹理；静态、解压和半高去重行分别提供同步读取描述。
  Cocos 包装、上传与复用统一位于 `environ/cocos2d/CocosTexturePresentation.*`，窗口与调试 UI 共用。
- 旧 GL 包装器同尺寸更新只换 GL 名字，却仍保留旧源引用。新包装器先保留新源再释放旧源，
  销毁时清空借用名字，GL 删除继续属于源。独立 lease 测试覆盖源之间存在所有权依赖时的交接次序。
- 宿主每次交帧核对逻辑裁剪与缩放，调试 UI 也不再仅以包装纹理地址判断是否更新可见范围。
  CPU 按物理上传尺寸复用，避免逻辑宽与 pitch/4 比较造成的反复分配。
- `FrameCapture.h` 提供不暴露 Cocos 的最终宿主截图入口，Cocos 实现保留原帧边界、异步保存和完成通知。
  模态绘制仍为既有 hook → drawScene → swap，未改动其时序。

## 最终验证

| 范围 | 结果与证据 |
|---|---|
| Headless 三平台 | [44728194229](https://github.com/2468785842/krkr2/actions/runs/44728194229) 13 个 job 全部通过；Windows x64/Linux x64/macOS ARM64 各实际执行 38 项 platform 测试，含新增 4 项呈现测试 |
| Windows 完整应用 | [44728185681](https://github.com/2468785842/krkr2/actions/runs/44728185681) 构建/链接、11 项 Cocos 契约和原有五组真实窗口交互通过；软件/OpenGL 图像与五轮呈现场景均退出0、无强停 |
| Linux 完整应用 | [44728190047](https://github.com/2468785842/krkr2/actions/runs/44728190047) 构建/链接、软件/OpenGL 图像与五轮呈现场景均通过，退出0、无强停 |
| Android ARM64 | [44727456172](https://github.com/2468785842/krkr2/actions/runs/44727456172) 完整 Debug APK 成功；下载实际 APK 核对 SHA256、全部 ELF 架构/库哈希/动态依赖及五份许可资源；348 个项目源，两份宿主适配源各编译一次 |
| 本地独立验证 | platform 38 项通过；新增4项 ASan/UBSan/leak 通过；Python 47 项、11个 foundation 依赖/接口守卫通过 |

Android 的 `de1c046` 与最终候选的生产 C++/资源/构建清单相同，后续只改测试控制器/文档。
三平台测试的注册查询会覆盖 artifact 中的 LastTest.log，因此执行数量已额外从原始 job 日志核实；
不能把空日志作为通过证据。最终 Linux CI 的 Python 实际执行 47 项。

初始算法图仍使用透明主层，核对7项操作、BMP 28个像素与保护点；追加呈现使用不透明主层，
依次为65×33、66×33带最右列色标、129×65、65×33、同尺寸assignImages后销毁原图层。
每轮核对独立事件、BMP、实际管理器和最终截图，最后必须正常退出。
两端软件/OpenGL 的右边界检查各得到960个合格样本；已实际查看初始图、色标帧和最后替换帧，共12张图。

## 基线与测量限制

基线 Linux [44722335954](https://github.com/2468785842/krkr2/actions/runs/44722335954)：
Build Linux 234s，测试包65,286,477字节，未压缩143,923,279字节；引擎120,654,944字节，
SHA256 48e8582d9cde62bfa0ef834785ff1a999a98ad87b64d3815d9f0de34587266a6。
最终包65,287,716字节，未压缩143,924,399字节，引擎120,656,064字节（增加1,120字节），
SHA256 c4b1b99a72cdd8fe7773ba1ad2557fa9351319e802bf73a91d856e6ee31bff1d。
两包均为92个文件并包含测试驱动/资源，不是剥离调试信息的正式分发包。

Windows 基线配置64s、构建197s，最终配置82s、构建19s；Linux最终Build步骤106s。
这些是带缓存的不同CI宿主，不能当作同机冷编译对照或声称编译提速。

直接引用扫描按明确的 Cocos 头前缀与 cocos2d:: 统计、去掉注释：69→70个文件、1395→1389处引用。
拆分新增了宿主实现文件，不能以文件数量作为依赖移除证明；CPU RenderManager.cpp 的直接引用为14→0。
visual 模块仍有传递 Cocos 依赖，未做独立 CPU 渲染器链接闭包。

## 失败过程与对照

`d921a24` 的 Windows [44724897614](https://github.com/2468785842/krkr2/actions/runs/44724897614) 和
Linux [44724901641](https://github.com/2468785842/krkr2/actions/runs/44724901641) 完整构建成功，
两端 OpenGL 四轮透明换图成功；软件第三轮得到窗口RGB(48,96,48)，控制器却假定每次都会减1，因而强停。
这些失败运行不计软件通过。

对照分支 `b03361e` 只把相同脚本放到 `9d7063a` 上，生产代码未变；
[44726497561](https://github.com/2468785842/krkr2/actions/runs/44726497561) 复现相同软件失败，
第三帧中央半幅像素与候选完全一致，SHA256均为
c99818e620b4547ee3ab31f2ff1708493a8b75fb2670b388ab47330aedd4652c。
因此确认该透明呈现舍入是已有行为。本批没有修改混合算法；初始透明算法图继续精确检查，
新增的资源/裁剪阶段改用不透明主层固定输入输出。

`de1c046` 的 Linux [44726893054](https://github.com/2468785842/krkr2/actions/runs/44726893054) 与
Windows [44726888431](https://github.com/2468785842/krkr2/actions/runs/44726888431) 又在单列色标检查失败：
实际截图可见亮色边缘，但线性采样后没有一个完全等于源色标的窗口像素。
确认BMP最右列精确为(224,208,224)，据此改用采样覆盖判据而非假设单列存在纯色平台：
主体G=64，标记G=208；旧65列裁剪最多贡献半个新增texel（G≤136），要求G≥180、R/B≥200，
即超过80%边列贡献。只统计右侧两逻辑列的中央竖带，排除宿主UI；Python用例明确拒绝半覆盖和亮色UI。
Layer BMP 和原始图像算法仍逐点精确相等，没有放宽算法像素容差。

## 剩余边界与下一步

Cocos仍负责GL状态缓存、视口/context重建事件、窗口、输入、UI、视频及部分平台服务。
此批完成公共呈现接口和宿主适配拆分，不删除Cocos port、不切换SDL3、不声称包体缩小或FPS提升。
macOS完整应用、Android设备和真实游戏未运行。
下一批优先隔离GL状态/context生命周期，再推进窗口/输入壳。
原始日志、截图、下载件和中间报告位于忽略的out/presentation-boundary；入库报告保留关键身份与证据。
