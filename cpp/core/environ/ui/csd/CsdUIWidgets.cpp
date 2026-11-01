#include "CsdUIWidgets.h"

#include "CsdUILayout.h"

#include <cocos/ui/CocosGUI.h>

using namespace cocos2d;
using namespace cocos2d::ui;

namespace Csd {

    namespace {

        Button *createIconButton(const char *normal, const char *pressed,
                                 const Size &size, const Vec2 &pos,
                                 const Vec2 &anchor, const char *iconPath,
                                 const Size &iconSize) {
            auto *btn = Button::create(normal, pressed, normal);
            btn->setScale9Enabled(true);
            btn->setContentSize(size);
            btn->setAnchorPoint(anchor);
            btn->setPosition(pos);
            if(iconPath) {
                auto *icon = ImageView::create(iconPath);
                icon->setContentSize(iconSize);
                icon->setAnchorPoint(Vec2(0.5f, 0.5f));
                icon->setPosition(Vec2(size.width * 0.5f, size.height * 0.5f));
                btn->addChild(icon);
            }
            return btn;
        }

        Layout *createHighlightPanel(const LayoutCtx &ctx) {
            auto *panel = Layout::create();
            panel->setName("highlight");
            panel->setContentSize(ctx.size);
            panel->setAnchorPoint(Vec2::ZERO);
            panel->setPosition(Vec2::ZERO);
            panel->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
            panel->setBackGroundColor(Color3B(192, 192, 192));
            panel->setBackGroundColorOpacity(102);
            panel->setVisible(false);
            return panel;
        }

        Layout *createMenuListRow(const LayoutCtx &ctx, const char *panelName,
                                  const char *btnName, const char *titleName,
                                  const char *titleText, const char *iconPath) {
            auto *panel = Layout::create();
            panel->setName(panelName);
            panel->setContentSize(ctx.size);
            panel->setTouchEnabled(true);

            auto *btn = Button::create("img/empty.png", "img/white.png",
                                       "img/empty.png");
            btn->setName(btnName);
            btn->setScale9Enabled(true);
            btn->setContentSize(ctx.size);
            btn->setAnchorPoint(Vec2::ZERO);
            btn->setPosition(Vec2::ZERO);
            panel->addChild(btn);

            const float iconSide = ctx.relWx(85, kCanvasMenuListW);
            if(iconPath) {
                auto *icon = ImageView::create(iconPath);
                icon->setContentSize(Size(iconSide, iconSide));
                icon->setAnchorPoint(Vec2(0.5f, 0.5f));
                icon->setPosition(
                    Vec2(ctx.relWx(50, kCanvasMenuListW), ctx.h() * 0.5f));
                panel->addChild(icon);
            }

            auto *title = Text::create(titleText, "NotoSansCJK-Regular.ttc",
                                       ctx.fontSize(72, kCanvasMenuListW));
            title->setName(titleName);
            title->setTextColor(Color4B(192, 192, 192, 255));
            title->setAnchorPoint(Vec2(0, 0.5f));
            title->setPosition(
                Vec2(ctx.relWx(100, kCanvasMenuListW), ctx.h() * 0.5f));
            panel->addChild(title);

            return panel;
        }

        Layout *createFileManageMenuRow(const LayoutCtx &ctx,
                                        const char *panelName,
                                        const char *btnName,
                                        const char *titleName,
                                        const char *titleText) {
            auto *panel = Layout::create();
            panel->setName(panelName);
            panel->setContentSize(ctx.size);
            panel->setTouchEnabled(true);

            auto *btn = Button::create("img/empty.png", "img/white.png",
                                       "img/empty.png");
            btn->setName(btnName);
            btn->setScale9Enabled(true);
            btn->setContentSize(ctx.size);
            btn->setAnchorPoint(Vec2::ZERO);
            btn->setPosition(Vec2::ZERO);
            panel->addChild(btn);

            auto *title =
                Text::create(titleText, "NotoSansCJK-Regular.ttc",
                             ctx.fontSize(64, kCanvasFileManageMenuW));
            title->setName(titleName);
            title->setTextColor(Color4B(192, 192, 192, 255));
            title->setAnchorPoint(Vec2(0, 0.5f));
            title->setPosition(
                Vec2(ctx.relWx(96, kCanvasFileManageMenuW), ctx.h() * 0.5f));
            panel->addChild(title);

            return panel;
        }

    } // namespace

