//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and
   contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Text Editor
//---------------------------------------------------------------------------
#ifndef PadImplH
#define PadImplH
//---------------------------------------------------------------------------
#include "tjsNative.h"
#include "PadIntf.h"

class TTVPPadForm;

//---------------------------------------------------------------------------
// tTJSNI_Pad : Pad Class C++ Native Instance
//---------------------------------------------------------------------------
class tTJSNI_Pad : public tTJSNI_BasePad {
    TTVPPadForm *Form{};

public:
    tjs_error Construct(tjs_int numparams, tTJSVariant **param,
                        iTJSDispatch2 *dsp) override;

    void Invalidate() override;

    // methods
    virtual void OpenFromStorage(const ttstr &name);

    virtual void SaveToStorage(const ttstr &name);

    // properties
    [[nodiscard]] virtual tjs_uint32 GetColor() const;

    virtual void SetColor(tjs_uint32 color);

    [[nodiscard]] virtual bool GetVisible() const;

    virtual void SetVisible(bool state);

    [[nodiscard]] virtual ttstr GetFileName() const;

    virtual void SetFileName(const ttstr &name);

    virtual void SetText(const ttstr &content);

    [[nodiscard]] virtual ttstr GetText() const;

    virtual void SetTitle(const ttstr &title);

    [[nodiscard]] virtual ttstr GetTitle() const;

    [[nodiscard]] virtual tjs_uint32 GetFontColor() const;

    virtual void SetFontColor(tjs_uint32 color);

    [[nodiscard]] virtual tjs_int GetFontHeight() const; // pixel
    virtual void SetFontHeight(tjs_int height);

    [[nodiscard]] virtual tjs_int GetFontSize() const; // point
    virtual void SetFontSize(tjs_int size);

    [[nodiscard]] virtual bool ContainsFontStyle(tjs_int style) const;

    virtual void AddFontStyle(tjs_int style);

    virtual void RemoveFontStyle(tjs_int style);

    [[nodiscard]] virtual ttstr GetFontName() const;

    virtual void SetFontName(const ttstr &name);

    [[nodiscard]] virtual bool IsReadOnly() const;

    virtual void SetReadOnly(bool ro);

    [[nodiscard]] virtual bool GetWordWrap() const;

    virtual void SetWordWrap(bool ww);

    [[nodiscard]] virtual tjs_int GetOpacity() const;

    virtual void SetOpacity(tjs_int opa);

    [[nodiscard]] virtual bool GetStatusBarVisible() const;

    virtual void SetStatusBarVisible(bool vis);

    [[nodiscard]] virtual tjs_int GetScrollBarsVisible() const;

    virtual void SetScrollBarsVisible(tjs_int vis);

    [[nodiscard]] virtual tjs_int GetBorderStyle() const;

    virtual void SetBorderStyle(tjs_int style);

    [[nodiscard]] virtual ttstr GetStatusText() const;

    virtual void SetStatusText(const ttstr &title);

    // form position and size
    [[nodiscard]] virtual tjs_int GetFormHeight() const;

    [[nodiscard]] virtual tjs_int GetFormWidth() const;

    [[nodiscard]] virtual tjs_int GetFormTop() const;

    [[nodiscard]] virtual tjs_int GetFormLeft() const;

    virtual void SetFormHeight(tjs_int value);

    virtual void SetFormWidth(tjs_int value);

    virtual void SetFormTop(tjs_int value);

    virtual void SetFormLeft(tjs_int value);

    //
    [[nodiscard]] virtual bool GetUserCreationMode() const;

    virtual void SetUserCreationMode(bool mode);

protected:
    bool UserCreationMode{}; // true if this form was created by the
                             // userscript,
    // otherwise (when created by the system as "Script Editor") this
    // will be false
    bool MultilineMode{};

private:
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#endif
