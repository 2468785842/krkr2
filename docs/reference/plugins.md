# 插件来源

本表保留插件原始资料入口，不表示这些插件全部内建、仍可下载或已经兼容。

| DLL name               | Source or binary location                                                                               |
|:-----------------------|:--------------------------------------------------------------------------------------------------------|
| AlphaMovie.dll         | http://kaede-software.com/krlm/plugin/alphamovie.zip                                                    |
| DrawDeviceD3D.dll      | https://github.com/krkrz/krkr2/tree/master/kirikiri2/trunk/kirikiri2/src/plugins/win32/drawdeviceD3D    |
| DrawDeviceD3DZ.dll     | https://github.com/krkrz/krkrz/tree/last_hodgepodge_repository/src/plugins/win32/drawdeviceD3D          |
| ExtKAGParser.dll       | http://keepcreating.g2.xrea.com/krkrplugins/ExtKAGParser/ExtKAGParser-0143.zip                          |
| KAGParserEx.dll        | https://github.com/wtnbgo/KAGParserEx                                                                   |
| KAGParserExb.dll       | https://github.com/sakano/krkr_archives/tree/master/kagex_plugin/KAGParserExb                           |
| KaichoTrans.dll        | http://keepcreating.g2.xrea.com/krkrplugins/KaichoTrans/KaichoTrans.zip                                 |
| LayerExDraw.dll        | https://github.com/wtnbgo/layerExDraw                                                                   |
| LayerExImage.dll       | https://github.com/wtnbgo/layerExImage                                                                  |
| LayerExSave.dll        | https://github.com/wtnbgo/layerExSave                                                                   |
| PSBFile.dll            | M2 Inc. PSB Library                                                                                     |
| PackinOne.dll          | nan                                                                                                     |
| TextRender.dll         | nan                                                                                                     |
| addFont.dll            | https://github.com/wtnbgo/addFont                                                                       |
| csvParser.dll          | https://github.com/wtnbgo/csvParser                                                                     |
| dirlist.dll            | https://github.com/krkrz/krkrz/tree/last_hodgepodge_repository/src/plugins/win32/dirlist                |
| emoteplayer.dll        | M2 Inc. : https://emote.mtwo.co.jp/download/                                                            |
| expat.dll              | https://github.com/krkrz/krkrz/tree/last_hodgepodge_repository/src/plugins/win32/expat                  |
| extNagano.dll          | https://web.archive.org/web/20120604091809fw_/http://ymtkyk.sakura.ne.jp/krkr.STG/plugin/extNagano.html |
| extrans.dll            | https://github.com/krkrz/SamplePlugin/tree/master/extrans                                               |
| fftgraph.dll           | https://github.com/krkrz/fftgraph                                                                       |
| savestruct.dll         | https://github.com/wtnbgo/saveStruct                                                                    |
| fstat.dll              | https://github.com/wtnbgo/fstat                                                                         |
| getSample.dll          | https://github.com/wtnbgo/getSample                                                                     |
| getabout.dll           | nan                                                                                                     |
| gfxEffect.dll          | http://kaede-software.com/krlm/plugin/gfx_effect.zip                                                    |
| json.dll               | https://github.com/wtnbgo/json                                                                          |
| kirikiroid2.dll        | nan                                                                                                     |
| layerExAlpha.dll       | nan                                                                                                     |
| layerExAreaAverage.dll | https://github.com/wtnbgo/layerExAreaAverage                                                            |
| layerExBTOA.dll        | https://github.com/wtnbgo/layerExBTOA                                                                   |
| layerExImage.dll       | https://github.com/wtnbgo/layerExImage                                                                  |
| layerExMovie.dll       | https://github.com/krkrz/krkrz/tree/last_hodgepodge_repository/src/plugins/win32/layerExMovie           |
| layerExRaster.dll      | https://github.com/wtnbgo/layerExRaster                                                                 |
| layerExShimmer.dll     | http://keepcreating.g2.xrea.com/krkrplugins/ShimmerPlugin/layerExShimmer.zip                            |
| lzfs.dll               | nan                                                                                                     |
| minizip.dll            | https://github.com/wtnbgo/minizip                                                                       |
| motionplayer.dll       | nan                                                                                                     |
| multiimage.dll         | nan                                                                                                     |
| perspective.dll        | https://github.com/krkrz/krkrz/tree/last_hodgepodge_repository/src/plugins/win32/layerExPerspective     |
| saveStruct.dll         | https://github.com/wtnbgo/saveStruct                                                                    |
| scriptsEx.dll          | https://github.com/wtnbgo/scriptsEx                                                                     |
| shrinkCopy.dll         | https://github.com/wtnbgo/shrinkCopy                                                                    |
| sqlite3.dll            | https://github.com/krkrz/krkrz/tree/last_hodgepodge_repository/src/plugins/win32/sqlite3                |
| util_generic.dll       | nan                                                                                                     |
| util_graph.dll         | nan                                                                                                     |
| util_system.dll        | nan                                                                                                     |
| varfile.dll            | https://github.com/wtnbgo/varfile                                                                       |
| win32dialog.dll        | https://github.com/wtnbgo/win32dialog                                                                   |
| windowEx.dll           | https://github.com/wtnbgo/windowEx                                                                      |
| wutcwf.dll             | https://github.com/krkrz/SamplePlugin/tree/master/wutcwf                                                |
| xp3filter.dll          | nan                                                                                                     |

## layerExBTOA 的 clipAlphaRect

`cpp/plugins/layerExBTOA.cpp` 内建 `Layer.clipAlphaRect(dleft, dtop, src,
sleft, stop, swidth, sheight, clear=void)`，只覆盖这个方法，不宣称整个插件完成。
按源图 alpha 乘目标 alpha；普通图层保留 RGB，`ltAddAlpha` 同步缩放预乘 RGB。
只修改目标 clip 与图像边界的交集，源 clip 不参与；clear 在 0–255 时处理交集内
操作矩形以外的区域，省略/void/越界值保留原像素。普通图层 clear 直接设 alpha，
预乘图层 clear 作为颜色和 alpha 的倍率。无交集时仍保持这一预乘契约。
源 mask 在写入前快照，支持自身重叠与 assignImages 共享存储；坐标计算使用 64 位中间值。
通过现有纹理读写接口处理 CPU/GPU 数据，并更新 imageModified 和对应显示区域。

行为核对来源：[wamsoft/layerExBTOA](https://github.com/wamsoft/layerExBTOA/tree/a57e7a25a9e079f95b93cf9768949e0f8789f685)
的 Main.cpp（clipAlphaRect、ClipAddAlpha）与 readme.txt，作者 わたなべごう；
原 readme 声明插件许可遵循吉里吉里本体。本实现重新组织原生 Layer 访问、裁剪与
快照流程，保留其整数透明度运算规则，没有引入 AGPL 项目代码。

启用 `KRKR2_ENABLE_PSBFILE_SMOKE=ON` 时同时注册 `layer-alpha-runtime`，
运行 `tools/game-smoke/layer_alpha.py` 的合成像素测试（需要显示服务）。
覆盖透明度端点、部分透明、clip、clear、负坐标、源边界、极值坐标、重叠/共享、
预乘颜色、错误参数与修改标志。真实游戏验收另记于活动 note。
