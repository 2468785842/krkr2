//
// Created by Li_Dong on 2024/12/9.
// source url: https://github.com/wamsoft/windowEx/blob
//
#include "ncbind/ncbind.hpp"
#include "DetectCPU.h"
#include "GraphicsLoaderIntf.h"
#include "EventIntf.h"
#include "WindowImpl.h"

#define NCB_MODULE_NAME TJS_W("windowEx.dll")

// ウィンドウクラス名取得用のバッファサイズ
#define CLASSNAME_MAX 1024

#define HTBORDER 18 // 沒有重設大小框線的視窗框線中。
#define HTBOTTOM 15 // 可重設大小的視窗的下水準框線中（使用者可以按鼠以垂直調整視窗大小）。
#define HTBOTTOMLEFT 16 // 可重設大小的視窗框線左下角（使用者可以按鼠以對角調整視窗大小）。
#define HTBOTTOMRIGHT 17 // 可重設大小的視窗框線右下角（使用者可以按鼠以對角調整視窗大小）。
#define TCAPTION 2 // 標題列中。
#define TCLIENT 1 // 工作區中。
#define TCLOSE 20 // [關閉] 按鈕中。
#define HTERROR -2 // 螢幕背景或視窗之間的分隔線上（與 HTNOWHERE 相同，不同之處在於 DefWindowProc 函式會產生系統嗶聲來指出錯誤）。
#define HTGROWBOX 4 // 大小方塊中（與 HTSIZE 相同）。
#define HTHELP 21 // [ 說明] 按鈕中。
#define HTHSCROLL 6 // 水平滾動條中。
#define HTLEFT 10 // 可重設大小的視窗左框線中（使用者可以按鼠水平調整視窗大小）。
#define HTMENU 5 // 功能表中。
#define HTMAXBUTTON 9 // [最大化] 按鈕中。
#define HTMINBUTTON 8 // [最小化] 按鈕中。
#define HTNOWHERE 0 // 畫面背景或視窗之間的分隔線上。
#define HTREDUCE 8 // [最小化] 按鈕中。
#define HTRIGHT 11 // 可重設大小的視窗右框線中（使用者可以按鼠水平調整視窗大小）。
#define HTSIZE 4 // 大小方塊中（與 HTGROWBOX 相同）。
#define HTSYSMENU 3 // 視窗選單或子視窗的 [ 關閉 ] 按鈕中。
#define HTTOP 12 // 視窗的上水平框線中。
#define HTTOPLEFT 13 // 視窗框線的左上角。
#define HTTOPRIGHT 14 // 視窗框線的右上角。
#define HTTRANSPARENT -1 // 目前由相同線程中另一個視窗所涵蓋的視窗中。
#define HTVSCROLL 7 // 垂直滾動條中。
#define HTZOOM 9 // [最大化] 按鈕中。
#define HTCAPTION 2 // In a title bar.
#define HTCLIENT 1 // In a client area.

typedef void* PVOID;
typedef PVOID HANDLE;
typedef HANDLE HMENU;
typedef HANDLE HWND;
typedef unsigned long DWORD;

typedef unsigned short WORD;
typedef uint UINT;
typedef uintptr_t ULONG_PTR;

// イベント名一覧
#define EXEV_MINIMIZE  TJS_W("onMinimize")
#define EXEV_MAXIMIZE  TJS_W("onMaximize")
#define EXEV_QUERYMAX  TJS_W("onMaximizeQuery")
#define EXEV_SHOW      TJS_W("onShow")
#define EXEV_HIDE      TJS_W("onHide")
#define EXEV_RESIZING  TJS_W("onResizing")
#define EXEV_MOVING    TJS_W("onMoving")
#define EXEV_MOVE      TJS_W("onMove")
#define EXEV_MVSZBEGIN TJS_W("onMoveSizeBegin")
#define EXEV_MVSZEND   TJS_W("onMoveSizeEnd")
#define EXEV_DISPCHG   TJS_W("onDisplayChanged")
#define EXEV_ENTERMENU TJS_W("onEnterMenuLoop")
#define EXEV_EXITMENU  TJS_W("onExitMenuLoop")
#define EXEV_ACTIVATE  TJS_W("onActivateChanged")
#define EXEV_SCREENSV  TJS_W("onScreenSave")
#define EXEV_MONITORPW TJS_W("onMonitorPower")
#define EXEV_NCMSMOVE  TJS_W("onNcMouseMove")
#define EXEV_NCMSLEAVE TJS_W("onNcMouseLeave")
#define EXEV_NCMSDOWN  TJS_W("onNcMouseDown")
#define EXEV_NCMSUP    TJS_W("onNcMouseUp")
#define EXEV_SYSMENU   TJS_W("onExSystemMenuSelected")
#define EXEV_KEYMENU   TJS_W("onStartKeyMenu")
#define EXEV_ACCELKEY  TJS_W("onAccelKeyMenu")
#define EXEV_NCMSEV    TJS_W("onNonCapMouseEvent")
#define EXEV_MSGHOOK   TJS_W("onWindowsMessageHook")

////////////////////////////////////////////////////////////////

struct WindowEx
{
    //--------------------------------------------------------------
    // ユーティリティ

    // ネイティブインスタンスポインタを取得
    static inline WindowEx * GetInstance(iTJSDispatch2 *obj) {
        return ncbInstanceAdaptor<WindowEx>::GetNativeInstance(obj);
    }

    // ウィンドウハンドルを取得
    static HWND GetHWND(iTJSDispatch2 *obj) {
        tTJSVariant val;
        obj->PropGet(0, TJS_W("HWND"), 0, &val, obj);
        return (HWND)(tjs_int64)(val);
    }

    //--------------------------------------------------------------
    // クラス追加メソッド(RawCallback形式)

    // minimize, maximize, showRestore
    static tjs_error TJS_INTF_METHOD minimize(   tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        return TJS_S_OK;
    }
    static tjs_error TJS_INTF_METHOD maximize(   tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        return TJS_S_OK;
    }
    static tjs_error TJS_INTF_METHOD showRestore(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        *r = obj;
        PostMessage(obj, n);
        return TJS_S_OK;
    }
    static tjs_error TJS_INTF_METHOD focusMenuByKey(tTJSVariant *r,tjs_int n,tTJSVariant **p,iTJSDispatch2 *obj) {
        return TJS_S_OK;

    }

    // resetWindowIcon
    static tjs_error TJS_INTF_METHOD resetWindowIcon(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        return true;
    }

