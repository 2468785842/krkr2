#include "ncbind.hpp"
#include "LayerExDraw.hpp"
#include "log/TVPLog.h"

#include "BinaryStream.h"
#include "FontImpl.h"

#include <vector>
#include <cstdio>
#include <cmath>
#include <freetype/freetype.h>

#include <opencv2/opencv.hpp>

#define NCB_MODULE_NAME TJS_W("layerExDraw.dll")

extern void TVPGetAllFontList(std::vector<ttstr> &list);

static std::vector<BLFontData> bl_font_data_vec;
static std::vector<BLFontFace> bl_font_face_vec;
static std::vector<ttstr> bl_font_face_name; // 方便一点吧

// GDI+ 初期化
void initGdiPlus() {
    const std::unique_ptr<tTJSBinaryStream> stream{ TVPCreateFontStream(
        TVPGetDefaultFontName()) };
    if(stream) {
        tjs_uint8 *fileData = new tjs_uint8[stream->GetSize()];
        stream->ReadBuffer(fileData, stream->GetSize());
        // 读取fontdata
        BLFontData bl_font_data;
        BLResult stat = bl_font_data.createFromData(fileData, stream->GetSize(),
                                                    NULL, NULL);
        delete[] fileData;
        if(stat != BL_SUCCESS) {
            TVPThrowExceptionMessage(TJS_W("blend2d cannot load:%1"),
                                     TVPGetDefaultFontName());
        }
        bl_font_data_vec.push_back(bl_font_data);
        // 加入face
        for(int i = 0; i < bl_font_data.faceCount(); i++) {
            BLFontFace ffc;
            stat = ffc.createFromData(bl_font_data, i);
            if(stat == BL_SUCCESS) {
                bl_font_face_vec.push_back(ffc);
                const BLString &name = ffc.familyName();
                bl_font_face_name.emplace_back(name.data(), name.size());
            }
        }
    }
    // Initialize GDI+.
}

// GDI+ 初期化
void deInitGdiPlus() {
    bl_font_data_vec.clear();
    bl_font_face_vec.clear();
    bl_font_face_name.clear();
}

/**
 * 画像読み込み処理
 * @param name ファイル名
 * @return 画像情報
 */
BLImage loadImage(const tjs_char *name) // 以后应该要让krkr内核进行图像解码
{
    BLImage image;
    BLResult ret = BL_SUCCESS;
    ttstr filename = TVPGetPlacedPath(name);
    if(filename.length()) {
        tTJSBinaryStream *in =
            TVPCreateBinaryStreamForRead(filename, TJS_W(""));
        if(in) {
            tjs_uint8 *fileData = new tjs_uint8[in->GetSize()];
            in->ReadBuffer(fileData, in->GetSize());
            ret = image.readFromData(fileData, in->GetSize());
            delete[] fileData;
            delete in;
        }
    }
    return image;
}

// --------------------------------------------------------
// フォント情報
// --------------------------------------------------------

/**
 * プライベートフォントの追加
 * @param fontFileName フォントファイル名
 */
void GdiPlus::addPrivateFont(const tjs_char *fontFileName) {
    ttstr filename = TVPGetPlacedPath(fontFileName);
    if(filename.length()) {
        tTJSBinaryStream *in =
            TVPCreateBinaryStreamForRead(filename, TJS_W(""));
        if(in) {
            tjs_uint8 *fileData = new tjs_uint8[in->GetSize()];
            in->ReadBuffer(fileData, in->GetSize());
            // 读取fontdata
            BLFontData bl_font_data;
            BLResult stat = bl_font_data.createFromData(fileData, in->GetSize(),
                                                        NULL, NULL);
            delete[] fileData;
            delete in;
            if(stat != BL_SUCCESS) {
                TVPThrowExceptionMessage(TJS_W("blend2d cannot load:%1"),
                                         fontFileName);
            }
            bl_font_data_vec.push_back(bl_font_data);
            // 加入face
            for(int i = 0; i < bl_font_data.faceCount(); i++) {
                BLFontFace ffc;
                stat = ffc.createFromData(bl_font_data, i);
                if(stat == BL_SUCCESS) {
                    bl_font_face_vec.push_back(ffc);
                    BLString name = ffc.familyName();
                    bl_font_face_name.emplace_back(name.data(), name.size());
                }
            }
            return;
        }
    }
    TVPThrowExceptionMessage(TJS_W("cannot open:%1"), fontFileName);
}

/**
 * フォント一覧の取得
 * @param privateOnly true ならプライベートフォントのみ取得
 */
tTJSVariant GdiPlus::getFontList(bool privateOnly) {
    iTJSDispatch2 *array = TJSCreateArrayObject();

    // 获取系统字体 直接用TVPFont的数据即可
    if(!privateOnly) {
        std::vector<ttstr> ret;
        TVPGetAllFontList(ret);
        for(auto ftN : ret) {
            if(!ftN.IsEmpty()) {
                tTJSVariant vname(ftN), *param = &vname;
                array->FuncCall(0, TJS_W("add"), NULL, 0, 1, &param, array);
            }
        }
    }

    // BLFontManager已保存字体
    for(const auto &i : bl_font_face_name) {
        if(i.length()) {
            tTJSVariant vname(i), *param = &vname;
            array->FuncCall(0, TJS_W("add"), NULL, 0, 1, &param, array);
        }
    }

    tTJSVariant ret(array, array);
    array->Release();
    return ret;
}

// --------------------------------------------------------
// フォント情報
// --------------------------------------------------------

/**
 * コンストラクタ
 */
FontInfo::FontInfo() :
    emSize(12), style(0), gdiPlusUnsupportedFont(false),
    forceSelfPathDraw(false), propertyModified(true) {}

/**
 * コンストラクタ
 * @param familyName フォントファミリー
 * @param emSize フォントのサイズ
 * @param style フォントスタイル
 */
FontInfo::FontInfo(const tjs_char *familyName, tjs_real emSize, tjs_int style) :
    gdiPlusUnsupportedFont(false), forceSelfPathDraw(false),
    propertyModified(true) {
    setFamilyName(familyName);
    setEmSize(emSize);
    setStyle(style);
}

/**
 * コピーコンストラクタ
 */
FontInfo::FontInfo(const FontInfo &orig) :
    emSize(orig.emSize), style(orig.style),
    gdiPlusUnsupportedFont(orig.gdiPlusUnsupportedFont),
    forceSelfPathDraw(orig.forceSelfPathDraw), propertyModified(true) {
    if(!orig.familyName.IsEmpty()) {
        setFamilyName(orig.familyName.c_str());
    }
}

/**
 * デストラクタ
 */
FontInfo::~FontInfo() { clear(); }

/**
 * フォント情報のクリア
 */
void FontInfo::clear() {
    FT_Done_Face(this->ftFace);
    this->ftFace = nullptr;
    //    this->fontFamily = nullptr;
    this->familyName = "";
    this->gdiPlusUnsupportedFont = false;
    this->propertyModified = true;
}


static bool hasBLFontFace(const ttstr &familyName) {
    for(const auto &name : bl_font_face_name) {
        if(name == familyName)
            return true;
    }
    return false;
}

// emSize は GDI+ の UnitPixel と同様にピクセル指定
// パス描画のみ僅かに縮小（clip 際のはみ出し対策。scaleFactor は 1/64 のまま）
static constexpr float kPathDrawEmSizeScale = 0.94f;

static void updateFtPixelSize(FT_Face face, tjs_real size) {
    if(!face)
        return;
    const FT_UInt pixelSize = static_cast<FT_UInt>(
        std::max(1.0, std::floor(size * kPathDrawEmSizeScale + 0.5)));
    FT_Set_Pixel_Sizes(face, 0, pixelSize);
}

void FontInfo::setEmSize(tjs_real size) {
    emSize = size;
    propertyModified = true;
    updateFtPixelSize(ftFace, emSize);
}

/**
 * フォントの指定
 */
void FontInfo::setFamilyName(const tjs_char *familyName) {
    if(forceSelfPathDraw) {
        propertyModified = true;
        clear();
        gdiPlusUnsupportedFont = true;
        if(familyName)
            this->familyName = familyName;
        if(!this->familyName.IsEmpty()) {
            const std::unique_ptr<tTJSBinaryStream> stream{ TVPCreateFontStream(
                this->familyName) };
            if(stream) {
                const auto bufferSize = static_cast<FT_Long>(stream->GetSize());
                buffer = std::make_unique<FT_Byte[]>(bufferSize);
                stream->ReadBuffer(buffer.get(), bufferSize);
                if(FT_New_Memory_Face(TVPGetFontLibrary(), buffer.get(),
                                      bufferSize, 0, &ftFace) == 0) {
                    updateFtPixelSize(ftFace, emSize);
                } else {
                    buffer.reset();
                    ftFace = nullptr;
                }
            }
        }
        return;
    }

    if(!familyName)
        return;
    if(this->familyName == familyName && (ftFace || !gdiPlusUnsupportedFont))
        return;

    propertyModified = true;
    clear();
    this->familyName = familyName;

    gdiPlusUnsupportedFont = !hasBLFontFace(this->familyName);

    const std::unique_ptr<tTJSBinaryStream> stream{ TVPCreateFontStream(
        this->familyName) };
    if(!stream)
        return;

    const auto bufferSize = static_cast<FT_Long>(stream->GetSize());
    buffer = std::make_unique<FT_Byte[]>(bufferSize);
    stream->ReadBuffer(buffer.get(), bufferSize);
    if(FT_New_Memory_Face(TVPGetFontLibrary(), buffer.get(), bufferSize, 0,
                          &ftFace) != 0) {
        buffer.reset();
        ftFace = nullptr;
        return;
    }
    updateFtPixelSize(ftFace, emSize);
}

void FontInfo::setForceSelfPathDraw(bool state) {
    forceSelfPathDraw = state;
    this->setFamilyName(familyName.c_str());
}

bool FontInfo::getForceSelfPathDraw() const { return forceSelfPathDraw; }

bool FontInfo::getSelfPathDraw() const {
    return forceSelfPathDraw || gdiPlusUnsupportedFont;
}

void FontInfo::updateSizeParams() const {
    if(!propertyModified)
        return;

    propertyModified = false;

    if(ftFace) {
        FT_Fixed scale = ftFace->size->metrics.y_scale;
        ascent = static_cast<float>(FT_MulFix(ftFace->ascender, scale)) / 64.0f;
        descent =
            static_cast<float>(FT_MulFix(ftFace->descender, scale)) / 64.0f;
        lineSpacing =
            static_cast<float>(FT_MulFix(ftFace->height, scale)) / 64.0f;
        ascentLeading = (lineSpacing - ascent) / 2.0f;
        descentLeading = -ascentLeading;
        return;
    }

    BLFont _font = getBLFont();
    if(!_font.empty()) {
        BLFontMetrics metrics = _font.metrics();
        ascent = metrics.ascent;
        descent = metrics.descent;
        // lineGap だけでは行高にならない（GDI+ の tmHeight / FT height に相当）
        lineSpacing = metrics.ascent + metrics.descent + metrics.lineGap;
        ascentLeading = (lineSpacing - ascent - descent) / 2.0f;
        descentLeading = -ascentLeading;
        return;
    }

    ascent = emSize * 0.8f;
    descent = emSize * 0.2f;
    lineSpacing = emSize * 1.2f;
    ascentLeading = 0;
    descentLeading = 0;
}


tjs_real FontInfo::getAscent() const {
    this->updateSizeParams();
    return ascent;
}


tjs_real FontInfo::getDescent() const {
    this->updateSizeParams();
    return descent;
}

tjs_real FontInfo::getAscentLeading() const {
    this->updateSizeParams();
    return ascentLeading;
}


tjs_real FontInfo::getDescentLeading() const {
    this->updateSizeParams();
    return descentLeading;
}

tjs_real FontInfo::getLineSpacing() const {
    this->updateSizeParams();
    return lineSpacing;
}

BLFont FontInfo::getBLFont() const {
    BLFont _font;
    BLFontFace _fontFace;
    for(size_t i = 0; i < bl_font_face_name.size(); i++) {
        if(bl_font_face_name.at(i) == familyName) {
            _fontFace = bl_font_face_vec.at(i);
            break;
        }
    }
    if(_fontFace.empty() && !bl_font_face_vec.empty()) {
        _fontFace = bl_font_face_vec[0];
    }
    if(_fontFace.empty())
        return _font;

    _font.createFromFace(_fontFace, emSize);
    return _font;
}


// --------------------------------------------------------
// アピアランス情報
// --------------------------------------------------------

Appearance::Appearance() {}

Appearance::~Appearance() { clear(); }
Appearance *Appearance::Clone() const {
    Appearance *newItm = new Appearance();
    for(auto itm : drawInfos) {
        DrawInfo newdraw(itm);
        newItm->drawInfos.push_back(newdraw);
    }
    return newItm;
}

/**
 * 情報のクリア
 */
void Appearance::clear() {
    drawInfos.clear();

    // customLineCapsも削除
    // std::vector<CustomLineCap*>::const_iterator i = customLineCaps.begin();
    // while (i != customLineCaps.end()) {
    //	delete *i;
    //	i++;
    //}
    // customLineCaps.clear();
}


// --------------------------------------------------------
// 各型変換処理
// --------------------------------------------------------

extern bool IsArray(const tTJSVariant &var);

/**
 * 座標情報の生成
 */
extern PointF getPoint(const tTJSVariant &var);

/**
 * 点の配列を取得
 */
static void getPoints(const tTJSVariant &var, std::vector<PointF> &points) {
    ncbPropAccessor info(var);
    int c = info.GetArrayCount();
    for(int i = 0; i < c; i++) {
        tTJSVariant p;
        if(info.checkVariant(i, p)) {
            points.push_back(getPoint(p));
        }
    }
}

static void getPoints(ncbPropAccessor &info, int n,
                      std::vector<PointF> &points) {
    tTJSVariant var;
    if(info.checkVariant(n, var)) {
        getPoints(var, points);
    }
}

static void getPoints(ncbPropAccessor &info, const tjs_char *n,
                      std::vector<PointF> &points) {
    tTJSVariant var;
    if(info.checkVariant(n, var)) {
        getPoints(var, points);
    }
}

// -----------------------------

/**
 * 矩形情報の生成
 */
static RectF getRect(const tTJSVariant &var);

/**
 * 矩形の配列を取得
 */
static void getRects(const tTJSVariant &var, std::vector<RectF> &rects) {
    ncbPropAccessor info(var);
    int c = info.GetArrayCount();
    for(int i = 0; i < c; i++) {
        tTJSVariant p;
        if(info.checkVariant(i, p)) {
            rects.push_back(getRect(p));
        }
    }
}

// -----------------------------

/**
 * 実数の配列を取得
 */
static void getReals(const tTJSVariant &var, std::vector<tjs_real> &points) {
    ncbPropAccessor info(var);
    int c = info.GetArrayCount();
    for(int i = 0; i < c; i++) {
        points.push_back((tjs_real)info.getRealValue(i));
    }
}

static void getReals(ncbPropAccessor &info, int n,
                     std::vector<tjs_real> &points) {
    tTJSVariant var;
    if(info.checkVariant(n, var)) {
        getReals(var, points);
    }
}

static void getReals(ncbPropAccessor &info, const tjs_char *n,
                     std::vector<tjs_real> &points) {
    tTJSVariant var;
    if(info.checkVariant(n, var)) {
        getReals(var, points);
    }
}

// -----------------------------

/**
 * 色の配列を取得
 */
static void getColors(const tTJSVariant &var, std::vector<tjs_uint32> &colors) {
    ncbPropAccessor info(var);
    int c = info.GetArrayCount();
    for(int i = 0; i < c; i++) {
        colors.push_back((tjs_uint32)info.getIntValue(i));
    }
}

