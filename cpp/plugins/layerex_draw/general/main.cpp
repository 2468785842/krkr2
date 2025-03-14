#define NCB_MODULE_NAME TJS_W("layerExDraw.dll")

#include "ncbind.hpp"

#include "LayerExDraw.hpp"

extern void initGdiPlus();

extern void deInitGdiPlus();

extern ImageClass *loadImage(const tjs_char *name);

extern RectFClass *getBounds(ImageClass *image);

// ----------------------------------------------------------------
// 実体型の登録
// 数値パラメータ系は配列か辞書を使えるような特殊コンバータを構築
// ----------------------------------------------------------------

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
        return obj->IsInstanceOf(0, nullptr, nullptr, TJS_W("Array"), obj) ==
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
            inst->methodname(&var);                                            \
            return var;                                                        \
        }                                                                      \
    };                                                                         \
    Property(TJS_W(#name), &AutoProp_##name::ProxyGet, (int)0, Proxy)

// ------------------------------------------------------
// 型コンバータ登録
// ------------------------------------------------------

NCB_TYPECONV_CAST_INTEGER(GpStatus);
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
                    dst = PointF{ (REAL)info.getRealValue(0),
                                  (REAL)info.getRealValue(1) };
                } else {
                    dst = PointF{ (REAL)info.getRealValue(TJS_W("x")),
                                  (REAL)info.getRealValue(TJS_W("y")) };
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

NCB_SET_CONVERTOR_DST(PointFClass, PointFConvertor);
NCB_REGISTER_SUBCLASS_DELAY(PointFClass) {
    NCB_CONSTRUCTOR((REAL, REAL));
    NCB_MEMBER_PROPERTY(x, REAL, X);
    NCB_MEMBER_PROPERTY(y, REAL, Y);
    NCB_METHOD(Equals);
}

PointFClass getPoint(const tTJSVariant &var) {
    PointFConvertor<PointF> conv{};
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
                    dst = RectF{ (REAL)info.getRealValue(0),
                                 (REAL)info.getRealValue(1),
                                 (REAL)info.getRealValue(2),
                                 (REAL)info.getRealValue(3) };
                } else {
                    dst = RectF{ (REAL)info.getRealValue(TJS_W("x")),
                                 (REAL)info.getRealValue(TJS_W("y")),
                                 (REAL)info.getRealValue(TJS_W("width")),
                                 (REAL)info.getRealValue(TJS_W("height")) };
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

NCB_SET_CONVERTOR_DST(RectFClass, RectFConvertor);
NCB_REGISTER_SUBCLASS_DELAY(RectFClass) {
    NCB_CONSTRUCTOR((REAL, REAL, REAL, REAL));
    NCB_MEMBER_PROPERTY(x, REAL, X);
    NCB_MEMBER_PROPERTY(y, REAL, Y);
    NCB_MEMBER_PROPERTY(width, REAL, Width);
    NCB_MEMBER_PROPERTY(height, REAL, Height);
    NCB_PROPERTY_RO(left, GetLeft);
    NCB_PROPERTY_RO(top, GetTop);
    NCB_PROPERTY_RO(right, GetRight);
    NCB_PROPERTY_RO(bottom, GetBottom);
    NCB_ARG_PROPERTY_RO(location, PointFClass, GetLocation);
    NCB_ARG_PROPERTY_RO(bounds, RectFClass, GetBounds);
    NCB_METHOD(Clone);
    // XXX	NCB_METHOD_DETAIL(Contains, Class, BOOL, Class::Contains,
    // (REAL,REAL));
    //	NCB_METHOD_DETAIL(ContainsPoint, Class, BOOL, Class::Contains,
    //(const
    // PointF&) const); 	NCB_METHOD_DETAIL(ContainsRect, Class,
    // BOOL, Class::Contains, (const RectF&));
    NCB_METHOD(Equals);
    NCB_METHOD_DETAIL(Inflate, Class, void, Class::Inflate, (REAL, REAL));
    NCB_METHOD_DETAIL(InflatePoint, Class, void, Class::Inflate,
                      (const PointFClass &));
    // XXX	NCB_METHOD_DETAIL(Intersect, Class, BOOL,
    // Class::Intersect, (const Rect&));
    NCB_METHOD(IntersectsWith);
    NCB_METHOD(IsEmptyArea);
    NCB_METHOD_DETAIL(Offset, Class, void, Class::Offset, (REAL, REAL));
    // XXX	NCB_METHOD_DETAIL(OffsetPoint, Class, void, Class::Offset,
    // (const Point&));
    NCB_METHOD(Union);
}

RectFClass getRect(const tTJSVariant &var) {
    RectFConvertor<RectF> conv;
    RectF ret;
    conv(ret, var);
    return ret;
}

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
    GdipWrapper() : obj(nullptr) {}

    // 関数の帰り値としてのオブジェクト生成時用。
    // そのまま渡されたポインタを使う
    GdipWrapper(GdipClassT *obj) : obj(obj) {}

    // コピーコンストラクタ
    // 内蔵オブジェクトは Cloneする
    GdipWrapper(const GdipWrapper &orig) : obj(nullptr) {
        if(orig.obj) {
            obj = orig.obj->Clone();
        }
    }

    // デストラクタ
    ~GdipWrapper() {
        if(obj) {
            delete obj;
        }
    }

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
                dst = NULL;
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
struct MatrixConvertor : public GdipTypeConvertor<MatrixClass> {

    void operator()(GdipClassP &dst, const tTJSVariant &src) override {
        WrapperT *obj;
        if(src.Type() == tvtObject) {
            if((obj = AdaptorT::GetNativeInstance(src.AsObjectNoAddRef()))) {
                dst = obj->getGdipObject();
            } else {
                ncbPropAccessor info(src);
                if(IsArray(src)) {
                    result = new MatrixClass{
                        (REAL)info.getRealValue(0), (REAL)info.getRealValue(1),
                        (REAL)info.getRealValue(2), (REAL)info.getRealValue(3),
                        (REAL)info.getRealValue(4), (REAL)info.getRealValue(5)
                    };
                } else {
                    result =
                        new MatrixClass{ (REAL)info.getRealValue(TJS_W("m11")),
                                         (REAL)info.getRealValue(TJS_W("m12")),
                                         (REAL)info.getRealValue(TJS_W("m21")),
                                         (REAL)info.getRealValue(TJS_W("m22")),
                                         (REAL)info.getRealValue(TJS_W("dx")),
                                         (REAL)info.getRealValue(TJS_W("dy")) };
                }
                dst = result;
            }
        } else {
            // FIXME: nullptr danger!!
            dst = NULL;
        }
    }
};

static tjs_error MatrixFactory(GdipWrapper<MatrixClass> **result,
                               tjs_int numparams, tTJSVariant **params,
                               iTJSDispatch2 *objthis) {
    MatrixClass *matrix = nullptr;
    RectF *rect = nullptr;
    PointF *point = nullptr;
    if(numparams == 0) {
        matrix = new MatrixClass();
    } else if(numparams == 2 &&
              (params[0]->Type() == tvtObject &&
               (rect = ncbInstanceAdaptor<RectF>::GetNativeInstance(
                    params[0]->AsObjectNoAddRef()))) &&
              (params[1]->Type() == tvtObject &&
               (point = ncbInstanceAdaptor<PointF>::GetNativeInstance(
                    params[0]->AsObjectNoAddRef())))) {
        matrix = new MatrixClass(*rect, *point);
    } else if(numparams == 6) {
        matrix = new MatrixClass(
            (REAL)params[0]->AsReal(), (REAL)params[1]->AsReal(),
            (REAL)params[2]->AsReal(), (REAL)params[3]->AsReal(),
            (REAL)params[4]->AsReal(), (REAL)params[5]->AsReal());
    } else {
        return TJS_E_INVALIDPARAM;
    }
    *result = new GdipWrapper<MatrixClass>(matrix);
    return TJS_S_OK;
}

NCB_REGISTER_GDIP_SUBCLASS2(MatrixClass, MatrixConvertor)
Factory(MatrixFactory);
NCB_GDIP_METHOD(OffsetX);
NCB_GDIP_METHOD(OffsetY);
NCB_GDIP_METHOD(Equals);
// NCB_GDIP_METHOD(getElements); // 配列を返す
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
//	NCB_GDIP_METHOD_DETAIL(TransformPoints, Class, Status,
// TransformPoints, (PointF*, INT)); XXX 引数が配列
// NCB_GDIP_METHOD_DETAIL(TransformVectors,
// Class, Status, TransformVectors, (PointF*, INT)); XXX 引数が配列
NCB_GDIP_METHOD(Translate);
}

// ------------------------------------------------------- GpImage

/**
 * イメージ用コンバータ
 * 文字列からも変更可能
 */
template <typename>
struct ImageConvertor : public GdipTypeConvertor<ImageClass> {
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
                    // FIXME: nullptr danger!!
                    dst = NULL;
                }
            }
        } else if(src.Type() == tvtString) { // 文字列から生成
            dst = result = loadImage(src.GetString());
        } else {
            // FIXME: nullptr danger!!
            dst = NULL;
        }
    }
};