    // setWindowIcon
    static tjs_error TJS_INTF_METHOD setWindowIcon(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {return true;
    }

    // getWindowRect
    static tjs_error TJS_INTF_METHOD getWindowRect(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        return TJS_S_OK;
    }

    // getClientRect
    static tjs_error TJS_INTF_METHOD getClientRect(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        return TJS_S_OK;
    }

    // getNormalRect
    static tjs_error TJS_INTF_METHOD getNormalRect(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        return TJS_S_OK;
    }

    // property maximized box
    static tjs_error TJS_INTF_METHOD getMaximizeBox(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        return TJS_S_OK;
    }
    static tjs_error TJS_INTF_METHOD setMaximizeBox(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        return TJS_S_OK;
    }

    // property minimized box
    static tjs_error TJS_INTF_METHOD getMinimizeBox(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
       return TJS_S_OK;
    }
    static tjs_error TJS_INTF_METHOD setMinimizeBox(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        return TJS_S_OK;
    }

    // property maximized
    static bool isMaximized(iTJSDispatch2 *obj) {
        return true;
    }
    static tjs_error TJS_INTF_METHOD getMaximized(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        if (r) *r = isMaximized(obj);
        return TJS_S_OK;
    }
    static tjs_error TJS_INTF_METHOD setMaximized(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        return TJS_S_OK;
    }

    // property minimized
    static bool isMinimized(iTJSDispatch2 *obj) {
        return false;
    }

    static tjs_error TJS_INTF_METHOD getMinimized(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        if (r) *r = isMinimized(obj);
        return TJS_S_OK;
    }
    static tjs_error TJS_INTF_METHOD setMinimized(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        return TJS_S_OK;
    }

    // property disableResize
    static tjs_error TJS_INTF_METHOD getDisableResize(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        WindowEx *self = GetInstance(obj);
        if (r) *r = (self != NULL && self->disableResize);
        return TJS_S_OK;
    }
    static tjs_error TJS_INTF_METHOD setDisableResize(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        WindowEx *self = GetInstance(obj);
        if (self == NULL) return TJS_E_ACCESSDENYED;
        self->disableResize = !!p[0]->AsInteger();
        return TJS_S_OK;
    }

    // property disableMove
    static tjs_error TJS_INTF_METHOD getDisableMove(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        WindowEx *self = GetInstance(obj);
        if (r) *r = (self != NULL && self->disableMove);
        return TJS_S_OK;
    }
    static tjs_error TJS_INTF_METHOD setDisableMove(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        WindowEx *self = GetInstance(obj);
        if (self == NULL) return TJS_E_ACCESSDENYED;
        self->disableMove = !!p[0]->AsInteger();
        //_resetExSystemMenu(self);
        return TJS_S_OK;
    }

    // setOverlayBitmap
    static tjs_error TJS_INTF_METHOD setOverlayBitmap(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        WindowEx *self = GetInstance(obj);
        return (self != NULL) ? self->_setOverlayBitmap(n, p) : TJS_E_ACCESSDENYED;
    }

    // property exSystemMenu
    static tjs_error TJS_INTF_METHOD getExSystemMenu(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        WindowEx *self = GetInstance(obj);
        if (r && self != NULL) *r = tTJSVariant(self->sysMenuModified, self->sysMenuModified);
        return TJS_S_OK;
    }
    static tjs_error TJS_INTF_METHOD setExSystemMenu(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        WindowEx *self = GetInstance(obj);
        if (self == NULL) return TJS_E_ACCESSDENYED;
        if (self->sysMenuModified != NULL) {
            self->resetSystemMenu();
            self->sysMenuModified->Release();
        }
        /**/self->sysMenuModified = p[0]->AsObject();
        /**/self->modifySystemMenu();
        return TJS_S_OK;
    }
    // resetExSystemMenu
    static tjs_error TJS_INTF_METHOD resetExSystemMenu(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        return TJS_S_OK;
    }

    // property enableNCMouseEvent
    static tjs_error TJS_INTF_METHOD getEnNCMEvent(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        WindowEx *self = GetInstance(obj);
        if (r) *r = (self != NULL && self->enableNCMEvent);
        return TJS_S_OK;
    }

    static tjs_error TJS_INTF_METHOD setEnNCMEvent(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        WindowEx *self = GetInstance(obj);
        if (self == NULL) return TJS_E_ACCESSDENYED;
        self->enableNCMEvent = !!p[0]->AsInteger();
        return TJS_S_OK;
    }

    // ncHitTest
    static tjs_error TJS_INTF_METHOD nonClientHitTest(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
       return TJS_S_OK;
    }

    // setMessageHook
    static tjs_error TJS_INTF_METHOD setMessageHook(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        WindowEx *self = GetInstance(obj);
        if (self == NULL) return TJS_E_ACCESSDENYED;
        bool on  = (n >= 1) && !!p[0]->AsInteger();
        bool ret = false;
        if (n >= 2) {
            tjs_int num = -1;
            if (p[1]->Type() == tvtString) {
                ttstr key(*p[1]);
                num = getWindowNotificationNum(key);
            } else {
                num = (tjs_int)*p[1];
            }
            if (num < 0 || num >= 0x400) return TJS_E_FAIL;
            ret = self->setMessageHookOnel(on, num);
        } else {
            ret = self->setMessageHookAll(on);
        }
        if (r) *r = ret;
        return TJS_S_OK;
    }

    // bringTo
    static tjs_error TJS_INTF_METHOD bringTo(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {

        return TJS_S_OK;
    }
    static tjs_error TJS_INTF_METHOD sendToBack(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        return TJS_S_OK;
    }

    void checkUpdateMenuItem(HMENU menu, int pos, UINT id);

    //--------------------------------------------------------------
    // 拡張イベント用

    // メンバが存在するか
    bool hasMember(tjs_char const *name) const {
        tTJSVariant func;
        return TJS_SUCCEEDED(self->PropGet(TJS_MEMBERMUSTEXIST, name, 0, &func, self));
    }

    // TJSメソッド呼び出し
    tjs_error funcCall(tjs_char const *name, tTJSVariant *result, tjs_int numparams = 0, tTJSVariant **params = 0) const {
//        return Try_iTJSDispatch2_FuncCall(self, 0, name, 0, result, numparams, params, self);
		return self->FuncCall(0, name, 0, result, numparams, params, self);
    }

    // 引数なしコールバック
    bool callback(tjs_char const *name) const {
        if (!hasMember(name)) return false;
        tTJSVariant rslt;
        funcCall(name, &rslt, 0, 0);
        return !!rslt.AsInteger();
    }

    // variant渡しコールバック
    bool callback(tjs_char const *name, tTJSVariant *v) const {
        if (!hasMember(name)) return false;
        tTJSVariant rslt;
        funcCall(name, &rslt, 1, &v);
        return !!rslt.AsInteger();
    }

    // 座標渡しコールバック
    bool callback(tjs_char const *name, int x, int y) const {
        tTJSVariant vx(x), vy(y);
        tTJSVariant rslt, *params[] = { &vx, &vy };
        funcCall(name, &rslt, 2, params);
        return !!rslt.AsInteger();
    }

    // ４個渡しコールバック
    bool callback(tjs_char const *name, int a, int b, int c, int d) const {
        tTJSVariant va(a), vb(b), vc(c), vd(d);
        tTJSVariant rslt, *params[] = { &va, &vb, &vc, &vd };
        funcCall(name, &rslt, 4, params);
        return !!rslt.AsInteger();
    }

    bool onMessage(tTVPWindowMessage *mes) {
        switch (mes->Msg) {
            case TVP_WM_ATTACH:
                cachedHWND = (HWND)mes->LParam;
                if (sysMenuModified != NULL) modifySystemMenu();
                break;
            case TVP_WM_DETACH:
                resetSystemMenu();
                if (sysMenu != NULL) {
//                    ::DestroyMenu(sysMenu);
                    sysMenu = NULL;
                    if (cachedHWND != NULL) ::GetSystemMenu(cachedHWND, TRUE);
                }
                cachedHWND = NULL;
                deleteOverlayBitmap();
                break;
        }
        HWND hwnd = cachedHWND;
        if (hwnd == NULL) return false;
        if (enableWinMsgHook && mes->Msg < 0x400 && (bitHooks[mes->Msg>>5] & (1<<(mes->Msg&31)))) {
            if (callback(EXEV_MSGHOOK, mes->Msg, mes->WParam, mes->LParam, 0)) return true;
        }
        switch (mes->Msg) {
            case WM_SETCURSOR:
                if (enableNCMEvent)
                    return callback(EXEV_NCMSEV, (int)LOWORD(mes->LParam), (int)HIWORD(mes->LParam));
                break;
            case WM_MENUCHAR:
                if (callback(EXEV_ACCELKEY, (int)LOWORD(mes->WParam), (int)HIWORD(mes->WParam))) {
                    LRESULT res =     mes->Result = ::DefWindowProc(hwnd, mes->Msg, mes->WParam, mes->LParam);
                    if (!HIWORD(res)) mes->Result |= (1 << 16);
                    return true;
                }
                break;
            case WM_SYSCOMMAND:
                if (sysMenuModMap != NULL && (mes->WParam & 0xFFFF) < 0xF000) {
                    tTJSVariant var;
                    if (TJS_SUCCEEDED(sysMenuModMap->PropGetByNum(0, (tjs_int)(mes->WParam & 0xFFFF), &var, sysMenuModMap))) {
                        return callback(EXEV_SYSMENU, &var);
                    }
                }
                switch (mes->WParam & 0xFFF0) {
                    case SC_MAXIMIZE:     return callback(EXEV_QUERYMAX);
                    case SC_SCREENSAVE:   return callback(EXEV_SCREENSV);
                    case SC_MONITORPOWER: return callback(EXEV_MONITORPW, (int)mes->LParam, 0);
                    case SC_KEYMENU:      return callback(EXEV_KEYMENU,   (int)mes->LParam, 0);
                    case SC_MOVE: if (disableMove) { mes->Result = 0; return true; } break;
                }
                break;
            case WM_SIZE:
                switch (mes->WParam) {
                    case SIZE_MINIMIZED: callback(EXEV_MINIMIZE); break;
                    case SIZE_MAXIMIZED: callback(EXEV_MAXIMIZE); break;
                }
                break;
            case WM_SHOWWINDOW:
                switch (mes->LParam) {
                    case SW_PARENTOPENING: callback(EXEV_SHOW); break;
                    case SW_PARENTCLOSING: callback(EXEV_HIDE); break;
                }
                break;
            case WM_QUERYOPEN:
                callback(EXEV_SHOW);
                break;
            case WM_ENTERSIZEMOVE: callback(EXEV_MVSZBEGIN); break;
            case WM_EXITSIZEMOVE:  callback(EXEV_MVSZEND);   break;
            case WM_SIZING: if (hasResizing) callback(EXEV_RESIZING, (RECT*)mes->LParam, mes->WParam); break;
            case WM_MOVING: if (hasMoving)   callback(EXEV_MOVING,   (RECT*)mes->LParam); break;
            case WM_MOVE:   if (hasMove)     callback(EXEV_MOVE, (int)LOWORD(mes->LParam), (int)HIWORD(mes->LParam)); break;
                // サイズ変更カーソルを抑制
            case WM_NCHITTEST:
                if (disableResize) {
                    LRESULT res = ::DefWindowProc(hwnd, mes->Msg, mes->WParam, mes->LParam);
                    switch (res) {
                        /**/             case HTLEFT:       case HTRIGHT:
                        case HTTOP:       case HTTOPLEFT:    case HTTOPRIGHT:
                        case HTBOTTOM: case HTBOTTOMLEFT: case HTBOTTOMRIGHT:
                            res = HTBORDER;
                            break;
                    }
                    mes->Result = res;
                    return true;
                }
                break;

            case WM_NCLBUTTONDOWN: callback(EXEV_NCMSDOWN, (int)LOWORD(mes->LParam), (int)HIWORD(mes->LParam), 0/*mbLeft*/,   (int)mes->WParam); break;
            case WM_NCRBUTTONDOWN: callback(EXEV_NCMSDOWN, (int)LOWORD(mes->LParam), (int)HIWORD(mes->LParam), 1/*mbRight*/,  (int)mes->WParam); break;
            case WM_NCMBUTTONDOWN: callback(EXEV_NCMSDOWN, (int)LOWORD(mes->LParam), (int)HIWORD(mes->LParam), 2/*mbMiddle*/, (int)mes->WParam); break;

            case WM_NCLBUTTONUP:   callback(EXEV_NCMSUP,   (int)LOWORD(mes->LParam), (int)HIWORD(mes->LParam), 0/*mbLeft*/,   (int)mes->WParam); break;
            case WM_NCRBUTTONUP:   callback(EXEV_NCMSUP,   (int)LOWORD(mes->LParam), (int)HIWORD(mes->LParam), 1/*mbRight*/,  (int)mes->WParam); break;
            case WM_NCMBUTTONUP:   callback(EXEV_NCMSUP,   (int)LOWORD(mes->LParam), (int)HIWORD(mes->LParam), 2/*mbMiddle*/, (int)mes->WParam); break;

            case WM_NCMOUSELEAVE:  callback(EXEV_NCMSLEAVE); break;
            case WM_NCMOUSEMOVE:
                if (hasNcMsMove)   callback(EXEV_NCMSMOVE, (int)LOWORD(mes->LParam), (int)HIWORD(mes->LParam), (int)mes->WParam, 0);
                break;
            case WM_INITMENUPOPUP:
                if (HIWORD(mes->LParam)) {
                    if (sysMenu != NULL && sysMenu == (HMENU)mes->WParam) modifySystemMenu();
                    if (disableResize || disableMove) {
                        // システムメニューサイズ変更抑制
                        mes->Result = ::DefWindowProc(hwnd, mes->Msg, mes->WParam, mes->LParam);
                        if (disableResize) ::EnableMenuItem((HMENU)mes->WParam, SC_SIZE, MF_GRAYED | MF_BYCOMMAND);
                        if (disableMove)   ::EnableMenuItem((HMENU)mes->WParam, SC_MOVE, MF_GRAYED | MF_BYCOMMAND);
                        return true;
                    }
                } else if (menuex) {
                    // メニューアイコン強制差し替え
                    mes->Result = ::DefWindowProc(hwnd, mes->Msg, mes->WParam, mes->LParam);
                    HMENU menu = (HMENU)mes->WParam;
                    checkUpdateMenuItem(menu);
                    return true;
                }
                break;
            case WM_NCPAINT:
                if (menuex) checkUpdateMenuItem(::GetMenu(hwnd));
                break;

                // メニュー開始終了
            case WM_ENTERMENULOOP:
                callback(EXEV_ENTERMENU);
                break;
            case WM_EXITMENULOOP:
                callback(EXEV_EXITMENU);
                break;

                // ディスプレイモード変更通知
            case WM_DISPLAYCHANGE:
                callback(EXEV_DISPCHG);
                break;

            case WM_ACTIVATE:
                return callback(EXEV_ACTIVATE, (int)(mes->WParam & 0xFFFF), (int)((mes->WParam >> 16) & 0xFFFF));
        }
        return false;
    }

    // メニュー更新処理（MenuItemEx用）
    void setMenuItemID(iTJSDispatch2*, uint , bool);

    // Message Receiver 登録・解除
    void regist(bool en) {

    }

    // ネイティブインスタンスの生成・破棄にあわせてレシーバを登録・解除する
    WindowEx(iTJSDispatch2 *obj)
            :   self(obj), menuex(0),
                sysMenuModified(0), sysMenuModMap(0),
                disableResize(false),
                disableMove(false),
                enableNCMEvent(false),
                enableWinMsgHook(false)
    {
        regist(true);
        setMessageHookAll(false);
    }

    ~WindowEx() {
        if (menuex)          menuex         ->Release();
        if (sysMenuModified) sysMenuModified->Release();
        resetSystemMenu();
        deleteOverlayBitmap();
        regist(false);
    }

    void checkExEvents() {
    }
    void deleteOverlayBitmap() {
    }

    void resetSystemMenu();
    void modifySystemMenu();
    bool setMessageHookOnel(bool on, tjs_int num) {
        return enableWinMsgHook;
    }

    bool setMessageHookAll(bool on) {
        return (enableWinMsgHook = on);
    }

    static tjs_int getWindowNotificationNum(ttstr key) {
        return 0;
    }

    static ttstr getWindowNotificationName(tjs_int num) {
        tTJSVariant tmp;
        if (!_getNotificationVariant(tmp))
            TVPThrowExceptionMessage(TJS_W("cache setup failed."));
        ncbPropAccessor nf(tmp);
        return nf.getStrValue(num);
    }

protected:
    tjs_error _setOverlayBitmap(tjs_int n, tTJSVariant **p) {
        return TJS_S_OK;
    }

    static bool _getNotificationVariant(tTJSVariant &tmp) {
        return true;
    }

private:
    iTJSDispatch2 *self, *menuex;
    iTJSDispatch2 *sysMenuModified, *sysMenuModMap; //< システムメニュー改変用
    bool disableResize; //< サイズ変更禁止
    bool disableMove; //< ウィンドウ移動禁止
    bool enableNCMEvent; //< WM_SETCURSORコールバック
    bool enableWinMsgHook; //< メッセージフック有効
    HWND cachedHWND;
    HMENU sysMenu;
};


// 拡張イベント用ネイティブインスタンスゲッタ
NCB_GET_INSTANCE_HOOK(WindowEx)
        {
                /**/  NCB_GET_INSTANCE_HOOK_CLASS () {}
        /**/ ~NCB_GET_INSTANCE_HOOK_CLASS () {}
        NCB_INSTANCE_GETTER(objthis) {
            ClassT* obj = GetNativeInstance(objthis);
            if (!obj) SetNativeInstance(objthis, (obj = new ClassT(objthis)));
            return obj;
        }
        };
// メソッド追加
NCB_ATTACH_CLASS_WITH_HOOK(WindowEx, Window)
{
Variant(TJS_W("nchtError"),       (tjs_int)(HTERROR & 0xFFFF));
Variant(TJS_W("nchtTransparent"), (tjs_int)(HTTRANSPARENT & 0xFFFF));
Variant(TJS_W("nchtNoWhere"),     (tjs_int)HTNOWHERE);
Variant(TJS_W("nchtClient"),      (tjs_int)HTCLIENT);
Variant(TJS_W("nchtCaption"),     (tjs_int)HTCAPTION);
Variant(TJS_W("nchtSysMenu"),     (tjs_int)HTSYSMENU);
Variant(TJS_W("nchtSize"),        (tjs_int)HTSIZE);
Variant(TJS_W("nchtGrowBox"),     (tjs_int)HTGROWBOX);
Variant(TJS_W("nchtMenu"),        (tjs_int)HTMENU);
Variant(TJS_W("nchtHScroll"),     (tjs_int)HTHSCROLL);
Variant(TJS_W("nchtVScroll"),     (tjs_int)HTVSCROLL);
Variant(TJS_W("nchtMinButton"),   (tjs_int)HTMINBUTTON);
Variant(TJS_W("nchtReduce"),      (tjs_int)HTREDUCE);
Variant(TJS_W("nchtMaxButton"),   (tjs_int)HTMAXBUTTON);
Variant(TJS_W("nchtZoom"),        (tjs_int)HTZOOM);
Variant(TJS_W("nchtLeft"),        (tjs_int)HTLEFT);
Variant(TJS_W("nchtRight"),       (tjs_int)HTRIGHT);
Variant(TJS_W("nchtTop"),         (tjs_int)HTTOP);
Variant(TJS_W("nchtTopLeft"),     (tjs_int)HTTOPLEFT);
Variant(TJS_W("nchtTopRight"),    (tjs_int)HTTOPRIGHT);
Variant(TJS_W("nchtBottom"),      (tjs_int)HTBOTTOM);
Variant(TJS_W("nchtBottomLeft"),  (tjs_int)HTBOTTOMLEFT);
Variant(TJS_W("nchtBottomRight"), (tjs_int)HTBOTTOMRIGHT);
Variant(TJS_W("nchtBorder"),      (tjs_int)HTBORDER);

RawCallback(TJS_W("minimize"),           &Class::minimize,          0);
RawCallback(TJS_W("maximize"),           &Class::maximize,          0);
//RawCallback(TJS_W("maximizeBox"),         &Class::getMaximizeBox,      &Class::setMaximizeBox, 0);
//RawCallback(TJS_W("minimizeBox"),         &Class::getMinimizeBox,      &Class::setMinimizeBox, 0);
//RawCallback(TJS_W("maximized"),           &Class::getMaximized,      &Class::setMaximized, 0);
//RawCallback(TJS_W("minimized"),           &Class::getMinimized,      &Class::setMinimized, 0);
RawCallback(TJS_W("showRestore"),        &Class::showRestore,       0);
RawCallback(TJS_W("resetWindowIcon"),    &Class::resetWindowIcon,   0);
RawCallback(TJS_W("setWindowIcon"),      &Class::setWindowIcon,     0);
RawCallback(TJS_W("getWindowRect"),      &Class::getWindowRect,     0);
RawCallback(TJS_W("getClientRect"),      &Class::getClientRect,     0);
RawCallback(TJS_W("getNormalRect"),      &Class::getNormalRect,     0);
RawCallback(TJS_W("disableResize"),       &Class::getDisableResize,  &Class::setDisableResize, 0);
RawCallback(TJS_W("disableMove"),         &Class::getDisableMove,    &Class::setDisableMove, 0);
RawCallback(TJS_W("setOverlayBitmap"),    &Class::setOverlayBitmap,  0);
RawCallback(TJS_W("exSystemMenu"),        &Class::getExSystemMenu,   &Class::setExSystemMenu, 0);
RawCallback(TJS_W("resetExSystemMenu"),   &Class::resetExSystemMenu, 0);
RawCallback(TJS_W("enableNCMouseEvent"),  &Class::getEnNCMEvent,     &Class::setEnNCMEvent, 0);
RawCallback(TJS_W("ncHitTest"),           &Class::nonClientHitTest,  0);
RawCallback(TJS_W("focusMenuByKey"),      &Class::focusMenuByKey,    0);
RawCallback(TJS_W("setMessageHook"),      &Class::setMessageHook,    0);
RawCallback(TJS_W("bringTo"),             &Class::bringTo,           0);
RawCallback(TJS_W("sendToBack"),          &Class::sendToBack,        0);

Method(     TJS_W("registerExEvent"),     &Class::checkExEvents);
Method(     TJS_W("getNotificationNum"),  &Class::getWindowNotificationNum);
Method(     TJS_W("getNotificationName"), &Class::getWindowNotificationName);
}

////////////////////////////////////////////////////////////////
struct MenuItemEx
{
    enum { BMP_ITEM, BMP_CHK, BMP_UNCHK, BMP_MAX };
    enum { BMT_NONE, BMT_SYS, BMT_BMP };

    // メニューを取得
    static HMENU GetHMENU(iTJSDispatch2 *obj) {
        if (!obj) return NULL;
        tTJSVariant val;
        iTJSDispatch2 *global = TVPGetScriptDispatch(), *mi;
        if (global) {
            global->PropGet(0, TJS_W("MenuItem"), 0, &val, obj);
            mi = val.AsObjectNoAddRef();
            val.Clear();
            global->Release();
        } else mi = obj;
        mi->PropGet(0, TJS_W("HMENU"), 0, &val, obj);
        return (HMENU)(tjs_int64)(val);
    }

    // 親メニューを取得
    static iTJSDispatch2* GetParentMenu(iTJSDispatch2 *obj) {
        tTJSVariant val;
        obj->PropGet(0, TJS_W("parent"), 0, &val, obj);
        return val.AsObjectNoAddRef();
    }
    // ルートメニューの子かどうか
    static bool IsRootChild(iTJSDispatch2 *obj) {
        tTJSVariant par, root;
        obj->PropGet(0, TJS_W("parent"), 0, &par,  obj);
        obj->PropGet(0, TJS_W("root"),   0, &root, obj);
        iTJSDispatch2 *p =  par.AsObjectNoAddRef();
        iTJSDispatch2 *r = root.AsObjectNoAddRef();
        return (p && r && p == r);
    }
    // （泥臭い手段で）インデックスを取得
    static UINT GetIndex(iTJSDispatch2 *obj, iTJSDispatch2 *parent) {
        tTJSVariant val, child;
        parent->PropGet(0, TJS_W("children"), 0, &child, parent);
        ncbPropAccessor charr(child);
        if (!charr.IsValid()) return (UINT)-1;

        obj->PropGet(0, TJS_W("index"), 0, &val, obj);
        int max = (int)val.AsInteger();
        UINT ret = (UINT)max;
        for (int i = 0; i <= max; i++) {
            tTJSVariant vitem;
            if (charr.checkVariant(i, vitem)) {
                ncbPropAccessor item(vitem);
                if (item.IsValid()) {
                    // 非表示の場合はカウントされない
                    if (!item.getIntValue(TJS_W("visible"))) {
                        if (i == max) return (UINT)-1;
                        ret--;
                    }
                }
            }
        }
        return ret;
    }
    // ウィンドウを取得
    static iTJSDispatch2* GetWindow(iTJSDispatch2 *obj) {
        if (!obj) return NULL;
        tTJSVariant val;
        obj->PropGet(0, TJS_W("root"), 0, &val, obj);
        obj = val.AsObjectNoAddRef();
        if (!obj) return NULL;
        val.Clear();
        obj->PropGet(0, TJS_W("window"), 0, &val, obj);
        return val.AsObjectNoAddRef();
    }
    static HWND GetHWND(iTJSDispatch2 *obj) {
        iTJSDispatch2 *win = GetWindow(obj);
        return nullptr;
    }

    // property rightJustify
    tjs_int getRightJustify() const { return rj > 0; }
    void setRightJustify(tTJSVariant v) {
        rj = !!v.AsInteger();
        updateMenuItemInfo();
    }

    // property bmpItem
    tjs_int getBmpItem() const { return getBmpSelect(BMP_ITEM); }
    void setBmpItem(tTJSVariant v) { setBmpSelect(v, BMP_ITEM); }

    // property bmpChecked
    tjs_int getBmpChecked() const { return getBmpSelect(BMP_CHK); }
    void setBmpChecked(tTJSVariant v) { setBmpSelect(v, BMP_CHK); }

    // property bmpUnchecked
    tjs_int getBmpUnchecked() const { return getBmpSelect(BMP_UNCHK); }
    void setBmpUnchecked(tTJSVariant v) { setBmpSelect(v, BMP_UNCHK); }

    tjs_int64 getBmpSelect(int sel) const {
        switch (bmptype[sel]) {
            case BMT_BMP: return -1;
            default:      return 0;
        }
    }
    void setBmpSelect(tTJSVariant &v, int sel) {
        updateMenuItemInfo();
    }

    UINT setMenuItemInfo(HMENU hmenu, int index_or_id, bool is_index) {
        return 0;
    }

    void updateMenuItemInfo() {
        iTJSDispatch2 *parent = GetParentMenu(obj);
        HMENU hmenu = GetHMENU(parent);
        if (hmenu == NULL) TVPThrowExceptionMessage(TJS_W("Cannot get parent menu."));

        if (!id || !setMenuItemInfo(hmenu, id, false)) {
            if (setMenuItemInfo(hmenu, GetIndex(obj, parent), true))
                updateMenuItemID();
        }
    }

    static UINT GetMenuItemID(iTJSDispatch2 *obj) {
        return 0;
    }
    void updateMenuItemID() {
        if (id != 0) setMenuItemID(false);
        id = GetMenuItemID(obj);
    }

    void setMenuItemID(bool isset) {
        iTJSDispatch2 *win = GetWindow(obj);
        if (win) {
            WindowEx *wex = WindowEx::GetInstance(win);
            if (wex)  wex->setMenuItemID(obj, id, isset);
        }
    }

    MenuItemEx(iTJSDispatch2* _obj) : obj(_obj), id(0), rj(-1) {
        updateMenuItemID();
    }
    ~MenuItemEx() {
        setMenuItemID(false);
    }
private:
    iTJSDispatch2 *obj;
    UINT id;
    tjs_int  rj;
    int    bmptype[BMP_MAX];

public:
    static bool InsertMenuItem(HMENU menu, iTJSDispatch2 *obj, WORD &curid, WORD idmv, iTJSDispatch2 *items, DWORD sysdt) {
        return true;
    }

    static HMENU CreateMenuList(HMENU menu, iTJSDispatch2 *obj, WORD &curid, WORD idmv, iTJSDispatch2 *items, ULONG_PTR sysdt) {
       return nullptr;
    }

    // MenuItem.popupEx(flags, x=cursorX, y=cursorY, hwnd=this.root.window, rect, menulist=this.children)
    static tjs_error TJS_INTF_METHOD popupEx(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *objthis) {
        return TJS_S_OK;
    }
};
NCB_GET_INSTANCE_HOOK(MenuItemEx)
        {
                /**/  NCB_GET_INSTANCE_HOOK_CLASS () {}
        /**/ ~NCB_GET_INSTANCE_HOOK_CLASS () {}
        NCB_INSTANCE_GETTER(objthis) {
            ClassT* obj = GetNativeInstance(objthis);
            if (!obj) SetNativeInstance(objthis, (obj = new ClassT(objthis)));
            return obj;
        }
        };
// Note: MIIM_TYPE is replaced by MIIM_BITMAP, MIIM_FTYPE, and MIIM_STRING.
#define HBMMENU_CALLBACK        -1
#define HBMMENU_SYSTEM          1
#define HBMMENU_MBAR_RESTORE    2
#define HBMMENU_MBAR_MINIMIZE   3
#define HBMMENU_MBAR_CLOSE      5
#define HBMMENU_MBAR_CLOSE_D    6
#define HBMMENU_MBAR_MINIMIZE_D 7
#define HBMMENU_POPUP_CLOSE     8
#define HBMMENU_POPUP_RESTORE   9
#define HBMMENU_POPUP_MAXIMIZE  10
#define HBMMENU_POPUP_MINIMIZE  11

NCB_ATTACH_CLASS_WITH_HOOK(MenuItemEx, MenuItem)
{
Variant(TJS_W("biSystem"),           (tjs_int64)HBMMENU_SYSTEM);
Variant(TJS_W("biRestore"),          (tjs_int64)HBMMENU_MBAR_RESTORE);
Variant(TJS_W("biMinimize"),         (tjs_int64)HBMMENU_MBAR_MINIMIZE);
Variant(TJS_W("biClose"),            (tjs_int64)HBMMENU_MBAR_CLOSE);
Variant(TJS_W("biCloseDisabled"),    (tjs_int64)HBMMENU_MBAR_CLOSE_D);
Variant(TJS_W("biMinimizeDisabled"), (tjs_int64)HBMMENU_MBAR_MINIMIZE_D);
Variant(TJS_W("biPopupClose"),       (tjs_int64)HBMMENU_POPUP_CLOSE);
Variant(TJS_W("biPopupRestore"),     (tjs_int64)HBMMENU_POPUP_RESTORE);
Variant(TJS_W("biPopupMaximize"),    (tjs_int64)HBMMENU_POPUP_MAXIMIZE);
Variant(TJS_W("biPopupMinimize"),    (tjs_int64)HBMMENU_POPUP_MINIMIZE);

Property(TJS_W("rightJustify"), &Class::getRightJustify, &Class::setRightJustify);
Property(TJS_W("bmpItem"),      &Class::getBmpItem,      &Class::setBmpItem     );
Property(TJS_W("bmpChecked"),   &Class::getBmpChecked,   &Class::setBmpChecked  );
Property(TJS_W("bmpUnchecked"), &Class::getBmpUnchecked, &Class::setBmpUnchecked);
}
NCB_ATTACH_FUNCTION(popupEx, MenuItem, MenuItemEx::popupEx);


void WindowEx::checkUpdateMenuItem(HMENU menu, int pos, UINT id) {
    if (id == 0 || id == (UINT)-1) return;

    ttstr idstr((tjs_int)(id));
    tTJSVariant var;

    tjs_error chk = menuex->PropGet(TJS_MEMBERMUSTEXIST, idstr.c_str(), idstr.GetHint(),  &var, menuex);
    if (TJS_SUCCEEDED(chk) && var.Type() == tvtObject) {
        iTJSDispatch2 *obj = var.AsObjectNoAddRef();
        MenuItemEx *ex = ncbInstanceAdaptor<MenuItemEx>::GetNativeInstance(obj);
        if (ex != NULL) ex->setMenuItemInfo(menu, pos, true);
    }
}

void WindowEx::setMenuItemID(iTJSDispatch2* obj, UINT id, bool set) {
    if (id == 0 || id == (UINT)-1) return;

    ttstr idstr((tjs_int)(id));
    tTJSVariant var(obj, obj);

    if (!menuex) menuex = TJSCreateDictionaryObject();
    menuex->PropSet(TJS_MEMBERENSURE, idstr.c_str(), idstr.GetHint(),  &var, menuex);
}

void WindowEx::resetSystemMenu() {
    if (sysMenuModMap != NULL) sysMenuModMap->Release();
    sysMenuModMap = NULL;
}
void WindowEx::modifySystemMenu() {

}


////////////////////////////////////////////////////////////////
struct ConsoleEx
{
    static tjs_error TJS_INTF_METHOD restoreMaximize(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        return TJS_S_OK;
    }

    static tjs_error TJS_INTF_METHOD maximize(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        return TJS_S_OK;
    }

    // getRect
    static tjs_error TJS_INTF_METHOD getRect(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        return TJS_S_OK;
    }

    static tjs_error TJS_INTF_METHOD setPos(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        if (n < 2) return TJS_E_BADPARAMCOUNT;
        return TJS_S_OK;
    }
    static tjs_error TJS_INTF_METHOD bringAfter(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        return TJS_S_OK;
    }

    static tjs_error TJS_INTF_METHOD getPlacement(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        return TJS_S_OK;
    }
    static tjs_error TJS_INTF_METHOD setPlacement(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        if (n < 1)return TJS_E_BADPARAMCOUNT;
        if (p[0]->Type() != tvtObject) return TJS_E_INVALIDPARAM;
        return TJS_S_OK;
    }
};

NCB_ATTACH_FUNCTION_WITHTAG(restoreMaximize, Debug_console, Debug.console, ConsoleEx::restoreMaximize);
NCB_ATTACH_FUNCTION_WITHTAG(maximize,        Debug_console, Debug.console, ConsoleEx::maximize);
NCB_ATTACH_FUNCTION_WITHTAG(getRect,         Debug_console, Debug.console, ConsoleEx::getRect);
NCB_ATTACH_FUNCTION_WITHTAG(setPos,          Debug_console, Debug.console, ConsoleEx::setPos);
NCB_ATTACH_FUNCTION_WITHTAG(getPlacement,    Debug_console, Debug.console, ConsoleEx::getPlacement);
NCB_ATTACH_FUNCTION_WITHTAG(setPlacement,    Debug_console, Debug.console, ConsoleEx::setPlacement);
NCB_ATTACH_FUNCTION_WITHTAG(bringAfter,      Debug_console, Debug.console, ConsoleEx::bringAfter);


////////////////////////////////////////////////////////////////
struct PadEx
{
    struct SearchWork {
        ttstr name, title;
        HWND result;
    };

    static HWND GetHWND(iTJSDispatch2 *obj) {
        tTJSVariant val, _uuid;
        TVPExecuteExpression(TJS_W("System.createUUID()"), &_uuid);
        obj->PropGet(0, TJS_W("title"), 0, &val,   obj);
        obj->PropSet(0, TJS_W("title"), 0, &_uuid, obj);

        SearchWork wk = { TJS_W("TTVPPadForm"), _uuid, NULL };
        obj->PropSet(0, TJS_W("title"), 0, &val, obj);
        return wk.result;
    }


    // メンバが存在するか
    bool hasMember(tjs_char const *name) const {
        tTJSVariant func;
        return TJS_SUCCEEDED(self->PropGet(TJS_MEMBERMUSTEXIST, name, 0, &func, self));
    }

    // TJSメソッド呼び出し
    tjs_error funcCall(tjs_char const *name, tTJSVariant *result, tjs_int numparams = 0, tTJSVariant **params = 0) const {
//        return Try_iTJSDispatch2_FuncCall(self, 0, name, 0, result, numparams, params, self);
		return self->FuncCall(0, name, 0, result, numparams, params, self);
    }

    // 引数なしコールバック
    bool callback(tjs_char const *name) const {
        if (!hasMember(name)) return false;
        tTJSVariant rslt;
        funcCall(name, &rslt, 0, 0);
        return !!rslt.AsInteger();
    }

    void onClose() { callback(TJS_W("onClose")); }


    PadEx(iTJSDispatch2 *obj) : self(obj), hwnd(0) {}
    ~PadEx()                { }
    void registerExEvents() { }

private:
    iTJSDispatch2 *self;
    HWND hwnd;
};

NCB_GET_INSTANCE_HOOK(PadEx)
        {
                /**/  NCB_GET_INSTANCE_HOOK_CLASS () {}
        /**/ ~NCB_GET_INSTANCE_HOOK_CLASS () {}
        NCB_INSTANCE_GETTER(objthis) {
            ClassT* obj = GetNativeInstance(objthis);
            if (!obj) SetNativeInstance(objthis, (obj = new ClassT(objthis)));
            return obj;
        }
        };
NCB_ATTACH_CLASS_WITH_HOOK(PadEx, Pad)
{
Method(     TJS_W("registerExEvent"),     &Class::registerExEvents);
}
////////////////////////////////////////////////////////////////

struct System
{
    static tjs_int getDoubleClickTime() { return -1; }
    static tjs_error TJS_INTF_METHOD getDisplayMonitors(
            tTJSVariant *result, tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis)
    {
        return TJS_S_OK;
    }

    static tjs_error TJS_INTF_METHOD getMonitorInfo(
            tTJSVariant *result, tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis)
    {
        return TJS_S_OK;
    }

    static tjs_error TJS_INTF_METHOD getCursorPos(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        return TJS_S_OK;
    }

    static tjs_error TJS_INTF_METHOD setCursorPos(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        if (n < 2) return TJS_E_BADPARAMCOUNT;
        return TJS_S_OK;
    }

    static tjs_error TJS_INTF_METHOD getSystemMetrics(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *objthis) {
        if (n < 1) return TJS_E_BADPARAMCOUNT;

        if (p[0]->Type() != tvtString) return TJS_E_INVALIDPARAM;
        ttstr key(p[0]->AsStringNoAddRef());
        if (key == TJS_W("")) return TJS_E_INVALIDPARAM;
        key.ToUppserCase();

        tTJSVariant tmp;
        iTJSDispatch2 *obj =  TVPGetScriptDispatch();
        bool hasval = TJS_SUCCEEDED(obj->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("System"), 0, &tmp, obj));
        obj->Release();
        if (!hasval) return TJS_E_FAIL;

        obj = tmp.AsObjectNoAddRef();
        tmp.Clear();
        if (TJS_FAILED(obj->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("metrics"), 0, &tmp, obj))) {
            ncbDictionaryAccessor dict;
            tmp = dict;
            if (TJS_FAILED(obj->PropSet(TJS_MEMBERENSURE, TJS_W("metrics"), 0, &tmp, obj)))
                return TJS_E_FAIL;
        }
        ncbPropAccessor metrics(tmp);
        tjs_int num = metrics.getIntValue(key.c_str(), -1);
        if (num < 0) return TJS_E_INVALIDPARAM;
        return TJS_S_OK;
    }