    Widget *createListItem(const Size &size, float scale) {
        LayoutCtx ctx{ size.width > 0 ? size : Size(kCanvasListItemW, 0),
                       scale };
        const float rowH = ctx.rowH(96, kCanvasListItemW);

        auto *root = Widget::create();
        root->setContentSize(Size(ctx.w(), rowH));

        auto *item =
            Button::create("img/empty.png", "img/gray.png", "img/empty.png");
        item->setName("item");
        item->setScale9Enabled(true);
        item->setContentSize(root->getContentSize());
        item->setAnchorPoint(Vec2(0.5f, 0.5f));
        item->setPosition(root->getContentSize() / 2);
        item->setTitleFontName("NotoSansCJK-Regular.ttc");
        item->setTitleFontSize(ctx.fontSize(64, kCanvasListItemW));
        item->setTitleColor(Color3B::WHITE);
        root->addChild(item);

        return root;
    }

    Widget *createSelectListItem(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float rowH = ctx.rowH(96, kCanvasComctrlW);

        auto *root = Widget::create();
        root->setContentSize(Size(ctx.w(), rowH));

        auto *text = Text::create("", "NotoSansCJK-Regular.ttc",
                                  ctx.fontSize(72, kCanvasComctrlW));
        text->setName("text");
        text->setContentSize(root->getContentSize());
        text->setAnchorPoint(Vec2::ZERO);
        text->setPosition(Vec2::ZERO);
        text->setTextColor(Color4B::WHITE);
        root->addChild(text);

        return root;
    }

    Widget *createComctrlSelectListItem(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float itemH = ctx.rowH(160);

        auto *root = Widget::create();
        root->setContentSize(Size(ctx.w(), itemH));

        LayoutCtx itemCtx{ root->getContentSize(), scale };
        root->addChild(createHighlightPanel(itemCtx));

        auto *title = Text::create("", "NotoSansCJK-Regular.ttc",
                                   ctx.fontSize(64, kCanvasComctrlW));
        title->setName("title");
        title->setAnchorPoint(Vec2(0, 0.5f));
        title->setPosition(
            Vec2(ctx.relWx(8, kCanvasComctrlW), ctx.relHy(110, 160.f)));
        title->setTextColor(Color4B::WHITE);
        root->addChild(title);

        auto *selected = Text::create("", "NotoSansCJK-Regular.ttc",
                                      ctx.fontSize(56, kCanvasComctrlW));
        selected->setName("selected");
        selected->setAnchorPoint(Vec2(1, 0.5f));
        selected->setPosition(Vec2(ctx.w() - ctx.relWx(8, kCanvasComctrlW),
                                   ctx.relHy(44.5f, 160.f)));
        selected->setTextColor(Color4B::WHITE);
        root->addChild(selected);

        return root;
    }

    Widget *createCheckBoxItem(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float itemH = ctx.rowH(96);

        auto *root = Widget::create();
        root->setContentSize(Size(ctx.w(), itemH));

        LayoutCtx itemCtx{ root->getContentSize(), scale };
        root->addChild(createHighlightPanel(itemCtx));

        const float boxSide = ctx.relWx(80, kCanvasComctrlW);
        const float titleH = ctx.relHy(80, 96.f);
        auto *title = Text::create("", "NotoSansCJK-Regular.ttc",
                                   ctx.fontSize(64, kCanvasComctrlW));
        title->setName("title");
        title->setContentSize(
            Size(ctx.w() - boxSide - ctx.relWx(23, kCanvasComctrlW), titleH));
        title->setAnchorPoint(Vec2(0, 0.5f));
        title->setPosition(Vec2(ctx.relWx(8, kCanvasComctrlW), itemH * 0.5f));
        title->setTextColor(Color4B::WHITE);
        root->addChild(title);

        auto *checkboxPanel = Layout::create();
        checkboxPanel->setContentSize(Size(boxSide, boxSide));
        checkboxPanel->setAnchorPoint(Vec2(1, 0.5f));
        checkboxPanel->setPosition(
            Vec2(ctx.w() - ctx.relWx(8, kCanvasComctrlW), itemH * 0.5f));
        root->addChild(checkboxPanel);

        auto *checkbox = CheckBox::create(
            "img/CheckBox_Normal.png", "img/CheckBox_Press.png",
            "img/CheckBox_Disable.png", "img/CheckBoxNode_Normal.png",
            "img/empty.png");
        checkbox->setName("checkbox");
        checkbox->setContentSize(Size(boxSide, boxSide));
        checkbox->setAnchorPoint(Vec2(0.5f, 0.5f));
        checkbox->setPosition(Vec2(boxSide * 0.5f, boxSide * 0.5f));
        checkboxPanel->addChild(checkbox);

        return root;
    }

