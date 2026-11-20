# 渲染器配置与宿主服务整块隔离

Status: proposed

用户要求继续完成一个大项后集中汇报。基于dev 4dc5d2a，范围为软件/OpenGL两套渲染器的
配置、内存压力、控制台日志、错误提示与本地化信息边界；保留Cocos后端，真实游戏暂缓。
完成条件为三平台独立测试、Windows/Linux完整应用及八配置矩阵、ARM64完整APK，随后同步dev。

## 实现与关键约束

RenderHost.h、RenderSettings.h、RenderCapabilities.h只使用标准C++；具体配置/平台/TJS适配在environ/RenderHost.cpp。
两套核心渲染器不再直接包含Application/Platform/ConfigManager/Locale/WindowImpl/RenderUtils。
原先GetValue会写入缺省值，故保留各键的按需读取时机，不创建全量配置快照；
IndividualConfigManager原有全局/游戏配置回退和写回逻辑继续由宿主调用。
原始renderer名称字符串保留，未知压缩值仍回退普通工厂；纹理上限保留device<1024时的旧覆盖规则。
GL扩展开关保持原MSVC忽略开关的行为，WIN32禁用unpack_subimage逻辑不变。
11个扩展名的读取/显示顺序保留，但用std::array代替跨结构体成员指针遍历。

诊断仅在测试驱动中输出实际选择的工厂、实际纹理上限与扩展状态，不再次读取配置。
矩阵为软件none/halfline/lz4/lz4+tlg5/未知值，OpenGL none/half/未知值，共8组；
half配置另检查512上限与Linux禁用copy_image扩展的回退。
每组从BMP重载静态源并验证像素，之后运行原7项图像操作、28个BMP样本和五轮呈现/正常退出。
这验证选中的工厂和压缩输入路径；不宣称所有硬件支持ETC2/PVRTC，也不涉及真正context丢失恢复。

## 当前验证

本地platform共42项（新增4项配置契约）通过，Python49项通过，依赖守卫通过。
后续远端结果与产物哈希记录在忽略的out/render-services-boundary，通过后迁为implemented。
