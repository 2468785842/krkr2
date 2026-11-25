# 管理界面状态

2026-11-25：Cocos 管理界面及其 `assets/img/`、`assets/out_ui/` 资源已删除。
当前仅构建 Windows/Linux SDL3 应用，通过[桌面命令行](desktop-cli.md)选择游戏、查看和覆盖设置。
脚本菜单、消息框、输入框及文件选择仍由原生平台服务提供，不依赖已删除的管理器。

Android/macOS 暂停构建；未来恢复时需要新的启动和设置界面，不能复用已删除的 Cocos 类。
旧实现可在 Git 基线 `c494ea0` 查阅。