    Widget *createSubDirItem(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float itemH = ctx.rowH(96);

        auto *root = Widget::create();
        root->setContentSize(Size(ctx.w(), itemH));

        LayoutCtx itemCtx{ root->getContentSize(), scale };
        root->addChild(createHighlightPanel(itemCtx));

        const float iconSide = ctx.relWx(80, kCanvasComctrlW);
        const float titleH = ctx.relHy(80, 96.f);
        const float iconGap = ctx.relWx(15, kCanvasComctrlW);
        auto *title = Text::create("", "NotoSansCJK-Regular.ttc",
                                   ctx.fontSize(64, kCanvasComctrlW));
        title->setName("title");
        title->setContentSize(Size(ctx.w() - iconSide - iconGap, titleH));
        title->setAnchorPoint(Vec2(0, 0.5f));
        title->setPosition(Vec2(ctx.relWx(8, kCanvasComctrlW), itemH * 0.5f));
        title->setTextColor(Color4B::WHITE);
        root->addChild(title);

        auto *dirIcon = Layout::create();
        dirIcon->setName("dir_icon");
        dirIcon->setContentSize(Size(iconSide, iconSide));
        dirIcon->setAnchorPoint(Vec2(1, 0.5f));
        dirIcon->setPosition(
            Vec2(ctx.w() - ctx.relWx(8, kCanvasComctrlW), itemH * 0.5f));

        const Size arrowSize(ctx.relWx(45, kCanvasComctrlW),
                             ctx.relHy(10, 96.f));
        auto *arrow1 = Layout::create();
        arrow1->setContentSize(arrowSize);
        arrow1->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        arrow1->setBackGroundColor(Color3B(191, 191, 191));
        arrow1->setAnchorPoint(Vec2(0, 0));
        arrow1->setPosition(
            Vec2(ctx.relWx(70, kCanvasComctrlW), iconSide * 0.5f));
        arrow1->setRotation(-135);
        dirIcon->addChild(arrow1);

        auto *arrow2 = Layout::create();
        arrow2->setContentSize(arrowSize);
        arrow2->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        arrow2->setBackGroundColor(Color3B(191, 191, 191));
        arrow2->setAnchorPoint(Vec2(0, 1));
        arrow2->setPosition(
            Vec2(ctx.relWx(70, kCanvasComctrlW), iconSide * 0.5f));
        arrow2->setRotation(135);
        dirIcon->addChild(arrow2);

        root->addChild(dirIcon);
        return root;
    }

