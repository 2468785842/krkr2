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

软件透视插件现有 `PerspectiveAlphaBlend_a` 在混合前按 RGB×255/256 截断，
这些非零不透明色块的结果比 OpenGL 少 1。期望值按 `tvpgl.h` 的旧公式分别固定，
未放宽像素容差，未修改生产混合代码；实际管理器仍必须由 capture 回执确认。

初始算法场景的软件最终窗口合成使用 alpha/256 转换，故 PNG 的主色精确期望为
(63,159,63)、(159,63,159)、(63,63,63)、(159,159,159)；OpenGL 为原色。
图层 BMP 仍按原色检查，仅软件透视面板再减 1。二者分别检验算法输出与显示合成，均不使用容差。
Linux workflow 通过 `imageops_contracts=true` 显式构建测试驱动，默认普通应用关闭此开关。

捕获模式还会依次提交 65×33、66×33、129×65、65×33 和同尺寸 `assignImages` 替换画面，
检查每轮独立事件、BMP 三个取样点及窗口中的主色和右边界标记，并记录逐轮图片哈希。
这覆盖奇数宽度、相同物理上传宽度中的逻辑尺寸变化、重复尺寸和内容更新；原算法图保存为 `ui-imageops.png`，五张追加帧为
`ui-presentation-1.png` 至 `ui-presentation-5.png`，`ui-frame.png` 是驱动最近一次捕获。
独立的引用寿命测试另检查同尺寸 GPU 适配器使用的先 retain 后 release 规则。

追加呈现阶段使用不透明主层，以确定的颜色检查上传、裁剪和复用；初始算法场景仍使用透明主层。
66×33 阶段最右一列有独立色标，要求真实窗口截图包含该色，防止纹理复用后仍沿用旧逻辑裁剪。
