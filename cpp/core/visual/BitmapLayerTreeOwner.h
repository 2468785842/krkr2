//---------------------------------------------------------------------------
/**
 * 描画先を Bitmap とする Layer Tree Owner
 * レイヤーに描かれて、合成された内容は、このクラスの保持する Bitmap
 * に描かれる 設定のために呼びれたメソッドもイベントとして通知される
 */
//---------------------------------------------------------------------------
//!@file レイヤーツリーオーナー
//---------------------------------------------------------------------------

#ifndef BitmapLayerTreeOwner_H
#define BitmapLayerTreeOwner_H

#include "LayerTreeOwnerImpl.h"

class tTJSNI_BitmapLayerTreeOwner : public tTJSNativeInstance,
                                    public tTVPLayerTreeOwner {
    iTJSDispatch2 *Owner;
    iTJSDispatch2 *BitmapObject;
    tTJSNI_Bitmap *BitmapNI;

public:
public:
    tTJSNI_BitmapLayerTreeOwner();
    ~tTJSNI_BitmapLayerTreeOwner() override;

    // tTJSNativeInstance
    tjs_error Construct(tjs_int numparams, tTJSVariant **param,
                        iTJSDispatch2 *tjs_obj) override;
    void Invalidate() override;

    iTJSDispatch2 *GetBitmapObjectNoAddRef();

    // tTVPLayerTreeOwner
    iTJSDispatch2 *GetOwnerNoAddRef() const override { return Owner; }

    void StartBitmapCompletion(iTVPLayerManager *manager) override;
    void NotifyBitmapCompleted(class iTVPLayerManager *manager, tjs_int x,
                               tjs_int y, tTVPBaseTexture *bmp,
                               const tTVPRect &cliprect, tTVPLayerType type,
                               tjs_int opacity) override;
    void EndBitmapCompletion(iTVPLayerManager *manager) override;

    void OnSetMouseCursor(tjs_int cursor) override;
    void OnGetCursorPos(tjs_int &x, tjs_int &y) override;
    void OnSetCursorPos(tjs_int x, tjs_int y) override;
    void OnReleaseMouseCapture() override;
    void OnSetHintText(iTJSDispatch2 *sender, const ttstr &hint) override;

    void OnResizeLayer(tjs_int w, tjs_int h) override;
    void OnChangeLayerImage() override;

    void OnSetAttentionPoint(tTJSNI_BaseLayer *layer, tjs_int x,
                             tjs_int y) override;
    void OnDisableAttentionPoint() override;
    void OnSetImeMode(tjs_int mode) override;
    void OnResetImeMode() override;

    tjs_int GetWidth() const { return BitmapNI->GetWidth(); }
    tjs_int GetHeight() const { return BitmapNI->GetHeight(); }
};

class tTJSNC_BitmapLayerTreeOwner : public tTJSNativeClass {
    typedef tTJSNativeClass inherited;

public:
    tTJSNC_BitmapLayerTreeOwner();
    static tjs_uint32 ClassID;

protected:
    tTJSNativeInstance *CreateNativeInstance() override;
};

extern tTJSNativeClass *TVPCreateNativeClass_BitmapLayerTreeOwner();
#endif