    Widget *createSliderIconItem(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        constexpr float CW = kCanvasSliderIconW;
        constexpr float CH = kCanvasSliderIconH;
        const float itemH = ctx.rowH(240, CW);

        auto *root = Widget::create();
        root->setContentSize(Size(ctx.w(), itemH));

        auto *title =
            Text::create("", "NotoSansCJK-Regular.ttc", ctx.fontSize(64, CW));
        title->setName("title");
        title->setContentSize(ctx.relSize(1152, 80, CW, CH));
        title->setAnchorPoint(Vec2(0, 0.5f));
        title->setPosition(Vec2(ctx.relWx(8, CW), ctx.relHy(192, CH)));
        title->setTextColor(Color4B::WHITE);
        root->addChild(title);

        const Size panelSize(ctx.w() - ctx.relWx(160, CW), ctx.relHy(144, CH));
        auto *panel = Layout::create();
        panel->setContentSize(panelSize);
        panel->setAnchorPoint(Vec2::ZERO);
        panel->setPosition(Vec2::ZERO);
        root->addChild(panel);

        auto *reset = Button::create("Default/Button_Normal.png",
                                     "Default/Button_Press.png",
                                     "Default/Button_Disable.png");
        reset->setName("reset");
        reset->setTitleText("preference_reset");
        reset->setTitleFontSize(ctx.fontSize(56, CW));
        reset->setContentSize(ctx.relSize(150, 80, CW, CH));
        reset->setAnchorPoint(Vec2(0.5f, 0));
        reset->setPosition(Vec2(ctx.relWx(90, CW), ctx.relHy(15, CH)));
        panel->addChild(reset);

        auto *slider = Slider::create("Default/Slider_Back.png",
                                      "Default/SliderNode_Normal.png");
        slider->loadProgressBarTexture("Default/Slider_PressBar.png");
        slider->setName("slider");
        slider->setContentSize(
            Size(panelSize.width - ctx.relWx(320, CW), ctx.relHy(14, CH)));
        slider->setAnchorPoint(Vec2(0.5f, 0.5f));
        slider->setPosition(Vec2(panelSize.width * 0.5f, ctx.relHy(97, CH)));
        panel->addChild(slider);

        auto *icon = Widget::create();
        icon->setName("icon");
        icon->setAnchorPoint(Vec2::ZERO);
        icon->setPosition(Vec2(ctx.relWx(1160, CW), ctx.relHy(120, CH)));
        root->addChild(icon);

        return root;
    }

    Widget *createSliderTextItem(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        constexpr float CW = kCanvasSliderTextW;
        constexpr float CH = kCanvasSliderTextH;
        const float itemH = ctx.rowH(240, CW);

        auto *root = Widget::create();
        root->setContentSize(Size(ctx.w(), itemH));

        auto *title =
            Text::create("", "NotoSansCJK-Regular.ttc", ctx.fontSize(64, CW));
        title->setName("title");
        title->setContentSize(ctx.relSize(512, 80, CW, CH));
        title->setAnchorPoint(Vec2(0, 0.5f));
        title->setPosition(Vec2(ctx.relWx(8, CW), ctx.relHy(192, CH)));
        title->setTextColor(Color4B::WHITE);
        root->addChild(title);

        const Size panelSize(ctx.w() - ctx.relWx(160, CW), ctx.relHy(144, CH));
        auto *panel = Layout::create();
        panel->setContentSize(panelSize);
        panel->setAnchorPoint(Vec2::ZERO);
        panel->setPosition(Vec2::ZERO);
        root->addChild(panel);

        auto *reset = Button::create("Default/Button_Normal.png",
                                     "Default/Button_Press.png",
                                     "Default/Button_Disable.png");
        reset->setName("reset");
        reset->setTitleText("preference_reset");
        reset->setTitleFontSize(ctx.fontSize(56, CW));
        reset->setContentSize(ctx.relSize(150, 80, CW, CH));
        reset->setAnchorPoint(Vec2(0.5f, 0));
        reset->setPosition(Vec2(ctx.relWx(90, CW), ctx.relHy(15, CH)));
        panel->addChild(reset);

        auto *slider = Slider::create("Default/Slider_Back.png",
                                      "Default/SliderNode_Normal.png");
        slider->loadProgressBarTexture("Default/Slider_PressBar.png");
        slider->setName("slider");
        slider->setContentSize(
            Size(panelSize.width * (160.f / 480.f), ctx.relHy(14, CH)));
        slider->setAnchorPoint(Vec2(0.5f, 0.5f));
        slider->setPosition(Vec2(panelSize.width * 0.5f, ctx.relHy(97, CH)));
        panel->addChild(slider);

        auto *text = Text::create("Text", "", ctx.fontSize(58, CW));
        text->setName("text");
        text->setAnchorPoint(Vec2(1, 0.5f));
        text->setPosition(Vec2(ctx.w() - ctx.relWx(35, CW), ctx.relHy(99, CH)));
        text->setTextColor(Color4B::WHITE);
        root->addChild(text);

        return root;
    }

