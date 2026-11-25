# 依赖清单与瘦身审计

本轮基于 `b6cb999`，核对源码调用、实际 CMake 编译/链接关系、vcpkg 安装状态和文件清单。
验证环境为 Linux、GCC 14.2、Release、`KRKR2_SDL3_ONLY=ON`、PSD 开启、tools/UI test driver 关闭。
Windows、macOS、Android 的新改动仍需对应 CI；此前通过的商业游戏测试属于原基线。

## 必需与可选的边界

“源码没有直接包含某库”不等于可以删掉该库。静态归档、平台适配和内建插件可能是实际消费者。
本项目保留已有脚本可观察功能；下列“可选”只指已有构建开关或将来可隔离的边界，不表示默认删除。

| 依赖 | 实际消费者与处理 |
|---|---|
| Boost.Locale、fmt、spdlog、Oniguruma | `core/tjs2` 字符编码/正则及 `core/log`；当前基础依赖，独立工具也使用 |
| FFmpeg | `core/movie/ffmpeg` 的解复用、解码、过滤、采样转换，以及 `external/libbpg`；不能只留下视频解码器 |
| FreeType | `core/visual/FreeTypeFontRasterizer.cpp`；默认压缩字体、内嵌 PNG 字形能力保留 |
| JPEG、PNG、WebP、JXR、libbpg | `core/visual/Load*` 与 `GraphicsLoaderIntf.cpp`；任意游戏素材都可能使用，保留格式支持 |
| xxHash、LZ4 | `RenderManager.cpp` 的纹理/像素缓存；xxHash 虽为 inline 调用，头文件依赖仍然有效 |
| Vorbis、OpusFile、Opus、Ogg | `sound/VorbisWaveDecoder.cpp` 同时实现 Vorbis/Opus 解码；保留解码，移除未使用的 Vorbis 编码链接 |
| OpenAL | `sound/win32/WaveMixer.cpp` 的音频设备；Android 还有 Oboe/miniaudio 路径，不能把这些视为无人使用的重复库 |
| 7zip | 7z 归档读取和 `environ/XP3ArchiveRepack.cpp` 的重打包；不能因有 libarchive 就直接删除 |
| libarchive、unrar | `base/UtilStreams.cpp`，实际编译目标是 `core_storage`；保留格式与解包接口，依赖归属已收回实际消费者 |
| XML、zlib、bzip2、liblzma、LZ4、zstd | libarchive 导入目标自身的传递依赖；不再靠 `base` 或配置模块偶然链接 |
| uchardet | `base/TextStream.cpp` 的文本编码识别 |
| nlohmann-json、pugixml | JSON 配置/语言及旧 XML 迁移，见[宿主资源与配置](host-resources.md) |
| Blend2D/AsmJit | 非 Windows 的 `plugins/layerex_draw`；Windows 使用 GDI+ 实现。是插件能力，不是遗留的 Cocos 依赖 |
| GLEW/OpenGL | 旧渲染路径和 motionplayer 的私有 GL 路径仍引用；SDL3 软件呈现不表示整个引擎已无 GL 代码 |
| SDL3 | 独立桌面窗口、输入、手柄及宿主服务 |
| GTK3 及其传递依赖 | Linux SDL3 的原生菜单、模态窗口与文件对话框；Windows 使用系统服务，不安装 GTK |
| Cocos、GLFW、TIFF 等旧宿主闭包 | macOS/Android 和旧混合宿主仍使用；`KRKR2_SDL3_ONLY=ON` 不进入这组依赖 |
| Boost.Iostreams/Phoenix/Spirit | PSD 插件，已有 `KRKR2_ENABLE_PSD=OFF`；默认开启保留兼容性 |
| Breakpad、dirent、平台音频/系统库 | 按平台选择；不能用 Linux 源码未引用推断 Windows/Android 无用 |
| Catch2、argparse、vcpkg 构建工具 | 测试、XP3 命令行工具与 SDK 构建期依赖；分别由 tests/tools/features 选择，不等于运行时包大小 |

## 本轮精简

- 删除引擎清单中的 GLM。工程内没有 GLM 包含、`glm::` 调用或 CMake 消费者，实际 SDK 也没有其他包依赖它。
  Cocos 自带的数学代码不靠这个额外 vcpkg 包。
- 引擎的 WebP 清单只选择 SIMD，取消未使用的 mux 和 near-lossless 编码 feature。
  图像模块只链接 `WebP::webpdecoder`，不再同时链接完整 WebP 和 demux。
  Cocos UI 自己的默认依赖可能再次启用这些 feature，因此只对实际原生 SDK 报告本轮 feature 收益。
- 音频模块删除 `Vorbis::vorbisenc`，保留 Vorbis/VorbisFile/Opus 解码。
- `core_base_module` 删除未消费的 unrar/libarchive/zstd 直接边；`core_storage` 保留 unrar/libarchive。
  libarchive 的压缩和 XML 依赖由 overlay 清单与导入目标完整传递，Windows 另传递 CNG 所需 bcrypt。
- libarchive overlay 不再构建 bsdtar、bsdcpio、bsdcat、bsdunzip；引擎使用库接口，没有调用这些 SDK 程序。
  gzip、bzip2、xz、LZ4、zstd 库功能保留，port-version 增为 2，避免继续复用旧缓存。

新增的 `libarchive_dependency_tests` 只链接 Catch2 和 `LibArchive::LibArchive`，
不链接引擎或手动补 codec 库。它进行五种压缩的真实 TAR 往返，并核对 XAR 平台边界。
Windows/Apple 有 CNG/CommonCrypto 时读取自制 XAR 的 XML TOC；当前 Linux SDK 关闭 OpenSSL、
没有 XAR 必需的 MD5/SHA1 后端，因此返回“不支持”，这是原配置边界，不能仅凭装有 LibXml2 就声称支持 XAR。

