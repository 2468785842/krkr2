# 自制 ImageOps 渲染回归

场景只使用自制色块，不包含商业游戏。使用真实 Window/Layer、`perspective.dll` 插件和
软件/OpenGL 两个管理器运行四种 stretchCopy、doBoxBlur、affineCopy 与 perspectiveCopy。

```sh
python tools/game-smoke/imageops_repro.py /path/to/krkr2 out/imageops-runtime --capture
```

`--capture` 要求使用 `KRKR2_ENABLE_UI_TEST_DRIVER=ON` 的显式测试构建。
Linux 可用 `LIBGL_ALWAYS_SOFTWARE=1 ALSOFT_DRIVERS=null xvfb-run -a`；Windows CI
在隔离 runtime 目录使用既有固定哈希 Mesa，不修改系统 OpenGL。
不传 capture 时仅验证图层像素与正常退出，不能声称最终 Cocos 帧已检查。

控制器为两个管理器各建全新目录并写入独立 `Kirikiroid2Preference.xml`，不改全局配置。
每项操作检查四个不靠边界的已知不透明像素（RGB 和 alpha）；合成图层导出 BMP 后，
控制器重新解码并检查 28 个样本和未绘制区保护像素。
颜色的 R/B 相同，用于隔离已有 OpenGL getMainPixel 的原始通道顺序；这不验证通道交换。

capture 模式还要求驱动报告实际管理器与请求一致、唯一序号的 capture-saved 完成回执、
实际 PNG 中每种场景颜色至少有 16 个像素，防止空帧或未绘制的启动器被误认为通过。
PNG 仍应实际查看，不能把颜色数量当作完整布局/边缘逐字节比对。

报告保存 exe、脚本、图片哈希、操作事件、退出码与 forced_stop。成功必须完成全部操作、
无脚本错误、正常退出 0；任何超时强停都失败。原始 process.log、imageops.bmp 与 ui-frame.png
留在各目录。仅固定不透明区域的检查不代表 alpha 混合、字体、所有 GPU 插值边界或真实游戏验收。