static tjs_error ImageFactory(GdipWrapper<ImageClass> **result,
                              tjs_int numparams, tTJSVariant **params,
                              iTJSDispatch2 *objthis) {
    if(numparams == 0) {
        *result = new GdipWrapper<ImageClass>();
        return TJS_S_OK;
    } else if(numparams > 0 && params[0]->Type() == tvtString) {
        ImageClass *image = loadImage(params[0]->GetString());
        if(image) {
            *result = new GdipWrapper<ImageClass>(image);
            return TJS_S_OK;
        } else {
            TVPThrowExceptionMessage(TJS_W("cannot open:%1"), *params[0]);
        }
    }
    return TJS_E_INVALIDPARAM;
}

static void ImageLoad(GdipWrapper<ImageClass> *obj, const tjs_char *filename) {
    ImageClass *image = loadImage(filename);
    if(image) {
        obj->setGdipObject(image);
    } else {
        TVPThrowExceptionMessage(TJS_W("cannot open:%1"), ttstr(filename));
    }
}

static tTJSVariant ImageClone(GdipWrapper<ImageClass> *obj) {
    typedef GdipWrapper<ImageClass> WrapperT;
    typedef ncbInstanceAdaptor<ImageClass> AdaptorT;
    tTJSVariant ret;
    ImageClass *src = obj->getGdipObject();
    if(src) {
        ImageClass *newimage = src->Clone();
        iTJSDispatch2 *adpobj =
            AdaptorT::CreateAdaptor((ImageClass *)new WrapperT(newimage));
        if(adpobj) {
            ret = tTJSVariant(adpobj, adpobj);
            adpobj->Release();
        } else {
            delete newimage;
        }
    }
    return ret;
}

