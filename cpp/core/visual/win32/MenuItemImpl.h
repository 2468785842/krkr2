//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and
   contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// "MenuItem" class implementation
//---------------------------------------------------------------------------
#ifndef MenuItemImplH
#define MenuItemImplH

// #include <Menus.hpp>
#include "MenuItemIntf.h"
// #include "Menus.hpp"

//---------------------------------------------------------------------------
// tTJSNI_MenuItem : MenuItem Native Instance
//---------------------------------------------------------------------------
class tTJSNI_MenuItem : public tTJSNI_BaseMenuItem {
    typedef tTJSNI_BaseMenuItem inherited;

    // TMenuItem* MenuItem;

    ttstr Caption;
    ttstr Shortcut;
    bool IsAttched;
    bool IsChecked;
    bool IsEnabled;
    bool IsRadio;
    bool IsVisible;

    tjs_int GroupIndex;

public:
    tTJSNI_MenuItem();
    tjs_error Construct(tjs_int numparams, tTJSVariant **param,
                        iTJSDispatch2 *tjs_obj) override;
    void Invalidate() override;

private:
    void MenuItemClick();

protected:
    [[nodiscard]] bool CanDeliverEvents() const override;
    // tTJSNI_BaseMenuItem::CanDeliverEvents override

public:
    void Add(tTJSNI_MenuItem *item) override;
    void Insert(tTJSNI_MenuItem *item, tjs_int index) override;
    void Remove(tTJSNI_MenuItem *item) override;

    void SetCaption(const ttstr &caption);
    void GetCaption(ttstr &caption) const;

    void SetChecked(bool b);
    [[nodiscard]] bool GetChecked() const;

    void SetEnabled(bool b);
    [[nodiscard]] bool GetEnabled() const;

    void SetGroup(tjs_int g);
    [[nodiscard]] tjs_int GetGroup() const;

    void SetRadio(bool b);
    [[nodiscard]] bool GetRadio() const;

    void SetShortcut(const ttstr &shortcut);
    void GetShortcut(ttstr &shortcut) const;

    void SetVisible(bool b);
    [[nodiscard]] bool GetVisible() const;

    [[nodiscard]] tjs_int GetIndex() const;
    void SetIndex(tjs_int newIndex);

    [[nodiscard]] tjs_int TrackPopup(tjs_uint32 flags, tjs_int x,
                                     tjs_int y) const;

    [[nodiscard]] const ObjectVector<tTJSNI_BaseMenuItem> &GetChildren() const {
        return Children;
    }

    //-- interface to plugin
    /*HMENU*/ [[nodiscard]] void *GetMenuItemHandleForPlugin() const;
};
//---------------------------------------------------------------------------

class WindowMenuProperty : public tTJSDispatch {
    tjs_error PropGet(tjs_uint32 flag, const tjs_char *membername,
                      tjs_uint32 *hint, tTJSVariant *result,
                      iTJSDispatch2 *objthis) override;
    tjs_error PropSet(tjs_uint32 flag, const tjs_char *membername,
                      tjs_uint32 *hint, const tTJSVariant *param,
                      iTJSDispatch2 *objthis) override;
};

void CreateShortCutKeyCodeTable();
//---------------------------------------------------------------------------
#endif