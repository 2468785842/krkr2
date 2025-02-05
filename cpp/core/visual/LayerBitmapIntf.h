//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Base Layer Bitmap implementation
//---------------------------------------------------------------------------
#ifndef LayerBitmapIntfH
#define LayerBitmapIntfH

#include "ComplexRect.h"
#include "tvpgl.h"
#include "argb.h"
#include "drawable.h"

#ifndef TVP_REVRGB
#define TVP_REVRGB(v)                                                          \
    ((v & 0xFF00FF00) | ((v >> 16) & 0xFF) | ((v & 0xFF) << 16))
#endif

/*[*/
//---------------------------------------------------------------------------
// tTVPBBBltMethod and tTVPBBStretchType
//---------------------------------------------------------------------------
enum tTVPBBBltMethod {
    bmCopy,
    bmCopyOnAlpha,
    bmAlpha,
    bmAlphaOnAlpha,
    bmAdd,
    bmSub,
    bmMul,
    bmDodge,
    bmDarken,
    bmLighten,
    bmScreen,
    bmAddAlpha,
    bmAddAlphaOnAddAlpha,
    bmAddAlphaOnAlpha,
    bmAlphaOnAddAlpha,
    bmCopyOnAddAlpha,
    bmPsNormal,
    bmPsAdditive,
    bmPsSubtractive,
    bmPsMultiplicative,
    bmPsScreen,
    bmPsOverlay,
    bmPsHardLight,
    bmPsSoftLight,
    bmPsColorDodge,
    bmPsColorDodge5,
    bmPsColorBurn,
    bmPsLighten,
    bmPsDarken,
    bmPsDifference,
    bmPsDifference5,
    bmPsExclusion
};

enum tTVPBBStretchType {
    stNearest = 0, // primal method; nearest neighbor method
    stFastLinear = 1, // fast linear interpolation (does not have so
                      // much precision)
    stLinear = 2, // (strict) linear interpolation
    stCubic = 3, // cubic interpolation
    stSemiFastLinear = 4,
    stFastCubic = 5,
    stLanczos2 = 6, // Lanczos 2 interpolation
    stFastLanczos2 = 7,
    stLanczos3 = 8, // Lanczos 3 interpolation
    stFastLanczos3 = 9,
    stSpline16 = 10, // Spline16 interpolation
    stFastSpline16 = 11,
    stSpline36 = 12, // Spline36 interpolation
    stFastSpline36 = 13,
    stAreaAvg = 14, // Area average interpolation
    stFastAreaAvg = 15,
    stGaussian = 16,
    stFastGaussian = 17,
    stBlackmanSinc = 18,
    stFastBlackmanSinc = 19,

    stTypeMask = 0x0000ffff, // stretch type mask
    stFlagMask = 0xffff0000, // flag mask

    stRefNoClip = 0x10000 // referencing source is not limited by the
                          // given rectangle (may allow to see the
                          // border pixel to interpolate)
};
/*]*/

//---------------------------------------------------------------------------
// FIXME: for including order problem
//---------------------------------------------------------------------------
#include "LayerBitmapImpl.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// t2DAffineMatrix
//---------------------------------------------------------------------------
struct t2DAffineMatrix {
    /* structure for subscribing following 2D affine transformation
     * matrix */
    /*
    |                          | a  b  0 |
    | [x', y', 1] =  [x, y, 1] | c  d  0 |
    |                          | tx ty 1 |
    |  thus,
    |
    |  x' =  ax + cy + tx
    |  y' =  bx + dy + ty
    */

    double a;
    double b;
    double c;
    double d;
    double tx;
    double ty;
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#define TVP_BB_COPY_MAIN 1
#define TVP_BB_COPY_MASK 2
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
extern tTVPGLGammaAdjustData TVPIntactGammaAdjustData;
extern tjs_int TVPDrawThreadNum;
extern tjs_int TVPGetProcessorNum();
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// iTVPBaseBitmap
//---------------------------------------------------------------------------
class tTVPNativeBaseBitmap;
class iTVPBaseBitmap : public tTVPNativeBaseBitmap {
public:
    // void operator =(const iTVPBaseBitmap & rhs) { Assign(rhs); }

    // metrics
    void SetSizeWithFill(tjs_uint w, tjs_uint h, tjs_uint32 fillvalue);

    // point access
    tjs_uint32 GetPoint(tjs_int x, tjs_int y) const;
    bool SetPoint(tjs_int x, tjs_int y, tjs_uint32 value);
    bool SetPointMain(tjs_int x, tjs_int y,
                      tjs_uint32 color); // for 32bpp
    bool SetPointMask(tjs_int x, tjs_int y,
                      tjs_int mask); // for 32bpp

    // drawing stuff
    virtual bool Fill(tTVPRect rect, tjs_uint32 value);

    bool FillColor(tTVPRect rect, tjs_uint32 color, tjs_int opa);

private:
    bool BlendColor(tTVPRect rect, tjs_uint32 color, tjs_int opa,
                    bool additive);

public:
    bool FillColorOnAlpha(tTVPRect rect, tjs_uint32 color, tjs_int opa) {
        return BlendColor(rect, color, opa, false);
    }

