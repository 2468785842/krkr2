# 渲染器配置与宿主服务整块隔离

Status: implemented

用户要求继续完成一个大项后集中汇报。基于 dev 4dc5d2a，本批隔离软件/OpenGL 两套渲染器的
配置、内存压力、控制台日志、错误提示与本地化服务；保留 Cocos 后端，真实游戏按用户要求暂缓。
生产代码最后调整为 9110223，桌面最终验证源码为 7398347（新增大尺寸存储探针，C++ 与前者一致）。

## 实现与语义

- `visual/RenderHost.h`、`RenderSettings.h`、`RenderCapabilities.h` 只使用标准 C++，
  配置/平台/TJS 适配集中在 `environ/RenderHost.cpp`。两套核心渲染器不再直接包含
  Application、Platform、ConfigManager、Locale、WindowImpl 或 RenderUtils；依赖守卫禁止重新引入。
- 原 GetValue 读取缺省值会写回配置，Individual 读取还保留既有全局/游戏回退求值顺序。
  因此各键继续按原调用时机读取，不创建全量快照，不重写配置回退机制。
  任意 renderer 注册名称及未知压缩字符串回退普通工厂的行为保留；管理器仍在构造时选定工厂，没有新增热更新。
- 纹理上限保留设备值小于 1024 时允许配置覆盖的旧规则。
  MSVC 仍读取但忽略 GL 扩展开关；WIN32 禁用 unpack_subimage 的逻辑不变。
- 11 个扩展名保留原顺序，改用有序 std::array 遍历，避免跨结构体成员作指针迭代。
  删除无调用的提示框辅助函数及从未启用的旧调试示例，原有效提示文案与返回路径保留。
- 显式测试驱动记录实际管理器、存储工厂、纹理上限与扩展状态，不再次读取配置。
  这些只在主动启用的测试构建输出，不改变产品 UI。

## 验证

八组配置为软件 none/halfline/lz4/lz4+tlg5/未知值，OpenGL none/half/未知值。
每组启动独立进程，从 64×64 和 128×128 非纯色 BMP 重载静态源，检查四个象限的内点；
128 输入使 GL half 实际跨过 64 阈值，进入 128→64 缩减。
随后保留全部七项图像操作、28 个 BMP 样本与保护像素、五轮换图/裁剪/复用和正常退出。
GL half 额外检查实际上限 512，Linux 同时禁用四个 copy_image 扩展覆盖回退；未放宽任何像素期望。

| 验证范围 | 实际结果与证据 |
|---|---|
| 本地独立测试 | platform 42 项；ASan/UBSan/泄漏检查的渲染配置与呈现契约 8 项；Python 49 项 |
| 三平台无窗口 CI | [44838237285](https://github.com/2468785842/krkr2/actions/runs/44838237285) 的 13 个 job 全通过；platform 每平台 42 项、17 个编译源，依赖闭包仅 Catch2 与 vcpkg 辅助 port；macOS 原生 GL 头编译通过 |
| Windows 完整应用 | [44839396982](https://github.com/2468785842/krkr2/actions/runs/44839396982)：15 项 Cocos 契约、49 项 Python、八配置场景和五组窗口/指针/焦点/最小化/文本回归通过，均未强停且退出 0 |
| Linux 完整应用 | [44839400282](https://github.com/2468785842/krkr2/actions/runs/44839400282)：15 项 Cocos 契约、八配置场景通过，均未强停且退出 0 |
| Android ARM64 | [44840297688](https://github.com/2468785842/krkr2/actions/runs/44840297688)：完整 Debug APK 通过，350 个项目源；下载后复核单一 ARM64 ABI、四份 ELF 库及依赖、APK/库哈希和六份 ImageOps 许可/来源文件 |
| 静态检查 | 11 个 foundation 目标依赖守卫通过；三个新公共头用标准 C++ 单独编译通过；clang-format 20.1.8、文档链接及 diff 检查通过 |

桌面报告下载后重新核对两种尺寸的输入 BMP、全部六轮截图哈希及八组实际配置；
实际查看软件 halfline/LZ4/TLG5、未知值回退与 GL half 的算法图/右边列截图，未见空帧或缺失边列。
Windows 的脚本/控制器使用 checkout 的 CRLF 换行，Linux 使用 LF；两者原始哈希均与同一 Git 源码对应，报告保留各自原始身份。
Windows 编译元数据确认 RenderHost、CocosGLHost、两套渲染器和两份 ImageOps 源各编译一次；
完整应用依赖检查继续无 OpenCV。
无窗口 CI 的 b856036 与最终版本独立测试源一致，后续调整仅涉及 GL 实现头和自制运行探针。

## 失败与修正

初始源码 b856036 的 Windows 44838232028、Linux 44838234531、Android 44838241219
均因显式包含 LayerBitmapImpl 而缺失 LayerBitmapIntf 的 tTVPBBBltMethod 等枚举声明，编译失败。
9110223 显式改为接口头，未恢复 WindowImpl 间接依赖；Android 首次依赖缓存成功保存后再执行修正版构建。
修正后的 64×64 首轮 Windows 44839048621 / Linux 44839052001 均通过八组矩阵；
之后发现 GL half 的缩减阈值为大于 64，故 7398347 补充独立 128×128 探针并再次完整验收，原算法输入保持不变。
早期失败、最终源码身份、各轮截图与 APK 哈希见[验证报告](../../../reports/render-host-services-2026-11-20.json)。

## 剩余边界

仅隔离直接宿主访问，完整 visual/CMake 链接仍有传递 Cocos 依赖；不删除 Cocos port，不切换 SDL3，
未测量或宣称 FPS、编译速度、包体改善。存储诊断描述选中的工厂，不能保证每张纹理使用特定内部编码。
ETC2/PVRTC 仍依赖设备格式支持，未作本批运行验收。
现有 context 通知继续只重建 shader，完整纹理/FBO/缓冲恢复尚未实现；后续需定义资源图及恢复顺序再做设备验证。
未运行真实游戏、Android 设备或 macOS 完整应用。
当前接口说明归入[纹理呈现与宿主边界](../../../../docs/subsystems/render-presentation.md)。