static tTJSVariant ImageBounds(GdipWrapper<ImageClass> *obj) {
    typedef ncbInstanceAdaptor<RectF> AdaptorT;
    tTJSVariant ret;
    ImageClass *src = obj->getGdipObject();
    if(src) {
        RectFClass *bounds = getBounds(src);
        iTJSDispatch2 *adpobj = AdaptorT::CreateAdaptor(bounds);
        if(adpobj) {
            ret = tTJSVariant(adpobj, adpobj);
            adpobj->Release();
        } else {
            delete bounds;
        }
    }
    return ret;
}

NCB_REGISTER_GDIP_SUBCLASS2(ImageClass, ImageConvertor)
Factory(ImageFactory);
NCB_METHOD_PROXY(load, ImageLoad);
NCB_METHOD_PROXY(Clone, ImageClone);
NCB_METHOD_PROXY(GetBounds, ImageBounds);
// GetAllPropertyItems
// NCB_GDIP_METHOD(GetBounds);
// GetEncoderParameterList
// GetEncoderParameterListSize
NCB_GDIP_METHOD(GetFlags);
// NCB_GDIP_METHOD(GetFrameCount);
// NCB_GDIP_METHOD(GetFrameDimensionCount);
// NCB_GDIP_METHOD(GetFrameDimensionList);
NCB_GDIP_METHOD(GetHeight);
NCB_GDIP_METHOD(GetHorizontalResolution);
NCB_GDIP_METHOD(GetLastStatus);
// NCB_GDIP_PROPERTY(palette, GetPalette, SetPalette);
// NCB_GDIP_METHOD(GetPaletteSize);
// GetPhysicalDimension
NCB_GDIP_METHOD(GetPixelFormat);
// NCB_GDIP_METHOD(GetPropertyCount);
// GetPropertyItemList
// GetPropertyItem
// SetPropertyItem
// GetPropertyItemSize
// GetPropertySize
// GetRawFormat
// GetThumbnailImage
NCB_GDIP_METHOD(GetType);
NCB_GDIP_METHOD(GetVerticalResolution);
NCB_GDIP_METHOD(GetWidth);
// RemovePropertyItem
NCB_GDIP_METHOD(RotateFlip);
// SelectActiveFrame
}

// ------------------------------------------------------
// 自前記述クラス登録
// ------------------------------------------------------

