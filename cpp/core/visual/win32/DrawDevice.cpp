//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
//!@file 描画デバイス管理
//---------------------------------------------------------------------------

#include "tjsCommHead.h"

#include <algorithm>
#include "DrawDevice.h"
#include "MsgIntf.h"
#include "LayerIntf.h"
#include "LayerManager.h"
#include "WindowIntf.h"
#include "DebugIntf.h"

//---------------------------------------------------------------------------
tTVPDrawDevice::tTVPDrawDevice() {
    // コンストラクタ
    Window = nullptr;
    PrimaryLayerManagerIndex = 0;
    DestRect.clear();
    ClipRect.clear();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
tTVPDrawDevice::~tTVPDrawDevice() {
    // すべての managers を開放する
    // TODO:
    // プライマリレイヤ無効化、あるいはウィンドウ破棄時の終了処理が正しいか？
    // managers は 開放される際、自身の登録解除を行うために
    // RemoveLayerManager() を呼ぶかもしれないので注意。
    // そのため、ここではいったん配列をコピーしてからそれぞれの
    // Release() を呼ぶ。
    std::vector<iTVPLayerManager *> backup = Managers;
    for(std::vector<iTVPLayerManager *>::iterator i = backup.begin();
        i != backup.end(); i++)
        (*i)->Release();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
bool tTVPDrawDevice::TransformToPrimaryLayerManager(tjs_int &x, tjs_int &y) {
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return false;
    return true;

    // プライマリレイヤマネージャのプライマリレイヤのサイズを得る
    tjs_int pl_w = LockedWidth, pl_h = LockedHeight;
    if(pl_w <= 0 && pl_h <= 0 && !manager->GetPrimaryLayerSize(pl_w, pl_h))
        return false;
    // pl_w = WinWidth; pl_h = WinHeight;

    // x , y は DestRect の 0, 0
    // を原点とした座標として渡されてきている
    tjs_int w = DestRect.get_width();
    tjs_int h = DestRect.get_height();
    x = w ? ((x - DestRect.left) * pl_w / w) : 0;
    y = h ? ((y - DestRect.top) * pl_h / h) : 0;

    return true;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
bool tTVPDrawDevice::TransformFromPrimaryLayerManager(tjs_int &x, tjs_int &y) {
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return false;
    return true;

    // プライマリレイヤマネージャのプライマリレイヤのサイズを得る
    tjs_int pl_w = LockedWidth, pl_h = LockedHeight;
    if(pl_w <= 0 && pl_h <= 0 && !manager->GetPrimaryLayerSize(pl_w, pl_h))
        return false;
    // pl_w = WinWidth; pl_h = WinHeight;

    // x , y は DestRect の 0, 0
    // を原点とした座標として渡されてきている
    x = pl_w ? (x * DestRect.get_width() / pl_w) : 0;
    y = pl_h ? (y * DestRect.get_height() / pl_h) : 0;
    x += DestRect.left;
    y += DestRect.top;
    return true;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
bool tTVPDrawDevice::TransformToPrimaryLayerManager(tjs_real &x, tjs_real &y) {
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return false;

    // プライマリレイヤマネージャのプライマリレイヤのサイズを得る
    tjs_int pl_w, pl_h;
    if(!manager->GetPrimaryLayerSize(pl_w, pl_h))
        return false;

    // x , y は DestRect の 0, 0
    // を原点とした座標として渡されてきている
    tjs_int w = DestRect.get_width();
    tjs_int h = DestRect.get_height();
    x = w ? (x * pl_w / w) : 0.0;
    y = h ? (y * pl_h / h) : 0.0;

    return true;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::Destruct() { delete this; }
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::SetWindowInterface(iTVPWindow *window) { Window = window; }
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::AddLayerManager(iTVPLayerManager *manager) {
    // Managers に manager を push する。AddRefするのを忘れないこと。
    Managers.push_back(manager);
    manager->AddRef();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::RemoveLayerManager(iTVPLayerManager *manager) {
    // Managers から manager を削除する。Releaseする。
    std::vector<iTVPLayerManager *>::iterator i =
        std::find(Managers.begin(), Managers.end(), manager);
    if(i == Managers.end())
        TVPThrowInternalError;
    (*i)->Release();
    Managers.erase(i);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::SetDestRectangle(const tTVPRect &rect) { DestRect = rect; }
//---------------------------------------------------------------------------

void tTVPDrawDevice::SetLockedSize(tjs_int w, tjs_int h) {
    LockedWidth = w;
    LockedHeight = h;
}

//---------------------------------------------------------------------------
void tTVPDrawDevice::SetClipRectangle(const tTVPRect &rect) { ClipRect = rect; }
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::GetSrcSize(tjs_int &w, tjs_int &h) {
    w = LockedWidth;
    h = LockedHeight;
    if(w > 0 && h > 0)
        return;
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;
    if(!manager->GetPrimaryLayerSize(w, h)) {
        w = 0;
        h = 0;
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::NotifyLayerResize(iTVPLayerManager *manager) {
    iTVPLayerManager *primary_manager =
        GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(primary_manager == manager)
        Window->NotifySrcResize();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::NotifyLayerImageChange(iTVPLayerManager *manager) {
    iTVPLayerManager *primary_manager =
        GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(primary_manager == manager)
        Window->RequestUpdate();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::OnClick(tjs_int x, tjs_int y) {
    if(!TransformToPrimaryLayerManager(x, y))
        return;
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;

    manager->NotifyClick(x, y);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::OnDoubleClick(tjs_int x, tjs_int y) {
    if(!TransformToPrimaryLayerManager(x, y))
        return;
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;

    manager->NotifyDoubleClick(x, y);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::OnMouseDown(tjs_int x, tjs_int y, tTVPMouseButton mb,
                                 tjs_uint32 flags) {
    if(!TransformToPrimaryLayerManager(x, y))
        return;
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;

    manager->NotifyMouseDown(x, y, mb, flags);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::OnMouseUp(tjs_int x, tjs_int y, tTVPMouseButton mb,
                               tjs_uint32 flags) {
    if(!TransformToPrimaryLayerManager(x, y))
        return;
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;

    manager->NotifyMouseUp(x, y, mb, flags);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::OnMouseMove(tjs_int x, tjs_int y, tjs_uint32 flags) {
    if(!TransformToPrimaryLayerManager(x, y))
        return;
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;

    manager->NotifyMouseMove(x, y, flags);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::OnReleaseCapture() {
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;

    manager->ReleaseCapture();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::OnMouseOutOfWindow() {
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;

    manager->NotifyMouseOutOfWindow();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::OnKeyDown(tjs_uint key, tjs_uint32 shift) {
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;

    manager->NotifyKeyDown(key, shift);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::OnKeyUp(tjs_uint key, tjs_uint32 shift) {
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;

    manager->NotifyKeyUp(key, shift);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::OnKeyPress(tjs_char key) {
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;

    manager->NotifyKeyPress(key);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::OnMouseWheel(tjs_uint32 shift, tjs_int delta, tjs_int x,
                                  tjs_int y) {
    if(!TransformToPrimaryLayerManager(x, y))
        return;
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;

    manager->NotifyMouseWheel(shift, delta, x, y);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::OnTouchDown(tjs_real x, tjs_real y, tjs_real cx,
                                 tjs_real cy, tjs_uint32 id) {
    if(!TransformToPrimaryLayerManager(x, y))
        return;
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;

    manager->NotifyTouchDown(x, y, cx, cy, id);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::OnTouchUp(tjs_real x, tjs_real y, tjs_real cx, tjs_real cy,
                               tjs_uint32 id) {
    if(!TransformToPrimaryLayerManager(x, y))
        return;
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;

    manager->NotifyTouchUp(x, y, cx, cy, id);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::OnTouchMove(tjs_real x, tjs_real y, tjs_real cx,
                                 tjs_real cy, tjs_uint32 id) {
    if(!TransformToPrimaryLayerManager(x, y))
        return;
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;

    manager->NotifyTouchMove(x, y, cx, cy, id);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::OnTouchScaling(tjs_real startdist, tjs_real curdist,
                                    tjs_real cx, tjs_real cy, tjs_int flag) {
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;

    manager->NotifyTouchScaling(startdist, curdist, cx, cy, flag);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::OnTouchRotate(tjs_real startangle, tjs_real curangle,
                                   tjs_real dist, tjs_real cx, tjs_real cy,
                                   tjs_int flag) {
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;

    manager->NotifyTouchRotate(startangle, curangle, dist, cx, cy, flag);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::OnMultiTouch() {
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;

    manager->NotifyMultiTouch();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::OnDisplayRotate(tjs_int orientation, tjs_int rotate,
                                     tjs_int bpp, tjs_int width,
                                     tjs_int height) {
    // 何もしない
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::RecheckInputState() {
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;

    manager->RecheckInputState();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::SetDefaultMouseCursor(iTVPLayerManager *manager) {
    iTVPLayerManager *primary_manager =
        GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!primary_manager)
        return;
    if(primary_manager == manager) {
        Window->SetDefaultMouseCursor();
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::SetMouseCursor(iTVPLayerManager *manager, tjs_int cursor) {
    iTVPLayerManager *primary_manager =
        GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!primary_manager)
        return;
    if(primary_manager == manager) {
        Window->SetMouseCursor(cursor);
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::GetCursorPos(iTVPLayerManager *manager, tjs_int &x,
                                  tjs_int &y) {
    iTVPLayerManager *primary_manager =
        GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!primary_manager)
        return;
    Window->GetCursorPos(x, y);
    if(primary_manager != manager || !TransformToPrimaryLayerManager(x, y)) {
        // プライマリレイヤマネージャ以外には座標 0,0 で渡しておく
        x = 0;
        y = 0;
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::SetCursorPos(iTVPLayerManager *manager, tjs_int x,
                                  tjs_int y) {
    iTVPLayerManager *primary_manager =
        GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!primary_manager)
        return;
    if(primary_manager == manager) {
        if(TransformFromPrimaryLayerManager(x, y))
            Window->SetCursorPos(x, y);
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::WindowReleaseCapture(iTVPLayerManager *manager) {
    iTVPLayerManager *primary_manager =
        GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!primary_manager)
        return;
    if(primary_manager == manager) {
        Window->WindowReleaseCapture();
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::SetHintText(iTVPLayerManager *manager,
                                 iTJSDispatch2 *sender, const ttstr &text) {
    iTVPLayerManager *primary_manager =
        GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!primary_manager)
        return;
    if(primary_manager == manager) {
        Window->SetHintText(sender, text);
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::SetAttentionPoint(iTVPLayerManager *manager,
                                       tTJSNI_BaseLayer *layer, tjs_int l,
                                       tjs_int t) {
    iTVPLayerManager *primary_manager =
        GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!primary_manager)
        return;
    if(primary_manager == manager) {
        if(TransformFromPrimaryLayerManager(l, t))
            Window->SetAttentionPoint(layer, l, t);
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::DisableAttentionPoint(iTVPLayerManager *manager) {
    iTVPLayerManager *primary_manager =
        GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!primary_manager)
        return;
    if(primary_manager == manager) {
        Window->DisableAttentionPoint();
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::SetImeMode(iTVPLayerManager *manager, tTVPImeMode mode) {
    iTVPLayerManager *primary_manager =
        GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!primary_manager)
        return;
    if(primary_manager == manager) {
        Window->SetImeMode(mode);
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::ResetImeMode(iTVPLayerManager *manager) {
    iTVPLayerManager *primary_manager =
        GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!primary_manager)
        return;
    if(primary_manager == manager) {
        Window->ResetImeMode();
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
tTJSNI_BaseLayer *tTVPDrawDevice::GetPrimaryLayer() {
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return nullptr;
    return manager->GetPrimaryLayer();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
tTJSNI_BaseLayer *tTVPDrawDevice::GetFocusedLayer() {
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return nullptr;
    return manager->GetFocusedLayer();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::SetFocusedLayer(tTJSNI_BaseLayer *layer) {
    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;
    manager->SetFocusedLayer(layer);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::RequestInvalidation(const tTVPRect &rect) {
    tjs_int l = rect.left, t = rect.top, r = rect.right, b = rect.bottom;
    if(!TransformToPrimaryLayerManager(l, t))
        return;
    if(!TransformToPrimaryLayerManager(r, b))
        return;
    r++; // 誤差の吸収(本当はもうちょっと厳密にやらないとならないがそれが問題になることはない)
    b++;

    iTVPLayerManager *manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
    if(!manager)
        return;
    manager->RequestInvalidation(tTVPRect(l, t, r, b));
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::Update() {
    // すべての layer manager の UpdateToDrawDevice を呼ぶ
    for(std::vector<iTVPLayerManager *>::iterator i = Managers.begin();
        i != Managers.end(); i++) {
        (*i)->UpdateToDrawDevice();
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::Show() {
    // なにもしない
}
//---------------------------------------------------------------------------
bool tTVPDrawDevice::WaitForVBlank(tjs_int *in_vblank, tjs_int *delayed) {
    return false;
}

//---------------------------------------------------------------------------
void tTVPDrawDevice::DumpLayerStructure() {
    // すべての layer manager の DumpLayerStructure を呼ぶ
    for(std::vector<iTVPLayerManager *>::iterator i = Managers.begin();
        i != Managers.end(); i++) {
        (*i)->DumpLayerStructure();
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTVPDrawDevice::SetShowUpdateRect(bool b) {
    // なにもしない
}

void tTVPDrawDevice::SetWindowSize(tjs_int w, tjs_int h) {
    WinWidth = w;
    WinHeight = h;
}

//---------------------------------------------------------------------------
bool tTVPDrawDevice::SwitchToFullScreen(int window, tjs_uint w, tjs_uint h,
                                        tjs_uint bpp, tjs_uint color,
                                        bool changeresolution) {
    return true;
#if 0
	// ChangeDisplaySettings を使用したフルスクリーン化
	bool success = false;
	DEVMODE dm;
	ZeroMemory(&dm, sizeof(DEVMODE));
	dm.dmSize = sizeof(DEVMODE);
	dm.dmPelsWidth = w;
	dm.dmPelsHeight = h;
	dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
	dm.dmBitsPerPel = bpp;
	LONG ret = ::ChangeDisplaySettings((DEVMODE*)&dm, CDS_FULLSCREEN);
	switch(ret)
	{
	case DISP_CHANGE_SUCCESSFUL:
		::SetWindowPos(window, HWND_TOP, 0, 0, w, h, SWP_SHOWWINDOW);
		success = true;
		break;
	case DISP_CHANGE_RESTART:
		TVPAddLog( (const tjs_char*)TVPChangeDisplaySettingsFailedDispChangeRestart );
		break;
	case DISP_CHANGE_BADFLAGS:
		TVPAddLog( (const tjs_char*)TVPChangeDisplaySettingsFailedDispChangeBadFlags );
		break;
	case DISP_CHANGE_BADPARAM:
		TVPAddLog( (const tjs_char*)TVPChangeDisplaySettingsFailedDispChangeBadParam );
		break;
	case DISP_CHANGE_FAILED:
		TVPAddLog( (const tjs_char*)TVPChangeDisplaySettingsFailedDispChangeFailed );
		break;
	case DISP_CHANGE_BADMODE:
		TVPAddLog( (const tjs_char*)TVPChangeDisplaySettingsFailedDispChangeBadMode );
		break;
	case DISP_CHANGE_NOTUPDATED:
 		TVPAddLog( (const tjs_char*)TVPChangeDisplaySettingsFailedDispChangeNotUpdated );
		break;
	default:
		TVPAddLog( TVPFormatMessage(TVPChangeDisplaySettingsFailedUnknownReason,ttstr((tjs_int)ret)) );
		break;
	}
	return success;
#endif
}
//---------------------------------------------------------------------------
void tTVPDrawDevice::RevertFromFullScreen(int window, tjs_uint w, tjs_uint h,
                                          tjs_uint bpp, tjs_uint color) {
    // ChangeDisplaySettings を使用したフルスクリーン解除
    //	::ChangeDisplaySettings(nullptr, 0);
}
//---------------------------------------------------------------------------
