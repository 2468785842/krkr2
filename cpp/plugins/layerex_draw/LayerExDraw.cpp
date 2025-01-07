#include "ncbind.hpp"
#include "LayerExDraw.hpp"
#include <vector>
#include <cstdio>
#include <cairo/cairo.h>

// GDI+ 基本情報
static GdiplusStartupInput gdiplusStartupInput;
static ULONG_PTR gdiplusToken;

/// プライベートフォント情報
static PrivateFontCollection *privateFontCollection = nullptr;
static vector<void *> fontDatas;

static float ToFloat(FIXED &pfx) {
    LONG l = *(LONG *) &pfx;

    return l / 65536.0f;
}

static PointFClass ToPointF(POINTFX *p) {
    return PointFClass{ToFloat(p->x), -ToFloat(p->y)};
}

// GDI+ 初期化
void initGdiPlus() {
    // Initialize GDI+.
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
}

// GDI+ 終了
void deInitGdiPlus() {
    // フォントデータの解放
    delete privateFontCollection;
    auto i = fontDatas.begin();
    while (i != fontDatas.end()) {
        delete[] *i;
        i++;
    }
    fontDatas.clear();
    GdiplusShutdown(gdiplusToken);
}

/**
 * 画像読み込み処理
 * @param name ファイル名
 * @return 画像情報
 */
ImageClass *loadImage(const tjs_char *name) {
    ImageClass *image = nullptr;
    ttstr filename = TVPGetPlacedPath(name);
    if (filename.length()) {
        ttstr localname(TVPGetLocallyAccessibleName(filename));
        if (localname.length()) {
            // 実ファイルが存在
            static_assert(sizeof(tjs_char) == sizeof(WCHAR), "loadImage Sizes differ!");
            const auto *n = reinterpret_cast<const WCHAR *>(localname.c_str());
            image = ImageClass::FromFile(n, false);
        }
    }
    if (image && image->GetLastStatus() != Ok) {
        delete image;
        image = nullptr;
    }
    return image;
}

RectFClass *getBounds(ImageClass *image) {
    RectFClass srcRect;
    Unit srcUnit;
    image->GetBounds(&srcRect, &srcUnit);
    REAL dpix = image->GetHorizontalResolution();
    REAL dpiy = image->GetVerticalResolution();

    // ピクセルに変換
    REAL x, y, width, height;
    switch (srcUnit) {
        case UnitPoint:        // 3 -- Each unit is a printer's point, or 1/72 inch.
            x = srcRect.X * dpix / 72;
            y = srcRect.Y * dpiy / 72;
            width = srcRect.Width * dpix / 72;
            height = srcRect.Height * dpix / 72;
            break;
        case UnitInch:       // 4 -- Each unit is 1 inch.
            x = srcRect.X * dpix;
            y = srcRect.Y * dpiy;
            width = srcRect.Width * dpix;
            height = srcRect.Height * dpix;
            break;
        case UnitDocument:   // 5 -- Each unit is 1/300 inch.
            x = srcRect.X * dpix / 300;
            y = srcRect.Y * dpiy / 300;
            width = srcRect.Width * dpix / 300;
            height = srcRect.Height * dpix / 300;
            break;
        case UnitMillimeter: // 6 -- Each unit is 1 millimeter.
            x = srcRect.X * dpix / 25.4F;
            y = srcRect.Y * dpiy / 25.4F;
            width = srcRect.Width * dpix / 25.4F;
            height = srcRect.Height * dpix / 25.4F;
            break;
        default:
            x = srcRect.X;
            y = srcRect.Y;
            width = srcRect.Width;
            height = srcRect.Height;
            break;
    }
    return new RectFClass{x, y, width, height};
}

// --------------------------------------------------------
// フォント情報
// --------------------------------------------------------

/**
 * プライベートフォントの追加
 * @param fontFileName フォントファイル名
 */
void GdiPlus::addPrivateFont(const tjs_char *fontFileName) {
    if (!privateFontCollection) {
        privateFontCollection = new PrivateFontCollection();
    }
    ttstr filename = TVPGetPlacedPath(fontFileName);
    if (filename.length()) {
        ttstr localname(TVPGetLocallyAccessibleName(filename));
        if (localname.length()) {
            // 実ファイルが存在
            static_assert(sizeof(tjs_char) == sizeof(WCHAR), "addPrivateFont Sizes differ!");
            const auto *n = reinterpret_cast<const WCHAR *>(localname.c_str());
            privateFontCollection->AddFontFile(n);
            return;
        } else {
            // メモリにロードして展開
            IStream *in = TVPCreateIStream(filename, TJS_BS_READ);
            if (in) {
                STATSTG stat;
                in->Stat(&stat, STATFLAG_NONAME);
                // サイズあふれ無視注意
                ULONG size = (ULONG) stat.cbSize.QuadPart;
                char *data = new char[size];
                if (in->Read(data, size, &size) == TJS_S_OK) {
                    privateFontCollection->AddMemoryFont(data, size);
                    fontDatas.push_back(data);
                } else {
                    delete[] data;
                }
                in->Release();
                return;
            }
        }
    }
    TVPThrowExceptionMessage(TJS_W("cannot open:%1"), fontFileName);
}

/**
 * 配列にフォントのファミリー名を格納
 * @param array 格納先配列
 * @param fontCollection フォント名を取得する元の FontCollection
 */
static void addFontFamilyName(iTJSDispatch2 *array, GpFontCollection *fontCollection) {
    int count;
    GdipGetFontCollectionFamilyCount(fontCollection, &count);
    auto *families = new GpFontFamily[count];
    GdipGetFontCollectionFamilyList(fontCollection, count, &families, &count);
    for (int i = 0; i < count; i++) {
        WCHAR familyName[LF_FACESIZE];
        GpFontFamily family = families[i];
        auto status = GdipGetFamilyName(&family, familyName, 0);
        if (status == Ok) {
            static_assert(sizeof(tjs_char) == sizeof(WCHAR), "addFontFamilyName Sizes differ!");
            tTJSVariant name(reinterpret_cast<tjs_char *>(familyName)), *param = &name;
            array->FuncCall(0, TJS_W("add"), nullptr, 0, 1, &param, array);
        }
    }
    delete[]families;
}

/**
 * フォント一覧の取得
 * @param privateOnly true ならプライベートフォントのみ取得
 */