NCB_REGISTER_SUBCLASS(FontInfo) {
    NCB_CONSTRUCTOR((const tjs_char *, REAL, INT));
    NCB_PROPERTY(familyName, getFamilyName, setFamilyName);
    NCB_PROPERTY(emSize, getEmSize, setEmSize);
    NCB_PROPERTY(style, getStyle, setStyle);
    NCB_PROPERTY(forceSelfPathDraw, getForceSelfPathDraw, setForceSelfPathDraw);
    NCB_PROPERTY_RO(ascent, getAscent);
    NCB_PROPERTY_RO(descent, getDescent);
    NCB_PROPERTY_RO(ascentLeading, getAscentLeading);
    NCB_PROPERTY_RO(descentLeading, getDescentLeading);
    NCB_PROPERTY_RO(lineSpacing, getLineSpacing);
}

NCB_REGISTER_SUBCLASS(Appearance) {
    NCB_CONSTRUCTOR(());
    NCB_METHOD(clear);
    NCB_METHOD(addBrush);
    NCB_METHOD(addPen);
}

NCB_REGISTER_SUBCLASS(DrawPath) {
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
}

#define ENUM(n) Variant(#n, (int)n)

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
    //	ENUM(ProfileNotFound);

    ENUM(FontStyleRegular);
    ENUM(FontStyleBold);
    ENUM(FontStyleItalic);
    // ENUM(FontStyleBoldItalic);
    Variant("FontStyleBoldItalic", 3);
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
    // ENUM(HatchStyleTotal);
    Variant("HatchStyleTotal", 0x35);
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
    NCB_SUBCLASS(PointF, PointFClass);
    NCB_SUBCLASS(RectF, RectFClass);
    NCB_SUBCLASS(Image, GdipWrapper<ImageClass>);
    NCB_SUBCLASS(Matrix, GdipWrapper<MatrixClass>);

    NCB_SUBCLASS(Font, FontInfo);
    NCB_SUBCLASS(Appearance, Appearance);
    NCB_SUBCLASS(Path, DrawPath);
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
        objthis,
        obj); // objthis に obj をネイティブインスタンスとして登録する
}
obj->reset();
return obj;
}

// デストラクタ（実際のメソッドが呼ばれた後に呼ばれる）
~NCB_GET_INSTANCE_HOOK_CLASS() {}
}
;

/**
 * GpImage はラッピングする必要があるので rawcallback で対応
 */
static tjs_error GetRecordImage(tTJSVariant *result, tjs_int numparams,
                                tTJSVariant **param, iTJSDispatch2 *objthis) {
    LayerExDraw *obj =
        ncbInstanceAdaptor<LayerExDraw>::GetNativeInstance(objthis, true);
    if(result)
        result->Clear();
    if(obj) {
        auto *image = obj->getRecordImage();
        if(image) {
            typedef GdipWrapper<ImageClass> WrapperT;
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
    // 基本的に非公開関数
    //	NCB_METHOD(drawPathString2);
    //	NCB_METHOD(measureString2);
    //	NCB_METHOD(measureStringInternal);

    NCB_METHOD(drawImage);
    NCB_METHOD(drawImageRect);
    NCB_METHOD(drawImageStretch);
    NCB_METHOD(drawImageAffine);

    NCB_PROPERTY(record, getRecord, setRecord);
    NCB_METHOD_RAW_CALLBACK(getRecordImage, GetRecordImage, 0);
    NCB_METHOD(redrawRecord);
    NCB_METHOD(saveRecord);
    NCB_METHOD(loadRecord);

    NCB_METHOD_RAW_CALLBACK(saveImage, &LayerExDraw::saveImage, 0);

    NCB_METHOD(getColorRegionRects);

#define ENUM(n) Variant(#n, (int)n)
    ENUM(EncoderValueCompressionLZW);
    ENUM(EncoderValueCompressionCCITT3);
    ENUM(EncoderValueCompressionCCITT4);
    ENUM(EncoderValueCompressionRle);
    ENUM(EncoderValueCompressionNone);
    ENUM(EncoderValueScanMethodInterlaced);
    ENUM(EncoderValueScanMethodNonInterlaced);
    ENUM(EncoderValueVersionGif87);
    ENUM(EncoderValueVersionGif89);
    ENUM(EncoderValueRenderProgressive);
    ENUM(EncoderValueRenderNonProgressive);
    ENUM(EncoderValueTransformRotate90);
    ENUM(EncoderValueTransformRotate180);
    ENUM(EncoderValueTransformRotate270);
    ENUM(EncoderValueTransformFlipHorizontal);
    ENUM(EncoderValueTransformFlipVertical);
}

// ----------------------------------- 起動・開放処理

NCB_PRE_REGIST_CALLBACK(initGdiPlus);
NCB_POST_UNREGIST_CALLBACK(deInitGdiPlus);