static void getColors(ncbPropAccessor &info, int n,
                      std::vector<tjs_uint32> &colors) {
    tTJSVariant var;
    if(info.checkVariant(n, var)) {
        getColors(var, colors);
    }
}

static void getColors(ncbPropAccessor &info, const tjs_char *n,
                      std::vector<tjs_uint32> &colors) {
    tTJSVariant var;
    if(info.checkVariant(n, var)) {
        getColors(var, colors);
    }
}

static RectF calculateBounds(const std::vector<PointF> &points) {
    RectF bounds;
    if(!points.empty()) {
        tjs_real x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        for(size_t i = 1; i < points.size(); i++) {
            x0 = std::min(x0, points[i].x);
            y0 = std::min(y0, points[i].y);
            x1 = std::max(x1, points[i].x);
            y1 = std::max(y1, points[i].y);
        }
        bounds.reset(x0, y0, x1 - x0, y1 - y0);
    }
    return bounds;
}

template <class T>
void commonBrushParameter(ncbPropAccessor &info, T *brush) {
    tTJSVariant var;
    // SetBlend
    if(info.checkVariant(TJS_W("blend"), var)) {
        std::vector<tjs_real> factors;
        std::vector<tjs_real> positions;
        ncbPropAccessor binfo(var);
        if(IsArray(var)) {
            getReals(binfo, 0, factors);
            getReals(binfo, 1, positions);
        } else {
            getReals(binfo, TJS_W("blendFactors"), factors);
            getReals(binfo, TJS_W("blendPositions"), positions);
        }
        int count = (int)factors.size();
        if((int)positions.size() > count) {
            count = (int)positions.size();
        }
        if(count > 0) {
            brush->SetBlend(&factors[0], &positions[0], count);
        }
    }
    // SetBlendBellShape
    if(info.checkVariant(TJS_W("blendBellShape"), var)) {
        ncbPropAccessor sinfo(var);
        if(IsArray(var)) {
            brush->SetBlendBellShape((tjs_real)sinfo.getRealValue(0),
                                     (tjs_real)sinfo.getRealValue(1));
        } else {
            brush->SetBlendBellShape(
                (tjs_real)info.getRealValue(TJS_W("focus")),
                (tjs_real)info.getRealValue(TJS_W("scale")));
        }
    }
    // SetBlendTriangularShape
    if(info.checkVariant(TJS_W("blendTriangularShape"), var)) {
        ncbPropAccessor sinfo(var);
        if(IsArray(var)) {
            brush->SetBlendTriangularShape((tjs_real)sinfo.getRealValue(0),
                                           (tjs_real)sinfo.getRealValue(1));
        } else {
            brush->SetBlendTriangularShape(
                (tjs_real)info.getRealValue(TJS_W("focus")),
                (tjs_real)info.getRealValue(TJS_W("scale")));
        }
    }
    // SetGammaCorrection
    if(info.checkVariant(TJS_W("useGammaCorrection"), var)) {
        brush->SetGammaCorrection((bool)var);
    }
    // SetInterpolationColors
    if(info.checkVariant(TJS_W("interpolationColors"), var)) {
        std::vector<tjs_uint32> colors;
        std::vector<tjs_real> positions;
        ncbPropAccessor binfo(var);
        if(IsArray(var)) {
            getColors(binfo, 0, colors);
            getReals(binfo, 1, positions);
        } else {
            getColors(binfo, TJS_W("presetColors"), colors);
            getReals(binfo, TJS_W("blendPositions"), positions);
        }
        int count = (int)colors.size();
        if((int)positions.size() > count) {
            count = (int)positions.size();
        }
        if(count > 0) {
            brush->SetInterpolationColors(&colors[0], &positions[0], count);
        }
    }
}

/**
 * ブラシの生成
 */
BLImage createHatchPattern(HatchStyle style, BLRgba32 foreColor,
                           BLRgba32 backColor, int size = 8) {
    BLImage pattern(size, size, BL_FORMAT_PRGB32);
    BLContext ctx(pattern);

    ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    ctx.setFillStyle(backColor);
    ctx.fillAll();

    ctx.setCompOp(BL_COMP_OP_SRC_OVER);
    ctx.setFillStyle(foreColor);

    switch(style) {
        case HatchStyleHorizontal:
            ctx.fillRect(0, size / 2, size, 1);
            break;
        case HatchStyleVertical:
            ctx.fillRect(size / 2, 0, 1, size);
            break;
        case HatchStyleForwardDiagonal:
            for(int i = -size; i < size; i += 2) {
                ctx.fillTriangle(i, 0, i + 1, 0, i + size + 1, size);
            }
            break;
        case HatchStyleBackwardDiagonal:
            for(int i = -size; i < size; i += 2) {
                ctx.fillTriangle(i, size, i + 1, size, i + size + 1, 0);
            }
            break;
        case HatchStyleCross:
            ctx.fillRect(0, size / 2, size, 1);
            ctx.fillRect(size / 2, 0, 1, size);
            break;
        case HatchStyleDiagonalCross:
            for(int i = -size; i < size; i += 2) {
                ctx.fillTriangle(i, 0, i + 1, 0, i + size + 1, size);
                ctx.fillTriangle(i, size, i + 1, size, i + size + 1, 0);
            }
            break;
        case HatchStyle05Percent:
            for(int y = 0; y < size; y += 20) {
                for(int x = 0; x < size; x += 20) {
                    ctx.fillRect(x, y, 1, 1);
                }
            }
            break;
        case HatchStyle50Percent:
            for(int y = 0; y < size; y += 2) {
                for(int x = y % 2; x < size; x += 2) {
                    ctx.fillRect(x, y, 1, 1);
                }
            }
            break;
        case HatchStyleSmallGrid:
            for(int i = 0; i < size; i += 2) {
                ctx.fillRect(0, i, size, 1);
                ctx.fillRect(i, 0, 1, size);
            }
            break;
        case HatchStyleWeave:
            for(int i = 0; i < size; i += 4) {
                ctx.fillRect(i, 0, 2, size);
                ctx.fillRect(0, i, size, 2);
            }
            break;
        case HatchStyleZigZag:
            for(int i = 0; i < size; i += 4) {
                ctx.fillRect(0, i, size, 1);
                ctx.fillRect(0, i + 2, size, 1);
            }
            break;
        case HatchStyleDottedGrid:
            for(int y = 0; y < size; y += 4) {
                for(int x = 0; x < size; x += 4) {
                    ctx.fillCircle(x, y, 0.5);
                }
            }
            break;
        default:
            for(int i = -size; i < size; i += 2) {
                ctx.fillTriangle(i, 0, i + 1, 0, i + size + 1, size);
            }
            break;
    }
    ctx.end();
    return pattern;
}
BLBrush *createBrush(const tTJSVariant colorOrBrush) {
    if(colorOrBrush.Type() != tvtObject) {
        // 纯色
        return new BLBrush((tjs_int)colorOrBrush);
    } else {
        // 種別ごとに作り分ける
        ncbPropAccessor info(colorOrBrush);
        BrushType type =
            (BrushType)info.getIntValue(TJS_W("type"), BrushTypeSolidColor);
        switch(type) {
            case BrushTypeSolidColor:
                return new BLBrush(
                    info.getIntValue(TJS_W("color"), 0xFFFFFFFF));
            case BrushTypeHatchFill: {
                HatchStyle hatchStyle = (HatchStyle)info.getIntValue(
                    TJS_W("hatchStyle"), HatchStyleHorizontal);
                BLRgba32 foreColor((tjs_uint32)info.getIntValue(
                    TJS_W("foreColor"), 0xFFFFFFFF));
                BLRgba32 backColor((tjs_uint32)info.getIntValue(
                    TJS_W("backColor"), 0xFF000000));
                return new BLBrush(
                    createHatchPattern(hatchStyle, foreColor, backColor));
            }
            case BrushTypeTextureFill: {
                ttstr imgname =
                    info.GetValue(TJS_W("image"), ncbTypedefs::Tag<ttstr>());
                BLImage image = loadImage(imgname.c_str());

                if(!image.empty()) {
                    BLPattern pattern(image);

                    WrapMode wrapMode = (WrapMode)info.getIntValue(
                        TJS_W("wrapMode"), WrapModeTile);
                    switch(wrapMode) {
                        case WrapModeTile:
                            pattern.setExtendMode(BL_EXTEND_MODE_REPEAT);
                            break;
                        case WrapModeTileFlipX:
                            pattern.setExtendMode(
                                BL_EXTEND_MODE_REFLECT_X_REPEAT_Y);
                            break;
                        case WrapModeTileFlipY:
                            pattern.setExtendMode(
                                BL_EXTEND_MODE_REPEAT_X_REFLECT_Y);
                            break;
                        case WrapModeTileFlipXY:
                            pattern.setExtendMode(BL_EXTEND_MODE_REFLECT);
                            break;
                        case WrapModeClamp:
                            pattern.setExtendMode(BL_EXTEND_MODE_PAD);
                            break;
                        default:
                            pattern.setExtendMode(BL_EXTEND_MODE_REPEAT);
                            break;
                    }

                    tTJSVariant dstRect;
                    if(info.checkVariant(TJS_W("dstRect"), dstRect)) {
                        RectF dstRect = getRect(&dstRect);
                        if(dstRect.x != 0 || dstRect.y != 0 ||
                           dstRect.w != image.width() ||
                           dstRect.h != image.height()) {

                            BLMatrix2D matrix;
                            matrix.reset();
                            matrix.scale(dstRect.w / image.width(),
                                         dstRect.h / image.height());
                            matrix.translate(dstRect.x, dstRect.y);
                            pattern.applyTransform(matrix);
                        }
                    }
                    return new BLBrush(pattern);
                }
                break;
            }
            case BrushTypePathGradient: {
                BLGradient gradient(BL_GRADIENT_TYPE_RADIAL);
                std::vector<PointF> points;
                getPoints(info, TJS_W("points"), points);
                if((int)points.size() == 0)
                    TVPThrowExceptionMessage(TJS_W("must set poins"));

                // TODO
                // WrapMode wrapMode = (WrapMode)info.getIntValue(L"wrapMode",
                // WrapModeTile);

                // 共通パラメータ TODO
                // commonBrushParameter(info, pbrush);

                if(!points.empty()) {
                    RectF bounds = calculateBounds(points);
                    float cx = bounds.x + bounds.w / 2;
                    float cy = bounds.y + bounds.h / 2;
                    float radius = std::max(bounds.w, bounds.h) / 2;

                    gradient.setValues(BLConicGradientValues(cx, cy, radius));

                    tTJSVariant var;
                    // SetCenterColor
                    if(info.checkVariant(TJS_W("centerColor"), var)) {
                        gradient.addStop(0.0,
                                         BLRgba32((tjs_uint32)(tjs_int)var));
                    }
                    // SetCenterPoint
                    if(info.checkVariant(TJS_W("centerPoint"), var)) {
                        // TODO
                    }
                    // SetSurroundColors
                    if(info.checkVariant(TJS_W("surroundColors"), var)) {
                        std::vector<tjs_uint32> colors;
                        getColors(var, colors);
                        if(!colors.empty()) {
                            gradient.addStop(1.0, BLRgba32(colors[0]));
                        }
                    }
                    // SetFocusScales
                    if(info.checkVariant(TJS_W("focusScales"), var)) {
                        // TODO
                    }
                    return new BLBrush(gradient);
                }
                break;
            }
            case BrushTypeLinearGradient: {
                BLGradient gradient(BL_GRADIENT_TYPE_LINEAR);

                tTJSVariant var;
                if(info.checkVariant(TJS_W("point1"), var) &&
                   info.checkVariant(TJS_W("point2"), var)) {
                    PointF p1 = getPoint(var);
                    info.checkVariant(TJS_W("point2"), var);
                    PointF p2 = getPoint(var);
                    gradient.setValues(
                        BLLinearGradientValues(p1.x, p1.y, p2.x, p2.y));
                } else if(info.checkVariant(TJS_W("rect"), var)) {
                    RectF rect = getRect(var);
                    float angle = info.getRealValue(TJS_W("angle"), 0.0f);

                    float rad = angle * M_PI / 180.0f;
                    float cx = rect.x + rect.w / 2;
                    float cy = rect.y + rect.h / 2;
                    float length =
                        std::sqrt(rect.w * rect.w + rect.h * rect.h) / 2;

                    gradient.setValues(
                        BLLinearGradientValues(cx - std::cos(rad) * length,
                                               cy - std::sin(rad) * length,
                                               cx + std::cos(rad) * length,
                                               cy + std::sin(rad) * length));
                } else {
                    TVPThrowExceptionMessage(
                        TJS_W("must set point1,2 or rect"));
                }

                gradient.addStop(0.0,
                                 BLRgba32((tjs_uint32)(tjs_int)info.getIntValue(
                                     TJS_W("color1"), 0)));
                gradient.addStop(1.0,
                                 BLRgba32((tjs_uint32)(tjs_int)info.getIntValue(
                                     TJS_W("color2"), 0)));

                // 共通パラメータ TODO
                // commonBrushParameter(info, pbrush);

                return new BLBrush(gradient);
            }
            default:
                TVPThrowExceptionMessage(TJS_W("invalid brush type"));
                break;
        }
    }
    return new BLBrush();
}

/**
 * ブラシの追加
 * @param colorOrBrush ARGB色指定またはブラシ情報（辞書）
 * @param ox 表示オフセットX
 * @param oy 表示オフセットY
 */
void Appearance::addBrush(tTJSVariant colorOrBrush, tjs_real ox, tjs_real oy) {
    drawInfos.push_back(DrawInfo(ox, oy, createBrush(colorOrBrush), 1));
}

/**
 * ペンの追加
 * @param colorOrBrush ARGB色指定またはブラシ情報（辞書）
 * @param widthOrOption ペン幅またはペン情報（辞書）
 * @param ox 表示オフセットX
 * @param oy 表示オフセットY
 */