    static tjs_error TJS_INTF_METHOD readEnvValue(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *objthis) {
        if (n < 1) return TJS_E_BADPARAMCOUNT;
        if (p[0]->Type() != tvtString) return TJS_E_INVALIDPARAM;
        ttstr name(p[0]->AsStringNoAddRef());
        if (name == TJS_W("")) return TJS_E_INVALIDPARAM;
        return TJS_S_OK;
    }

    static tjs_error TJS_INTF_METHOD expandEnvString(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *objthis) {
        if (n < 1) return TJS_E_BADPARAMCOUNT;
        return TJS_S_OK;
    }

    static tjs_error TJS_INTF_METHOD setApplicationIcon(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *obj) {
        return TJS_S_OK;
    }

    static bool setIconicPreview(bool en) {
        return true;
    }
};

// Systemに関数を追加
NCB_ATTACH_FUNCTION(getDisplayMonitors, System, System::getDisplayMonitors);
NCB_ATTACH_FUNCTION(getMonitorInfo,     System, System::getMonitorInfo);
NCB_ATTACH_FUNCTION(getCursorPos,       System, System::getCursorPos);
NCB_ATTACH_FUNCTION(setCursorPos,       System, System::setCursorPos);
NCB_ATTACH_FUNCTION(getSystemMetrics,   System, System::getSystemMetrics);
NCB_ATTACH_FUNCTION(readEnvValue,       System, System::readEnvValue);
NCB_ATTACH_FUNCTION(expandEnvString,    System, System::expandEnvString);
NCB_ATTACH_FUNCTION(setApplicationIcon, System, System::setApplicationIcon);
NCB_ATTACH_FUNCTION(setIconicPreview,   System, System::setIconicPreview);
NCB_ATTACH_FUNCTION(getDoubleClickTime, System, System::getDoubleClickTime);
NCB_ATTACH_FUNCTION(breathe,            System, TVPBreathe);
NCB_ATTACH_FUNCTION(isBreathing,        System, TVPGetBreathing);
NCB_ATTACH_FUNCTION(clearGraphicCache,  System, TVPClearGraphicCache);
NCB_ATTACH_FUNCTION(getAboutString,     System, TVPGetAboutString);
NCB_ATTACH_FUNCTION(getCPUType,         System, TVPGetCPUType);