    Widget *createDeletableItem(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float itemH = ctx.rowH(96);

        auto *root = Widget::create();
        root->setContentSize(Size(ctx.w(), itemH));

        auto *scrollview = ScrollView::create();
        scrollview->setName("scrollview");
        scrollview->setDirection(ScrollView::Direction::HORIZONTAL);
        scrollview->setContentSize(root->getContentSize());
        scrollview->setInnerContainerSize(root->getContentSize());
        scrollview->setAnchorPoint(Vec2::ZERO);
        scrollview->setPosition(Vec2::ZERO);
        scrollview->setTouchEnabled(true);
        root->addChild(scrollview);

        const float delSide = ctx.relWx(60, kCanvasComctrlW);
        auto *deleteIcon =
            Button::create("img/Cancel_Normal.png", "img/Cancel_Press.png",
                           "img/Cancel_Press.png");
        deleteIcon->setName("delete");
        deleteIcon->setContentSize(Size(delSide, delSide));
        deleteIcon->setAnchorPoint(Vec2::ZERO);
        deleteIcon->setPosition(Vec2(ctx.w(), 0));
        scrollview->addChild(deleteIcon);

        return root;
    }

    Widget *createFileManageMenu(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float rowH = ctx.rowH(96, kCanvasFileManageMenuW);
        const float titleBarH = ctx.relHy(96, kCanvasFileManageMenuH);

        auto *root = Widget::create();
        root->setContentSize(ctx.size);

        auto *bg = Layout::create();
        bg->setContentSize(ctx.size);
        bg->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        bg->setBackGroundColor(Color3B(42, 42, 42));
        bg->setAnchorPoint(Vec2::ZERO);
        bg->setPosition(Vec2::ZERO);
        root->addChild(bg);

        auto *title =
            Text::create("file_operate_menu_text", "NotoSansCJK-Regular.ttc",
                         ctx.fontSize(64, kCanvasFileManageMenuW));
        title->setName("title");
        title->setTextColor(Color4B(192, 192, 192, 255));
        title->setAnchorPoint(Vec2(0.5f, 0.5f));
        title->setPosition(Vec2(ctx.w() * 0.5f, ctx.h() - titleBarH * 0.5f));
        root->addChild(title);

        auto *list = ListView::create();
        list->setName("list");
        list->setDirection(ScrollView::Direction::VERTICAL);
        list->setContentSize(Size(ctx.w(), ctx.h() - titleBarH));
        list->setAnchorPoint(Vec2::ZERO);
        list->setPosition(Vec2::ZERO);
        list->setClippingEnabled(true);
        list->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        list->setBackGroundColor(Color3B(42, 42, 42));
        root->addChild(list);

        const Size rowSize(ctx.w(), rowH);
        LayoutCtx rowCtx{ rowSize, scale };
        const struct {
            const char *p, *b, *t, *txt;
        } rows[] = {
            { "unselect", "btnUnselect", "titleUnselect", "filemgr_unselect" },
            { "view", "btnView", "titleView", "filemgr_browse" },
            { "paste", "btnPaste", "titlePaste", "filemgr_paste" },
            { "copy", "btnCopy", "titleCopy", "filemgr_copy" },
            { "unpack", "btnUnpack", "titleUnpack", "filemgr_unpack" },
            { "rename", "btnRename", "titleRename", "filemgr_rename" },
            { "cut", "btnCut", "titleCut", "filemgr_cut" },
            { "delete", "btnDelete", "titleDelete", "filemgr_delete" },
            { "sendto", "btnSendTo", "titleSendTo", "filemgr_sendto" },
        };
        for(const auto &r : rows) {
            list->pushBackCustomItem(
                createFileManageMenuRow(rowCtx, r.p, r.b, r.t, r.txt));
        }

        return root;
    }

    Widget *createListViewPopup(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };

        auto *root = Widget::create();
        root->setContentSize(ctx.size);

