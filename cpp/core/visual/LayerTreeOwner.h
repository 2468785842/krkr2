//---------------------------------------------------------------------------
/**
 * レイヤーツリーを保持する機能を
 * Windowのみでなく、一般化し、このインターフェイス
 * を持つクラスであれば、レイヤーツリーを持てるようにする
 *
 */
//---------------------------------------------------------------------------
//!@file レイヤーツリーオーナー
//---------------------------------------------------------------------------
#ifndef LayerTreeOwner_H
#define LayerTreeOwner_H
#include "drawable.h"

class iTVPLayerTreeOwner {
public:
    // LayerManager/Layer -> LTO
    virtual void RegisterLayerManager(class iTVPLayerManager *manager) = 0;
    virtual void UnregisterLayerManager(class iTVPLayerManager *manager) = 0;

    virtual void StartBitmapCompletion(iTVPLayerManager *manager) = 0;
    virtual void NotifyBitmapCompleted(class iTVPLayerManager *manager,
                                       tjs_int x, tjs_int y,
                                       class tTVPBaseTexture *bmp,
                                       const struct tTVPRect &cliprect,
                                       enum tTVPLayerType type,
                                       tjs_int opacity) = 0;
    virtual void EndBitmapCompletion(iTVPLayerManager *manager) = 0;

    virtual void SetMouseCursor(class iTVPLayerManager *manager,
                                tjs_int cursor) = 0;
    virtual void GetCursorPos(class iTVPLayerManager *manager, tjs_int &x,
                              tjs_int &y) = 0;
    virtual void SetCursorPos(class iTVPLayerManager *manager, tjs_int x,
                              tjs_int y) = 0;
    virtual void ReleaseMouseCapture(class iTVPLayerManager *manager) = 0;

    virtual void SetHint(class iTVPLayerManager *manager, iTJSDispatch2 *sender,
                         const ttstr &hint) = 0;

    virtual void NotifyLayerResize(class iTVPLayerManager *manager) = 0;
    virtual void NotifyLayerImageChange(class iTVPLayerManager *manager) = 0;

    virtual void SetAttentionPoint(class iTVPLayerManager *manager,
                                   class tTJSNI_BaseLayer *layer, tjs_int x,
                                   tjs_int y) = 0;
    virtual void DisableAttentionPoint(class iTVPLayerManager *manager) = 0;

    virtual void SetImeMode(class iTVPLayerManager *manager,
                            tjs_int mode) = 0; // mode == tTVPImeMode
    virtual void ResetImeMode(class iTVPLayerManager *manager) = 0;

    virtual iTJSDispatch2 *GetOwnerNoAddRef() const = 0;
    // LTO -> LayerManager/Layer
    // LTO からの通知は必要要件ではない
};

#endif