////////////////////////////////////////////////////////////////

struct Scripts
{
    static bool outputErrorLogOnEval;

    // property Scripts.outputErrorLogOnEval
    static bool   setEvalErrorLog(bool v) {
        bool ret = outputErrorLogOnEval;
        /**/       outputErrorLogOnEval = v;
        return ret;
    }

    // Scripts.eval オーバーライド
    static tjs_error TJS_INTF_METHOD eval(tTJSVariant *r, tjs_int n, tTJSVariant **p, iTJSDispatch2 *objthis) {
        if (outputErrorLogOnEval) return evalOrig->FuncCall(0, NULL, NULL, r, n, p, objthis);

        if(n < 1) return TJS_E_BADPARAMCOUNT;
        ttstr content = *p[0], name;
        tjs_int lineofs = 0;
        if(n >= 2) name    = *p[1];
        if(n >= 3) lineofs = *p[2];

        TVPExecuteExpression(content, name, lineofs, r);
        return TJS_S_OK;
    }
    // 元の Scripts.eval を保存・復帰
    static void Regist() {
        tTJSVariant var;
        TVPExecuteExpression(TJS_W("Scripts.eval"), &var);
        evalOrig = var.AsObject();
    }
    static void UnRegist() {
        if (evalOrig) evalOrig->Release();
        evalOrig = NULL;
    }
    static iTJSDispatch2 *evalOrig;
};

