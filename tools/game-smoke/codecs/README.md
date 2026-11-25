# 自制 WebP 解码样本

三个 4×4 灰色（RGB=128）样本，无第三方素材。`gray-alpha.webp` 的 alpha=128，其他为 255。
`gray-lossless.webp` 与 alpha 样本为无损，`gray-lossy.webp` 为 quality=75 的有损编码。
由 Pillow 的 `Image.new(...).save(..., lossless=True)` / `quality=75` 生成。

`audio_startup_repro.py --codec vorbis --webp` 在真实 Layer 中检查尺寸、RGB（有损容差 2）和 alpha，
并播放自制 `tone.ogg`，检查实际 OpenAL PCM 中的 440 Hz 信号。
`tone.ogg` 是 48 kHz 单声道、两秒、振幅 4000 的 440 Hz 正弦波，使用 ffmpeg libvorbis quality=4 编码。
沿用工作线程启动延迟探针；需要 Linux、X server、OpenAL wave 后端和测试延迟库。
默认不加这些选项时，原 WAV 首缓冲验收保持原样。
