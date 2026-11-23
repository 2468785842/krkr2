# 普通构建的 OpenGL 注册与构造失败清理

Status: implemented

## 问题与证据

在 `1fddb56` 加资源/XML 拆分的 dirty 工作区，用 Windows Debug 普通构建运行
`tools/game-smoke/imageops_repro.py`。`KRKR2_ENABLE_UI_TEST_DRIVER=OFF`；
桌面实际编译定义 `KRKR2_USE_COCOS_UI=0`（旧 cache 的 ON 条目不代表当前目录变量和编译命令）。

软件场景正常退出，OpenGL 场景在 ready 前以 `0xC0000005` 退出。
失败 exe SHA-256 为 `6b0c9e3a7d5e9ed55b27ad84c761c84641b06cdf3222112e99e35f9dedb7128f`。
原始报告在 `out/config-native-validation/runtime-smoke/`。
本机为 Intel UHD Graphics 630、驱动 31.0.101.2141；型号本身不是故障归因。

CDB 首次访问冲突栈为：

```text
tTVPNativeBaseBitmap::~tTVPNativeBaseBitmap
iTVPBaseBitmap::~iTVPBaseBitmap
tTVPBaseTexture 构造失败的展开
TVPThrowExceptionMessage
TVPGetRenderManager(const ttstr &)
TVPGetRenderManager()
tTVPBaseTexture::tTVPBaseTexture
tTVPTempBitmapHolder::AddRef
tTJSNI_BaseLayer::tTJSNI_BaseLayer
```

被析构指针为 MSVC 未初始化内存模式 `cdcdcdcd`。
GL 翻译单元只通过内部静态注册对象注册工厂；普通桌面裁剪管理页、关闭测试驱动后，
缺少有效链接引用。测试驱动的 `TVPCheckGLExtension` 以及移动管理页的 GL 查询能隐式保留该对象。
未对引入提交做二分定位，不据此指定最早引入版本。

失败 exe/PDB 保存在 `out/config-native-validation/opengl-cdb/failing-runtime/`，
PDB SHA-256 为 `8a8dabe4f3bccfc00f3c1227ab729894e1dd7740e535d9684b9fafcd7bedb4d6`。
首个 CDB 日志为 `opengl-cdb/debugger.log`；调试器在取栈后退出，不作为正常退出证据。
另一次基于函数通配符的断点尝试没有匹配到代码符号，该尝试不构成“没有 GL 符号”的证明。

## 实现

- `visual/RenderManager.cpp` 的内建注册器显式引用软件与 OpenGL 工厂。
  `ogl/RenderManager_ogl.cpp` 提供实际创建函数，避免依赖静态对象被偶然拉入最终链接。
  注册时不创建管理器或访问 GL context。
- `impl/LayerBitmapImpl.h` 的 Bitmap 从 nullptr 开始，派生构造失败时可安全展开。
- `RenderCleanup.h` 的退出回收遍历工厂中已完成 Initialize 的管理器，
  Windows/Android 不再为退出而查询、创建默认渲染器。
- `renderer_error_repro.py` 生成未知渲染器、Layer 构造异常捕获及 AfterStartup 退出场景，
  由 `KRKR2_ENABLE_SCRIPT_SMOKE=ON` 登记为 `renderer-error-runtime`。

## 验证

最终 Windows 普通构建 SHA-256：
`474d7599dfba1e117dbae850c56ad3159d1734a2c1aa912f33e03cda367d3138`。
仍为 Debug、测试驱动关闭、实际管理 UI 编译定义为 0；没有通过启用测试驱动绕过注册问题。

- `cmake --build --preset "Windows Debug Build" --target krkr2 ...` 构建通过。
  完整命令在 `out/config-native-validation/app.cmd`，成功链接见 `app-exit-final.log`。
- `imageops_repro.py ... out/config-native-validation/runtime-complete` 的软件/OpenGL 两场景均通过
  七项算法像素检查、ready/finished、退出码 0，forced_stop=false。
  场景 SHA-256：`646b99f2f46d3c7bddf18b4ac130c5c5951058a6220b5f18ded3678edc9e561e`。
  软件 BMP 为 `395f5bb131813c513681734ed29859ea0682c6640e0fe80bebfc47e1375d711d`，
  GL BMP 为 `8f01fb5c23ca9b5bc6db7f324e05d6cfdf5e6fcee5998a1b8043b4eab2225e86`。
  两张 Layer BMP 均实际查看；不是 OS 窗口或宿主 UI 截图。
- CTest 实际注册并通过 18 项配置/资源对照、17 项 Cocos 节点/输入/视频解绑契约。
- `renderer-error-runtime` 注册 1 项、通过 1 项：记录构造错误和 exit-requested，退出码 0、无强停。
  startup SHA-256 为 `864fb6a28cffd585e3c308531011d5b64fd578e9c5fe611eb5906ef1cc5a4d85`，
  AfterStartup 为 `e343a3279a200ce86f612d365d9a63998fa84c45a9c025b9594c9f7d586638be`。
  报告为 `out/windows/debug/script-smoke/renderer-error/renderer-error-18nehsrn/report.json`。
- 既有 textstream/parser-error 两项真实脚本回归及 15 项 CLI 检查通过。

## 失败与范围

早期错误配置 Timer 探针在记录 caught 后超时，被控制器强停；这些失败未算通过。
在 startup 顶层直接 System.exit 的探针也未退出：`ScriptMgnIntf.cpp` 在启动脚本返回后才设置
TVPStartupSuccess，System 的退出方法在此前不会执行退出。最终用正式 AfterStartup 钩子在启动成功后、
首帧前验证清理，不声称非法 renderer 配置可以继续绘制或正常推进帧定时器。

一次 MSVC 链接进程在 7 分钟内无进展；确认没有其他编译/链接任务后，停止本次链接进程及由本次构建
创建的 PDB 服务，使用原参数重试成功。没有修改全局工具链、安装驱动或更换 GL 实现。

本记录的游戏运行证据仅为 Windows 自制场景。Android 退出调用点已修改但未构建 APK；
Linux/macOS 的完整渲染运行、真实 context 丢失、商业游戏与移动端生命周期仍需独立验收。