        auto *listView = ListView::create();
        listView->setName("list");
        listView->setDirection(ListView::Direction::VERTICAL);
        listView->setBounceEnabled(true);
        listView->setTouchEnabled(true);
        listView->setContentSize(ctx.size);
        listView->setAnchorPoint(Vec2::ZERO);
        listView->setPosition(Vec2::ZERO);
        root->addChild(listView);

        return root;
    }

    Widget *createGameMenu(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float barH = ctx.h();
        const Size btnSize(ctx.w() / 5.f, barH);

        auto *root = Widget::create();
        root->setContentSize(ctx.size);

        const float handlerSide = ctx.relWx(48, kCanvasGameMenuW);
        auto *handler = ImageView::create("img/menu_handler.png");
        handler->setName("handler");
        handler->setContentSize(Size(handlerSide, handlerSide));
        handler->setAnchorPoint(Vec2::ZERO);
        handler->setPosition(Vec2(ctx.w() - handlerSide, barH));
        handler->setOpacity(38);
        handler->setTouchEnabled(true);
        root->addChild(handler);

        auto *panel2 = Layout::create();
        panel2->setContentSize(ctx.size);
        panel2->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel2->setBackGroundColor(Color3B(42, 42, 42));
        panel2->setAnchorPoint(Vec2::ZERO);
        panel2->setPosition(Vec2::ZERO);
        panel2->setTouchEnabled(true);
        root->addChild(panel2);

        const Size icon64 =
            ctx.relSize(64, 64, kCanvasGameMenuW, kCanvasGameMenuH);
        const Size icon72 =
            ctx.relSize(72, 72, kCanvasGameMenuW, kCanvasGameMenuH);

        auto *btnGameMenu = createIconButton("img/empty.png", "img/white.png",
                                             btnSize, Vec2::ZERO, Vec2::ZERO,
                                             "img/menu_icon.png", icon64);
        btnGameMenu->setName("btn_gamemenu");
        panel2->addChild(btnGameMenu);

        auto *btnWindow =
            createIconButton("img/empty.png", "img/white.png", btnSize,
                             Vec2(btnSize.width, barH * 0.5f), Vec2(0, 0.5f),
                             "img/windows_icon.png", icon72);
        btnWindow->setName("btn_window");
        panel2->addChild(btnWindow);

        auto *btnMouse =
            createIconButton("img/empty.png", "img/white.png", btnSize,
                             Vec2(btnSize.width * 2, barH * 0.5f),
                             Vec2(0, 0.5f), nullptr, Size::ZERO);
        btnMouse->setName("btn_mousemode");
        auto *iconTouch = ImageView::create("img/touch_icon.png");
        iconTouch->setName("icon_touch");
        iconTouch->setContentSize(icon64);
        iconTouch->setAnchorPoint(Vec2(0.5f, 0.5f));
        iconTouch->setPosition(Vec2(btnSize.width * 0.5f, barH * 0.5f));
        btnMouse->addChild(iconTouch);
        auto *iconMouse = ImageView::create("img/mouse_icon.png");
        iconMouse->setName("icon_mouse");
        iconMouse->setContentSize(icon64);
        iconMouse->setAnchorPoint(Vec2(0.5f, 0.5f));
        iconMouse->setPosition(Vec2(btnSize.width * 0.5f, barH * 0.5f));
        btnMouse->addChild(iconMouse);
        panel2->addChild(btnMouse);

        auto *btnKeyboard = createIconButton(
            "img/empty.png", "img/white.png", btnSize,
            Vec2(btnSize.width * 3, barH * 0.5f), Vec2(0, 0.5f),
            "img/keyboard_icon.png",
            ctx.relSize(64, 45, kCanvasGameMenuW, kCanvasGameMenuH));
        btnKeyboard->setName("btn_keyboard");
        panel2->addChild(btnKeyboard);

        auto *btnExit =
            createIconButton("img/empty.png", "img/white.png", btnSize,
                             Vec2(ctx.w() - btnSize.width, barH * 0.5f),
                             Vec2(0, 0.5f), "img/exit_icon.png", icon64);
        btnExit->setName("btn_exit");
        panel2->addChild(btnExit);

        return root;
    }

    Widget *createMenuList(const Size &size, float scale) {
        // 与 MenuList.csd 一致：宽 720，高度由 showMenu 按屏高换算后传入
        LayoutCtx ctx{ size.width > 0.f ? size : Size(kCanvasMenuListW, 0.f),
                       scale };
        const float rowH = ctx.rowH(96, kCanvasMenuListW);

        auto *root = Widget::create();
        root->setContentSize(ctx.size);

        auto *bg = Layout::create();
        bg->setContentSize(ctx.size);
        bg->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        bg->setBackGroundColor(Color3B(42, 42, 42));
        bg->setAnchorPoint(Vec2::ZERO);
        bg->setPosition(Vec2::ZERO);
        root->addChild(bg);

        const Size rowSize(ctx.w(), rowH);
        LayoutCtx rowCtx{ rowSize, scale };

        auto *rotateRow =
            createMenuListRow(rowCtx, "useless_0", "btnRotate", "titleRotate",
                              "menu_rotate", "img/circle_arrow.png");
        rotateRow->setAnchorPoint(Vec2::ZERO);
        rotateRow->setPosition(Vec2(0, ctx.h() - rowH));
        root->addChild(rotateRow);

        auto *menulist = ListView::create();
        menulist->setName("menulist");
        menulist->setDirection(ScrollView::Direction::VERTICAL);
        menulist->setContentSize(Size(ctx.w(), ctx.relH(0.467f)));
        menulist->setAnchorPoint(Vec2::ZERO);
        menulist->setPosition(Vec2(0, ctx.relH(0.267f)));
        menulist->setClippingEnabled(true);
        root->addChild(menulist);

        menulist->pushBackCustomItem(createMenuListRow(
            rowCtx, "globalPref", "btnGlobalPref", "titleGlobalPref",
            "menu_global_preference", "img/syssetting_btn_ff.png"));
        menulist->pushBackCustomItem(createMenuListRow(
            rowCtx, "newLocalPref", "btnNewLocalPref", "titleNewLocalPref",
            "menu_new_local_pref", "img/syssetting_btn_ff.png"));
        menulist->pushBackCustomItem(createMenuListRow(
            rowCtx, "localPref", "btnLocalPref", "titleLocalPref",
            "menu_local_pref", "img/syssetting_btn_ff.png"));
        menulist->pushBackCustomItem(
            createMenuListRow(rowCtx, "newFolder", "btnNewFolder",
                              "titleNewFolder", "menu_new_folder", nullptr));
        menulist->pushBackCustomItem(
            createMenuListRow(rowCtx, "repack", "btnRepack", "titleRepack",
                              "menu_archive_repack", nullptr));
        menulist->pushBackCustomItem(createMenuListRow(
            rowCtx, "help", "btnHelp", "titleHelp", "menu_help", nullptr));
        menulist->pushBackCustomItem(createMenuListRow(
            rowCtx, "about", "btnAbout", "titleAbout", "menu_about", nullptr));
        menulist->pushBackCustomItem(createMenuListRow(
            rowCtx, "exit", "btnExit", "titleExit", "menu_exit", nullptr));

        return root;
    }

    Widget *createRecentListItem(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float itemH = ctx.rowH(120, kCanvasRecentItemW);
        const float scrollH = ctx.relHy(60, kCanvasRecentItemH);
        const float btnSide = ctx.relWx(50, kCanvasRecentItemW);

        auto *root = Widget::create();
        root->setContentSize(Size(ctx.w(), itemH));

        auto *scrollview = ScrollView::create();
        scrollview->setName("scrollview");
        scrollview->setDirection(ScrollView::Direction::BOTH);
        scrollview->setContentSize(Size(ctx.w(), scrollH));
        scrollview->setInnerContainerSize(Size(ctx.w(), scrollH));
        scrollview->setAnchorPoint(Vec2::ZERO);
        scrollview->setPosition(Vec2(0, itemH - scrollH));
        scrollview->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        scrollview->setBackGroundColor(Color3B(42, 42, 42));
        scrollview->setTouchEnabled(true);
        root->addChild(scrollview);

        const float delPanelSide = ctx.relWx(60, kCanvasRecentItemW);
        auto *panelDelete = Layout::create();
        panelDelete->setName("panel_delete");
        panelDelete->setContentSize(Size(delPanelSide, delPanelSide));
        panelDelete->setAnchorPoint(Vec2(0, 0.5f));
        panelDelete->setPosition(Vec2(ctx.w(), scrollH * 0.5f));
        panelDelete->setTouchEnabled(true);
        scrollview->addChild(panelDelete);

        auto *btnDelete =
            Button::create("img/Cancel_Normal.png", "img/Cancel_Press.png",
                           "img/Cancel_Press.png");
        btnDelete->setName("btn_delete");
        btnDelete->setContentSize(Size(delPanelSide, delPanelSide));
        btnDelete->setAnchorPoint(Vec2::ZERO);
        btnDelete->setPosition(Vec2::ZERO);
        panelDelete->addChild(btnDelete);

        auto *prefix = Text::create("", "NotoSansCJK-Regular.ttc",
                                    ctx.fontSize(36, kCanvasRecentItemW));
        prefix->setName("prefix");
        prefix->setTextColor(Color4B(128, 128, 128, 255));
        prefix->setAnchorPoint(Vec2::ZERO);
        prefix->setPosition(Vec2::ZERO);
        scrollview->addChild(prefix);

        auto *path = Text::create("", "NotoSansCJK-Regular.ttc",
                                  ctx.fontSize(50, kCanvasRecentItemW));
        path->setName("path");
        path->setTextColor(Color4B::WHITE);
        path->setAnchorPoint(Vec2::ZERO);
        path->setPosition(Vec2(ctx.relWx(90, kCanvasRecentItemW), 0));
        prefix->addChild(path);

        auto *file = Text::create("", "NotoSansCJK-Regular.ttc",
                                  ctx.fontSize(36, kCanvasRecentItemW));
        file->setName("file");
        file->setTextColor(Color4B(128, 128, 128, 255));
        file->setAnchorPoint(Vec2::ZERO);
        file->setPosition(Vec2(ctx.relWx(102, kCanvasRecentItemW), 0));
        path->addChild(file);

        auto *btnJump =
            Button::create("img/back_btn_off.png", "img/back_btn_on.png",
                           "img/back_btn_on.png");
        btnJump->setName("btn_jump");
        btnJump->setContentSize(Size(btnSide, btnSide));
        btnJump->setAnchorPoint(Vec2(0.5f, 0));
        btnJump->setPosition(Vec2(ctx.w() - ctx.relWx(40, kCanvasRecentItemW),
                                  ctx.relHy(5, kCanvasRecentItemH)));
        btnJump->setFlippedX(true);
        root->addChild(btnJump);

        auto *btnConf = Button::create("img/syssetting_btn_ff.png",
                                       "img/syssetting_btn_on.png",
                                       "img/syssetting_btn_on.png");
        btnConf->setName("btn_conf");
        btnConf->setContentSize(Size(btnSide, btnSide));
        btnConf->setAnchorPoint(Vec2(0.5f, 0));
        btnConf->setPosition(Vec2(ctx.w() - ctx.relWx(105, kCanvasRecentItemW),
                                  ctx.relHy(5, kCanvasRecentItemH)));
        root->addChild(btnConf);

        auto *btnPlay = Button::create("img/triangle.png", "img/triangle.png",
                                       "img/triangle.png");
        btnPlay->setName("btn_play");
        btnPlay->setContentSize(Size(btnSide, btnSide));
        btnPlay->setAnchorPoint(Vec2(0.5f, 0));
        btnPlay->setPosition(Vec2(ctx.relWx(114, kCanvasRecentItemW),
                                  ctx.relHy(5, kCanvasRecentItemH)));
        btnPlay->setColor(Color3B(160, 160, 160));
        root->addChild(btnPlay);

        auto *separator = Layout::create();
        separator->setContentSize(
            Size(ctx.w(), ctx.relHy(2, kCanvasRecentItemH)));
        separator->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        separator->setBackGroundColor(Color3B(160, 160, 160));
        separator->setAnchorPoint(Vec2::ZERO);
        separator->setPosition(Vec2::ZERO);
        root->addChild(separator);

        return root;
    }

} // namespace Csd