void Appearance::addPen(tTJSVariant colorOrBrush, tTJSVariant widthOrOption,
                        tjs_real ox, tjs_real oy) {
    BLPen *pen = nullptr;
    tjs_real width = 1.0;
    if(colorOrBrush.Type() == tvtObject) {
        BLBrush *brush = createBrush(colorOrBrush);
        pen = new BLPen(brush, width);
        delete brush;
    } else {
        pen = new BLPen((tjs_uint32)(tjs_int)colorOrBrush, width);
    }
    if(widthOrOption.Type() != tvtObject) {
        pen->strokeWidth = ((tjs_real)(tjs_real)widthOrOption);
    } else {
        ncbPropAccessor info(widthOrOption);
        tjs_real penWidth = 1.0;
        tTJSVariant var;

        // SetWidth
        if(info.checkVariant(TJS_W("width"), var)) {
            penWidth = (tjs_real)(tjs_real)var;
        }
        pen->strokeWidth = penWidth;

        // SetAlignment
        if(info.checkVariant(TJS_W("alignment"), var)) {
            // TODO
        }
        // SetCompoundArray
        if(info.checkVariant(TJS_W("compoundArray"), var)) {
            // TODO
        }

        // SetDashCap
        if(info.checkVariant(TJS_W("dashCap"), var)) {
            // TODO
        }
        // SetDashOffset
        if(info.checkVariant(TJS_W("dashOffset"), var)) {
            pen->strokeOptions.dashOffset = (tjs_real)(tjs_real)var;
        }

        // SetDashStyle
        // SetDashPattern
        if(info.checkVariant(TJS_W("dashStyle"), var)) {
            if(IsArray(var)) {
                std::vector<tjs_real> reals;
                getReals(var, reals);
                BLArray<double> bla;
                for(size_t i = 0; i < reals.size(); i++) {
                    bla.append(reals.at(i));
                }
                pen->strokeOptions.dashArray = bla;
            } else {
                DashStyle dashStyle = (DashStyle)(tjs_int)var;
                BLArray<double> bla;
                switch(dashStyle) {

                    case DashStyleSolid:
                        break;
                    case DashStyleDash:
                        bla.append(5.0f);
                        bla.append(3.0f);
                        break;
                    case DashStyleDot:
                        bla.append(1.0f);
                        bla.append(3.0f);
                        break;
                    case DashStyleDashDot:
                        bla.append(5.0f);
                        bla.append(3.0f);
                        bla.append(1.0f);
                        bla.append(3.0f);
                        break;
                    case DashStyleDashDotDot:
                        bla.append(5.0f);
                        bla.append(3.0f);
                        bla.append(1.0f);
                        bla.append(3.0f);
                        bla.append(1.0f);
                        bla.append(3.0f);
                        break;
                }
                pen->strokeOptions.dashArray = bla;
            }
        }

        // SetStartCap
        // SetCustomStartCap
        if(info.checkVariant(TJS_W("startCap"), var)) {
            BLStrokeCap retCap;
            BLPath custom;
            if(getLineCap(var, retCap, custom, penWidth)) {
                if(custom.empty())
                    pen->strokeOptions.startCap = retCap;
                else {
                    pen->startCapType = 1;
                    pen->startCap = custom;
                }
            }
        }

        // SetEndCap
        // SetCustomEndCap
        if(info.checkVariant(TJS_W("endCap"), var)) {
            BLStrokeCap retCap;
            BLPath custom;
            if(getLineCap(var, retCap, custom, penWidth)) {
                if(custom.empty())
                    pen->strokeOptions.endCap = retCap;
                else {
                    pen->endCapType = 1;
                    pen->endCap = custom;
                }
            }
        }

        // SetLineJoin
        if(info.checkVariant(TJS_W("lineJoin"), var)) {
            LineJoin lineJoin = (LineJoin)(tjs_int)var;
            switch(lineJoin) {
                case LineJoinMiter: {
                    pen->strokeOptions.join = BL_STROKE_JOIN_MITER_BEVEL;
                    break;
                }
                case LineJoinBevel: {
                    pen->strokeOptions.join = BL_STROKE_JOIN_BEVEL;
                    break;
                }
                case LineJoinRound: {
                    pen->strokeOptions.join = BL_STROKE_JOIN_ROUND;
                    break;
                }
                case LineJoinMiterClipped: {
                    pen->strokeOptions.join = BL_STROKE_JOIN_MITER_CLIP;
                    break;
                }
                default:
                    pen->strokeOptions.join = BL_STROKE_JOIN_MITER_BEVEL;
            }
            pen->strokeOptions.join = (LineJoin)(tjs_int)var;
        }

        // SetMiterLimit
        if(info.checkVariant(TJS_W("miterLimit"), var)) {
            pen->strokeOptions.miterLimit = (tjs_real)(tjs_real)var;
        }
    }
    drawInfos.push_back(DrawInfo(ox, oy, pen, 0));
}

bool Appearance::getLineCap(tTJSVariant &in, BLStrokeCap &cap, BLPath &custom,
                            tjs_real pw) {
    switch(in.Type()) {
        case tvtVoid:
        case tvtInteger: {
            LineCap lcap = (LineCap)(tjs_int)in;
            switch(lcap) {
                case LineCapFlat: {
                    cap = BL_STROKE_CAP_BUTT;
                    break;
                }
                case LineCapRound: {
                    cap = BL_STROKE_CAP_ROUND;
                    break;
                }
                case LineCapSquare: {
                    cap = BL_STROKE_CAP_SQUARE;
                    break;
                }
                case LineCapTriangle: {
                    cap = BL_STROKE_CAP_TRIANGLE;
                    break;
                }
                default: {
                    cap = BL_STROKE_CAP_BUTT;
                    break;
                }
            }
            break;
        }
        case tvtObject: {
            ncbPropAccessor info(in);
            tjs_real width = pw, height = pw;
            tTJSVariant var;
            if(info.checkVariant(TJS_W("width"), var))
                width = ((tjs_real)(tjs_real)var) * pw;
            if(info.checkVariant(TJS_W("height"), var))
                height = ((tjs_real)(tjs_real)var) * pw;
            bool filled = (bool)info.getIntValue(TJS_W("filled"), 1);
            tjs_real middleInset = 0;
            if(info.checkVariant(TJS_W("middleInset"), var)) // TODO
                middleInset = (tjs_real)(tjs_real)var;

            if(filled) {
                custom.moveTo(-width / 2, -height);
                custom.lineTo(0, 0);
                custom.lineTo(width / 2, -height);
                custom.close();
            } else {
                custom.moveTo(-width / 2, -height);
                custom.lineTo(0, 0);
                custom.lineTo(width / 2, -height);
            }
        } break;
        default:
            return false;
    }
    return true;
}

Path::Path() {}

Path::~Path() {}

/**
 * 現在の図形を閉じずに次の図形を開始します
 */
void Path::startFigure() { figureStarted = true; }

/**
 * 現在の図形を閉じます
 */
void Path::closeFigure() { path.close(); }

/**
 * 円弧の描画
 * @param x 左上座標
 * @param y 左上座標
 * @param width 横幅
 * @param height 縦幅
 * @param startAngle 時計方向円弧開始位置
 * @param sweepAngle 描画角度
 */
void Path::drawArc(tjs_real x, tjs_real y, tjs_real width, tjs_real height,
                   tjs_real startAngle, tjs_real sweepAngle) {
    float startRad = startAngle * M_PI / 180.0f;
    float sweepRad = sweepAngle * M_PI / 180.0f;
    float rx = width / 2.0f;
    float ry = height / 2.0f;
    float cx = x + rx;
    float cy = y + ry;
    path.arcTo(cx, cy, rx, ry, startRad, sweepRad);
}

/**
 * ベジェ曲線の描画
 * @param app アピアランス
 * @param x1
 * @param y1
 * @param x2
 * @param y2
 * @param x3
 * @param y3
 * @param x4
 * @param y4
 */
void Path::drawBezier(tjs_real x1, tjs_real y1, tjs_real x2, tjs_real y2,
                      tjs_real x3, tjs_real y3, tjs_real x4, tjs_real y4) {
    if(!figureStarted) {
        path.moveTo(x1, y1);
        figureStarted = true;
    }
    path.cubicTo(x2, y2, x3, y3, x4, y4);
}

/**
 * 連続ベジェ曲線の描画
 * @param app アピアランス
 * @param points 点の配列
 */
void Path::drawBeziers(tTJSVariant points) {
    std::vector<PointF> ps;
    getPoints(points, ps);

    if(ps.size() < 4 || (ps.size() - 1) % 3 != 0)
        return;

    if(!figureStarted && !ps.empty()) {
        path.moveTo(ps[0].x, ps[0].y);
        figureStarted = true;
    }

    for(size_t i = 1; i < ps.size(); i += 3) {
        if(i + 2 < ps.size()) {
            path.cubicTo(ps[i].x, ps[i].y, ps[i + 1].x, ps[i + 1].y,
                         ps[i + 2].x, ps[i + 2].y);
        }
    }
}

/**
 * Closed cardinal spline の描画
 * @param app アピアランス
 * @param points 点の配列
 */
void Path::drawClosedCurve(tTJSVariant points) {
    drawClosedCurve2(points, 0.5);
}

/**
 * Closed cardinal spline の描画
 * @param app アピアランス
 * @param points 点の配列
 * @pram tension tension
 */
void Path::drawClosedCurve2(tTJSVariant points, tjs_real tension) {
    std::vector<PointF> ps;
    getPoints(points, ps);
    if(ps.size() < 2)
        return;

    // 计算Cardinal spline控制点
    for(size_t i = 0; i < ps.size(); i++) {
        PointF p0 = ps[(i + ps.size() - 1) % ps.size()];
        PointF p1 = ps[i];
        PointF p2 = ps[(i + 1) % ps.size()];
        PointF p3 = ps[(i + 2) % ps.size()];

        // Cardinal spline公式
        PointF cp1 = p1 + (p2 - p0) * tension / 3.0;
        PointF cp2 = p2 - (p3 - p1) * tension / 3.0;

        if(i == 0) {
            path.moveTo(p1.x, p1.y);
            figureStarted = true;
        }
        path.cubicTo(cp1.x, cp1.y, cp2.x, cp2.y, p2.x, p2.y);
    }

    path.close();
}

/**
 * cardinal spline の描画
 * @param app アピアランス
 * @param points 点の配列
 */
void Path::drawCurve(tTJSVariant points) { drawCurve3(points, 0.5f, 0, -1); }

/**
 * cardinal spline の描画
 * @param app アピアランス
 * @param points 点の配列
 * @parma tension tension
 */
void Path::drawCurve2(tTJSVariant points, tjs_real tension) {
    drawCurve3(points, tension, 0, -1);
}

/**
 * cardinal spline の描画
 * @param app アピアランス
 * @param points 点の配列
 * @param offset
 * @param numberOfSegments
 * @param tension tension
 */
void Path::drawCurve3(tTJSVariant points, int offset, int numberOfSegments,
                      tjs_real tension) {
    std::vector<PointF> ps;
    getPoints(points, ps);
    if(ps.size() < 2)
        return;

    if(numberOfSegments < 0)
        numberOfSegments = (int)ps.size() - 1;
    if(offset < 0 || offset + numberOfSegments >= (int)ps.size())
        return;

    if(!figureStarted && offset < ps.size()) {
        path.moveTo(ps[offset].x, ps[offset].y);
        figureStarted = true;
    }

    for(int i = offset; i < offset + numberOfSegments; i++) {
        PointF p0 = (i > 0) ? ps[i - 1] : ps[i];
        PointF p1 = ps[i];
        PointF p2 = ps[i + 1];
        PointF p3 = (i + 2 < ps.size()) ? ps[i + 2] : ps[i + 1];

        PointF cp1 = p1 + (p2 - p0) * tension / 3.0f;
        PointF cp2 = p2 - (p3 - p1) * tension / 3.0f;

        path.cubicTo(cp1.x, cp1.y, cp2.x, cp2.y, p2.x, p2.y);
    }
}

/**
 * 円錐の描画
 * @param x 左上座標
 * @param y 左上座標
 * @param width 横幅
 * @param height 縦幅
 * @param startAngle 時計方向円弧開始位置
 * @param sweepAngle 描画角度
 */
void Path::drawPie(tjs_real x, tjs_real y, tjs_real width, tjs_real height,
                   tjs_real startAngle, tjs_real sweepAngle) {
    float rx = width / 2.0f;
    float ry = height / 2.0f;
    float cx = x + rx;
    float cy = y + ry;

    float startRad = startAngle * M_PI / 180.0f;
    float sweepRad = sweepAngle * M_PI / 180.0f;
    float endRad = startRad + sweepRad;

    // 移动到中心
    path.moveTo(cx, cy);
    figureStarted = true;

    // 画到起始点
    float startX = cx + rx * cos(startRad);
    float startY = cy + ry * sin(startRad);
    path.lineTo(startX, startY);

    // 画圆弧
    path.arcTo(cx, cy, rx, ry, startRad, sweepRad);

    // 回到中心并闭合
    path.lineTo(cx, cy);
    path.close();
}

/**
 * 楕円の描画
 * @param app アピアランス
 * @param x
 * @param y
 * @param width
 * @param height
 */
void Path::drawEllipse(tjs_real x, tjs_real y, tjs_real width,
                       tjs_real height) {
    float cx = x + width / 2.0f;
    float cy = y + height / 2.0f;
    float rx = width / 2.0f;
    float ry = height / 2.0f;

    BLEllipse ellipse(cx, cy, rx, ry);
    path.addEllipse(ellipse);
    figureStarted = false; // 椭圆是独立图形
}

/**
 * 線分の描画
 * @param app アピアランス
 * @param x1 始点X座標
 * @param y1 始点Y座標
 * @param x2 終点X座標
 * @param y2 終点Y座標
 */
void Path::drawLine(tjs_real x1, tjs_real y1, tjs_real x2, tjs_real y2) {
    if(!figureStarted) {
        path.moveTo(x1, y1);
        figureStarted = true;
    } else {
        path.lineTo(x1, y1);
    }
    path.lineTo(x2, y2);
}

/**
 * 連続線分の描画
 * @param app アピアランス
 * @param points 点の配列
 */
void Path::drawLines(tTJSVariant points) {
    std::vector<PointF> ps;
    getPoints(points, ps);
    if(ps.empty())
        return;

    if(!figureStarted) {
        path.moveTo(ps[0].x, ps[0].y);
        figureStarted = true;
    }

    for(size_t i = 1; i < ps.size(); i++) {
        path.lineTo(ps[i].x, ps[i].y);
    }
}

/**
 * 多角形の描画
 * @param app アピアランス
 * @param points 点の配列

 */
void Path::drawPolygon(tTJSVariant points) {
    std::vector<PointF> ps;
    getPoints(points, ps);
    if(ps.empty())
        return;

    path.moveTo(ps[0].x, ps[0].y);
    figureStarted = true;

    for(size_t i = 1; i < ps.size(); i++) {
        path.lineTo(ps[i].x, ps[i].y);
    }

    path.close();
}

/**
 * 矩形の描画
 * @param app アピアランス
 * @param x
 * @param y
 * @param width
 * @param height
 */
void Path::drawRectangle(tjs_real x, tjs_real y, tjs_real width,
                         tjs_real height) {
    BLRect rect(x, y, width, height);
    path.addRect(rect, BL_GEOMETRY_DIRECTION_CW);
    figureStarted = false;
}

/**
 * 複数矩形の描画
 * @param app アピアランス
 * @param rects 矩形情報の配列
 */
void Path::drawRectangles(tTJSVariant rects) {
    std::vector<RectF> rs;
    getRects(rects, rs);

    for(const auto &rect : rs) {
        path.addRect(BLRect(rect.x, rect.y, rect.w, rect.h),
                     BL_GEOMETRY_DIRECTION_CW);
    }
    figureStarted = false;
}

// --------------------------------------------------------
// フォント描画系
// --------------------------------------------------------

