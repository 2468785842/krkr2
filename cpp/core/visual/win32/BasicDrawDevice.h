
#ifndef BASIC_DRAW_DEVICE_H
#define BASIC_DRAW_DEVICE_H

#include "DrawDevice.h"
// #include <d3d9.h>

//---------------------------------------------------------------------------
//! @brief
//! 「Basic」デバイス(もっとも基本的な描画を行うのみのデバイス)
//---------------------------------------------------------------------------
class tTVPBasicDrawDevice : public tTVPDrawDevice {
    typedef tTVPDrawDevice inherited;

    void *TargetWindow;
    bool IsMainWindow{};
    bool DrawUpdateRectangle;
    bool BackBufferDirty;

    void *Direct3D;
    void *Direct3DDevice;
    void *Texture;
    int D3dPP{};
    int DispMode{};

    //	UINT	CurrentMonitor;
    void *TextureBuffer; //!< テクスチャのサーフェースへのメモリポインタ
                         // 	long	TexturePitch; //!<
                         // テクスチャのピッチ

    tjs_uint TextureWidth; //!< テクスチャの横幅
    tjs_uint TextureHeight; //!< テクスチャの縦幅

    bool ShouldShow; //!< show で実際に画面に画像を転送すべきか

    tjs_uint VsyncInterval;

public:
    tTVPBasicDrawDevice(); //!< コンストラクタ

private:
    ~tTVPBasicDrawDevice() override; //!< デストラクタ

    void InvalidateAll();
#if 0
	UINT GetMonitorNumber( HWND window );

	bool IsTargetWindowActive() const;

	bool GetDirect3D9Device();
	HRESULT InitializeDirect3DState();
	HRESULT DecideD3DPresentParameters();

 	bool CreateD3DDevice();
#endif
    void DestroyD3DDevice() {}

    // 	bool CreateTexture();
    // 	void DestroyTexture();

    void TryRecreateWhenDeviceLost();
    //	void ErrorToLog( HRESULT hr );
    //	void CheckMonitorMoved();

public:
    //	void SetToRecreateDrawer() { DestroyD3DDevice(); }
    enum tDrawerType {
        dtNone, //!< drawer なし
        dtDrawDib, //!< もっとも単純なdrawer
        dtDBGDI, // GDI によるダブルバッファリングを行うdrawer
        dtDBDD, // DirectDraw によるダブルバッファリングを行うdrawer
        dtDBD3D // Direct3D によるダブルバッファリングを行うdrawer
    } DrawerType = dtDrawDib,
      PreferredDrawerType = dtDrawDib;
    [[nodiscard]] tDrawerType GetDrawerType() const { return DrawerType; }
    void SetPreferredDrawerType(tDrawerType type) {
        PreferredDrawerType = type;
    }
    [[nodiscard]] tDrawerType GetPreferredDrawerType() const {
        return PreferredDrawerType;
    }

public:
    //	void EnsureDevice();

    //---- LayerManager の管理関連
    void AddLayerManager(iTVPLayerManager *manager) override;

    //---- 描画位置・サイズ関連
    //	virtual void SetTargetWindow(HWND wnd, bool
    // is_main);
    void SetDestRectangle(const tTVPRect &rect) override;
    void NotifyLayerResize(iTVPLayerManager *manager) override;

    //---- 再描画関連
    void Show() override;
    //	virtual bool WaitForVBlank( tjs_int*
    // in_vblank, tjs_int*
    // delayed );

    //---- LayerManager からの画像受け渡し関連
    void StartBitmapCompletion(iTVPLayerManager *manager) override;
    void NotifyBitmapCompleted(iTVPLayerManager *manager, tjs_int x, tjs_int y,
                               tTVPBaseTexture *bmp, const tTVPRect &cliprect,
                               tTVPLayerType type, tjs_int opacity) override;
    void EndBitmapCompletion(iTVPLayerManager *manager) override;

    //---- デバッグ支援
    void SetShowUpdateRect(bool b) override;
#if 0
//---- フルスクリーン
	virtual bool SwitchToFullScreen( HWND window, tjs_uint w, tjs_uint h, tjs_uint bpp, tjs_uint color, bool changeresolution );
	virtual void RevertFromFullScreen( HWND window, tjs_uint w, tjs_uint h, tjs_uint bpp, tjs_uint color );
#endif
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTJSNI_BasicDrawDevice
//---------------------------------------------------------------------------
class tTJSNI_BasicDrawDevice : public tTJSNativeInstance {
    typedef tTJSNativeInstance inherited;

    tTVPBasicDrawDevice *Device;

public:
    tTJSNI_BasicDrawDevice();
    ~tTJSNI_BasicDrawDevice() override;
    tjs_error Construct(tjs_int numparams, tTJSVariant **param,
                        iTJSDispatch2 *tjs_obj) override;
    void Invalidate() override;

public:
    [[nodiscard]] tTVPBasicDrawDevice *GetDevice() const { return Device; }
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTJSNC_BasicDrawDevice
//---------------------------------------------------------------------------
class tTJSNC_BasicDrawDevice : public tTJSNativeClass {
public:
    tTJSNC_BasicDrawDevice();

    static tjs_uint32 ClassID;

private:
    iTJSNativeInstance *CreateNativeInstance() override;
};
//---------------------------------------------------------------------------

#endif // BASIC_DRAW_DEVICE_H