iTJSDispatch2 * Scripts::evalOrig = NULL;  // Scripts.evalの元のオブジェクト
bool            Scripts::outputErrorLogOnEval = true; // 切り替えフラグ

// Scriptsに関数を追加
NCB_ATTACH_FUNCTION(eval,            Scripts, Scripts::eval);
NCB_ATTACH_FUNCTION(setEvalErrorLog, Scripts, Scripts::setEvalErrorLog);

////////////////////////////////////////////////////////////////
// コールバック指定

static void PreRegistCallback()
{
    Scripts::Regist();
}

static void PostUnregistCallback()
{
    Scripts::UnRegist();
}
NCB_PRE_REGIST_CALLBACK(      PreRegistCallback);
NCB_POST_UNREGIST_CALLBACK(PostUnregistCallback);

static HWND FUN_00b11b94(HWND hwnd, int* r) {
    if(!hwnd) return hwnd;

    undefined auVar3 [16];
    long local_48 = (long)hwnd + 0x10;

    if (*(long *)((long)hwnd + 8) != 0) {
        local_48 = *(long *)((long)hwnd + 8);
    }
    auVar3 = FUN_00ae89a8(local_40,&local_48);
    if ((auVar3 & (undefined  [16])0x1) == (undefined  [16])0x0) {
        switchD_00b11bfc_caseD_5:
        local_40[0] = (void *)0x0;
    }
    return nullptr;
}

static void PostMessage(iTJSDispatch2* obj, tjs_int n) {
    int local_30 = 0;
    int r;
    HWND hwnd = WindowEx::GetHWND(obj);

    switch(local_30) {
        case 1:
            /* WARNING: Subroutine does not return */
//            FUN_00b0cd8c(hwnd, 4);
            std::abort();
        case 2:
            FUN_00b11b94(hwnd, &r); // 这可能表示正常的消息处理，通常对应于常见的窗口消息（如 WM_PAINT、WM_TIMER 等）或成功完成的操作。
        break;
        case 3:
            std::abort();
            /* WARNING: Subroutine does not return */
//            FUN_00b0cd8c(hwnd, 4);// 可能对应于消息处理失败（如 WM_QUIT、WM_DESTROY）或其他需要终止的状态。
        case 4:
            break;
    }
}