void LayerExDraw::updateRect(RectF &rect) {
    if(updateWhenDraw) {
        // 更新処理
        tTVPRect rc(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
        _this->Update(rc);
    }
}

/**
 * コンストラクタ
 */
LayerExDraw::LayerExDraw(DispatchT obj) :
    layerExBase_GL(obj), width(-1), height(-1), pitch(0), buffer(NULL),
    bitmap(NULL), context(NULL), metaGraphics(NULL), clipLeft(-1), clipTop(-1),
    clipWidth(-1), clipHeight(-1), smoothingMode(SmoothingModeAntiAlias),
    textRenderingHint(TextRenderingHintAntiAlias), updateWhenDraw(true) {
    viewTransform.reset();
    transform.reset();
}

/**
 * デストラクタ
 */
LayerExDraw::~LayerExDraw() {
    destroyRecord();
    if(context)
        delete context;
    if(bitmap)
        delete bitmap;
}

namespace {

    // Layer の clip は整数境界 [left,right)×[top,bottom)。字形は float
    // で僅かにはみ出す。
    static constexpr int kLayerClipInkPad = 3;

    static BLRect makePaddedClipRect(int clipLeft, int clipTop, int clipWidth,
                                     int clipHeight, int bmpW, int bmpH) {
        if(clipWidth < 0 || clipHeight < 0)
            return BLRect(0, 0, bmpW, bmpH);

        const int l = std::max(0, clipLeft - kLayerClipInkPad);
        const int t = std::max(0, clipTop - kLayerClipInkPad);
        const int r = std::min(bmpW, clipLeft + clipWidth + kLayerClipInkPad);
        const int b = std::min(bmpH, clipTop + clipHeight + kLayerClipInkPad);
        return BLRect(l, t, std::max(0, r - l), std::max(0, b - t));
    }

} // namespace

void LayerExDraw::reset() {
    layerExBase_GL::reset();
    if(!(context && width == _width && height == _height && pitch == _pitch &&
         buffer == _buffer)) {
        if(context)
            delete context;
        if(bitmap)
            delete bitmap;
        width = _width;
        height = _height;
        pitch = _pitch;
        buffer = _buffer;
        bitmap = new BLImage;
        bitmap->createFromData(width, height, BL_FORMAT_PRGB32, buffer, pitch);
        context = new BLContext;
        context->setCompOp(BL_COMP_OP_SRC_OVER);
        updateTransform();

        clipWidth = clipHeight = -1;
    }
    if(_clipLeft != clipLeft || _clipTop != clipTop ||
       _clipWidth != clipWidth || _clipHeight != clipHeight) {
        clipLeft = _clipLeft;
        clipTop = _clipTop;
        clipWidth = _clipWidth;
        clipHeight = _clipHeight;
        const BLRect clipRect = makePaddedClipRect(clipLeft, clipTop, clipWidth,
                                                   clipHeight, width, height);
        context->begin(*bitmap);
        context->restoreClipping();
        context->clipToRect(clipRect);
        context->end();
    }
}

void LayerExDraw::updateViewTransform() {
    calcTransform.reset();
    calcTransform.transform(transform);
    calcTransform.transform(viewTransform);
    redrawRecord();
}

/**
 * 表示トランスフォームの指定
 * @param matrix トランスフォームマトリックス
 */
void LayerExDraw::setViewTransform(GdipMatrix *trans) {
    if(!viewTransform.equals(trans->_core)) {
        viewTransform.reset();
        viewTransform.transform(trans->_core);
        updateViewTransform();
    }
}

void LayerExDraw::resetViewTransform() {
    viewTransform.reset();
    updateViewTransform();
}

void LayerExDraw::rotateViewTransform(tjs_real angle) {
    viewTransform = BLMatrix2D::makeRotation(angle);
    updateViewTransform();
}

void LayerExDraw::scaleViewTransform(tjs_real sx, tjs_real sy) {
    viewTransform = BLMatrix2D::makeScaling(sx, sy);
    updateViewTransform();
}

void LayerExDraw::translateViewTransform(tjs_real dx, tjs_real dy) {
    viewTransform = BLMatrix2D::makeTranslation(dx, dy);
    updateViewTransform();
}

void LayerExDraw::updateTransform() {
    calcTransform.reset();
    calcTransform.transform(transform);
    calcTransform.transform(viewTransform);
}

/**
 * トランスフォームの指定
 * @param matrix トランスフォームマトリックス
 */
void LayerExDraw::setTransform(GdipMatrix *trans) {
    if(!transform.equals(trans->_core)) {
        transform.reset();
        transform.transform(trans->_core);
        updateTransform();
    }
}

void LayerExDraw::resetTransform() {
    transform.reset();
    updateTransform();
}

void LayerExDraw::rotateTransform(tjs_real angle) {
    transform = BLMatrix2D::makeRotation(angle);
    updateTransform();
}

void LayerExDraw::scaleTransform(tjs_real sx, tjs_real sy) {
    transform = BLMatrix2D::makeScaling(sx, sy);
    updateTransform();
}

void LayerExDraw::translateTransform(tjs_real dx, tjs_real dy) {
    transform = BLMatrix2D::makeTranslation(dx, dy);
    updateTransform();
}

/**
 * 画面の消去
 * @param argb 消去色
 */
void LayerExDraw::clear(tjs_uint32 argb) {
    context->begin(*bitmap);
    context->setFillStyle(BLRgba32(argb));
    context->fillAll();
    context->end();
    if(metaGraphics) {
        createRecord();
        metaGraphics->bgColor = BLRgba32(argb);
    }
    _this->Update();
}


/**
 * パスの描画
 * @param app アピアランス
 * @param path パス
 */
RectF LayerExDraw::drawPath(const Appearance *app, const Path *path) {
    return _drawPath(app, &path->path);
}

static RectF transformRect(const BLMatrix2D &matrix, const RectF &rect) {
    // 获取矩形的四个角点
    BLPoint corners[4] = { BLPoint(rect.x, rect.y),
                           BLPoint(rect.x + rect.w, rect.y),
                           BLPoint(rect.x, rect.y + rect.h),
                           BLPoint(rect.x + rect.w, rect.y + rect.h) };

    // 变换所有点
    for(int i = 0; i < 4; i++) {
        matrix.mapPoint(corners[i]);
    }

    // 计算变换后的边界
    tjs_real minX = corners[0].x;
    tjs_real maxX = corners[0].x;
    tjs_real minY = corners[0].y;
    tjs_real maxY = corners[0].y;

    for(int i = 1; i < 4; i++) {
        if(corners[i].x < minX)
            minX = corners[i].x;
        if(corners[i].x > maxX)
            maxX = corners[i].x;
        if(corners[i].y < minY)
            minY = corners[i].y;
        if(corners[i].y > maxY)
            maxY = corners[i].y;
    }

    return RectF(minX, minY, maxX - minX, maxY - minY);
}

/**
 * パスの領域情報を取得
 * @param app 表示表現
 * @param path 描画するパス
 */
RectF LayerExDraw::getPathExtents(const Appearance *app, const BLPath *path) {
    // TODO
    return RectF();
}

void LayerExDraw::draw(BLImage *ctx, const BLPen *pen, const BLMatrix2D *matrix,
                       const BLPath *path) {
    if(!context || !ctx || !pen)
        return;
    // 开始绘制
    context->begin(*ctx);
    // 设置矩阵
    context->setTransform(calcTransform);
    if(matrix) {
        context->applyTransform(*matrix);
    }
    // 设置描边选项
    // context->setStrokeOptions(pen->strokeOptions);
    context->setStrokeWidth(pen->strokeWidth);
    // 设置描边样式
    if(pen->brush) // TODO 不知道有啥用
        ;
    else
        context->setStrokeStyle(pen->color);
    // 描边
    context->strokePath(*path);
    // 是否有自定义键帽
    if(pen->startCapType == 1) // TODO 第一个点得不到啊！！！
        ;
    if(pen->endCapType == 1) {
        BLPoint pt;
        path->getLastVertex(&pt);
        BLMatrix2D endPose = BLMatrix2D::makeTranslation(pt.x, pt.y);
        context->applyTransform(endPose);
        context->strokePath(pen->endCap);
    }

    // 结束
    context->end();
}

void LayerExDraw::fill(BLImage *ctx, const BLBrush *brush,
                       const BLMatrix2D *matrix, const BLPath *path) {
    if(!context || !ctx || !brush)
        return;
    // 开始绘制
    context->begin(*bitmap);
    // 设置矩阵
    context->setTransform(calcTransform);
    if(matrix)
        context->applyTransform(*matrix);
    // 设置填充类型
    switch(brush->type) {
        case BrushTypeSolidColor:
            context->setFillStyle(brush->solidBrush);
            break;
        case BrushTypeHatchFill:
        case BrushTypeTextureFill:
            context->setFillStyle(brush->textureBrush);
            break;
        case BrushTypePathGradient:
        case BrushTypeLinearGradient:
            context->setFillStyle(brush->pathGradientBrush);
            break;
        default:
            context->setFillStyle(BLRgba32(0, 0, 0, 255));
            break;
    }
    // 填充
    context->fillPath(*path);
    // 结束
    context->end();
}

/**
 * パスを描画する
 * @param app 表示表現
 * @param path 描画するパス
 * @return 更新領域情報
 */
RectF LayerExDraw::_drawPath(const Appearance *app, const BLPath *path) {
    RectF bounds;
    if(!context || !app || !path || path->empty())
        return bounds;

    for(const auto &drawInfo : app->drawInfos) {
        if(!drawInfo.info)
            continue;

        if(metaGraphics) {
            GdipImage::Info info;
            info.app = app->Clone();
            info.path = new BLPath;
            info.path->assign(*path);
            metaGraphics->vectorGraph.push_back(info);
            metaGraphics->transMtx = transform;
        }

        BLMatrix2D drawMatrix =
            BLMatrix2D::makeTranslation(drawInfo.ox, drawInfo.oy);

        if(drawInfo.type == 0) {
            BLPen *pen = static_cast<BLPen *>(drawInfo.info);
            draw(bitmap, pen, &drawMatrix, path);
        } else {
            BLBrush *brush = static_cast<BLBrush *>(drawInfo.info);
            fill(bitmap, brush, &drawMatrix, path);
        }
    }
    // cv::Mat rgba(height, width, CV_8UC4, buffer, pitch);
    // std::string title("afterImg");
    // title.append(cvName);
    // cv::imshow(title, rgba);

    updateRect(bounds);
    return bounds;
}

/**
 * 円弧の描画
 * @param x 左上座標
 * @param y 左上座標
 * @param width 横幅
 * @param height 縦幅
 * @param startAngle 時計方向円弧開始位置
 * @param sweepAngle 描画角度
 * @return 更新領域情報
 */
RectF LayerExDraw::drawArc(const Appearance *app, tjs_real x, tjs_real y,
                           tjs_real width, tjs_real height, tjs_real startAngle,
                           tjs_real sweepAngle) {
    BLPath path;
    float rx = width / 2.0f;
    float ry = height / 2.0f;
    float cx = x + rx;
    float cy = y + ry;
    float startRad = startAngle * M_PI / 180.0f;
    float sweepRad = sweepAngle * M_PI / 180.0f;

    path.moveTo(cx + rx * cos(startRad), cy + ry * sin(startRad));
    path.arcTo(cx, cy, rx, ry, startRad, sweepRad);

    return _drawPath(app, &path);
}

/**
 * ベジェ曲線の描画
 * @param app アピアランス
 * @param x1
 * @param y1
 * @param x2
 * @param y2
 * @param x3
 * @param y3
 * @param x4
 * @param y4
 * @return 更新領域情報
 */
RectF LayerExDraw::drawBezier(const Appearance *app, tjs_real x1, tjs_real y1,
                              tjs_real x2, tjs_real y2, tjs_real x3,
                              tjs_real y3, tjs_real x4, tjs_real y4) {
    BLPath path;
    path.moveTo(x1, y1);
    path.cubicTo(x2, y2, x3, y3, x4, y4);

    return _drawPath(app, &path);
}

/**
 * 連続ベジェ曲線の描画
 * @param app アピアランス
 * @param points 点の配列
 * @return 更新領域情報
 */
RectF LayerExDraw::drawBeziers(const Appearance *app, tTJSVariant points) {
    std::vector<PointF> ps;
    getPoints(points, ps);

    if(ps.size() < 4 || (ps.size() - 1) % 3 != 0)
        return RectF();

    BLPath path;
    path.moveTo(ps[0].x, ps[0].y);

    for(size_t i = 1; i < ps.size(); i += 3) {
        if(i + 2 < ps.size()) {
            path.cubicTo(ps[i].x, ps[i].y, ps[i + 1].x, ps[i + 1].y,
                         ps[i + 2].x, ps[i + 2].y);
        }
    }

    return _drawPath(app, &path);
}

/**
 * Closed cardinal spline の描画
 * @param app アピアランス
 * @param points 点の配列
 * @return 更新領域情報
 */
RectF LayerExDraw::drawClosedCurve(const Appearance *app, tTJSVariant points) {
    return drawClosedCurve2(app, points, 0.5f);
}

/**
 * Closed cardinal spline の描画
 * @param app アピアランス
 * @param points 点の配列
 * @pram tension tension
 * @return 更新領域情報
 */
RectF LayerExDraw::drawClosedCurve2(const Appearance *app, tTJSVariant points,
                                    tjs_real tension) {
    std::vector<PointF> ps;
    getPoints(points, ps);

    if(ps.size() < 2)
        return RectF();

    BLPath path;
    for(size_t i = 0; i < ps.size(); i++) {
        PointF p0 = ps[(i + ps.size() - 1) % ps.size()];
        PointF p1 = ps[i];
        PointF p2 = ps[(i + 1) % ps.size()];
        PointF p3 = ps[(i + 2) % ps.size()];

        PointF cp1 = p1 + (p2 - p0) * tension / 3.0f;
        PointF cp2 = p2 - (p3 - p1) * tension / 3.0f;

        if(i == 0) {
            path.moveTo(p1.x, p1.y);
        }
        path.cubicTo(cp1.x, cp1.y, cp2.x, cp2.y, p2.x, p2.y);
    }
    path.close();

    return _drawPath(app, &path);
}

/**
 * cardinal spline の描画
 * @param app アピアランス
 * @param points 点の配列
 * @return 更新領域情報
 */
RectF LayerExDraw::drawCurve(const Appearance *app, tTJSVariant points) {
    return drawCurve3(app, points, 0, -1, 0.5f);
}

/**
 * cardinal spline の描画
 * @param app アピアランス
 * @param points 点の配列
 * @parma tension tension
 * @return 更新領域情報
 */
RectF LayerExDraw::drawCurve2(const Appearance *app, tTJSVariant points,
                              tjs_real tension) {
    return drawCurve3(app, points, 0, -1, tension);
}

/**
 * cardinal spline の描画
 * @param app アピアランス
 * @param points 点の配列
 * @param offset
 * @param numberOfSegments
 * @param tension tension
 * @return 更新領域情報
 */
RectF LayerExDraw::drawCurve3(const Appearance *app, tTJSVariant points,
                              int offset, int numberOfSegments,
                              tjs_real tension) {
    std::vector<PointF> ps;
    getPoints(points, ps);

    if(ps.size() < 2)
        return RectF();

    if(numberOfSegments < 0)
        numberOfSegments = static_cast<int>(ps.size()) - 1;

    if(offset < 0 || offset + numberOfSegments >= static_cast<int>(ps.size()))
        return RectF();

    BLPath path;
    path.moveTo(ps[offset].x, ps[offset].y);

    for(int i = offset; i < offset + numberOfSegments; i++) {
        PointF p0 = (i > 0) ? ps[i - 1] : ps[i];
        PointF p1 = ps[i];
        PointF p2 = ps[i + 1];
        PointF p3 = (i + 2 < ps.size()) ? ps[i + 2] : ps[i + 1];

        PointF cp1 = p1 + (p2 - p0) * tension / 3.0f;
        PointF cp2 = p2 - (p3 - p1) * tension / 3.0f;

        path.cubicTo(cp1.x, cp1.y, cp2.x, cp2.y, p2.x, p2.y);
    }

    return _drawPath(app, &path);
}

/**
 * 円錐の描画
 * @param x 左上座標
 * @param y 左上座標
 * @param width 横幅
 * @param height 縦幅
 * @param startAngle 時計方向円弧開始位置
 * @param sweepAngle 描画角度
 * @return 更新領域情報
 */
RectF LayerExDraw::drawPie(const Appearance *app, tjs_real x, tjs_real y,
                           tjs_real width, tjs_real height, tjs_real startAngle,
                           tjs_real sweepAngle) {
    BLPath path;
    float rx = width / 2.0f;
    float ry = height / 2.0f;
    float cx = x + rx;
    float cy = y + ry;
    float startRad = startAngle * M_PI / 180.0f;
    float sweepRad = sweepAngle * M_PI / 180.0f;

    path.moveTo(cx, cy);
    path.lineTo(cx + rx * cos(startRad), cy + ry * sin(startRad));
    path.arcTo(cx, cy, rx, ry, startRad, sweepRad);
    path.close();

    return _drawPath(app, &path);
}

/**
 * 楕円の描画
 * @param app アピアランス
 * @param x
 * @param y
 * @param width
 * @param height
 * @return 更新領域情報
 */
RectF LayerExDraw::drawEllipse(const Appearance *app, tjs_real x, tjs_real y,
                               tjs_real width, tjs_real height) {
    BLPath path;
    BLEllipse ellipse(x + width / 2, y + height / 2, width / 2, height / 2);
    path.addEllipse(ellipse);

    return _drawPath(app, &path);
}

/**
 * 線分の描画
 * @param app アピアランス
 * @param x1 始点X座標
 * @param y1 始点Y座標
 * @param x2 終点X座標
 * @param y2 終点Y座標
 * @return 更新領域情報
 */
RectF LayerExDraw::drawLine(const Appearance *app, tjs_real x1, tjs_real y1,
                            tjs_real x2, tjs_real y2) {
    BLPath path;
    path.moveTo(x1, y1);
    path.lineTo(x2, y2);

    return _drawPath(app, &path);
}

/**
 * 連続線分の描画
 * @param app アピアランス
 * @param points 点の配列
 * @return 更新領域情報
 */
RectF LayerExDraw::drawLines(const Appearance *app, tTJSVariant points) {
    std::vector<PointF> ps;
    getPoints(points, ps);

    if(ps.empty())
        return RectF();

    BLPath path;
    path.moveTo(ps[0].x, ps[0].y);

    for(size_t i = 1; i < ps.size(); i++) {
        path.lineTo(ps[i].x, ps[i].y);
    }

    return _drawPath(app, &path);
}

/**
 * 多角形の描画
 * @param app アピアランス
 * @param points 点の配列
 * @return 更新領域情報
 */
RectF LayerExDraw::drawPolygon(const Appearance *app, tTJSVariant points) {
    std::vector<PointF> ps;
    getPoints(points, ps);

    if(ps.empty())
        return RectF();

    BLPath path;
    path.moveTo(ps[0].x, ps[0].y);

    for(size_t i = 1; i < ps.size(); i++) {
        path.lineTo(ps[i].x, ps[i].y);
    }
    path.close();

    return _drawPath(app, &path);
}

/**
 * 矩形の描画
 * @param app アピアランス
 * @param x
 * @param y
 * @param width
 * @param height
 * @return 更新領域情報
 */
RectF LayerExDraw::drawRectangle(const Appearance *app, tjs_real x, tjs_real y,
                                 tjs_real width, tjs_real height) {
    BLPath path;
    path.addRect(BLRect(x, y, width, height));

    return _drawPath(app, &path);
}

/**
 * 複数矩形の描画
 * @param app アピアランス
 * @param rects 矩形情報の配列
 * @return 更新領域情報
 */
RectF LayerExDraw::drawRectangles(const Appearance *app, tTJSVariant rects) {
    std::vector<RectF> rs;
    getRects(rects, rs);

    BLPath path;
    for(const auto &rect : rs) {
        path.addRect(BLRect(rect.x, rect.y, rect.w, rect.h));
    }

    return _drawPath(app, &path);
}

/**
 * 文字列のパスベースでの描画
 * @param font フォント
 * @param app アピアランス
 * @param x 描画位置X
 * @param y 描画位置Y
 * @param text 描画テキスト
 * @return 更新領域情報
 */
RectF LayerExDraw::drawPathString(const FontInfo *font, const Appearance *app,
                                  tjs_real x, tjs_real y,
                                  const tjs_char *text) {
    // general 版は常に drawPathString2（FreeType パス）を使用
    return drawPathString2(font, app, x, y, text);
}

/**
 * 文字列の描画
 * @param font フォント
 * @param app アピアランス（ブラシのみ参照されます）
 * @param x 描画位置X
 * @param y 描画位置Y
 * @param text 描画テキスト
 * @return 更新領域情報
 */
RectF LayerExDraw::drawString(const FontInfo *font, const Appearance *app,
                              tjs_real x, tjs_real y, const tjs_char *text) {
    if(font->getSelfPathDraw())
        return drawPathString2(font, app, x, y, text);

    RectF bounds;
    BLFont blFont = font->getBLFont();
    assert(!blFont.empty());
    // 开始
    context->begin(*bitmap);
    // 设置矩阵
    context->setTransform(calcTransform);
    // 绘制字体
    for(const auto &drawInfo : app->drawInfos) {
        if(!drawInfo.info)
            continue;

        if(metaGraphics) {
            // unsupport
        }

        if(drawInfo.type == 1) {
            // 只考虑笔刷
            BLBrush *brush = static_cast<BLBrush *>(drawInfo.info);
            if(!brush)
                continue;
            // Blend2D はベースライン座標。GDI+ と同じく y を上端とみなす。
            BLPoint position =
                BLPoint(x + drawInfo.ox, y + drawInfo.oy + font->getAscent());
            // 设置填充类型
            switch(brush->type) {
                case BrushTypeSolidColor:
                    context->setFillStyle(brush->solidBrush);
                    break;
                case BrushTypeHatchFill:
                case BrushTypeTextureFill:
                    context->setFillStyle(brush->textureBrush);
                    break;
                case BrushTypePathGradient:
                case BrushTypeLinearGradient:
                    context->setFillStyle(brush->pathGradientBrush);
                    break;
                default:
                    context->setFillStyle(BLRgba32(0, 0, 0, 255));
                    break;
            }
            // 绘制
            context->fillUtf16Text(position, blFont,
                                   reinterpret_cast<const uint16_t *>(text),
                                   TJS_strlen(text));
        }
    }
    context->end();

    updateRect(bounds);
    return bounds;
}

/**
 * 文字列の描画領域情報の取得
 * @param font フォント
 * @param text 描画テキスト
 * @return 描画領域情報
 */
RectF LayerExDraw::measureString(const FontInfo *font, const tjs_char *text) {
    if(font->getSelfPathDraw() || font->ftFace)
        return measureString2(font, text);

    BLFont blFont = font->getBLFont();
    if(blFont.empty())
        return RectF();

    BLTextMetrics tm;
    BLGlyphBuffer gb;
    gb.setUtf16Text(reinterpret_cast<const uint16_t *>(text), TJS_strlen(text));
    blFont.shape(gb);
    blFont.getTextMetrics(gb, tm);

    // boundingBox はベースライン原点。上端を y=0 に正規化して GDI+ と揃える。
    const float top = static_cast<float>(tm.boundingBox.y0);
    const float h = static_cast<float>(tm.boundingBox.y1 - tm.boundingBox.y0);
    const float w = static_cast<float>(tm.boundingBox.x1 - tm.boundingBox.x0);
    return RectF(static_cast<float>(tm.boundingBox.x0), top, w, h);
}

/**
 * 文字列に外接する領域情報の取得
 * @param font フォント
 * @param text 描画テキスト
 * @return 領域情報の辞書 left, top, width, height
 */
RectF LayerExDraw::measureStringInternal(const FontInfo *font,
                                         const tjs_char *text) {
    if(font->getSelfPathDraw() || font->ftFace)
        return measureStringInternal2(font, text);

    return measureString(font, text);
}

/**
 * 画像の描画。コピー先は元画像の Bounds を配慮した位置、サイズは Pixel
 * 指定になります。
 * @param x コピー先原点
 * @param y  コピー先原点
 * @param src コピー元画像
 * @return 更新領域情報
 */
RectF LayerExDraw::drawImage(tjs_real x, tjs_real y, GdipImage *src) {
    return drawImageRect(x, y, src, 0, 0, src->_core.width(),
                         src->_core.height());
}

/**
 * 画像の矩形コピー
 * @param dleft コピー先左端
 * @param dtop  コピー先上端
 * @param src コピー元画像
 * @param sleft 元矩形の左端
 * @param stop  元矩形の上端
 * @param swidth 元矩形の横幅
 * @param sheight  元矩形の縦幅
 * @return 更新領域情報
 */
RectF LayerExDraw::drawImageRect(tjs_real dleft, tjs_real dtop, GdipImage *src,
                                 tjs_real sleft, tjs_real stop, tjs_real swidth,
                                 tjs_real sheight) {
    return drawImageAffine(src, sleft, stop, swidth, sheight, true, 1, 0, 0, 1,
                           dleft, dtop);
}

/**
 * 画像の拡大縮小コピー
 * @param dleft コピー先左端
 * @param dtop  コピー先上端
 * @param dwidth コピー先の横幅
 * @param dheight  コピー先の縦幅
 * @param src コピー元画像
 * @param sleft 元矩形の左端
 * @param stop  元矩形の上端
 * @param swidth 元矩形の横幅
 * @param sheight  元矩形の縦幅
 * @return 更新領域情報
 */
RectF LayerExDraw::drawImageStretch(tjs_real dleft, tjs_real dtop,
                                    tjs_real dwidth, tjs_real dheight,
                                    GdipImage *src, tjs_real sleft,
                                    tjs_real stop, tjs_real swidth,
                                    tjs_real sheight) {
    return drawImageAffine(src, sleft, stop, swidth, sheight, true,
                           dwidth / swidth, 0, 0, dheight / sheight, dleft,
                           dtop);
}

/**
 * 画像のアフィン変換コピー
 * @param sleft 元矩形の左端
 * @param stop  元矩形の上端
 * @param swidth 元矩形の横幅
 * @param sheight  元矩形の縦幅
 * @param affine アフィンパラメータの種類(true:変換行列, false:座標指定),
 * @return 更新領域情報
 */
RectF LayerExDraw::drawImageAffine(GdipImage *src, tjs_real sleft,
                                   tjs_real stop, tjs_real swidth,
                                   tjs_real sheight, bool affine, tjs_real A,
                                   tjs_real B, tjs_real C, tjs_real D,
                                   tjs_real E, tjs_real F) {
    RectF bounds;

    BLMatrix2D matrix;
    if(affine) {
        matrix.reset(A, B, C, D, E, F);
    } else {
        tjs_real a = C - A;
        tjs_real b = D - B;
        tjs_real c = E - A;
        tjs_real d = F - B;
        tjs_real e = A;
        tjs_real f = B;
        matrix.reset(a, b, c, d, e, f);
    }

    if(src->type == 0) // 常规图像
    {
        if(!src->_core || src->_core.empty())
            return RectF();

        BLRectI srcRect(sleft, stop, swidth, sheight);
        context->begin(*bitmap);
        context->setTransform(matrix);
        context->blitImage(BLPoint(0, 0), src->_core, srcRect);
        context->end();
        if(metaGraphics) {
            // unsupport
            // 下次换一个位图系统吧，buffer图还是不太行
        }
    } else if(src->type == 1) {
        BLMatrix2D savedMtx = calcTransform;
        calcTransform = matrix;
        BLMatrix2D srcRectTransform = BLMatrix2D::makeIdentity();
        srcRectTransform.scale(swidth / src->GetWidth(),
                               sheight / src->GetHeight());
        srcRectTransform.translate(-sleft, -stop);
        calcTransform.transform(srcRectTransform);
        calcTransform.transform(src->transMtx);
        for(auto infoItm : src->vectorGraph) {
            if(!context || !infoItm.app || !infoItm.path ||
               infoItm.path->empty())
                continue;

            for(const auto &drawInfo : infoItm.app->drawInfos) {
                if(!drawInfo.info)
                    continue;

                BLMatrix2D drawMatrix =
                    BLMatrix2D::makeTranslation(drawInfo.ox, drawInfo.oy);
                if(drawInfo.type == 0) {
                    BLPen *pen = static_cast<BLPen *>(drawInfo.info);
                    draw(bitmap, pen, &drawMatrix, infoItm.path);
                } else {
                    BLBrush *brush = static_cast<BLBrush *>(drawInfo.info);
                    fill(bitmap, brush, &drawMatrix, infoItm.path);
                }
            }
        }
        calcTransform = savedMtx;
    }

    updateRect(bounds);
    return bounds;
}

void LayerExDraw::createRecord() {
    destroyRecord();
    if(!metaGraphics) {
        metaGraphics = new GdipImage(width, height);
    }
}

/**
 * 記録内容の現在の解像度での再描画
 */
bool LayerExDraw::redrawRecord() {
    // 再描画処理
    GdipImage *image = getRecordImage();
    if(image) {
        delete image;
        return true;
    }
    return false;
}
/**
 * デストラクタ
 */
void LayerExDraw::destroyRecord() {
    if(metaGraphics) {
        delete metaGraphics;
        metaGraphics = NULL;
    }
}


/**
 * @param record 描画内容を記録するかどうか
 */
void LayerExDraw::setRecord(bool record) {
    if(record) {
        if(!metaGraphics) {
            createRecord();
        }
    } else {
        if(metaGraphics) {
            destroyRecord();
        }
    }
}

bool LayerExDraw::redraw(GdipImage *image) {
    if(image) {
        RectF bounds = image->GetBounds();
        BLRect ret(bounds.x, bounds.y, bounds.w, bounds.h);
        if(metaGraphics) {
            // unsupport
        }
        context->begin(*bitmap);
        context->setFillStyle(BLRgba32(0x0));
        context->clearAll();
        context->setTransform(viewTransform);
        context->blitImage(ret, image->_core);
        context->setTransform(calcTransform);
        context->end();
        _this->Update();
        return true;
    }
    return false;
}

/**
 * 記録内容を Image として取得
 * @return 成功したら true
 */
GdipImage *LayerExDraw::getRecordImage() {
    GdipImage *image = NULL;
    if(metaGraphics) {
        image = new GdipImage(*metaGraphics);
        if(image) {
            redraw(image);
        }
    }
    return image;
}

/**
 * 記録内容の保存
 * @param filename 保存ファイル名
 * @return 成功したら true
 */
bool LayerExDraw::saveRecord(const tjs_char *filename) {
    bool ret = false;
    if(bitmap) {
        BLArray<uint8_t> bmpdata;
        BLImageCodec codc;
        ttstr ext = TVPExtractStorageExt(filename);
        if(codc.findByExtension(ext.AsStdString().c_str()) == BL_SUCCESS ||
           codc.findByExtension("bmp") == BL_SUCCESS) {
            bitmap->writeToData(bmpdata, codc);
            tTJSBinaryStream *out = TVPCreateBinaryStreamForWrite(filename, "");
            if(out) {
                out->WriteBuffer(bmpdata.data(), bmpdata.size());
                delete out;
            }
        }
        // 再描画処理
        GdipImage *image = getRecordImage();
        if(image) {
            delete image;
        }
    }
    return ret;
}


/**
 * 記録内容の読み込み
 * @param filename 読み込みファイル名
 * @return 成功したら true
 */
bool LayerExDraw::loadRecord(const tjs_char *filename) {
    BLImage image;
    if(filename && ((image = loadImage(filename)))) {
        createRecord();
        GdipImage gdip_image{ image };
        redraw(&gdip_image);
    }
    return false;
}

/**
 * グリフアウトラインの取得
 * @param font フォント
 * @param offset オフセット
 * @param path グリフを書き出すパス
 * @param glyph 描画するグリフ
 */
void LayerExDraw::getGlyphOutline(const FontInfo *fontInfo, PointF &offset,
                                  BLPath *path, tjs_uint charcode) {
    if(!fontInfo->ftFace)
        return;

    if(charcode == '\n' || charcode == '\r' || charcode == '\t')
        return;

    // 加载字形
    FT_UInt glyphIndex = FT_Get_Char_Index(fontInfo->ftFace, charcode);
    if(glyphIndex == 0) {
        if(charcode < 0x20)
            return;
        // 不支持此字符
        G_PluginLog.error("not find Unicode >> {} << in FontFamily",
                          ttstr{ (tjs_char)charcode }.AsNarrowStdString());
    }

    FT_Int32 flags = FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP;
    if(FT_Load_Glyph(fontInfo->ftFace, glyphIndex, flags) != 0) {
        // 字形加载失败
        G_PluginLog.error("FT Load Glyph Failed!");
        return;
    }

    // 获取字形度量
    FT_GlyphSlot glyph = fontInfo->ftFace->glyph;

    // 字形格式检查
    if(glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
        // 非矢量字形，无法处理
        G_PluginLog.error("Not Vector Fonts Can't resolve!");
        return;
    }

    static constexpr float scaleFactor = 1.0f / 64.0f;

    BLPoint glyphOffset{ offset.x, offset.y + fontInfo->getAscent() };

    FT_Outline outline = glyph->outline;

    const auto getPoint = [&](int index) -> BLPoint {
        return { outline.points[index].x * scaleFactor + glyphOffset.x,
                 -outline.points[index].y * scaleFactor + glyphOffset.y };
    };

    // 获取二次贝塞尔曲线结束点的逻辑
    std::function getConicEndPoint = [&](int contourStart, int index,
                                         int contourEnd) -> BLPoint {
        int nextIndex = index == contourEnd ? contourStart : index + 1;
        FT_Byte nextTag = FT_CURVE_TAG(outline.tags[nextIndex]);

        if(nextTag == FT_CURVE_TAG_ON) {
            return getPoint(nextIndex);
        }

        // 如果下一个点还是 Control 点，结束点就是两者的中点
        BLPoint pCurrent = getPoint(index);
        BLPoint pNext = getPoint(nextIndex);
        return { (pCurrent.x + pNext.x) / 2.0f, (pCurrent.y + pNext.y) / 2.0f };
    };

    // Full Definition:
    // http://freetype.org/freetype2/docs/glyphs/glyphs-6.html
    for(int i = 0; i < outline.n_contours; ++i) {
        int contourStart = (i == 0) ? 0 : outline.contours[i - 1] + 1;
        int contourEnd = outline.contours[i];

        if(contourStart == contourEnd)
            continue;

        FT_Byte contourStartTag = FT_CURVE_TAG(outline.tags[contourStart]);
        BLPoint contourStartP = getPoint(contourStart);

        // 如果起点是一个二次控制点（Conic），FreeType
        // 的规则规定实际起点是轮廓最后一点与第一点的中点
        // 为了安全起见，这里先初始化 moveTo。实际后续逻辑中会进一步处理。
        path->moveTo(contourStartP.x, contourStartP.y);

        FT_Byte prevTag{}, currentTag{};
        BLPoint prevP{}, currentP{};

        for(int j = contourStart + 1; j <= contourEnd; ++j) {
            prevTag = FT_CURVE_TAG(outline.tags[j - 1]);
            currentTag = FT_CURVE_TAG(outline.tags[j]);

            prevP = getPoint(j - 1);
            currentP = getPoint(j);

            // 1. 三次贝塞尔曲线 (Cubic Bezier)
            if(prevTag == FT_CURVE_TAG_CUBIC && currentTag == FT_CURVE_TAG_ON) {
                BLPoint startP = getPoint(j - 3);
                BLPoint control = getPoint(j - 2);
                path->cubicTo(control.x, control.y, prevP.x, prevP.y,
                              currentP.x, currentP.y);
                continue;
            }

            // 2. 直线 (Line)
            if(prevTag == FT_CURVE_TAG_ON && currentTag == FT_CURVE_TAG_ON) {
                path->lineTo(currentP.x, currentP.y);
                continue;
            }

            // 3. 正常的二次贝塞尔曲线：从 ON 到 CONIC (控制点)
            if(prevTag == FT_CURVE_TAG_ON && currentTag == FT_CURVE_TAG_CONIC) {
                BLPoint endP = getConicEndPoint(contourStart, j, contourEnd);
                path->quadTo(currentP.x, currentP.y, endP.x, endP.y);
                continue;
            }

            // 4. 连续的二次贝塞尔控制点：CONIC 到 CONIC
            if(prevTag == FT_CURVE_TAG_CONIC &&
               currentTag == FT_CURVE_TAG_CONIC) {
                BLPoint midStartP = { (prevP.x + currentP.x) / 2.0f,
                                      (prevP.y + currentP.y) / 2.0f };
                BLPoint endP = getConicEndPoint(contourStart, j, contourEnd);
                path->quadTo(currentP.x, currentP.y, endP.x, endP.y);
            }
        }

        // --- 闭合多边形 (Outline Close) 逻辑处理 ---
        if(currentTag == FT_CURVE_TAG_ON &&
           contourStartTag == FT_CURVE_TAG_ON) {
            path->lineTo(contourStartP.x, contourStartP.y);
        }

        if(currentTag == FT_CURVE_TAG_ON &&
           contourStartTag == FT_CURVE_TAG_CONIC) {
            BLPoint endP =
                getConicEndPoint(contourStart, contourStart, contourEnd);
            path->quadTo(contourStartP.x, contourStartP.y, endP.x, endP.y);
        }

        if(currentTag == FT_CURVE_TAG_CONIC &&
           contourStartTag == FT_CURVE_TAG_ON) {
            path->quadTo(currentP.x, currentP.y, contourStartP.x,
                         contourStartP.y);
        }

        if(currentTag == FT_CURVE_TAG_CUBIC) {
            BLPoint startP = getPoint(contourEnd - 2);
            path->cubicTo(prevP.x, prevP.y, currentP.x, currentP.y,
                          contourStartP.x, contourStartP.y);
        }

        // 对应 GdipClosePathFigure(path);
        path->close();
    }

    offset.x += static_cast<float>(glyph->metrics.horiAdvance) * scaleFactor;
}

/*
 * テキストアウトラインの取得
 * @param font フォント
 * @param offset オフセット
 * @param path グリフを書き出すパス
 * @param text 描画するテキスト
 */
void LayerExDraw::getTextOutline(const FontInfo *fontInfo, PointF &offset,
                                 BLPath *path, ttstr text) {
    if(text.IsEmpty())
        return;

    const float lineStartX = offset.x;
    const float lineStep = static_cast<float>(fontInfo->getLineSpacing());

    for(tjs_int i = 0; i < text.GetLen(); i++) {
        const tjs_char ch = text[i];
        if(ch == '\n') {
            offset.x = lineStartX;
            offset.y += lineStep;
            continue;
        }
        if(ch == '\r') {
            if(i + 1 < text.GetLen() && text[i + 1] == '\n')
                continue;
            offset.x = lineStartX;
            offset.y += lineStep;
            continue;
        }
        this->getGlyphOutline(fontInfo, offset, path, ch);
    }
}

namespace {

    int countTextLines(const tjs_char *text) {
        if(!text || !*text)
            return 1;

        int lines = 1;
        for(tjs_int i = 0; text[i]; ++i) {
            if(text[i] == '\n')
                lines++;
            else if(text[i] == '\r' && text[i + 1] != '\n')
                lines++;
        }
        return lines;
    }

    static bool getPathInkBox(const BLPath &path, BLBox *box) {
        if(path.getControlBox(box) == BL_SUCCESS && box->y1 > box->y0)
            return true;
        return path.getBoundingBox(box) == BL_SUCCESS && box->y1 > box->y0;
    }

    // measureString2 / drawPathString2 共通:
    // パス実寸＋ディセンダント余白で行高を決める
    float calcPathTextHeight(const FontInfo *font, const BLPath &path,
                             bool useMeasurePadding, int lineCount) {
        lineCount = std::max(1, lineCount);
        const float lineH = static_cast<float>(font->getLineSpacing());
        const float lineMinH = useMeasurePadding ? lineH * 1.124f : lineH;
        const float minH = lineMinH * static_cast<float>(lineCount);

        BLBox box{};
        if(!getPathInkBox(path, &box))
            return minH;

        const float pathH = static_cast<float>(box.y1 - box.y0);
        constexpr float kInkMargin = 2.0f;
        if(!useMeasurePadding)
            return std::max(minH, pathH + kInkMargin);

        const float descentPad = std::fabs(font->getDescent());
        return std::max(minH, pathH + descentPad + kInkMargin + 1.0f);
    }

} // namespace

/**
 * 文字列の描画更新領域情報の取得(OpenTypeフォント対応)
 * @param font フォント
 * @param text 描画テキスト
 * @return 更新領域情報の辞書 left, top, width, height
 */
RectF LayerExDraw::measureString2(const FontInfo *font, const tjs_char *text) {
    // 文字列のパスを準備
    BLPath path{};
    PointF offset{ 0.0f, 0.0f };
    this->getTextOutline(font, offset, &path, text);

    BLBox box{};
    const BLResult blr = path.getBoundingBox(&box);

    RectF result{};

    if(blr == BL_SUCCESS) {
        // GDI+ のコードは元の result.Width（外接矩形の幅）に加算しています
        float pathWidth = static_cast<float>(box.x1 - box.x0);
        result.w = pathWidth + static_cast<float>(0.167f * font->emSize * 2.0f);
    } else {
        // パスが空などの場合は幅0
        result.w = static_cast<float>(0.167f * font->emSize * 2.0f);
    }

    result.x = 0.0f;
    result.y = 0.0f;
    result.h = calcPathTextHeight(font, path, true, countTextLines(text));

    return result;
}

/**
 * 文字列に外接する領域情報の取得(OpenTypeのPostScriptフォント対応)
 * @param font フォント
 * @param text 描画テキスト
 * @return 更新領域情報の辞書 left, top, width, height
 */
RectF LayerExDraw::measureStringInternal2(const FontInfo *font,
                                          const tjs_char *text) {
    // 文字列のパスを準備
    BLPath path{};
    PointF offset{ 0.0f, 0.0f };
    this->getTextOutline(font, offset, &path, text);

    BLBox box{};
    const BLResult blr = path.getBoundingBox(&box);

    RectF result{};

    if(blr == BL_SUCCESS) {
        // GDI+版と同様、X と Height
        // のみを上書きし、Width（w）はパスの幅をそのまま使用
        result.w = static_cast<float>(box.x1 - box.x0);
    } else {
        result.w = 0.0f;
    }

    // GDI+版の「REAL(LONG(0.167 * font->emSize))」を再現
    result.x = static_cast<float>(static_cast<long>(0.167f * font->emSize));
    result.y = 0.0f;
    result.h = calcPathTextHeight(font, path, false, countTextLines(text));

    return result;
}

/**
 * 文字列の描画(OpenTypeフォント対応)
 * @param font フォント
 * @param app アピアランス
 * @param x 描画位置X
 * @param y 描画位置Y
 * @param text 描画テキスト
 * @return 更新領域情報
 */
RectF LayerExDraw::drawPathString2(const FontInfo *font, const Appearance *app,
                                   tjs_real x, tjs_real y,
                                   const tjs_char *text) {
    BLPath path{};

    // Windows / general 版と同じ座標微調整（LONG キャストで一致）
    PointF offset{
        x + static_cast<float>(static_cast<long>(0.167 * font->emSize)) - 0.5f,
        y - 0.5f
    };
    this->getTextOutline(font, offset, &path, text);

    RectF result = _drawPath(app, &path);

    result.x = x;
    result.y = y;
    result.w += static_cast<float>(0.167f * font->emSize * 2.0f);
    result.h = calcPathTextHeight(font, path, true, countTextLines(text));

    return result;
}

// 両方自前コンバータ
#define NCB_SET_CONVERTOR_BOTH(type, convertor)                                \
    NCB_TYPECONV_SRCMAP_SET(type, convertor<type>, true);                      \
    NCB_TYPECONV_DSTMAP_SET(type, convertor<type>, true)

// SRCだけ自前コンバータ
#define NCB_SET_CONVERTOR_SRC(type, convertor)                                 \
    NCB_TYPECONV_SRCMAP_SET(type, convertor<type>, true);                      \
    NCB_TYPECONV_DSTMAP_SET(type, ncbNativeObjectBoxing::Unboxing, true)

// DSTだけ自前コンバータ
#define NCB_SET_CONVERTOR_DST(type, convertor)                                 \
    NCB_TYPECONV_SRCMAP_SET(type, ncbNativeObjectBoxing::Boxing, true);        \
    NCB_TYPECONV_DSTMAP_SET(type, convertor<type>, true)

/**
 * 配列かどうかの判定
 * @param var VARIANT
 * @return 配列なら true
 */
bool IsArray(const tTJSVariant &var) {
    if(var.Type() == tvtObject) {
        iTJSDispatch2 *obj = var.AsObjectNoAddRef();
        return obj->IsInstanceOf(0, NULL, NULL, TJS_W("Array"), obj) ==
            TJS_S_TRUE;
    }
    return false;
}

// メンバ変数をプロパティとして登録
#define NCB_MEMBER_PROPERTY(name, type, membername)                            \
    struct AutoProp_##name {                                                   \
        static void ProxySet(Class *inst, type value) {                        \
            inst->membername = value;                                          \
        }                                                                      \
        static type ProxyGet(Class *inst) { return inst->membername; }         \
    };                                                                         \
    NCB_PROPERTY_PROXY(name, AutoProp_##name::ProxyGet,                        \
                       AutoProp_##name::ProxySet)

// ポインタ引数型の getter を変換登録
#define NCB_ARG_PROPERTY_RO(name, type, methodname)                            \
    struct AutoProp_##name {                                                   \
        static type ProxyGet(Class *inst) {                                    \
            type var;                                                          \
            inst->methodname(var);                                             \
            return var;                                                        \
        }                                                                      \
    };                                                                         \
    Property(TJS_W(#name), &AutoProp_##name::ProxyGet, (int)0, Proxy)

// ------------------------------------------------------
// 型コンバータ登録
// ------------------------------------------------------

NCB_TYPECONV_CAST_INTEGER(Status);
NCB_TYPECONV_CAST_INTEGER(MatrixOrder);
NCB_TYPECONV_CAST_INTEGER(ImageType);
NCB_TYPECONV_CAST_INTEGER(RotateFlipType);
NCB_TYPECONV_CAST_INTEGER(SmoothingMode);
NCB_TYPECONV_CAST_INTEGER(TextRenderingHint);

// ------------------------------------------------------- PointF
template <class T>
struct PointFConvertor {
    typedef ncbInstanceAdaptor<T> AdaptorT;
    template <typename ANYT>
    void operator()(ANYT &adst, const tTJSVariant &src) {
        if(src.Type() == tvtObject) {
            T *obj = AdaptorT::GetNativeInstance(src.AsObjectNoAddRef());
            if(obj) {
                dst = *obj;
            } else {
                ncbPropAccessor info(src);
                if(IsArray(src)) {
                    dst = PointF((tjs_real)info.getRealValue(0),
                                 (tjs_real)info.getRealValue(1));
                } else {
                    dst = PointF((tjs_real)info.getRealValue(TJS_W("x")),
                                 (tjs_real)info.getRealValue(TJS_W("y")));
                }
            }
        } else {
            dst = T();
        }
        adst = ncbTypeConvertor::ToTarget<ANYT>::Get(&dst);
    }

private:
    T dst;
};

NCB_SET_CONVERTOR_DST(PointF, PointFConvertor);
NCB_REGISTER_SUBCLASS_DELAY(PointF) {
    NCB_CONSTRUCTOR((tjs_real, tjs_real));
    NCB_MEMBER_PROPERTY(x, tjs_real, x);
    NCB_MEMBER_PROPERTY(y, tjs_real, y);
    NCB_METHOD(Equals);
};

PointF getPoint(const tTJSVariant &var) {
    PointFConvertor<PointF> conv;
    PointF ret;
    conv(ret, var);
    return ret;
}

// ------------------------------------------------------- RectF
template <class T>
struct RectFConvertor {
    typedef ncbInstanceAdaptor<T> AdaptorT;
    template <typename ANYT>
    void operator()(ANYT &adst, const tTJSVariant &src) {
        if(src.Type() == tvtObject) {
            T *obj = AdaptorT::GetNativeInstance(src.AsObjectNoAddRef());
            if(obj) {
                dst = *obj;
            } else {
                ncbPropAccessor info(src);
                if(IsArray(src)) {
                    dst = RectF((tjs_real)info.getRealValue(0),
                                (tjs_real)info.getRealValue(1),
                                (tjs_real)info.getRealValue(2),
                                (tjs_real)info.getRealValue(3));
                } else {
                    dst = RectF((tjs_real)info.getRealValue(TJS_W("x")),
                                (tjs_real)info.getRealValue(TJS_W("y")),
                                (tjs_real)info.getRealValue(TJS_W("width")),
                                (tjs_real)info.getRealValue(TJS_W("height")));
                }
            }
        } else {
            dst = T();
        }
        adst = ncbTypeConvertor::ToTarget<ANYT>::Get(&dst);
    }

private:
    T dst;
};

static RectF getRect(const tTJSVariant &var) {
    RectFConvertor<RectF> conv;
    RectF ret;
    conv(ret, var);
    return ret;
}

NCB_SET_CONVERTOR_DST(RectF, RectFConvertor);
NCB_REGISTER_SUBCLASS_DELAY(RectF) {
    NCB_CONSTRUCTOR((tjs_real, tjs_real, tjs_real, tjs_real));
    NCB_MEMBER_PROPERTY(x, tjs_real, x);
    NCB_MEMBER_PROPERTY(y, tjs_real, y);
    NCB_MEMBER_PROPERTY(width, tjs_real, w);
    NCB_MEMBER_PROPERTY(height, tjs_real, h);
    NCB_PROPERTY_RO(left, GetLeft);
    NCB_PROPERTY_RO(top, GetTop);
    NCB_PROPERTY_RO(right, GetRight);
    NCB_PROPERTY_RO(bottom, GetBottom);
    NCB_ARG_PROPERTY_RO(location, PointF, GetLocation);
    NCB_ARG_PROPERTY_RO(bounds, RectF, GetBounds);
    NCB_METHOD(Clone);
    NCB_METHOD(Equals);
    NCB_METHOD_DETAIL(Inflate, Class, void, Class::Inflate,
                      (tjs_real, tjs_real));
    NCB_METHOD_DETAIL(InflatePoint, Class, void, Class::Inflate,
                      (const PointF &));
    NCB_METHOD(IntersectsWith);
    NCB_METHOD(IsEmptyArea);
    NCB_METHOD_DETAIL(Offset, Class, void, Class::Offset, (tjs_real, tjs_real));
    NCB_METHOD(Union);
};

// --------------------------------------------------------------------
// GDI+のデフォルトコンストラクタ/コピーコンストラクタを持たない型の登録
// --------------------------------------------------------------------

/**
 * GDI+オブジェクトのラッピング用テンプレートクラス
 */
template <class T>
class GdipWrapper {
    typedef T GdipClassT;
    typedef GdipWrapper<GdipClassT> WrapperT;

protected:
    GdipClassT *obj;

public:
    // デフォルトコンストラクタ
    GdipWrapper() : obj(NULL) {}

    // 関数の帰り値としてのオブジェクト生成時用。
    // そのまま渡されたポインタを使う
    GdipWrapper(GdipClassT *obj) : obj(obj) {}

    // コピーコンストラクタ
    // 内蔵オブジェクトは Cloneする
    GdipWrapper(const GdipWrapper &orig) : obj(NULL) {
        if(orig.obj) {
            obj = orig.obj->Clone();
        }
    }

    // デストラクタ
    ~GdipWrapper() { delete obj; }

    GdipClassT *getGdipObject() { return obj; }

    void setGdipObject(GdipClassT *src) {
        if(obj) {
            delete obj;
        }
        obj = src;
    }

    struct BridgeFunctor {
        GdipClassT *operator()(WrapperT *p) const { return p->getGdipObject(); }
    };

    template <class CastT>
    struct CastBridgeFunctor {
        CastT *operator()(WrapperT *p) const {
            return (CastT *)p->getGdipObject();
        }
    };
};

/**
 * GDI+オブジェクトをラッピングしたクラス用のコンバータ（汎用）
 */
template <class T>
struct GdipTypeConvertor {
    typedef typename ncbTypeConvertor::Stripper<T>::Type GdipClassT;
    typedef T *GdipClassP;
    typedef GdipWrapper<GdipClassT> WrapperT;
    typedef ncbInstanceAdaptor<WrapperT> AdaptorT;

protected:
    GdipClassT *result; // 結果の一時保持用
public:
    GdipTypeConvertor() : result(nullptr) {}

    ~GdipTypeConvertor() { delete result; }

    virtual void operator()(GdipClassP &dst, const tTJSVariant &src) {
        WrapperT *obj;
        if(src.Type() == tvtObject &&
           (obj = AdaptorT::GetNativeInstance(src.AsObjectNoAddRef()))) {
            dst = obj->getGdipObject();
        } else {
            // FIXME: nullptr danger!!
            dst = NULL;
        }
    }

    void operator()(tTJSVariant &dst, const GdipClassP &src) {
        if(src != nullptr) {
            iTJSDispatch2 *adpobj = AdaptorT::CreateAdaptor(new WrapperT(src));
            if(adpobj) {
                dst = tTJSVariant(adpobj, adpobj);
                adpobj->Release();
            } else {
                // FIXME: nullptr danger!!
                // dst = NULL;
            }
        } else {
            dst.Clear();
        }
    }
};

// コンバータ登録用登録用マクロ

#define NCB_GDIP_CONVERTOR(type)                                               \
    NCB_SET_CONVERTOR(type *, GdipTypeConvertor<type>);                        \
    NCB_SET_CONVERTOR(const type *, GdipTypeConvertor<const type>)

#define NCB_GDIP_CONVERTOR2(type, convertor)                                   \
    NCB_SET_CONVERTOR(type *, convertor<type>);                                \
    NCB_SET_CONVERTOR(const type *, convertor<const type>)

// ラッピング処理用
#define NCB_REGISTER_GDIP_SUBCLASS(Class)                                      \
    NCB_GDIP_CONVERTOR(Class);                                                 \
    NCB_REGISTER_SUBCLASS(GdipWrapper<Class>) {                                \
        typedef Class GdipClass;
#define NCB_REGISTER_GDIP_SUBCLASS2(Class, Convertor)                          \
    NCB_GDIP_CONVERTOR2(Class, Convertor);                                     \
    NCB_REGISTER_SUBCLASS(GdipWrapper<Class>) {                                \
        typedef Class GdipClass;
#define NCB_GDIP_METHOD(name)                                                  \
    Method(TJS_W(#name), &GdipClass::name,                                     \
           Bridge<GdipWrapper<GdipClass>::BridgeFunctor>())
#define NCB_GDIP_MCAST(ret, method, args)                                      \
    static_cast<ret(GdipClass::*) args>(&GdipClass::method)
#define NCB_GDIP_METHOD2(name, ret, method, args)                              \
    Method(TJS_W(#name), NCB_GDIP_MCAST(ret, method, args),                    \
           Bridge<GdipWrapper<GdipClass>::BridgeFunctor>())
#define NCB_GDIP_PROPERTY(name, get, set)                                      \
    Property(TJS_W(#name), &GdipClass::get, &GdipClass::set,                   \
             Bridge<GdipWrapper<GdipClass>::BridgeFunctor>())
// XXX うまくうごかない
#define NCB_GDIP_PROPERTY_RO(name, get)                                        \
    Property(TJS_W(#name), &GdipClass::get, (int)0,                            \
             Bridge<GdipWrapper<GdipClass>::BridgeFunctor>())
#define NCB_GDIP_MEMBER_PROPERTY(name, type, membername)                       \
    struct AutoProp_##name {                                                   \
        static void ProxySet(GdipClass *inst, type value) {                    \
            inst->membername = value;                                          \
        }                                                                      \
        static type ProxyGet(GdipClass *inst) { return inst->membername; }     \
    };                                                                         \
    Property(TJS_W(#name), AutoProp_##name::ProxyGet,                          \
             AutoProp_##name::ProxySet,                                        \
             Bridge<GdipWrapper<GdipClass>::BridgeFunctor>())


// ------------------------------------------------------- Matrix

template <typename>
struct MatrixConvertor : GdipTypeConvertor<GdipMatrix> {
    void operator()(GdipClassP &dst, const tTJSVariant &src) {
        WrapperT *obj;
        if(src.Type() == tvtObject) {
            if((obj = AdaptorT::GetNativeInstance(src.AsObjectNoAddRef()))) {
                dst = obj->getGdipObject();
            } else {
                ncbPropAccessor info(src);
                if(IsArray(src)) {
                    this->result = new GdipMatrix(
                        BLMatrix2D((tjs_real)info.getRealValue(0),
                                   (tjs_real)info.getRealValue(1),
                                   (tjs_real)info.getRealValue(2),
                                   (tjs_real)info.getRealValue(3),
                                   (tjs_real)info.getRealValue(4),
                                   (tjs_real)info.getRealValue(5)));
                } else {
                    this->result = new GdipMatrix(
                        BLMatrix2D((tjs_real)info.getRealValue(TJS_W("m11")),
                                   (tjs_real)info.getRealValue(TJS_W("m12")),
                                   (tjs_real)info.getRealValue(TJS_W("m21")),
                                   (tjs_real)info.getRealValue(TJS_W("m22")),
                                   (tjs_real)info.getRealValue(TJS_W("dx")),
                                   (tjs_real)info.getRealValue(TJS_W("dy"))));
                }
                dst = this->result;
            }
        } else {
            dst = NULL;
        }
    }
};

static tjs_error MatrixFactory(GdipWrapper<GdipMatrix> **result,
                               tjs_int numparams, tTJSVariant **params,
                               iTJSDispatch2 *objthis) {
    BLMatrix2D *matrix = NULL;
    RectF *rect = NULL;
    PointF *point = NULL;
    if(numparams == 0) {
        matrix = new BLMatrix2D();
    } else if(numparams == 2 &&
              (params[0]->Type() == tvtObject &&
               (rect = ncbInstanceAdaptor<RectF>::GetNativeInstance(
                    params[0]->AsObjectNoAddRef()))) &&
              (params[1]->Type() == tvtObject &&
               (point = ncbInstanceAdaptor<PointF>::GetNativeInstance(
                    params[0]->AsObjectNoAddRef())))) {
        ncbPropAccessor rectObj(*params[0]);
        ncbPropAccessor pointsObj(*params[1]);
        RectF srcRect = getRect(*params[0]);
        std::vector<PointF> destPoints;
        getPoints(*params[1], destPoints);
        if(destPoints.size() >= 3) {
            float srcWidth = srcRect.w;
            float srcHeight = srcRect.h;
            float dx1 = destPoints[0].x;
            float dy1 = destPoints[0].y;
            float dx2 = destPoints[1].x;
            float dy2 = destPoints[1].y;
            float dx3 = destPoints[2].x;
            float dy3 = destPoints[2].y;
            float sx1 = srcRect.x;
            float sy1 = srcRect.y;
            float sx2 = srcRect.x + srcWidth;
            float sy2 = srcRect.y;
            float sx3 = srcRect.x;
            float sy3 = srcRect.y + srcHeight;

            float denom =
                sx1 * (sy2 - sy3) + sx2 * (sy3 - sy1) + sx3 * (sy1 - sy2);

            if(fabs(denom) < 1e-10) {
                matrix = new BLMatrix2D();
            } else {
                float a = (dx1 * (sy2 - sy3) + dx2 * (sy3 - sy1) +
                           dx3 * (sy1 - sy2)) /
                    denom;
                float b = (dy1 * (sy2 - sy3) + dy2 * (sy3 - sy1) +
                           dy3 * (sy1 - sy2)) /
                    denom;
                float c = (dx1 * (sx3 - sx2) + dx2 * (sx1 - sx3) +
                           dx3 * (sx2 - sx1)) /
                    denom;
                float d = (dy1 * (sx3 - sx2) + dy2 * (sx1 - sx3) +
                           dy3 * (sx2 - sx1)) /
                    denom;
                float e = (dx1 * (sx2 * sy3 - sx3 * sy2) +
                           dx2 * (sx3 * sy1 - sx1 * sy3) +
                           dx3 * (sx1 * sy2 - sx2 * sy1)) /
                    denom;
                float f = (dy1 * (sx2 * sy3 - sx3 * sy2) +
                           dy2 * (sx3 * sy1 - sx1 * sy3) +
                           dy3 * (sx1 * sy2 - sx2 * sy1)) /
                    denom;

                matrix = new BLMatrix2D(a, b, c, d, e, f);
            }
        } else {
            matrix = new BLMatrix2D();
        }
    } else if(numparams == 6) {
        matrix = new BLMatrix2D(
            (tjs_real)params[0]->AsReal(), (tjs_real)params[1]->AsReal(),
            (tjs_real)params[2]->AsReal(), (tjs_real)params[3]->AsReal(),
            (tjs_real)params[4]->AsReal(), (tjs_real)params[5]->AsReal());
    } else {
        return TJS_E_INVALIDPARAM;
    }
    *result = new GdipWrapper<GdipMatrix>(new GdipMatrix(BLMatrix2D(*matrix)));
    return TJS_S_OK;
}

NCB_REGISTER_GDIP_SUBCLASS2(GdipMatrix, MatrixConvertor)
Factory(MatrixFactory);
NCB_GDIP_METHOD(OffsetX);
NCB_GDIP_METHOD(OffsetY);
NCB_GDIP_METHOD(Equals);
NCB_GDIP_METHOD(SetElements);
NCB_GDIP_METHOD(GetLastStatus);
NCB_GDIP_METHOD(Invert);
NCB_GDIP_METHOD(IsIdentity);
NCB_GDIP_METHOD(IsInvertible);
NCB_GDIP_METHOD(Multiply);
NCB_GDIP_METHOD(Reset);
NCB_GDIP_METHOD(Rotate);
NCB_GDIP_METHOD(RotateAt);
NCB_GDIP_METHOD(Scale);
NCB_GDIP_METHOD(Shear);
NCB_GDIP_METHOD(Translate);
}
;

// ------------------------------------------------------- Image

/**
 * イメージ用コンバータ
 * 文字列からも変更可能
 */
template <class T>
struct ImageConvertor : public GdipTypeConvertor<GdipImage> {
    void operator()(GdipClassP &dst, const tTJSVariant &src) {
        if(src.Type() == tvtObject) {
            WrapperT *obj;
            if((obj = AdaptorT::GetNativeInstance(src.AsObjectNoAddRef()))) {
                dst = obj->getGdipObject();
            } else {
                LayerExDraw *layer =
                    ncbInstanceAdaptor<LayerExDraw>::GetNativeInstance(
                        src.AsObjectNoAddRef());
                if(layer) {
                    dst = *layer;
                } else {
                    dst = NULL;
                }
            }
        } else if(src.Type() == tvtString) { // 文字列から生成
            dst = this->result = new GdipImage(loadImage(src.GetString()));
        } else {
            dst = NULL;
        }
    }
};


static tjs_error ImageFactory(GdipWrapper<GdipImage> **result,
                              tjs_int numparams, tTJSVariant **params,
                              iTJSDispatch2 *objthis) {
    if(numparams == 0) {
        *result = new GdipWrapper<GdipImage>();
        return TJS_S_OK;
    } else if(numparams > 0 && params[0]->Type() == tvtString) {
        BLImage image = loadImage(params[0]->GetString());
        if(!image.empty()) {
            *result = new GdipWrapper<GdipImage>(new GdipImage(image));
            return TJS_S_OK;
        } else {
            TVPThrowExceptionMessage(TJS_W("cannot open:%1"), *params[0]);
        }
    }
    return TJS_E_INVALIDPARAM;
}

static void ImageLoad(GdipWrapper<GdipImage> *obj, const tjs_char *filename) {
    GdipImage *image = new GdipImage(loadImage(filename));
    if(image) {
        obj->setGdipObject(image);
    } else {
        TVPThrowExceptionMessage(TJS_W("cannot open:%1"), ttstr(filename));
    }
}

static tTJSVariant ImageClone(GdipWrapper<GdipImage> *obj) {
    typedef GdipWrapper<GdipImage> WrapperT;
    typedef ncbInstanceAdaptor<WrapperT> AdaptorT;
    tTJSVariant ret;
    GdipImage *src = obj->getGdipObject()->Clone();
    if(src) {
        iTJSDispatch2 *adpobj = AdaptorT::CreateAdaptor(new WrapperT(src));
        if(adpobj) {
            ret = tTJSVariant(adpobj, adpobj);
            adpobj->Release();
        } else {
            delete src;
        }
    }
    return ret;
}

static tTJSVariant ImageBounds(GdipWrapper<GdipImage> *obj) {
    typedef ncbInstanceAdaptor<RectF> AdaptorT;
    tTJSVariant ret;
    RectF src = obj->getGdipObject()->GetBounds();
    RectF *bounds = new RectF(src);
    iTJSDispatch2 *adpobj = AdaptorT::CreateAdaptor(bounds);
    if(adpobj) {
        ret = tTJSVariant(adpobj, adpobj);
        adpobj->Release();
    } else {
        delete bounds;
    }
    return ret;
}

NCB_REGISTER_GDIP_SUBCLASS2(GdipImage, ImageConvertor)
Factory(ImageFactory);
NCB_METHOD_PROXY(load, ImageLoad);
NCB_METHOD_PROXY(Clone, ImageClone);
NCB_METHOD_PROXY(GetBounds, ImageBounds);
NCB_GDIP_METHOD(GetFlags);
NCB_GDIP_METHOD(GetHeight);
NCB_GDIP_METHOD(GetHorizontalResolution);
NCB_GDIP_METHOD(GetLastStatus);
NCB_GDIP_METHOD(GetPixelFormat);
NCB_GDIP_METHOD(GetType);
NCB_GDIP_METHOD(GetVerticalResolution);
NCB_GDIP_METHOD(GetWidth);
NCB_GDIP_METHOD(RotateFlip);
}
;

// ------------------------------------------------------
// 自前記述クラス登録
// ------------------------------------------------------

NCB_REGISTER_SUBCLASS(FontInfo) {
    NCB_CONSTRUCTOR((const tjs_char *, tjs_real, tjs_int));
    NCB_PROPERTY(familyName, getFamilyName, setFamilyName);
    NCB_PROPERTY(emSize, getEmSize, setEmSize);
    NCB_PROPERTY(style, getStyle, setStyle);
    NCB_PROPERTY(forceSelfPathDraw, getForceSelfPathDraw, setForceSelfPathDraw);
    NCB_PROPERTY_RO(ascent, getAscent);
    NCB_PROPERTY_RO(descent, getDescent);
    NCB_PROPERTY_RO(ascentLeading, getAscentLeading);
    NCB_PROPERTY_RO(descentLeading, getDescentLeading);
    NCB_PROPERTY_RO(lineSpacing, getLineSpacing);
};

NCB_REGISTER_SUBCLASS(Appearance) {
    NCB_CONSTRUCTOR(());
    NCB_METHOD(clear);
    NCB_METHOD(addBrush);
    NCB_METHOD(addPen);
};

NCB_REGISTER_SUBCLASS(Path) {
    NCB_CONSTRUCTOR(());
    NCB_METHOD(startFigure);
    NCB_METHOD(closeFigure);
    NCB_METHOD(drawArc);
    NCB_METHOD(drawPie);
    NCB_METHOD(drawBezier);
    NCB_METHOD(drawBeziers);
    NCB_METHOD(drawClosedCurve);
    NCB_METHOD(drawClosedCurve2);
    NCB_METHOD(drawCurve);
    NCB_METHOD(drawCurve2);
    NCB_METHOD(drawCurve3);
    NCB_METHOD(drawEllipse);
    NCB_METHOD(drawLine);
    NCB_METHOD(drawLines);
    NCB_METHOD(drawPolygon);
    NCB_METHOD(drawRectangle);
    NCB_METHOD(drawRectangles);
};

#define ENUM(n) Variant(#n, (int)n)
#define NCB_SUBCLASS_NAME(name) NCB_SUBCLASS(name, name)
#define NCB_GDIP_SUBCLASS(Class) NCB_SUBCLASS(Class, GdipWrapper<Class>)

NCB_REGISTER_CLASS(GdiPlus) {
    // enums

    // Status
    ENUM(Ok);
    ENUM(GenericError);
    ENUM(InvalidParameter);
    ENUM(OutOfMemory);
    ENUM(ObjectBusy);
    ENUM(InsufficientBuffer);
    ENUM(NotImplemented);
    ENUM(Win32Error);
    ENUM(WrongState);
    ENUM(Aborted);
    ENUM(FileNotFound);
    ENUM(ValueOverflow);
    ENUM(AccessDenied);
    ENUM(UnknownImageFormat);
    ENUM(FontFamilyNotFound);
    ENUM(FontStyleNotFound);
    ENUM(NotTrueTypeFont);
    ENUM(UnsupportedGdiplusVersion);
    ENUM(GdiplusNotInitialized);
    ENUM(PropertyNotFound);
    ENUM(PropertyNotSupported);
    ENUM(ProfileNotFound);

    ENUM(FontStyleRegular);
    ENUM(FontStyleBold);
    ENUM(FontStyleItalic);
    ENUM(FontStyleBoldItalic);
    ENUM(FontStyleUnderline);
    ENUM(FontStyleStrikeout);

    ENUM(BrushTypeSolidColor);
    ENUM(BrushTypeHatchFill);
    ENUM(BrushTypeTextureFill);
    ENUM(BrushTypePathGradient);
    ENUM(BrushTypeLinearGradient);

    ENUM(DashCapFlat);
    ENUM(DashCapRound);
    ENUM(DashCapTriangle);

    ENUM(DashStyleSolid);
    ENUM(DashStyleDash);
    ENUM(DashStyleDot);
    ENUM(DashStyleDashDot);
    ENUM(DashStyleDashDotDot);

    ENUM(HatchStyleHorizontal);
    ENUM(HatchStyleVertical);
    ENUM(HatchStyleForwardDiagonal);
    ENUM(HatchStyleBackwardDiagonal);
    ENUM(HatchStyleCross);
    ENUM(HatchStyleDiagonalCross);
    ENUM(HatchStyle05Percent);
    ENUM(HatchStyle10Percent);
    ENUM(HatchStyle20Percent);
    ENUM(HatchStyle25Percent);
    ENUM(HatchStyle30Percent);
    ENUM(HatchStyle40Percent);
    ENUM(HatchStyle50Percent);
    ENUM(HatchStyle60Percent);
    ENUM(HatchStyle70Percent);
    ENUM(HatchStyle75Percent);
    ENUM(HatchStyle80Percent);
    ENUM(HatchStyle90Percent);
    ENUM(HatchStyleLightDownwardDiagonal);
    ENUM(HatchStyleLightUpwardDiagonal);
    ENUM(HatchStyleDarkDownwardDiagonal);
    ENUM(HatchStyleDarkUpwardDiagonal);
    ENUM(HatchStyleWideDownwardDiagonal);
    ENUM(HatchStyleWideUpwardDiagonal);
    ENUM(HatchStyleLightVertical);
    ENUM(HatchStyleLightHorizontal);
    ENUM(HatchStyleNarrowVertical);
    ENUM(HatchStyleNarrowHorizontal);
    ENUM(HatchStyleDarkVertical);
    ENUM(HatchStyleDarkHorizontal);
    ENUM(HatchStyleDashedDownwardDiagonal);
    ENUM(HatchStyleDashedUpwardDiagonal);
    ENUM(HatchStyleDashedHorizontal);
    ENUM(HatchStyleDashedVertical);
    ENUM(HatchStyleSmallConfetti);
    ENUM(HatchStyleLargeConfetti);
    ENUM(HatchStyleZigZag);
    ENUM(HatchStyleWave);
    ENUM(HatchStyleDiagonalBrick);
    ENUM(HatchStyleHorizontalBrick);
    ENUM(HatchStyleWeave);
    ENUM(HatchStylePlaid);
    ENUM(HatchStyleDivot);
    ENUM(HatchStyleDottedGrid);
    ENUM(HatchStyleDottedDiamond);
    ENUM(HatchStyleShingle);
    ENUM(HatchStyleTrellis);
    ENUM(HatchStyleSphere);
    ENUM(HatchStyleSmallGrid);
    ENUM(HatchStyleSmallCheckerBoard);
    ENUM(HatchStyleLargeCheckerBoard);
    ENUM(HatchStyleOutlinedDiamond);
    ENUM(HatchStyleSolidDiamond);
    ENUM(HatchStyleTotal);
    ENUM(HatchStyleLargeGrid);
    ENUM(HatchStyleMin);
    ENUM(HatchStyleMax);

    ENUM(LinearGradientModeHorizontal);
    ENUM(LinearGradientModeVertical);
    ENUM(LinearGradientModeForwardDiagonal);
    ENUM(LinearGradientModeBackwardDiagonal);

    ENUM(LineCapFlat);
    ENUM(LineCapSquare);
    ENUM(LineCapRound);
    ENUM(LineCapTriangle);
    ENUM(LineCapNoAnchor);
    ENUM(LineCapSquareAnchor);
    ENUM(LineCapRoundAnchor);
    ENUM(LineCapDiamondAnchor);
    ENUM(LineCapArrowAnchor);

    ENUM(LineJoinMiter);
    ENUM(LineJoinBevel);
    ENUM(LineJoinRound);
    ENUM(LineJoinMiterClipped);

    ENUM(PenAlignmentCenter);
    ENUM(PenAlignmentInset);

    ENUM(WrapModeTile);
    ENUM(WrapModeTileFlipX);
    ENUM(WrapModeTileFlipY);
    ENUM(WrapModeTileFlipXY);
    ENUM(WrapModeClamp);

    ENUM(MatrixOrderPrepend);
    ENUM(MatrixOrderAppend);

    ENUM(ImageTypeUnknown);
    ENUM(ImageTypeBitmap);
    ENUM(ImageTypeMetafile);

    ENUM(RotateNoneFlipNone);
    ENUM(Rotate90FlipNone);
    ENUM(Rotate180FlipNone);
    ENUM(Rotate270FlipNone);
    ENUM(RotateNoneFlipX);
    ENUM(Rotate90FlipX);
    ENUM(Rotate180FlipX);
    ENUM(Rotate270FlipX);
    ENUM(RotateNoneFlipY);
    ENUM(Rotate90FlipY);
    ENUM(Rotate180FlipY);
    ENUM(Rotate270FlipY);
    ENUM(RotateNoneFlipXY);
    ENUM(Rotate90FlipXY);
    ENUM(Rotate180FlipXY);
    ENUM(Rotate270FlipXY);

    ENUM(SmoothingModeInvalid);
    ENUM(SmoothingModeDefault);
    ENUM(SmoothingModeHighSpeed);
    ENUM(SmoothingModeHighQuality);
    ENUM(SmoothingModeNone);
    ENUM(SmoothingModeAntiAlias);

    ENUM(TextRenderingHintSystemDefault);
    ENUM(TextRenderingHintSingleBitPerPixelGridFit);
    ENUM(TextRenderingHintSingleBitPerPixel);
    ENUM(TextRenderingHintAntiAliasGridFit);
    ENUM(TextRenderingHintAntiAlias);
    ENUM(TextRenderingHintClearTypeGridFit);

    // statics
    NCB_METHOD(addPrivateFont);
    NCB_METHOD(getFontList);

    // classes
    NCB_SUBCLASS_NAME(PointF);
    NCB_SUBCLASS_NAME(RectF);

    SubClass(TJS_W("Image"), TypeWrap<GdipWrapper<GdipImage>>());
    SubClass(TJS_W("Matrix"), TypeWrap<GdipWrapper<GdipMatrix>>());

    NCB_SUBCLASS(Font, FontInfo);
    NCB_SUBCLASS(Appearance, Appearance);
    NCB_SUBCLASS(Path, Path);
}

NCB_GET_INSTANCE_HOOK(LayerExDraw){
    // インスタンスゲッタ
    NCB_INSTANCE_GETTER(objthis){
        // objthis を iTJSDispatch2* 型の引数とする
        ClassT *obj =
            GetNativeInstance(objthis); // ネイティブインスタンスポインタ取得
if(!obj) {
    obj = new ClassT(objthis); // ない場合は生成する
    SetNativeInstance(
        objthis, obj); // objthis に obj をネイティブインスタンスとして登録する
}
obj->reset();
return obj;
}
// デストラクタ（実際のメソッドが呼ばれた後に呼ばれる）
~NCB_GET_INSTANCE_HOOK_CLASS() {}
}
;

#define LAYEREX_METHOD(type, name)                                             \
    Method(TJS_W(#name), &Type::name,                                          \
           Bridge<LayerExDraw::BridgeFunctor<type>>())

/**
 * Image はラッピングする必要があるので rawcallback で対応
 */
static tjs_error GetRecordImage(tTJSVariant *result, tjs_int numparams,
                                tTJSVariant **param, iTJSDispatch2 *objthis) {
    LayerExDraw *obj =
        ncbInstanceAdaptor<LayerExDraw>::GetNativeInstance(objthis, true);
    if(result)
        result->Clear();
    if(obj) {
        GdipImage *image = obj->getRecordImage();
        if(image) {
            typedef GdipWrapper<GdipImage> WrapperT;
            WrapperT *wrap = new WrapperT(image);
            iTJSDispatch2 *adpobj =
                ncbInstanceAdaptor<WrapperT>::CreateAdaptor(wrap);
            if(adpobj) {
                if(result)
                    *result = tTJSVariant(adpobj, adpobj);
                adpobj->Release();
            } else {
                delete wrap;
                delete image;
            }
        }
    }
    return TJS_S_OK;
}

// フックつきアタッチ
NCB_ATTACH_CLASS_WITH_HOOK(LayerExDraw, Layer) {
    NCB_PROPERTY(updateWhenDraw, getUpdateWhenDraw, setUpdateWhenDraw);
    NCB_PROPERTY(smoothingMode, getSmoothingMode, setSmoothingMode);
    NCB_PROPERTY(textRenderingHint, getTextRenderingHint, setTextRenderingHint);

    NCB_METHOD(setViewTransform);
    NCB_METHOD(resetViewTransform);
    NCB_METHOD(rotateViewTransform);
    NCB_METHOD(scaleViewTransform);
    NCB_METHOD(translateViewTransform);

    NCB_METHOD(setTransform);
    NCB_METHOD(resetTransform);
    NCB_METHOD(rotateTransform);
    NCB_METHOD(scaleTransform);
    NCB_METHOD(translateTransform);

    NCB_METHOD(clear);
    NCB_METHOD(drawPath);
    NCB_METHOD(drawArc);
    NCB_METHOD(drawPie);
    NCB_METHOD(drawBezier);
    NCB_METHOD(drawBeziers);
    NCB_METHOD(drawClosedCurve);
    NCB_METHOD(drawClosedCurve2);
    NCB_METHOD(drawCurve);
    NCB_METHOD(drawCurve2);
    NCB_METHOD(drawCurve3);
    NCB_METHOD(drawEllipse);
    NCB_METHOD(drawLine);
    NCB_METHOD(drawLines);
    NCB_METHOD(drawPolygon);
    NCB_METHOD(drawRectangle);
    NCB_METHOD(drawRectangles);
    NCB_METHOD(drawPathString);
    NCB_METHOD(drawString);
    NCB_METHOD(measureString);
    NCB_METHOD(measureStringInternal);

    NCB_METHOD(drawImage);
    NCB_METHOD(drawImageRect);
    NCB_METHOD(drawImageStretch);
    NCB_METHOD(drawImageAffine);

    NCB_PROPERTY(record, getRecord, setRecord);
    NCB_METHOD_RAW_CALLBACK(getRecordImage, GetRecordImage, 0);
    NCB_METHOD(redrawRecord);
    NCB_METHOD(saveRecord);
    NCB_METHOD(loadRecord);
}

// ----------------------------------- 起動・開放処理

NCB_PRE_REGIST_CALLBACK(initGdiPlus);
NCB_POST_UNREGIST_CALLBACK(deInitGdiPlus);