    bool FillColorOnAddAlpha(tTVPRect rect, tjs_uint32 color, tjs_int opa) {
        return BlendColor(rect, color, opa, true);
    }

    bool RemoveConstOpacity(tTVPRect rect, tjs_int level);

    bool FillMask(tTVPRect rect, tjs_int value);

    virtual bool CopyRect(tjs_int x, tjs_int y, const iTVPBaseBitmap *ref,
                          tTVPRect refrect) {
        return CopyRect(x, y, ref, refrect,
                        TVP_BB_COPY_MAIN | TVP_BB_COPY_MASK);
    }

    virtual bool CopyRect(tjs_int x, tjs_int y, const iTVPBaseBitmap *ref,
                          tTVPRect refrect, tjs_int plane);

    /**
     * @param ref : コピー元画像(9patch形式)
     * @param margin : 9patchの右下にある描画領域指定を取得する
     */
    bool Copy9Patch(const iTVPBaseBitmap *ref, tTVPRect &margin);

    bool Blt(tjs_int x, tjs_int y, const iTVPBaseBitmap *ref, tTVPRect refrect,
             tTVPBBBltMethod method, tjs_int opa, bool hda = true);
    bool Blt(tjs_int x, tjs_int y, const iTVPBaseBitmap *ref,
             const tTVPRect &refrect, tTVPLayerType type, tjs_int opa,
             bool hda = true);

public:
    bool StretchBlt(tTVPRect cliprect, tTVPRect destrect,
                    const iTVPBaseBitmap *ref, tTVPRect refrect,
                    tTVPBBBltMethod method, tjs_int opa, bool hda = true,
                    tTVPBBStretchType type = stNearest, tjs_real typeopt = 0.0);

public:
    bool AffineBlt(tTVPRect destrect, const iTVPBaseBitmap *ref,
                   tTVPRect refrect, const tTVPPointD *points,
                   tTVPBBBltMethod method, tjs_int opa,
                   tTVPRect *updaterect = nullptr, bool hda = true,
                   tTVPBBStretchType mode = stNearest, bool clear = false,
                   tjs_uint32 clearcolor = 0);

    bool AffineBlt(tTVPRect destrect, const iTVPBaseBitmap *ref,
                   tTVPRect refrect, const t2DAffineMatrix &matrix,
                   tTVPBBBltMethod method, tjs_int opa,
                   tTVPRect *updaterect = nullptr, bool hda = true,
                   tTVPBBStretchType mode = stNearest, bool clear = false,
                   tjs_uint32 clearcolor = 0);

private:
    bool InternalDoBoxBlur(tTVPRect rect, tTVPRect area, bool hasalpha);

public:
    bool DoBoxBlur(const tTVPRect &rect, const tTVPRect &area);
    bool DoBoxBlurForAlpha(const tTVPRect &rect, const tTVPRect &area);

    virtual void UDFlip(const tTVPRect &rect);

    virtual void LRFlip(const tTVPRect &rect);

    void DoGrayScale(tTVPRect rect);

    void AdjustGamma(tTVPRect rect, const tTVPGLGammaAdjustData &data);
    void AdjustGammaForAdditiveAlpha(tTVPRect rect,
                                     const tTVPGLGammaAdjustData &data);

    void ConvertAddAlphaToAlpha();
    void ConvertAlphaToAddAlpha();

    // font and text related functions are implemented in each
    // platform.
};
//---------------------------------------------------------------------------
class iTVPRenderManager;
class tTVPBaseBitmap : public iTVPBaseBitmap // for ProvinceImage
{
public:
    tTVPBaseBitmap(tjs_uint w, tjs_uint h, tjs_uint bpp = 32);
    tTVPBaseBitmap(const iTVPBaseBitmap &r) : iTVPBaseBitmap(r) {}
    virtual bool AssignBitmap(tTVPBitmap *bmp);
    iTVPRenderManager *GetRenderManager() override;
    bool Fill(tTVPRect rect, tjs_uint32 value) override;

    bool CopyRect(tjs_int x, tjs_int y, const iTVPBaseBitmap *ref,
                  tTVPRect refrect) override {
        return CopyRect(x, y, ref, refrect,
                        TVP_BB_COPY_MAIN | TVP_BB_COPY_MASK);
    }

    bool CopyRect(tjs_int x, tjs_int y, const iTVPBaseBitmap *ref,
                  tTVPRect refrect, tjs_int plane) override;
    void UDFlip(const tTVPRect &rect) override;
    void LRFlip(const tTVPRect &rect) override;
};
//---------------------------------------------------------------------------
class tTVPBaseTexture : public iTVPBaseBitmap {
public:
    tTVPBaseTexture(tjs_uint w, tjs_uint h, tjs_uint bpp = 32);
    tTVPBaseTexture(const iTVPBaseBitmap &r) : iTVPBaseBitmap(r) {}
    virtual bool AssignBitmap(tTVPBitmap *bmp);
    iTVPRenderManager *GetRenderManager() override;
    void Update(const void *pixel, unsigned int pitch, int x, int y, int w,
                int h);
};
#endif
