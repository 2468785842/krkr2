# XP3 独立构建的 Windows 首批验证

Status: implemented

历史记录：其中缺失工具和配置失败描述属于当时环境；当前命令以构建指南为准。

首批（2026-11-01）Windows 验证：MSVC 19.44、Debug、x64-windows-static-md，以上独立配置、`cmake --build out/windows/xp3 --target xp3 -j 6` 及 smoke 命令通过；再次构建输出 `ninja: no work to do`。独立 vcpkg 安装状态中没有 cocos2dx、opencv4、ffmpeg、catch2、libarchive、unrar，compile_commands 中没有 visual/movie/plugins 源码且没有重复源文件。首次安装使用部分本地缓存，未测量同机全量基线，不能据此宣称耗时缩减比例。
完整引擎另用 `out/windows/engine-config-check`、`VCPKG_MANIFEST_INSTALL=OFF` 和现有 `out/windows/debug/vcpkg_installed` 检查配置，在 `cpp/core/base/CMakeLists.txt:60` 因 `Could NOT find LibXml2 (missing: LIBXML2_LIBRARY LIBXML2_INCLUDE_DIR)` 停止；后续已在隔离目录补齐 LibXml2，改用导入目标，并完成使用新裁剪 cocos 的应用链接，详见 [实施记录](../simplification/2026-11-01-dependency-slimming.md)。未重新安装整套引擎依赖。`python scripts/check_core_module_deps.py` 因 dev 已删除该文件而无法执行。该批尚未验证 Linux/macOS/Android；归档续批的独立验证见实施记录。