tTJSVariant GdiPlus::getFontList(bool privateOnly) {
    iTJSDispatch2 *array = TJSCreateArrayObject();
    if (privateFontCollection) {
        addFontFamilyName(array, privateFontCollection->getFontCollection());
    }
    if (!privateOnly) {
        GpFontCollection installedFontCollection;
        addFontFamilyName(array, &installedFontCollection);
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
FontInfo::FontInfo() : fontFamily(nullptr),
                       emSize(12), style(0),
                       gdiPlusUnsupportedFont(false),
                       forceSelfPathDraw(false),
                       propertyModified(true) {}

/**
 * コンストラクタ
 * @param familyName フォントファミリー
 * @param emSize フォントのサイズ
 * @param style フォントスタイル
 */
FontInfo::FontInfo(
        const tjs_char *familyName,
        REAL emSize, INT style
) : fontFamily(nullptr), gdiPlusUnsupportedFont(false),
    forceSelfPathDraw(false), propertyModified(true) {
    setFamilyName(familyName);
    setEmSize(emSize);
    setStyle(style);
}

/**
 * コピーコンストラクタ
 */
FontInfo::FontInfo(const FontInfo &orig) {
    fontFamily = nullptr;
    if (orig.fontFamily) {
        GdipCloneFontFamily(orig.fontFamily, &fontFamily);
    }
    emSize = orig.emSize;
    style = orig.style;
}

/**
 * デストラクタ
 */
FontInfo::~FontInfo() {
    clear();
}

/**
 * フォント情報のクリア
 */
void FontInfo::clear() {
    GdipDeleteFontFamily(fontFamily);
    fontFamily = nullptr;
    familyName = "";
    gdiPlusUnsupportedFont = false;
    propertyModified = true;
}

/**
 * フォントの指定
 */
void
FontInfo::setFamilyName(const tjs_char *familyName) {
    static_assert(sizeof(tjs_char) == sizeof(WCHAR), "setFamilyName Sizes differ!");

    propertyModified = true;

    if (forceSelfPathDraw) {
        clear();
        gdiPlusUnsupportedFont = true;
        this->familyName = familyName;
        return;
    }

    if (familyName) {
        clear();
        if (privateFontCollection) {
            auto status = GdipCreateFontFamilyFromName(
                    reinterpret_cast<const WCHAR *>(familyName),
                    privateFontCollection->getFontCollection(),
                    &this->fontFamily
            );

            if (status == Ok) {
                this->familyName = familyName;
                return;
            } else {
                clear();
            }
        }
        auto status = GdipCreateFontFamilyFromName(
                reinterpret_cast<const WCHAR *>(familyName),
                nullptr,
                &this->fontFamily
        );

        if (status == Ok) {
            this->familyName = familyName;
            return;
        } else {
            clear();
            gdiPlusUnsupportedFont = true;
            this->familyName = familyName;
        }
    }
}

void FontInfo::setForceSelfPathDraw(bool state) {
    forceSelfPathDraw = state;
    ttstr _name = familyName;
    this->setFamilyName(_name.c_str());
}

bool FontInfo::getForceSelfPathDraw() const {
    return forceSelfPathDraw;
}

bool FontInfo::getSelfPathDraw() const {
    return forceSelfPathDraw || gdiPlusUnsupportedFont;
}

void FontInfo::updateSizeParams() const {
    if (!propertyModified)
        return;

    propertyModified = false;

    // 创建 Cairo 上下文
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t *cr = cairo_create(surface);

    // 获取字体度量
    cairo_select_font_face(cr, familyName.AsStdString().c_str(),
                           (style & 2) ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL,
                           (style & 1) ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);

    cairo_set_font_size(cr, emSize);

    // 获取字体度量
    cairo_font_extents_t font_extents;
    cairo_font_extents(cr, &font_extents);
    this->ascent = REAL(this->fontFamily->cellascent);
    this->descent = REAL(this->fontFamily->celldescent);

    this->ascentLeading = REAL((this->fontFamily->height - ascent - descent) / 2);
    this->descentLeading = REAL(-this->descent);
//    ascentLeading = ascent - REAL(otm->otmAscent);
//    descentLeading = descent - REAL(- otm->otmDescent);

    this->lineSpacing = REAL(this->fontFamily->linespacing);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

REAL FontInfo::getAscent() const {
    this->updateSizeParams();
    return ascent;
}


REAL FontInfo::getDescent() const {
    this->updateSizeParams();
    return descent;
}

REAL FontInfo::getAscentLeading() const {
    this->updateSizeParams();
    return ascentLeading;
}

REAL FontInfo::getDescentLeading() const {
    this->updateSizeParams();
    return descentLeading;
}

REAL FontInfo::getLineSpacing() const {
    this->updateSizeParams();
    return lineSpacing;
}

// --------------------------------------------------------
// アピアランス情報
// --------------------------------------------------------

Appearance::Appearance() = default;

Appearance::~Appearance() {
    clear();
}

/**
 * 情報のクリア
 */
void
Appearance::clear() {
    drawInfos.clear();

    // customLineCapsも削除
    auto i = customLineCaps.begin();
    while (i != customLineCaps.end()) {
        delete *i;
        i++;
    }
    customLineCaps.clear();
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
void getPoints(const tTJSVariant &var, vector<PointF> &points) {
    ncbPropAccessor info(var);
    int c = info.GetArrayCount();
    for (int i = 0; i < c; i++) {
        tTJSVariant p;
        if (info.checkVariant(i, p)) {
            points.push_back(getPoint(p));
        }
    }
}

static void getPoints(ncbPropAccessor &info, int n, vector<PointF> &points) {
    tTJSVariant var;
    if (info.checkVariant(n, var)) {
        getPoints(var, points);
    }
}

static void getPoints(ncbPropAccessor &info, const tjs_char *n, vector<PointF> &points) {
    tTJSVariant var;
    if (info.checkVariant(n, var)) {
        getPoints(var, points);
    }
}

// -----------------------------

/**
 * 矩形情報の生成
 */
extern RectF getRect(const tTJSVariant &var);

/**
 * 矩形の配列を取得
 */
void getRects(const tTJSVariant &var, vector<RectF> &rects) {
    ncbPropAccessor info(var);
    int c = info.GetArrayCount();
    for (int i = 0; i < c; i++) {
        tTJSVariant p;
        if (info.checkVariant(i, p)) {
            rects.push_back(getRect(p));
        }
    }
}

// -----------------------------

/**
 * 実数の配列を取得
 */
static void getReals(const tTJSVariant &var, vector<REAL> &points) {
    ncbPropAccessor info(var);
    int c = info.GetArrayCount();
    for (int i = 0; i < c; i++) {
        points.push_back((REAL) info.getRealValue(i));
    }
}

static void getReals(ncbPropAccessor &info, int n, vector<REAL> &points) {
    tTJSVariant var;
    if (info.checkVariant(n, var)) {
        getReals(var, points);
    }
}

static void getReals(ncbPropAccessor &info, const tjs_char *n, vector<REAL> &points) {
    tTJSVariant var;
    if (info.checkVariant(n, var)) {
        getReals(var, points);
    }
}

// -----------------------------

/**
 * 色の配列を取得
 */
static void getColors(const tTJSVariant &var, vector<Color> &colors) {
    ncbPropAccessor info(var);
    int c = info.GetArrayCount();
    for (int i = 0; i < c; i++) {
        colors.push_back(Color{(ARGB) info.getIntValue(i)});
    }
}

static void getColors(ncbPropAccessor &info, int n, vector<Color> &colors) {
    tTJSVariant var;
    if (info.checkVariant(n, var)) {
        getColors(var, colors);
    }
}

static void getColors(ncbPropAccessor &info, const tjs_char *n, vector<Color> &colors) {
    tTJSVariant var;
    if (info.checkVariant(n, var)) {
        getColors(var, colors);
    }
}

template<typename T>
void commonBrushParameter(ncbPropAccessor &info, T *brush) {
    tTJSVariant var;
    // SetBlend
    if (info.checkVariant(TJS_W("blend"), var)) {
        vector<REAL> factors;
        vector<REAL> positions;
        ncbPropAccessor binfo(var);
        if (IsArray(var)) {
            getReals(binfo, 0, factors);
            getReals(binfo, 1, positions);
        } else {
            getReals(binfo, TJS_W("blendFactors"), factors);
            getReals(binfo, TJS_W("blendPositions"), positions);
        }
        int count = (int) factors.size();
        if ((int) positions.size() > count) {
            count = (int) positions.size();
        }
        if (count > 0) {
            brush->SetBlend(&factors[0], &positions[0], count);
        }
    }
    // SetBlendBellShape
    if (info.checkVariant(TJS_W("blendBellShape"), var)) {
        ncbPropAccessor sinfo(var);
        if (IsArray(var)) {
            brush->SetBlendBellShape((REAL) sinfo.getRealValue(0),
                                     (REAL) sinfo.getRealValue(1));
        } else {
            brush->SetBlendBellShape((REAL) info.getRealValue(TJS_W("focus")),
                                     (REAL) info.getRealValue(TJS_W("scale")));
        }
    }
    // SetBlendTriangularShape
    if (info.checkVariant(TJS_W("blendTriangularShape"), var)) {
        ncbPropAccessor sinfo(var);
        if (IsArray(var)) {
            brush->SetBlendTriangularShape((REAL) sinfo.getRealValue(0),
                                           (REAL) sinfo.getRealValue(1));
        } else {
            brush->SetBlendTriangularShape((REAL) info.getRealValue(TJS_W("focus")),
                                           (REAL) info.getRealValue(TJS_W("scale")));
        }
    }
    // SetGammaCorrection
    if (info.checkVariant(TJS_W("useGammaCorrection"), var)) {
        brush->SetGammaCorrection((BOOL) var);
    }
    // SetInterpolationColors
    if (info.checkVariant(TJS_W("interpolationColors"), var)) {
        vector<Color> colors;
        vector<REAL> positions;
        ncbPropAccessor binfo(var);
        if (IsArray(var)) {
            getColors(binfo, 0, colors);
            getReals(binfo, 1, positions);
        } else {
            getColors(binfo, TJS_W("presetColors"), colors);
            getReals(binfo, TJS_W("blendPositions"), positions);
        }
        int count = (int) colors.size();
        if ((int) positions.size() > count) {
            count = (int) positions.size();
        }
        if (count > 0) {
            brush->SetInterpolationColors(&colors[0], &positions[0], count);
        }
    }
}

/**
 * ブラシの生成
 */
BrushBase *createBrush(const tTJSVariant &colorOrBrush) {
    BrushBase *brush;
    if (colorOrBrush.Type() != tvtObject) {
        brush = new SolidBrush{Color{(ARGB) (tjs_int) colorOrBrush}};
    } else {
        // 種別ごとに作り分ける
        ncbPropAccessor info(colorOrBrush);
        auto type = (BrushType) info.getIntValue(TJS_W("type"), BrushTypeSolidColor);
        switch (type) {
            case BrushTypeSolidColor:
                brush = new SolidBrush{Color{
                        (ARGB) (tjs_int) info.getIntValue(TJS_W("color"), 0xffffffff)}};
                break;
            case BrushTypeHatchFill:
                brush = new HatchBrush(
                        (HatchStyle) info.getIntValue(TJS_W("hatchStyle"), HatchStyleHorizontal),
                        Color{(ARGB) (tjs_int) info.getIntValue(TJS_W("foreColor"), 0xffffffff)},
                        Color{(ARGB) (tjs_int) info.getIntValue(TJS_W("backColor"), 0xff000000)});
                break;
            case BrushTypeTextureFill: {
                ttstr imgname = info.GetValue(TJS_W("image"), ncbTypedefs::Tag<ttstr>());
                auto *image = loadImage(imgname.c_str());
                if (image) {
                    auto wrapMode = (WrapMode) info.getIntValue(TJS_W("wrapMode"),
                                                                WrapModeTile);
                    tTJSVariant dstRect;
                    if (info.checkVariant(TJS_W("dstRect"), dstRect)) {
                        brush = new TextureBrush{image, wrapMode, getRect(dstRect)};
                    } else {
                        brush = new TextureBrush{image, wrapMode};
                    }
                    delete image;
                }
                break;
            }
            case BrushTypePathGradient: {
                PathGradientBrush *pbrush;
                vector<PointF> points;
                getPoints(info, TJS_W("points"), points);
                if ((int) points.size() == 0) TVPThrowExceptionMessage(TJS_W("must set poins"));
                WrapMode wrapMode = (WrapMode) info.getIntValue(TJS_W("wrapMode"), WrapModeTile);
                pbrush = new PathGradientBrush{&points[0], (int) points.size(), wrapMode};

                // 共通パラメータ
                commonBrushParameter(info, pbrush);

                tTJSVariant var;
                //SetCenterColor
                if (info.checkVariant(TJS_W("centerColor"), var)) {
                    pbrush->SetCenterColor(Color{(ARGB) (tjs_int) var});
                }
                //SetCenterPoint
                if (info.checkVariant(TJS_W("centerPoint"), var)) {
                    pbrush->SetCenterPoint(getPoint(var));
                }
                //SetFocusScales
                if (info.checkVariant(TJS_W("focusScales"), var)) {
                    ncbPropAccessor sinfo(var);
                    if (IsArray(var)) {
                        pbrush->SetFocusScales((REAL) sinfo.getRealValue(0),
                                               (REAL) sinfo.getRealValue(1));
                    } else {
                        pbrush->SetFocusScales((REAL) info.getRealValue(TJS_W("xScale")),
                                               (REAL) info.getRealValue(TJS_W("yScale")));
                    }
                }
                //SetSurroundColors
                if (info.checkVariant(TJS_W("surroundColors"), var)) {
                    vector<Color> colors;
                    getColors(var, colors);
                    int size = (int) colors.size();
                    pbrush->SetSurroundColors(&colors[0], &size);
                }
                brush = pbrush;
            }
                break;
            case BrushTypeLinearGradient: {
                LinearGradientBrush *lbrush;
                Color color1{(ARGB) (tjs_int) info.getIntValue(TJS_W("color1"), 0)};
                Color color2{(ARGB) (tjs_int) info.getIntValue(TJS_W("color2"), 0)};

                tTJSVariant var;
                if (info.checkVariant(TJS_W("point1"), var)) {
                    PointF point1 = getPoint(var);
                    info.checkVariant(TJS_W("point2"), var);
                    PointF point2 = getPoint(var);
                    lbrush = new LinearGradientBrush{point1, point2, color1, color2};
                } else if (info.checkVariant(TJS_W("rect"), var)) {
                    RectF rect = getRect(var);
                    if (info.HasValue(TJS_W("angle"))) {
                        // アングル指定がある場合
                        lbrush = new LinearGradientBrush{
                                rect, color1, color2,
                                (REAL) info.getRealValue(TJS_W("angle"),
                                                         0),
                                static_cast<bool>(info.getIntValue(
                                        TJS_W("isAngleScalable"),
                                        0))};
                    } else {
                        // 無い場合はモードを参照
                        lbrush = new LinearGradientBrush{
                                rect, color1, color2,
                                (LinearGradientMode) info.getIntValue(
                                        TJS_W("mode"),
                                        LinearGradientModeHorizontal)};
                    }
                } else {
                    TVPThrowExceptionMessage(TJS_W("must set point1,2 or rect"));
                }

                // 共通パラメータ
                commonBrushParameter(info, lbrush);

                // SetWrapMode
                if (info.checkVariant(TJS_W("wrapMode"), var)) {
                    lbrush->SetWrapMode((WrapMode) (tjs_int) var);
                }
                brush = lbrush;
            }
                break;
            default:
                TVPThrowExceptionMessage(TJS_W("invalid brush type"));
                break;
        }
    }
    return brush;
}

/**
 * ブラシの追加
 * @param colorOrBrush ARGB色指定またはブラシ情報（辞書）
 * @param ox 表示オフセットX
 * @param oy 表示オフセットY
 */
void
Appearance::addBrush(const tTJSVariant &colorOrBrush, REAL ox, REAL oy) {
    drawInfos.emplace_back(ox, oy, createBrush(colorOrBrush));
}

/**
 * ペンの追加
 * @param colorOrBrush ARGB色指定またはブラシ情報（辞書）
 * @param widthOrOption ペン幅またはペン情報（辞書）
 * @param ox 表示オフセットX
 * @param oy 表示オフセットY
 */
void Appearance::addPen(
        tTJSVariant colorOrBrush, tTJSVariant widthOrOption,
        REAL ox, REAL oy
) {
    Pen *pen;
    REAL width = 1.0;
    if (colorOrBrush.Type() == tvtObject) {
        BrushBase *brush = createBrush(colorOrBrush);
        pen = new Pen{brush, width};
        delete brush;
    } else {
        pen = new Pen(Color{(ARGB) (tjs_int) colorOrBrush}, width);
    }
    if (widthOrOption.Type() != tvtObject) {
        pen->SetWidth((REAL) (tjs_real) widthOrOption);
    } else {
        ncbPropAccessor info(widthOrOption);
        REAL penWidth = 1.0;
        tTJSVariant var;

        // SetWidth
        if (info.checkVariant(TJS_W("width"), var)) {
            penWidth = (REAL) (tjs_real) var;
        }
        pen->SetWidth(penWidth);

        // SetAlignment
        if (info.checkVariant(TJS_W("alignment"), var)) {
            pen->SetAlignment((PenAlignment) (tjs_int)
                    var);
        }
        // SetCompoundArray
        if (info.checkVariant(TJS_W("compoundArray"), var)) {
            vector<REAL> reals;
            getReals(var, reals);
            pen->SetCompoundArray(&reals[0], (int) reals.size());
        }

        // SetDashCap
        if (info.checkVariant(TJS_W("dashCap"), var)) {
            pen->SetDashCap((GpDashCap) (tjs_int) var);
        }
        // SetDashOffset
        if (info.checkVariant(TJS_W("dashOffset"), var)) {
            pen->SetDashOffset((REAL) (tjs_real) var);
        }

        // SetDashStyle
        // SetDashPattern
        if (info.checkVariant(TJS_W("dashStyle"), var)) {
            if (IsArray(var)) {
                vector<REAL> reals;
                getReals(var, reals);
                pen->SetDashStyle(DashStyleCustom);
                pen->SetDashPattern(&reals[0], (int) reals.size());
            } else {
                pen->SetDashStyle((GpDashStyle) (tjs_int) var);
            }
        }

        // SetStartCap
        // SetCustomStartCap
        if (info.checkVariant(TJS_W("startCap"), var)) {
            GpLineCap cap = LineCapFlat;
            CustomLineCap *custom = nullptr;
            if (getLineCap(var, cap, custom, penWidth)) {
                if (custom != nullptr) pen->SetCustomStartCap(custom);
                else pen->SetStartCap(cap);
            }
        }

        // SetEndCap
        // SetCustomEndCap
        if (info.checkVariant(TJS_W("endCap"), var)) {
            GpLineCap cap = LineCapFlat;
            CustomLineCap *custom = nullptr;
            if (getLineCap(var, cap, custom, penWidth)) {
                if (custom != nullptr) pen->SetCustomEndCap(custom);
                else pen->SetEndCap(cap);
            }
        }

        // SetLineJoin
        if (info.checkVariant(TJS_W("lineJoin"), var)) {
            pen->SetLineJoin((GpLineJoin) (tjs_int)
                    var);
        }

        // SetMiterLimit
        if (info.checkVariant(TJS_W("miterLimit"), var)) {
            pen->SetMiterLimit((REAL) (tjs_real) var);
        }
    }
    drawInfos.push_back(DrawInfo{ox, oy, pen});
}

bool Appearance::getLineCap(tTJSVariant &in, GpLineCap &cap, CustomLineCap *&custom, REAL pw) {
    switch (in.Type()) {
        case tvtVoid:
        case tvtInteger:
            cap = (GpLineCap) (tjs_int) in;
            break;
        case tvtObject: {
            ncbPropAccessor info(in);
            REAL width = pw, height = pw;
            tTJSVariant var;
            if (info.checkVariant(TJS_W("width"), var)) width = (REAL) (tjs_real) var;
            if (info.checkVariant(TJS_W("height"), var)) height = (REAL) (tjs_real) var;
            BOOL filled = (BOOL) info.getIntValue(TJS_W("filled"), 1);
            GpAdjustableArrowCap *arrow{nullptr};
            GdipCreateAdjustableArrowCap(height, width, filled, &arrow);
            if (info.checkVariant(TJS_W("middleInset"), var))
                GdipSetAdjustableArrowCapMiddleInset(arrow, (REAL) (tjs_real) var);
            customLineCaps.push_back((custom = reinterpret_cast<CustomLineCap *>(arrow)));
        }
            break;
        default:
            return false;
    }
    return true;
}


// --------------------------------------------------------
// フォント描画系
// --------------------------------------------------------

void
LayerExDraw::updateRect(RectF &rect) {
    if (updateWhenDraw) {
        // 更新処理
        tTJSVariant vars[4] = {rect.X, rect.Y, rect.Width, rect.Height};
        tTJSVariant *varsp[4] = {vars, vars + 1, vars + 2, vars + 3};
        _pUpdate(4, varsp);
    }
}

/**
 * コンストラクタ
 */
LayerExDraw::LayerExDraw(DispatchT obj)
        : layerExBase(obj), width(-1), height(-1), pitch(0), bitmap(nullptr),
          graphics(nullptr),
          clipLeft(-1), clipTop(-1), clipWidth(-1), clipHeight(-1),
          smoothingMode(SmoothingModeAntiAlias), textRenderingHint(TextRenderingHintAntiAlias),
          metaBuffer(nullptr), metaStream(nullptr), metafile(nullptr),
          metaGraphics(nullptr),
          updateWhenDraw(true) {
}

/**
 * デストラクタ
 */
LayerExDraw::~LayerExDraw() {
    destroyRecord();
    GdipDeleteGraphics(this->graphics);
    GdipDisposeImage(this->bitmap);
}

void LayerExDraw::reset() {
    layerExBase::reset();
    // 変更されている場合はつくりなおし
    if (!(graphics && width == _width
          && height == _height && pitch == _pitch
    )) {
        GdipDeleteGraphics(this->graphics);
        GdipDisposeImage(this->bitmap);
        this->graphics = nullptr;
        this->bitmap = nullptr;
        width = _width;
        height = _height;
        pitch = _pitch;
        // TODO Init bimap from file
//        buffer = _buffer;
//        bitmap = new Bitmap(width, height, pitch, PixelFormat32bppARGB, (unsigned char *) buffer);
//        GdipCreateBitmapFrom
        GdipGetImageGraphicsContext(this->bitmap, &this->graphics);
        GdipSetCompositingMode(this->graphics, CompositingModeSourceOver);
        GdipSetWorldTransform(this->graphics, (GpMatrix *) calcTransform);
        clipWidth = clipHeight = -1;
    }
    // クリッピング領域変更の場合は設定しなおし
    if (_clipLeft != clipLeft ||
        _clipTop != clipTop ||
        _clipWidth != clipWidth ||
        _clipHeight != clipHeight) {
        clipLeft = _clipLeft;
        clipTop = _clipTop;
        clipWidth = _clipWidth;
        clipHeight = _clipHeight;
        GpRegion *clip{nullptr};
        Rect r{clipLeft, clipTop, clipWidth, clipHeight};
        GdipCreateRegionRectI(&r, &clip);
//        graphics->SetClip(&clip);
        GdipSetClipRegion(this->graphics, clip, CombineModeReplace);
        GdipDeleteRegion(clip);
    }
}

void
LayerExDraw::updateViewTransform() {
    calcTransform.Reset();
    calcTransform.Multiply(&transform, MatrixOrderAppend);
    calcTransform.Multiply(&viewTransform, MatrixOrderAppend);
    GdipSetWorldTransform(this->graphics, (GpMatrix *) calcTransform);
    redrawRecord();
}

/**
 * 表示トランスフォームの指定
 * @param matrix トランスフォームマトリックス
 */
void LayerExDraw::setViewTransform(/* const */ MatrixClass *trans) {
    if (!viewTransform.Equals(trans)) {
        viewTransform.Reset();
        viewTransform.Multiply(trans);
        updateViewTransform();
    }
}

void
LayerExDraw::resetViewTransform() {
    viewTransform.Reset();
    updateViewTransform();
}

void
LayerExDraw::rotateViewTransform(REAL angle) {
    viewTransform.Rotate(angle, MatrixOrderAppend);
    updateViewTransform();
}

void
LayerExDraw::scaleViewTransform(REAL sx, REAL sy) {
    viewTransform.Scale(sx, sy, MatrixOrderAppend);
    updateViewTransform();
}

void
LayerExDraw::translateViewTransform(REAL dx, REAL dy) {
    viewTransform.Translate(dx, dy, MatrixOrderAppend);
    updateViewTransform();
}

void
LayerExDraw::updateTransform() {
    calcTransform.Reset();
    calcTransform.Multiply(&transform, MatrixOrderAppend);
    calcTransform.Multiply(&viewTransform, MatrixOrderAppend);
    GdipSetWorldTransform(this->graphics, (GpMatrix *) calcTransform);
    if (metaGraphics) {
        GdipSetWorldTransform(this->metaGraphics, (GpMatrix *) transform);
    }
}

/**
 * トランスフォームの指定
 * @param matrix トランスフォームマトリックス
 */
void
LayerExDraw::setTransform(/* const */ MatrixClass *trans) {
    if (!transform.Equals(trans)) {
        transform.Reset();
        transform.Multiply(trans);
        updateTransform();
    }
}

void
LayerExDraw::resetTransform() {
    transform.Reset();
    updateTransform();
}

void
LayerExDraw::rotateTransform(REAL angle) {
    transform.Rotate(angle, MatrixOrderAppend);
    updateTransform();
}

void
LayerExDraw::scaleTransform(REAL sx, REAL sy) {
    transform.Scale(sx, sy, MatrixOrderAppend);
    updateTransform();
}

void
LayerExDraw::translateTransform(REAL dx, REAL dy) {
    transform.Translate(dx, dy, MatrixOrderAppend);
    updateTransform();
}

/**
 * 画面の消去
 * @param argb 消去色
 */
void
LayerExDraw::clear(ARGB argb) {
//    graphics->Clear(Color { argb });
    GdipGraphicsClear(this->graphics, argb);
    if (metaGraphics) {
        createRecord();
//        metaGraphics->Clear(Color{argb});
        GdipGraphicsClear(this->metaGraphics, argb);
    }
    _pUpdate(0, nullptr);
}

/**
 * パスの領域情報を取得
 * @param app 表示表現
 * @param path 描画するパス
 */
RectF
LayerExDraw::getPathExtents(const Appearance *app, const Path *path) {
    // 領域記録用
    RectF rect;

    // 描画情報を使って次々描画
    bool first = true;
    auto i = app->drawInfos.begin();
    while (i != app->drawInfos.end()) {
        if (i->info) {
            MatrixClass matrix{1, 0, 0, 1, i->ox, i->oy};
            matrix.Multiply(&calcTransform, MatrixOrderAppend);
            switch (i->type) {
                case 0: {
                    Pen *pen = (Pen *) i->info;
                    if (first) {
                        path->GetBounds(&rect, &matrix, pen);
                        first = false;
                    } else {
                        RectF r;
                        path->GetBounds(&r, &matrix, pen);
                        rect.Union(rect, rect, r);
                    }
                }
                    break;
                case 1:
                    if (first) {
                        path->GetBounds(&rect, &matrix, nullptr);
                        first = false;
                    } else {
                        RectF r;
                        path->GetBounds(&r, &matrix, nullptr);
                        rect.Union(rect, rect, r);
                    }
                    break;
            }
        }
        i++;
    }
    return rect;
}

void
LayerExDraw::draw(Graphics *graphics, const Pen *pen, const Matrix *matrix,
                  const Path *path) {
    GraphicsContainer container = graphics->BeginContainer();
    graphics->MultiplyTransform(matrix);
    graphics->SetSmoothingMode(smoothingMode);
    graphics->DrawPath(pen, path);
    graphics->EndContainer(container);
}

void
LayerExDraw::fill(Graphics *graphics, const Brush *brush, const Matrix *matrix,
                  const Path *path) {
    GraphicsContainer container = graphics->BeginContainer();
    graphics->MultiplyTransform(matrix);
    graphics->SetSmoothingMode(smoothingMode);
    graphics->FillPath(brush, path);
    graphics->EndContainer(container);
}

/**
 * パスを描画する
 * @param app 表示表現
 * @param path 描画するパス
 * @return 更新領域情報
 */
RectF LayerExDraw::_drawPath(const Appearance *app, const Path *path) {
    // 領域記録用
    RectF rect;

    // 描画情報を使って次々描画
    bool first = true;
    auto i = app->drawInfos.begin();
    while (i != app->drawInfos.end()) {
        if (i->info) {
            MatrixClass matrix{1, 0, 0, 1, i->ox, i->oy};
            switch (i->type) {
                case 0: {
                    auto *pen = (GpPen *) (Pen *) i->info;
                    draw(graphics, pen, &matrix, path);
                    if (metaGraphics) {
                        draw(metaGraphics, pen, &matrix, path);
                    }
                    matrix.Multiply(&calcTransform, MatrixOrderAppend);
                    if (first) {
                        path->GetBounds(&rect, &matrix, pen);
                        first = false;
                    } else {
                        RectF r;
                        path->GetBounds(&r, &matrix, pen);
                        rect.Union(rect, rect, r);
                    }
                }
                    break;
                case 1:
                    fill(graphics, (Brush *) i->info, &matrix, path);
                    if (metaGraphics) {
                        fill(metaGraphics, (Brush *) i->info, &matrix, path);
                    }
                    matrix.Multiply(&calcTransform, MatrixOrderAppend);
                    if (first) {
                        path->GetBounds(&rect, &matrix, nullptr);
                        first = false;
                    } else {
                        RectF r;
                        path->GetBounds(&r, &matrix, nullptr);
                        rect.Union(rect, rect, r);
                    }
                    break;
            }
        }
        i++;
    }
    updateRect(rect);
    return rect;
}

/**
 * パスの描画
 * @param app アピアランス
 * @param path パス
 */
RectF
LayerExDraw::drawPath(const Appearance *app, const Path *path) {
    return _drawPath(app, path);
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
RectF
LayerExDraw::drawArc(const Appearance *app, REAL x, REAL y, REAL width, REAL height,
                     REAL startAngle, REAL sweepAngle) {
    Path path;
    path.AddArc(x, y, width, height, startAngle, sweepAngle);
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
RectF
LayerExDraw::drawBezier(const Appearance *app, REAL x1, REAL y1, REAL x2, REAL y2, REAL x3, REAL y3,
                        REAL x4, REAL y4) {
    Path path;
    path.AddBezier(x1, y1, x2, y2, x3, y3, x4, y4);
    return _drawPath(app, &path);
}

/**
 * 連続ベジェ曲線の描画
 * @param app アピアランス
 * @param points 点の配列
 * @return 更新領域情報
 */
RectF
LayerExDraw::drawBeziers(const Appearance *app, tTJSVariant points) {
    vector<PointF> ps;
    getPoints(points, ps);
    Path path;
    path.AddBeziers(&ps[0], (int) ps.size());
    return _drawPath(app, &path);
}

/**
 * Closed cardinal spline の描画
 * @param app アピアランス
 * @param points 点の配列
 * @return 更新領域情報
 */
RectF
LayerExDraw::drawClosedCurve(const Appearance *app, tTJSVariant points) {
    vector<PointF> ps;
    getPoints(points, ps);
    Path path;
    path.AddClosedCurve(&ps[0], (int) ps.size());
    return _drawPath(app, &path);
}

/**
 * Closed cardinal spline の描画
 * @param app アピアランス
 * @param points 点の配列
 * @pram tension tension
 * @return 更新領域情報
 */
RectF
LayerExDraw::drawClosedCurve2(const Appearance *app, tTJSVariant points, REAL tension) {
    vector<PointF> ps;
    getPoints(points, ps);
    Path path;
    path.AddClosedCurve(&ps[0], (int) ps.size(), tension);
    return _drawPath(app, &path);
}

/**
 * cardinal spline の描画
 * @param app アピアランス
 * @param points 点の配列
 * @return 更新領域情報
 */
RectF
LayerExDraw::drawCurve(const Appearance *app, tTJSVariant points) {
    vector<PointF> ps;
    getPoints(points, ps);
    Path path;
    path.AddCurve(&ps[0], (int) ps.size());
    return _drawPath(app, &path);
}

/**
 * cardinal spline の描画
 * @param app アピアランス
 * @param points 点の配列
 * @parma tension tension
 * @return 更新領域情報
 */
RectF
LayerExDraw::drawCurve2(const Appearance *app, tTJSVariant points, REAL tension) {
    vector<PointF> ps;
    getPoints(points, ps);
    Path path;
    path.AddCurve(&ps[0], (int) ps.size(), tension);
    return _drawPath(app, &path);
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
RectF
LayerExDraw::drawCurve3(const Appearance *app, tTJSVariant points, int offset, int numberOfSegments,
                        REAL tension) {
    vector<PointF> ps;
    getPoints(points, ps);
    Path path;
    path.AddCurve(&ps[0], (int) ps.size(), offset, numberOfSegments, tension);
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
RectF
LayerExDraw::drawPie(const Appearance *app, REAL x, REAL y, REAL width, REAL height,
                     REAL startAngle, REAL sweepAngle) {
    Path path;
    path.AddPie(x, y, width, height, startAngle, sweepAngle);
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
RectF
LayerExDraw::drawEllipse(const Appearance *app, REAL x, REAL y, REAL width, REAL height) {
    Path path;
    path.AddEllipse(x, y, width, height);
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
RectF LayerExDraw::drawLine(
        const Appearance *app, REAL x1, REAL y1, REAL x2, REAL y2
) {
    Path *path{nullptr};
    GdipCreatePath(FillModeAlternate, &path);
    GdipAddPathLine(path, x1, y1, x2, y2);
    RectF r = _drawPath(app, path);
    GdipDeletePath(path);
    return r;
}

/**
 * 連続線分の描画
 * @param app アピアランス
 * @param points 点の配列
 * @return 更新領域情報
 */
RectF
LayerExDraw::drawLines(const Appearance *app, tTJSVariant points) {
    vector<PointF> ps;
    getPoints(points, ps);
    Path path;
    path.AddLines(&ps[0], (int) ps.size());
    return _drawPath(app, &path);
}

/**
 * 多角形の描画
 * @param app アピアランス
 * @param points 点の配列
 * @return 更新領域情報
 */
RectF
LayerExDraw::drawPolygon(const Appearance *app, tTJSVariant points) {
    vector<PointF> ps;
    getPoints(points, ps);
    Path path;
    path.AddPolygon(&ps[0], (int) ps.size());
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
RectF
LayerExDraw::drawRectangle(const Appearance *app, REAL x, REAL y, REAL width, REAL height) {
    Path path;
    RectF rect(x, y, width, height);
    path.AddRectangle(rect);
    return _drawPath(app, &path);
}

/**
 * 複数矩形の描画
 * @param app アピアランス
 * @param rects 矩形情報の配列
 * @return 更新領域情報
 */
RectF
LayerExDraw::drawRectangles(const Appearance *app, tTJSVariant rects) {
    vector<RectF> rs;
    getRects(rects, rs);
    Path path;
    path.AddRectangles(&rs[0], (int) rs.size());
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
RectF
LayerExDraw::drawPathString(const FontInfo *font, const Appearance *app, REAL x, REAL y,
                            const tjs_char *text) {
    if (font->getSelfPathDraw())
        return drawPathString2(font, app, x, y, text);

    // 文字列のパスを準備
    Path path;
    path.AddString(text, -1, font->fontFamily, font->style, font->emSize, PointF(x, y),
                   StringFormat::GenericDefault());
    return _drawPath(app, &path);
}

static void transformRect(Matrix &calcTransform, RectF &rect) {
    PointF points[4]; // 元座標値
    points[0].X = rect.X;
    points[0].Y = rect.Y;
    points[1].X = rect.X + rect.Width;
    points[1].Y = rect.Y;
    points[2].X = rect.X;
    points[2].Y = rect.Y + rect.Height;
    points[3].X = rect.X + rect.Width;
    points[3].Y = rect.Y + rect.Height;
    // 描画領域を再計算
    calcTransform.TransformPoints(points, 4);
    REAL minx = points[0].X;
    REAL maxx = points[0].X;
    REAL miny = points[0].Y;
    REAL maxy = points[0].Y;
    for (int i = 1; i < 4; i++) {
        if (points[i].X < minx) { minx = points[i].X; }
        if (points[i].X > maxx) { maxx = points[i].X; }
        if (points[i].Y < miny) { miny = points[i].Y; }
        if (points[i].Y > maxy) { maxy = points[i].Y; }
    }
    rect.X = minx;
    rect.Y = miny;
    rect.Width = maxx - minx;
    rect.Height = maxy - miny;
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
RectF
LayerExDraw::drawString(const FontInfo *font, const Appearance *app, REAL x, REAL y,
                        const tjs_char *text) {
    if (font->getSelfPathDraw())
        return drawPathString2(font, app, x, y, text);

    graphics->SetTextRenderingHint(textRenderingHint);
    if (metaGraphics) {
        metaGraphics->SetTextRenderingHint(textRenderingHint);
    }

    // 領域記録用
    RectF rect;
    // 描画フォント
    Font f(font->fontFamily, font->emSize, font->style, UnitPixel);

    // 描画情報を使って次々描画
    bool first = true;
    vector<Appearance::DrawInfo>::const_iterator i = app->drawInfos.begin();
    while (i != app->drawInfos.end()) {
        if (i->info) {
            if (i->type == 1) { // ブラシのみ
                Brush *brush = (Brush *) i->info;
                PointF p(x + i->ox, y + i->oy);
                graphics->DrawString(text, -1, &f, p, StringFormat::GenericDefault(), brush);
                if (metaGraphics) {
                    metaGraphics->DrawString(text, -1, &f, p, StringFormat::GenericDefault(),
                                             brush);
                }
                // 更新領域計算
                if (first) {
                    graphics->MeasureString(text, -1, &f, p, StringFormat::GenericDefault(), &rect);
                    transformRect(calcTransform, rect);
                    first = false;
                } else {
                    RectF r;
                    graphics->MeasureString(text, -1, &f, p, StringFormat::GenericDefault(), &r);
                    transformRect(calcTransform, r);
                    rect.Union(rect, rect, r);
                }
                break;
            }
        }
        i++;
    }
    updateRect(rect);
    return rect;
}

/**
 * 文字列の描画領域情報の取得
 * @param font フォント
 * @param text 描画テキスト
 * @return 描画領域情報
 */
RectF
LayerExDraw::measureString(const FontInfo *font, const tjs_char *text) {
    if (font->getSelfPathDraw())
        return measureString2(font, text);

    RectF rect;
    graphics->SetTextRenderingHint(textRenderingHint);
    Font f(font->fontFamily, font->emSize, font->style, UnitPixel);
    graphics->MeasureString(text, -1, &f, PointF(0, 0), StringFormat::GenericDefault(), &rect);
    return rect;
}

/**
 * 文字列に外接する領域情報の取得
 * @param font フォント
 * @param text 描画テキスト
 * @return 領域情報の辞書 left, top, width, height
 */
RectF
LayerExDraw::measureStringInternal(const FontInfo *font, const tjs_char *text) {
    if (font->getSelfPathDraw())
        return measureStringInternal2(font, text);

    RectF rect;
    graphics->SetTextRenderingHint(textRenderingHint);
    Font f(font->fontFamily, font->emSize, font->style, UnitPixel);
    graphics->MeasureString(text, -1, &f, PointF(0, 0), StringFormat::GenericDefault(), &rect);
    CharacterRange charRange(0, INT(wcslen(text)));
    StringFormat stringFormat = StringFormat::GenericDefault();
    stringFormat.SetMeasurableCharacterRanges(1, &charRange);
    Region region;
    graphics->MeasureCharacterRanges(text, -1, &f, rect, &stringFormat, 1, &region);
    RectF regionBounds;
    region.GetBounds(&regionBounds, graphics);
    return regionBounds;
}

/**
 * 画像の描画。コピー先は元画像の Bounds を配慮した位置、サイズは Pixel 指定になります。
 * @param x コピー先原点
 * @param y  コピー先原点
 * @param src コピー元画像
 * @return 更新領域情報
 */
RectF
LayerExDraw::drawImage(REAL x, REAL y, ImageClass *src) {
    RectF rect;
    if (src) {
        RectF *bounds = getBounds(src);
        rect = drawImageRect(x + bounds->X, y + bounds->Y, src, 0, 0, bounds->Width,
                             bounds->Height);
        delete bounds;
        updateRect(rect);
    }
    return rect;
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
RectF LayerExDraw::drawImageRect(
        REAL dleft, REAL dtop, ImageClass *src,
        REAL sleft, REAL stop, REAL swidth, REAL sheight
) {
    return drawImageAffine(src, sleft, stop, swidth, sheight, true, 1, 0, 0, 1, dleft, dtop);
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
RectF
LayerExDraw::drawImageStretch(REAL dleft, REAL dtop, REAL dwidth, REAL dheight, ImageClass *src,
                              REAL sleft, REAL stop, REAL swidth, REAL sheight) {
    return drawImageAffine(src, sleft, stop, swidth, sheight, true, dwidth / swidth, 0, 0,
                           dheight / sheight, dleft, dtop);
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
RectF
LayerExDraw::drawImageAffine(ImageClass *src, REAL sleft, REAL stop, REAL swidth, REAL sheight,
                             bool affine, REAL A, REAL B, REAL C, REAL D, REAL E, REAL F) {
    RectF rect;
    if (src) {
        PointF points[4]; // 元座標値
        if (affine) {
#define AFFINEX(x, y) A*x+C*y+E
#define AFFINEY(x, y) B*x+D*y+F
            points[0].X = AFFINEX(0, 0);
            points[0].Y = AFFINEY(0, 0);
            points[1].X = AFFINEX(swidth, 0);
            points[1].Y = AFFINEY(swidth, 0);
            points[2].X = AFFINEX(0, sheight);
            points[2].Y = AFFINEY(0, sheight);
            points[3].X = AFFINEX(swidth, sheight);
            points[3].Y = AFFINEY(swidth, sheight);
        } else {
            points[0].X = A;
            points[0].Y = B;
            points[1].X = C;
            points[1].Y = D;
            points[2].X = E;
            points[2].Y = F;
            points[3].X = C - A + E;
            points[3].Y = D - B + F;
        }
        graphics->DrawImage(src, points, 3, sleft, stop, swidth, sheight, UnitPixel, nullptr,
                            nullptr,
                            nullptr);
        if (metaGraphics) {
            metaGraphics->DrawImage(src, points, 3, sleft, stop, swidth, sheight, UnitPixel,
                                    nullptr,
                                    nullptr, nullptr);
        }

        // 描画領域を取得
        calcTransform.TransformPoints(points, 4);
        REAL minx = points[0].X;
        REAL maxx = points[0].X;
        REAL miny = points[0].Y;
        REAL maxy = points[0].Y;
        for (int i = 1; i < 4; i++) {
            if (points[i].X < minx) { minx = points[i].X; }
            if (points[i].X > maxx) { maxx = points[i].X; }
            if (points[i].Y < miny) { miny = points[i].Y; }
            if (points[i].Y > maxy) { maxy = points[i].Y; }
        }
        rect.X = minx;
        rect.Y = miny;
        rect.Width = maxx - minx;
        rect.Height = maxy - miny;

        updateRect(rect);
    }
    return rect;
}

void LayerExDraw::createRecord() {
    destroyRecord();
    if ((metaBuffer = ::GlobalAlloc(GMEM_MOVEABLE, 0))) {
        if (::CreateStreamOnHGlobal(metaBuffer, FALSE, &metaStream) == S_OK) {
            metafile = new Metafile(metaStream, metaHDC, EmfTypeEmfPlusOnly);
            metaGraphics = new Graphics(metafile);
            metaGraphics->SetCompositingMode(CompositingModeSourceOver);
            metaGraphics->SetTransform(&transform);
        }
    }
}

/**
 * 記録情報の破棄
 */
void
LayerExDraw::destroyRecord() {
    delete metaGraphics;
    metaGraphics = nullptr;
    delete metafile;
    metafile = nullptr;
    if (metaStream) {
        metaStream->Release();
        metaStream = nullptr;
    }
    if (metaBuffer) {
        ::GlobalFree(metaBuffer);
        metaBuffer = nullptr;
    }
}


/**
 * @param record 描画内容を記録するかどうか
 */
void
LayerExDraw::setRecord(bool record) {
    if (record) {
        if (!metafile) {
            createRecord();
        }
    } else {
        if (metafile) {
            destroyRecord();
        }
    }
}

bool LayerExDraw::redraw(ImageClass *image) {
    if (image) {
        RectF *bounds = getBounds(image);
        if (metaGraphics) {
            GdipGraphicsClear(this->metaGraphics, 0);
            GdipResetWorldTransform(this->metaGraphics);
            GdipDrawImageRect(
                    this->metaGraphics, (ImageClass *) image,
                    bounds->X, bounds->Y, bounds->Width, bounds->Height
            );
            GpMatrix *tmp{nullptr};
            GdipSetWorldTransform(this->metaGraphics, tmp);
            transform = MatrixClass{tmp};
        }
        GdipGraphicsClear(this->metaGraphics, 0);
        GdipSetWorldTransform(this->metaGraphics, (GpMatrix *) viewTransform);

        graphics->DrawImage(image, bounds->X, bounds->Y, bounds->Width, bounds->Height);
        graphics->SetTransform(&calcTransform);
        delete bounds;
        _pUpdate(0, nullptr);
        return true;
    }

    return false;
}

/**
 * 記録内容を ImageClass として取得
 * @return 成功したら true
 */
ImageClass *LayerExDraw::getRecordImage() {
    ImageClass *image = nullptr;
    if (metafile) {
        // メタ情報を取得するには一度閉じる必要がある
        if (metaGraphics) {
            delete metaGraphics;
            metaGraphics = nullptr;
        }

        //閉じたあと継続するための再描画先を別途構築
        HGLOBAL oldBuffer = metaBuffer;
        metaBuffer = nullptr;
        createRecord();

        // 再描画
        if (oldBuffer) {
            IStream *pStream = nullptr;
            if (::CreateStreamOnHGlobal(oldBuffer, FALSE, &pStream) == S_OK) {
                image = ImageClass::FromStream(pStream, false);
                if (image) {
                    redraw(image);
                }
                pStream->Release();
            }
            ::GlobalFree(oldBuffer);
        }
    }
    return image;
}

/**
 * 記録内容の現在の解像度での再描画
 */
bool LayerExDraw::redrawRecord() {
    // 再描画処理
    ImageClass *image = getRecordImage();
    if (image) {
        delete image;
        return true;
    }
    return false;
}

/**
 * 記録内容の保存
 * @param filename 保存ファイル名
 * @return 成功したら true
 */
bool
LayerExDraw::saveRecord(const tjs_char *filename) {
    bool ret = false;
    if (metafile) {
        // メタ情報を取得するには一度閉じる必要がある
        delete metaGraphics;
        metaGraphics = nullptr;
        ULONG size;
        // ファイルに書き出す
        if (metaBuffer && (size = (ULONG) ::GlobalSize(metaBuffer)) > 0) {
            IStream *out = TVPCreateIStream(filename, TJS_BS_WRITE);
            if (out) {
                void *pBuffer = ::GlobalLock(metaBuffer);
                if (pBuffer) {
                    ret = (out->Write(pBuffer, size, &size) == S_OK);
                    ::GlobalUnlock(metaBuffer);
                }
                out->Release();
            }
        }
        // 再描画処理
        ImageClass *image = getRecordImage();
        if (image) {
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
bool
LayerExDraw::loadRecord(const tjs_char *filename) {
    ImageClass *image;
    if (filename && (image = loadImage(filename))) {
        createRecord();
        redraw(image);
        delete image;
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
void
LayerExDraw::getGlyphOutline(const FontInfo *fontInfo, PointF &offset, Path *path,
                             UINT glyph) {
    static const MAT2 mat = {{0, 1},
                             {0, 0},
                             {0, 0},
                             {0, 1}};

    GLYPHMETRICS gm;

    DWORD flags = GGO_BEZIER;
    // フィッティング指定が無ければ UNHINTEDにする。xs
    if (!(textRenderingHint & 1))
        flags |= GGO_UNHINTED;

    int size = GetGlyphOutlineW(metaHDC,
                                glyph,
                                flags, //  | GGO_GLYPH_INDEX,
                                &gm,
                                0,
                                nullptr,
                                &mat
    );
    char *buffer = nullptr;
    if (size > 0) {
        buffer = new char[size];
        int result = GetGlyphOutlineW(metaHDC,
                                      glyph,
                                      flags, //  | GGO_GLYPH_INDEX,
                                      &gm,
                                      size,
                                      buffer,
                                      &mat
        );
        if (result <= 0) {
            delete[] buffer;
            return;
        }
    } else {
        GetGlyphOutlineW(metaHDC,
                         glyph,
                         GGO_METRICS,
                         &gm,
                         0,
                         nullptr,
                         &mat
        );
    }

    int index = 0;
    PointF aOffset = offset;
    aOffset.Y += fontInfo->getAscent();

    while (index < size) {
        TTPOLYGONHEADER *header = (TTPOLYGONHEADER * )(buffer + index);
        int endCurve = index + header->cb;
        index += sizeof(TTPOLYGONHEADER);
        PointF p0 = ToPointF(&header->pfxStart) + aOffset;
        while (index < endCurve) {
            TTPOLYCURVE *hcurve = (TTPOLYCURVE * )(buffer + index);
            index += 2 * sizeof(WORD);
            POINTFX *points = (POINTFX *) (buffer + index);
            index += hcurve->cpfx * sizeof(POINTFX);
            std::vector<PointF> pts(1 + hcurve->cpfx);
            pts[0] = p0;
            for (int i = 0; i < hcurve->cpfx; i++)
                pts[i + 1] = ToPointF(points + i) + aOffset;
            p0 = pts[pts.size() - 1];
            switch (hcurve->wType) {
                case TT_PRIM_LINE:
                    path->AddLines(&pts[0], int(pts.size()));
                    break;

                case TT_PRIM_QSPLINE:
                    TVPAddLog(ttstr(TJS_W("qspline")));
                    break;

                case TT_PRIM_CSPLINE:
                    path->AddBeziers(&pts[0], int(pts.size()));
                    break;
            }
        }

        path->CloseFigure();
    }

    offset.X += gm.gmCellIncX;

    delete[] buffer;
}

/*
 * テキストアウトラインの取得
 * @param font フォント
 * @param offset オフセット
 * @param path グリフを書き出すパス
 * @param text 描画するテキスト
 */
void
LayerExDraw::getTextOutline(const FontInfo *fontInfo, PointF &offset, GpPath *path,
                            ttstr text) {
    if (metaHDC == nullptr)
        return;

    if (text.IsEmpty())
        return;

    LOGFONT font;
    memset(&font, 0, sizeof(font));
    font.lfHeight = -(LONG(fontInfo->emSize));
    font.lfWeight = (fontInfo->style & 1) ? FW_BOLD : FW_REGULAR;
    font.lfItalic = fontInfo->style & 2;
    font.lfUnderline = fontInfo->style & 4;
    font.lfStrikeOut = fontInfo->style & 8;
    font.lfCharSet = DEFAULT_CHARSET;
    wcscpy_s(font.lfFaceName, fontInfo->familyName.c_str());

    HFONT hFont = CreateFontIndirect(&font);
    HGDIOBJ hOldFont = SelectObject(metaHDC, hFont);

    for (tjs_int i = 0; i < text.GetLen(); i++) {
        this->getGlyphOutline(fontInfo, offset, path, text[i]);
    }

    SelectObject(metaHDC, hOldFont);
    DeleteObject(hFont);
}

/**
 * 文字列の描画更新領域情報の取得(OpenTypeフォント対応)
 * @param font フォント
 * @param text 描画テキスト
 * @return 更新領域情報の辞書 left, top, width, height
 */
RectF
LayerExDraw::measureString2(const FontInfo *font, const tjs_char *text) {
    // 文字列のパスを準備
    Path path;
    PointF offset(0, 0);
    this->getTextOutline(font, offset, &path, text);
    RectF result;
    path.GetBounds(&result, nullptr, nullptr);
    result.X = 0;
    result.Y = 0;
    result.Width += REAL(0.167 * font->emSize * 2);
    result.Height = REAL(font->getLineSpacing() * 1.124);
    return result;
}

/**
 * 文字列に外接する領域情報の取得(OpenTypeのPostScriptフォント対応)
 * @param font フォント
 * @param text 描画テキスト
 * @return 更新領域情報の辞書 left, top, width, height
 */
RectF
LayerExDraw::measureStringInternal2(const FontInfo *font, const tjs_char *text) {
    // 文字列のパスを準備
    Path path;
    PointF offset(0, 0);
    this->getTextOutline(font, offset, &path, text);
    RectF result;
    path.GetBounds(&result, nullptr, nullptr);
    result.X = REAL(LONG(0.167 * font->emSize));
    result.Y = 0;
    result.Height = font->getLineSpacing();
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
RectF
LayerExDraw::drawPathString2(const FontInfo *font, const Appearance *app, REAL x, REAL y,
                             const tjs_char *text) {
    // 文字列のパスを準備
    Path path;
    PointF offset(x + LONG(0.167 * font->emSize) - 0.5f, y - 0.5f);
    this->getTextOutline(font, offset, &path, text);
    RectF result = _drawPath(app, &path);
    result.X = x;
    result.Y = y;
    result.Width += REAL(0.167 * font->emSize * 2);
    result.Height = REAL(font->getLineSpacing() * 1.124);
    return result;
}

static bool getEncoder(const tjs_char *mimeType, CLSID *pClsid) {
    UINT num = 0, size = 0;
    ::GetImageEncodersSize(&num, &size);
    if (size > 0) {
        ImageCodecInfo *pImageCodecInfo = (ImageCodecInfo *) malloc(size);
        if (pImageCodecInfo) {
            GetImageEncoders(num, size, pImageCodecInfo);
            for (UINT j = 0; j < num; ++j) {
                if (wcscmp(pImageCodecInfo[j].MimeType, mimeType) == 0) {
                    *pClsid = pImageCodecInfo[j].Clsid;
                    free(pImageCodecInfo);
                    return true;
                }
            }
            free(pImageCodecInfo);
        }
    }
    return false;
}

/**
 * エンコードパラメータ情報の参照用
 */
class EncoderParameterGetter : public tTJSDispatch /** EnumMembers 用 */
{
public:
    struct EncoderInfo {
        const char *name{};
        GUID guid{};
        long value{};

        EncoderInfo(
                const char *name, GUID guid, long value
        ) : name(name), guid(guid),
            value(value) {};

        EncoderInfo() = default;
    } infos[7];

    EncoderParameters *params;

    EncoderParameterGetter() {
        infos[0] = EncoderInfo("compression", GdipEncoderCompression, -1);
        infos[1] = EncoderInfo("scanmethod", GdipEncoderScanMethod, -1);
        infos[2] = EncoderInfo("version", GdipEncoderVersion, -1);
        infos[3] = EncoderInfo("render", GdipEncoderRenderMethod, -1);
        infos[4] = EncoderInfo("tansform", GdipEncoderTransformation, -1);
        infos[5] = EncoderInfo("quality", GdipEncoderQuality, -1);
        infos[6] = EncoderInfo("depth", GdipEncoderColorDepth, 24);
        params = (EncoderParameters *) malloc(
                sizeof(EncoderParameters) + 6 * sizeof(EncoderParameter));
    };

    ~EncoderParameterGetter() override {
        delete params;
    }

    void checkResult() {
        int n = 0;
        for (auto &info: infos) {
            if (info.value >= 0) {
                params->Parameter[n].Guid = info.guid;
                params->Parameter[n].Type = EncoderParameterValueTypeLong;
                params->Parameter[n].NumberOfValues = 1;
                params->Parameter[n].Value = &info.value;
                n++;
            }
        }
        params->Count = n;
    }

    virtual tjs_error TJS_INTF_METHOD FuncCall( // function invocation
            tjs_uint32 flag,            // calling flag
            const tjs_char *membername,// member name ( nullptr for a default member )
            tjs_uint32 *hint,            // hint for the member name (in/out)
            tTJSVariant *result,        // result
            tjs_int numparams,            // number of parameters
            tTJSVariant **param,        // parameters
            iTJSDispatch2 *objthis        // object as "this"
    ) {
        if (numparams > 1) {
            tTVInteger flag = param[1]->AsInteger();
            if (!(flag & TJS_HIDDENMEMBER)) {
                ttstr name = *param[0];
                for (auto &info: infos) {
                    if (name == info.name) {
                        info.value = (tjs_int) *param[1];
                        break;
                    }
                }
            }
        }
        if (result) {
            *result = true;
        }
        return TJS_S_OK;
    }
};

/**
 * 画像の保存
 */
tjs_error LayerExDraw::saveImage(
        tTJSVariant *result, tjs_int numparams,
        tTJSVariant **param, iTJSDispatch2 *objthis
) {
    // rawcallback だと hook がきいてない模様
    LayerExDraw *self = ncbInstanceAdaptor<LayerExDraw>::GetNativeInstance(objthis);
    if (!self) {
        self = new LayerExDraw(objthis);
        ncbInstanceAdaptor<LayerExDraw>::SetNativeInstance(objthis, self);
    }
    self->reset();

    if (numparams < 1) return TJS_E_BADPARAMCOUNT;
    ttstr filename = TVPNormalizeStorageName(param[0]->AsStringNoAddRef());
    TVPGetLocalName(filename);
    ttstr type;
    if (numparams > 1) {
        type = *param[1];
    } else {
        type = TJS_W("image/bmp");
    }
    CLSID clsid;
    if (!getEncoder(type.c_str(), &clsid)) {
        TVPThrowExceptionMessage(TJS_W("unknown format:%1"), type);
    }

    auto *caller = new EncoderParameterGetter();
    // パラメータ辞書がある
    if (numparams > 2 && param[2]->Type() == tvtObject) {
        tTJSVariantClosure closure(caller);
        param[2]->AsObjectClosureNoAddRef().EnumMembers(TJS_IGNOREPROP, &closure, nullptr);
    }
    caller->checkResult();
    static_assert(sizeof(tjs_char) == sizeof(WCHAR), "saveImage Sizes differ!");
    const auto *n = reinterpret_cast<const WCHAR *>(filename.c_str());
    GpStatus ret = GdipSaveImageToFile(self->bitmap, n, &clsid, caller->params);
    caller->Release();

    if (result) {
        *result = ret == 0;
    }
    return TJS_S_OK;
}

static ARGB getColor(GpBitmap *bitmap, int x, int y) {
    ARGB c;
    GdipBitmapGetPixel(bitmap, x, y, &c);
    return c;
}

tTJSVariant
LayerExDraw::getColorRegionRects(ARGB color) {
    iTJSDispatch2 *array = TJSCreateArrayObject();
    if (bitmap) {
        uint width{};
        uint height{};
        GdipGetImageWidth(this->bitmap, &width);
        GdipGetImageHeight(this->bitmap, &height);
        GpRegion *region{nullptr};
        GdipCreateRegion(&region);
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {
                if (getColor(bitmap, i, j) == color) {
                    int x0 = i++;
                    while (i < width && getColor(bitmap, i, j) == color) i++;
                    Rect r{x0, j, i - x0, 1};
                    GdipCombineRegionRectI(region, &r, CombineModeReplace);
                }
            }
        }

        // 矩形一覧取得
        GpMatrix matrix;
        int count{};
        GdipGetRegionScansCount(region, &count, &matrix);
        if (count > 0) {
            auto *rects = new RectF[count];
            GdipGetRegionScans(region, rects, &count, &matrix);
            for (int i = 0; i < count; i++) {
                RectF *rect = &rects[i];
                tTJSVariant x(rect->X);
                tTJSVariant y(rect->Y);
                tTJSVariant w(rect->Width);
                tTJSVariant h(rect->Height);
                tTJSVariant *points[4] = {&x, &y, &w, &h};
                static tjs_uint32 pushHint;
                iTJSDispatch2 *rarray = TJSCreateArrayObject();
                rarray->FuncCall(0, TJS_W("push"), &pushHint, 0, 4, points, rarray);
                tTJSVariant var(rarray, rarray), *param = &var;
                rarray->Release();
                array->FuncCall(0, TJS_W("push"), &pushHint, 0, 1, &param, array);
            }
            delete[] rects;
        }
        GdipDeleteRegion(region);
    }
    tTJSVariant ret(array, array);
    array->Release();
    return ret;
}
