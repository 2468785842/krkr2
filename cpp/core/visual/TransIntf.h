//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Transition handler mamagement & default transition handlers
//---------------------------------------------------------------------------
#ifndef TransIntfH
#define TransIntfH
//---------------------------------------------------------------------------

#include "LayerBitmapIntf.h"
#include "transhandler.h"

//---------------------------------------------------------------------------
// iTVPSimpleOptionProvider implementation
//---------------------------------------------------------------------------
class tTVPSimpleOptionProvider : public iTVPSimpleOptionProvider {
    tjs_uint RefCount;
    tTJSVariantClosure Object;
    ttstr String;

public:
    tTVPSimpleOptionProvider(tTJSVariantClosure object);
    ~tTVPSimpleOptionProvider();

    tjs_error AddRef();
    tjs_error Release();

    tjs_error GetAsNumber(
        /*in*/ const tjs_char *name, /*out*/ tjs_int64 *value);
    tjs_error GetAsString(
        /*in*/ const tjs_char *name, /*out*/ const tjs_char **out);

    tjs_error GetValue(
        /*in*/ const tjs_char *name, /*out*/ tTJSVariant *dest);

    tjs_error Reserved2() { return TJS_E_NOTIMPL; }

    tjs_error GetDispatchObject(iTJSDispatch2 **dsp);
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// iTVPSimpleImageProvider implementation
//---------------------------------------------------------------------------
class tTVPSimpleImageProvider : public iTVPSimpleImageProvider {
public:
    tjs_error LoadImage(
        /*in*/ const tjs_char *name, /*in*/ tjs_int bpp,
        /*in*/ tjs_uint32 key,
        /*in*/ tjs_uint w,
        /*in*/ tjs_uint h,
        /*out*/ iTVPScanLineProvider **scpro);
};
//---------------------------------------------------------------------------
extern tTVPSimpleImageProvider TVPSimpleImageProvider;
//---------------------------------------------------------------------------
#if 0
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(iTVPScanLineProvider *, TVPSLPLoadImage, (const ttstr &name, tjs_int bpp,
	tjs_uint32 key, tjs_uint w, tjs_uint h));
//---------------------------------------------------------------------------
#endif

//---------------------------------------------------------------------------
// iTVPScanLineProvider implementation for image provider ( holds
// tTVPBaseBitmap
// )
//---------------------------------------------------------------------------
// provides layer scanline
class tTVPScanLineProviderForBaseBitmap : public iTVPScanLineProvider {
    tjs_uint RefCount;
    bool Own;
    iTVPBaseBitmap *Bitmap;

public:
    tTVPScanLineProviderForBaseBitmap(iTVPBaseBitmap *bmp, bool own = false);
    ~tTVPScanLineProviderForBaseBitmap();

    void Attach(iTVPBaseBitmap *bmp); // attach bitmap

    tjs_error AddRef();
    tjs_error Release();

    tjs_error GetWidth(/*in*/ tjs_int *width);
    tjs_error GetHeight(/*in*/ tjs_int *height);
#if 0
	tjs_error GetPixelFormat(/*out*/tjs_int *bpp);
	tjs_error GetPitchBytes(/*out*/tjs_int *pitch);
	tjs_error GetScanLine(/*in*/tjs_int line,
			/*out*/const void ** scanline);
	tjs_error GetScanLineForWrite(/*in*/tjs_int line,
			/*out*/void ** scanline);
#endif
    virtual iTVPTexture2D *GetTexture() override;
    virtual iTVPTexture2D *GetTextureForRender() override;
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// handler management functions
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(void, TVPAddTransHandlerProvider,
                 (iTVPTransHandlerProvider * pro));
TJS_EXP_FUNC_DEF(void, TVPRemoveTransHandlerProvider,
                 (iTVPTransHandlerProvider * pro));
iTVPTransHandlerProvider *TVPFindTransHandlerProvider(const ttstr &name);
//---------------------------------------------------------------------------

/*[*/
//---------------------------------------------------------------------------
// scroll transition handler
//---------------------------------------------------------------------------
enum tTVPScrollTransFrom { sttLeft, sttTop, sttRight, sttBottom };
enum tTVPScrollTransStay { ststNoStay, ststStayDest, ststStaySrc };
/*]*/
//---------------------------------------------------------------------------

class tTVPCrossFadeTransHandlerProvider : public iTVPTransHandlerProvider {
    tjs_int RefCount;

public:
    tTVPCrossFadeTransHandlerProvider();
    virtual ~tTVPCrossFadeTransHandlerProvider();
    ;

    tjs_error AddRef();

    tjs_error Release();

    tjs_error GetName(
        /*out*/ const tjs_char **name);

    tjs_error StartTransition(
        /*in*/ iTVPSimpleOptionProvider *options, // option provider
        /*in*/ iTVPSimpleImageProvider *imagepro, // image provider
        /*in*/ tTVPLayerType layertype, // destination layer type
        /*in*/ tjs_uint src1w, tjs_uint src1h, // source 1 size
        /*in*/ tjs_uint src2w, tjs_uint src2h, // source 2 size
        /*out*/ tTVPTransType *type, // transition type
        /*out*/ tTVPTransUpdateType *updatetype, // update typwe
        /*out*/ iTVPBaseTransHandler **handler // transition handler
    );

    virtual iTVPBaseTransHandler *GetTransitionObject(
        /*in*/ iTVPSimpleOptionProvider *options, // option provider
        /*in*/ iTVPSimpleImageProvider *imagepro, // image provider
        /*in*/ tTVPLayerType layertype,
        /*in*/ tjs_uint src1w, tjs_uint src1h, // source 1 size
        /*in*/ tjs_uint src2w, tjs_uint src2h); // source 2 size
};

#endif