## 实际测量

SDK 数值来自 vcpkg 文件所有权清单，包含静态库、头文件、工具、许可证和构建辅助文件，
按文件逻辑大小计数，不是文件系统占用块数，也不是发布包大小。

| 项目 | 修改前 | 修改后 |
|---|---:|---:|
| 安装的 port/架构组合 | 164 | 163 |
| SDK 清单文件字节 | 1,610,123,432 | 1,600,154,469 |
| 未剥离应用字节 | 121,353,680 | 121,353,648 |
| 编译单元 | 368 | 369 |

SDK 减少 **9,968,963 字节，约 9.51 MiB**，主要是 GLM 头文件、归档 CLI 和未使用的 WebP 构建产物。
应用大小基本不变；32 字节链接布局差异不是有意义的运行包瘦身收益。新增的编译单元是独立归档依赖测试。
初批仅清理 GLM/WebP/Vorbis 时，前后应用 SHA256 完全相同；进一步收紧归档边后链接布局改变。

另做 libarchive 3.7.7 源码的单库冷构建目录对照：同一 SDK、Release、Ninja、3 并发，
顺序为 CLI 开/关/关/开。开启耗时 22.093、20.505 秒；关闭耗时 21.081、20.084 秒。
区间重叠，暂不能宣称稳定的构建提速，更不能外推为整仓冷构建收益；本轮确定收益是减少构建产物和依赖边。

最大体积来源仍是 FFmpeg（约 663.38 MiB）和 GTK3 自身（约 505.20 MiB）。
GTK3 的安装依赖闭包为 69 个包、约 726.39 MiB，含与其他功能共享的项，不能与其他闭包直接相加。
以本次 Linux 明确选择的 engine/foundation/psd/tests/sdl3/native-dialogs 根计算，
其中 44 个包、约 699.38 MiB 不由其他根保留；这是 SDK 层的潜在范围，不是可立即删除的功能或发布包收益。

## 复查方法

```sh
python scripts/audit_dependencies.py out/native-independent out/dependency-audit/report.json
python -m unittest discover -s scripts/tests -v
python scripts/check_core_module_deps.py
python scripts/check_native_runtime.py out/native-independent
LIBGL_ALWAYS_SOFTWARE=1 ALSOFT_DRIVERS=null xvfb-run -a \
  ctest --test-dir out/native-independent --output-on-failure --no-tests=error
```

审计脚本读取实际 CMakeCache、compile_commands、vcpkg status 与每包的 `.list`。
它报告版本、feature、直接/反向包依赖、各目录字节、缺失文件和实际收到 OpenMP 选项的源文件。
它不会把头文件搜索命中当成使用证明，也不会自动猜测 manifest 条件依赖是否在当前平台启用。
复用的 SDK 可能含旧包，应同时检查安装计划、应用链接和源码调用；脚本会拒绝空安装状态/编译数据库。

本轮 Linux 完整构建与 **121 项 CTest** 通过。新增编码探针通过真实 Layer 检查 WebP 有损、无损和 alpha，
通过真实 OpenAL PCM 的 440 Hz 分量验证 Ogg/Vorbis 播放；默认 WAV 首缓冲探针保持原样。
格式样本均为自制，见 `tools/game-smoke/codecs/`。静态归档消费者和审计脚本分别加入应用测试及现有 Python CI。
原生 CI 同时保存 SDK 审计 JSON，供不同平台复查实际安装闭包。

## 下一批优先级

1. **修正线程依赖归属。** `utils/impl/ThreadImpl.cpp` 中存在 OpenMP pragma，但本次实际编译命令没有 OpenMP 选项；
   选项挂在 `krkr2core` 接口上，87 个消费者收到选项，却没有传到执行循环的源文件。
   必须先补任务次数、异常与嵌套调用契约及像素/性能基线，再决定将依赖移到真实目标或复用标准线程调度。
   本轮不改变线程执行语义。
2. **隔离 Linux GTK 服务，再评估替换。** 先列清菜单、消息框、文件选择、模态生命周期和 UTF-8 路径契约。
   仅用 SDL 文件选择 API 替换一个对话框，不能据此移除整套 GTK 服务。
3. **区分 SDK 调试信息与运行时代码。** FFmpeg overlay 对 Release 也启用 debug=3，SDK 静态库含大量调试信息。
   如提供明确的 Release SDK 符号选项，可测磁盘/编译开销；不能把这等同于减少游戏 codec 或包体。
4. **保持格式能力的可选边界。** PSD 已有关闭开关；BPG/JXR、音频 codec 的进一步拆分需要素材语料和明确的“不支持”行为，
   不默认删除冷门格式。不要通过关闭 XAR/压缩支持来掩盖静态链接错误。

## 后续平台工作量边界

macOS 独立宿主还需要 SDL3 组合根、Cocoa 原生菜单/对话框服务、Apple GL/字体/路径接线及实际打包验收。
现有 SDL3-only 入口限制在 Windows/Linux；“SDL3 支持 macOS”不能作为本项目已接好的证据。

Android 的生产入口、资源/OBB 读取、窗口/GLES context 和生命周期仍在 Cocos 宿主中。
后续至少分为入口与打包、资源/设置路径、IME/触摸/手柄、后台恢复/设备处理、音视频呈现五批。
已有真实 Framework/JNI 输入测试可复用，但不替代 SDL Activity、实际 GPU、权限或真机游戏验收。
因此下一步优先收紧共享依赖和平台服务边界，暂不把 Android/macOS 当作简单更换链接库。
