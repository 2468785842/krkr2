#include "CsdUIFactory.h"

#include "CsdUILayout.h"

#include <algorithm>

#include <2d/CCSprite.h>
#include <cocos/ui/CocosGUI.h>

using namespace cocos2d;
using namespace cocos2d::ui;

namespace Csd {

    namespace {

        ImageView *makeImage(const char *path, const Size &size,
                             const Vec2 &pos,
                             const Vec2 &anchor = Vec2(0.5f, 0.5f),
                             const Color3B &color = Color3B::WHITE) {
            auto *img = ImageView::create(path);
            img->setContentSize(size);
            img->setAnchorPoint(anchor);
            img->setPosition(pos);
            img->setColor(color);
            return img;
        }

        /** help/TouchModeTips.csd 画布 960×360 */
        Widget *buildTouchModeTips(const LayoutCtx &ctx) {
            constexpr float CW = kCanvasAllTipsW;
            constexpr float CH = 360.f;

            auto *root = Widget::create();
            root->setContentSize(ctx.size);
            root->setAnchorPoint(Vec2::ZERO);

            const Size icon64 = ctx.relSize(64, 64, CW, CH);
            root->addChild(makeImage("img/touch_icon.png", icon64,
                                     Vec2(ctx.relWx(90, CW), ctx.relHy(40, CH)),
                                     Vec2(0.5f, 0.5f), Color3B(208, 208, 208)));
            root->addChild(
                makeImage("img/mouse_icon.png", icon64,
                          Vec2(ctx.relWx(94, CW), ctx.relHy(213, CH)),
                          Vec2(0.5f, 0.5f), Color3B(208, 208, 208)));

            const Size touchSize = ctx.relSize(64, 68, CW, CH);
            root->addChild(makeImage(
                "img/touch1.png", touchSize,
                Vec2(ctx.relWx(200, CW), ctx.relHy(40, CH)), Vec2(0.5f, 0.5f)));
            root->addChild(makeImage(
                "img/touch2.png", touchSize,
                Vec2(ctx.relWx(220, CW), ctx.relHy(40, CH)), Vec2(0.5f, 0.5f)));

            root->addChild(makeImage(
                "img/back_btn_off.png", icon64,
                Vec2(ctx.relWx(92, CW), ctx.relHy(316, CH)), Vec2(0.5f, 0.5f)));
            auto *back2 = makeImage("img/back_btn_off.png", icon64,
                                    Vec2(ctx.relWx(92, CW), ctx.relHy(302, CH)),
                                    Vec2(0.5f, 0.5f));
            back2->setRotation(-90);
            root->addChild(back2);

            auto *divider = Layout::create();
            divider->setContentSize(ctx.relSize(8, 62, CW, CH));
            divider->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
            divider->setBackGroundColor(Color3B(192, 192, 192));
            divider->setAnchorPoint(Vec2(0.5f, 0.5f));
            divider->setPosition(Vec2(ctx.relWx(92, CW), ctx.relHy(309, CH)));
            root->addChild(divider);

            root->addChild(makeImage(
                "img/mouse_2btn.png", ctx.relSize(43, 64, CW, CH),
                Vec2(ctx.relWx(390, CW), ctx.relHy(40, CH)), Vec2(0.5f, 0.5f)));

            return root;
        }

        /** help/ScreenModeTips.csd 画布 960×180 */
        Widget *buildScreenModeTips(const LayoutCtx &ctx) {
            constexpr float CW = kCanvasAllTipsW;
            constexpr float CH = 180.f;

            auto *root = Widget::create();
            root->setContentSize(ctx.size);
            root->setAnchorPoint(Vec2::ZERO);

            auto *winIcon = Sprite::create("img/windows_icon.png");
            winIcon->setAnchorPoint(Vec2(0.5f, 0.5f));
            winIcon->setPosition(Vec2(ctx.relWx(62, CW), ctx.relHy(177, CH)));
            winIcon->setContentSize(ctx.relSize(80, 80, CW, CH));
            root->addChild(winIcon);

            auto *outer = Layout::create();
            outer->setContentSize(ctx.relSize(200, 120, CW, CH));
            outer->setAnchorPoint(Vec2::ZERO);
            outer->setPosition(Vec2(ctx.relWx(193, CW), ctx.relHy(112, CH)));
            outer->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
            outer->setBackGroundColor(Color3B(208, 208, 208));
            root->addChild(outer);

            auto *inner = Layout::create();
            inner->setContentSize(ctx.relSize(194, 114, CW, CH));
            inner->setAnchorPoint(Vec2(0.5f, 0.5f));
            inner->setPosition(Vec2(outer->getContentSize().width * 0.5f,
                                    outer->getContentSize().height * 0.5f));
            inner->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
            inner->setBackGroundColor(Color3B(42, 42, 42));
            outer->addChild(inner);

            auto *preview = Layout::create();
            preview->setContentSize(ctx.relSize(140, 114, 194, 114));
            preview->setAnchorPoint(Vec2(0.5f, 0.5f));
            preview->setPosition(Vec2(inner->getContentSize().width * 0.5f,
                                      inner->getContentSize().height * 0.5f));
            preview->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
            preview->setBackGroundColor(Color3B(60, 177, 255));
            inner->addChild(preview);

            auto *abc = Text::create("ABC", "NotoSansCJK-Regular.ttc",
                                     ctx.fontSize(64, kCanvasComctrlW));
            abc->setAnchorPoint(Vec2(0.5f, 0.5f));
            abc->setPosition(Vec2(preview->getContentSize().width * 0.5f,
                                  preview->getContentSize().height * 0.5f));
            abc->setTextColor(Color4B::BLACK);
            preview->addChild(abc);

            return root;
        }

        Layout *makePauseIcon(const LayoutCtx &ctx, const char *name,
                              const Color3B &color) {
            const Size iconSize =
                ctx.relSize(60, 60, kCanvasMediaFootW, kCanvasMediaFootH);
            const Size barSize =
                ctx.relSize(20, 60, kCanvasMediaFootW, kCanvasMediaFootH);
            auto *root = Layout::create();
            root->setName(name);
            root->setContentSize(iconSize);
            root->setAnchorPoint(Vec2(0.5f, 0.5f));

            auto addBar = [&](float xRatio) {
                auto *bar = Layout::create();
                bar->setContentSize(barSize);
                bar->setAnchorPoint(Vec2(0.5f, 0.5f));
                bar->setPosition(
                    Vec2(iconSize.width * xRatio, iconSize.height * 0.5f));
                bar->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
                bar->setBackGroundColor(color);
                root->addChild(bar);
            };
            addBar(0.35f);
            addBar(0.65f);
            return root;
        }

        Widget *buildPlayBtn(const LayoutCtx &ctx) {
            const float btnSide =
                std::min(ctx.h(), ctx.relWx(180, kCanvasMediaFootW));
            const Size btnSize(btnSide, btnSide);
            const float inset = ctx.relWx(40, kCanvasMediaFootW);
            const Size circleSize(btnSide - inset, btnSide - inset);
            const Size iconSize =
                ctx.relSize(72, 72, kCanvasMediaFootW, kCanvasMediaFootH);
            const Vec2 center(btnSize.width * 0.5f, btnSize.height * 0.5f);

            auto *playBtn = Layout::create();
            playBtn->setName("PlayBtn");
            playBtn->setContentSize(btnSize);
            playBtn->setAnchorPoint(Vec2(0.5f, 0.5f));
            playBtn->setTouchEnabled(true);

            auto *normal =
                makeImage("img/circle_white.png", circleSize, center);
            normal->setName("PlayBtnNormal");
            playBtn->addChild(normal);

            auto *press = makeImage("img/circle_white.png", circleSize, center,
                                    Vec2(0.5f, 0.5f), Color3B(128, 128, 128));
            press->setName("PlayBtnPress");
            playBtn->addChild(press);

            auto *playIconN = makeImage(
                "img/triangle.png", iconSize,
                Vec2(center.x + ctx.relWx(9, kCanvasMediaFootW), center.y));
            playIconN->setName("PlayIconNormal");
            playBtn->addChild(playIconN);

            auto *playIconP = makeImage(
                "img/triangle.png", iconSize,
                Vec2(center.x + ctx.relWx(9, kCanvasMediaFootW), center.y),
                Vec2(0.5f, 0.5f), Color3B(128, 128, 128));
            playIconP->setName("PlayIconPress");
            playBtn->addChild(playIconP);

            auto *pauseN =
                makePauseIcon(ctx, "PauseIconNormal", Color3B::WHITE);
            pauseN->setPosition(center);
            playBtn->addChild(pauseN);

            auto *pauseP =
                makePauseIcon(ctx, "PauseIconPress", Color3B(128, 128, 128));
            pauseP->setPosition(center);
            playBtn->addChild(pauseP);

            return playBtn;
        }

        Text *makeMediaText(const LayoutCtx &ctx, const char *name,
                            float designFont, const Vec2 &pos,
                            const Vec2 &anchor) {
            auto *text =
                Text::create("Text Label", "NotoSansCJK-Regular.ttc",
                             ctx.fontSize(designFont, kCanvasMediaNaviW));
            text->setName(name);
            text->setAnchorPoint(anchor);
            text->setPosition(pos);
            text->setTextColor(Color4B::WHITE);
            text->enableOutline(Color4B(0, 0, 255, 255), 1);
            text->enableShadow(Color4B(110, 110, 110, 255), Size(2, -2));
            return text;
        }

    } // namespace

    Widget *createEmpty(const cocos2d::Size &, float) { return nullptr; }

    Widget *createMainFileSelector(const cocos2d::Size &size, float scale) {

        const cocos2d::Size midLineSize(8 * scale, size.height);
        const cocos2d::Size bothSize((size.width - midLineSize.width) / 2,
                                     size.height);

        const auto root = Layout::create();
        root->setAnchorPoint(cocos2d::Vec2::ZERO);
        root->setContentSize(size);
        root->setLayoutType(Layout::Type::HORIZONTAL);

        // 左侧 recentList
        const auto recentList = ListView::create();
        recentList->setName("recentList");
        recentList->setDirection(ScrollView::Direction::VERTICAL);
        recentList->setContentSize(bothSize);
        recentList->setTouchEnabled(true);
        recentList->setBounceEnabled(true);
        recentList->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        recentList->setBackGroundColor(cocos2d::Color3B(42, 42, 42));
        recentList->setBackGroundColorOpacity(255);
        root->addChild(recentList);

        // 中间线
        const auto ml = Layout::create();
        ml->setName("ml");
        ml->setContentSize(midLineSize);
        ml->setTouchEnabled(true);
        ml->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        ml->setBackGroundColor(cocos2d::Color3B(121, 121, 121));
        ml->setBackGroundColorOpacity(255);
        root->addChild(ml);

        // 右侧 fileList
        const auto fileList = Layout::create();
        fileList->setName("fileList");
        fileList->setLayoutType(Layout::Type::VERTICAL);
        fileList->setContentSize(bothSize);
        fileList->setTouchEnabled(true);
        fileList->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        fileList->setBackGroundColor(cocos2d::Color3B(42, 42, 42));
        fileList->setBackGroundColorOpacity(255);
        root->addChild(fileList);

        return root;
    }

    Widget *createTableView(const cocos2d::Size &size, float) {
        const auto root = Widget::create();
        root->setAnchorPoint(cocos2d::Vec2::ZERO);
        root->setContentSize(size);

        Layout *table = Layout::create();
        table->setName("table");
        table->setContentSize(size);
        table->setPosition(cocos2d::Vec2::ZERO);
        table->setAnchorPoint(cocos2d::Vec2::ZERO);
        table->setTouchEnabled(true);
        table->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        table->setBackGroundColor(cocos2d::Color3B(42, 42, 42));
        table->setBackGroundColorOpacity(255);
        root->addChild(table);

        return root;
    }

    Widget *createNaviBar(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        constexpr float CW = kCanvasNaviBarW;
        constexpr float CH = kCanvasNaviBarH;
        const float sideMargin = ctx.relWx(20, CW);
        const Size btnSize = ctx.relSize(100, 100, CW, CH);
        const float titleX = ctx.relWx(110, CW);
        const Size titleSize(ctx.relWx(500, CW), ctx.h());
        const float yCenter = ctx.h() * 0.5f;

        const auto root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(ctx.size);

        const auto background = Layout::create();
        background->setName("background");
        background->setContentSize(ctx.size);
        background->setTouchEnabled(true);
        background->setAnchorPoint(Vec2(0, 0));
        background->setPosition(Vec2::ZERO);
        background->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        background->setBackGroundColor(Color3B(42, 42, 42));
        background->setBackGroundColorOpacity(255);

        const auto leftBtn =
            Button::create("img/back_btn_off.png", "img/back_btn_on.png",
                           "img/back_btn_on.png");
        leftBtn->setName("left");
        leftBtn->setTouchEnabled(true);
        leftBtn->setContentSize(btnSize);
        leftBtn->setPosition(Vec2(sideMargin, yCenter));
        leftBtn->setAnchorPoint(Vec2(0, 0.5f));

        const auto titleBtn =
            Button::create("img/empty.png", "img/gray.png", "img/empty.png");
        auto *l = Label::create();
        l->setSystemFontName("NotoSansCJK-Regular.ttc");
        titleBtn->ignoreContentAdaptWithSize(false);
        titleBtn->setTitleLabel(l);
        titleBtn->setName("title");
        titleBtn->setContentSize(titleSize);
        titleBtn->setPosition(Vec2(titleX, yCenter));
        titleBtn->setAnchorPoint(Vec2(0, 0.5f));
        titleBtn->setTitleFontSize(ctx.fontSize(64, CW));
        titleBtn->setTitleAlignment(TextHAlignment::CENTER,
                                    TextVAlignment::CENTER);
        titleBtn->setTouchEnabled(true);
        titleBtn->setTitleColor(Color3B(199, 199, 199));

        background->addChild(leftBtn);
        background->addChild(titleBtn);
        root->addChild(background);
        return root;
    }

    Widget *createNaviBarWithMenu(const cocos2d::Size &size, float scale) {

        LayoutCtx ctx{ size, scale };
        constexpr float CW = kCanvasNaviBarW;
        constexpr float CH = kCanvasNaviBarH;
        const float sideMargin = ctx.relWx(20, CW);
        const Size btnSize = ctx.relSize(80, 80, CW, CH);
        const float titleX = ctx.relWx(110, CW);
        const Size titleSize(ctx.w() - titleX * 2, ctx.h());
        const float yCenter = ctx.h() * 0.5f;

        // 创建根节点：容器层
        const auto root = Widget::create();
        root->setAnchorPoint(cocos2d::Vec2::ZERO);
        root->setContentSize(ctx.size);

        // 创建 背景
        const auto background = Layout::create();
        background->setName("background");
        background->setContentSize(ctx.size);
        background->setTouchEnabled(true);
        background->setAnchorPoint(cocos2d::Vec2(0, 0));
        background->setPosition(cocos2d::Vec2::ZERO);
        background->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        background->setBackGroundColor(cocos2d::Color3B(42, 42, 42));
        background->setBackGroundColorOpacity(255);

        // 添加左按钮
        const auto leftBtn =
            Button::create("img/back_btn_off.png", "img/back_btn_on.png",
                           "img/back_btn_on.png");
        leftBtn->setName("left");
        leftBtn->setTouchEnabled(true);
        leftBtn->setContentSize(btnSize);
        leftBtn->setPosition(cocos2d::Vec2(sideMargin, yCenter));
        leftBtn->setAnchorPoint(cocos2d::Vec2(0, 0.5));

        // 中间标题按钮
        const auto titleBtn =
            Button::create("img/empty.png", "img/gray.png", "img/empty.png");
        cocos2d::Label *l = cocos2d::Label::create();
        l->setSystemFontName("NotoSansCJK-Regular.ttc");
        titleBtn->ignoreContentAdaptWithSize(false);
        titleBtn->setTitleLabel(l);
        titleBtn->setName("title");
        titleBtn->setContentSize(titleSize);
        titleBtn->setPosition(cocos2d::Vec2(titleX, yCenter));
        titleBtn->setAnchorPoint(cocos2d::Vec2(0, 0.5));
        titleBtn->setTitleFontSize(ctx.fontSize(64, CW));
        titleBtn->setTitleAlignment(cocos2d::TextHAlignment::CENTER,
                                    cocos2d::TextVAlignment::CENTER);
        titleBtn->setTouchEnabled(true);
        titleBtn->setTitleColor(cocos2d::Color3B(199, 199, 199));

        // 右侧按钮
        const auto rightBtn = Button::create(
            "img/menu_icon.png", "img/menu_press.png", "img/menu_icon.png");
        rightBtn->setName("right");
        rightBtn->setTouchEnabled(true);
        rightBtn->setContentSize(btnSize);
        rightBtn->setPosition(cocos2d::Vec2(ctx.w() - sideMargin, yCenter));
        rightBtn->setAnchorPoint(cocos2d::Vec2(1, 0.5));

        background->addChild(leftBtn);
        background->addChild(titleBtn);
        background->addChild(rightBtn);

        root->addChild(background);

        return root;
    }

    Widget *createFileItem(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float margin = ctx.relWx(8, kCanvasFileItemW);
        const cocos2d::Size rootSize(size.width - margin * 2,
                                     size.height - margin * 2);
        const cocos2d::Size &highlightFocusSize = size;
        const cocos2d::Size rightMenuSize =
            ctx.relSize(80, 80, kCanvasFileItemW, kCanvasFileItemH);

        const cocos2d::Size underlineSize(rootSize.width,
                                          ctx.relHy(4, kCanvasFileItemH));
        const cocos2d::Size filenameSize(rootSize.width - rightMenuSize.width,
                                         rootSize.height -
                                             underlineSize.height);

        const auto root = Widget::create();
        root->setAnchorPoint(cocos2d::Vec2::ZERO);
        root->setPosition(cocos2d::Vec2(margin, margin));
        root->setContentSize(rootSize);

        const auto filename = Text::create("", "NotoSansCJK-Regular.ttc",
                                           ctx.fontSize(64, kCanvasFileItemW));
        filename->setName("filename");
        filename->setContentSize(filenameSize);
        filename->setSwallowTouches(false);
        filename->setAnchorPoint(cocos2d::Vec2::ZERO);
        filename->setPosition(Vec2(ctx.relWx(8, kCanvasFileItemW),
                                   ctx.relHy(8, kCanvasFileItemH)));
        filename->setTextColor(cocos2d::Color4B::WHITE);

        // underline: 底部 4 高度灰线
        const auto underline = Layout::create();
        underline->setName("underline");
        underline->setAnchorPoint(cocos2d::Vec2::ZERO);
        underline->setContentSize(underlineSize);
        underline->setPosition(cocos2d::Vec2::ZERO);
        underline->setBackGroundColorType(
            Layout::BackGroundColorType::GRADIENT);
        underline->setBackGroundColor(cocos2d::Color3B(229, 229, 229),
                                      cocos2d::Color3B(42, 42, 42));

        // highlight button（点击区域）
        const auto highlight =
            Button::create("img/empty.png", "img/white.png", "img/empty.png");
        highlight->setName("highlight");
        highlight->setContentSize(highlightFocusSize);
        highlight->setPosition(cocos2d::Vec2::ZERO);
        highlight->setAnchorPoint(cocos2d::Vec2::ZERO);
        highlight->setOpacity(51); // Alpha
        highlight->setSwallowTouches(false);
        highlight->ignoreContentAdaptWithSize(false);

        // select_check checkbox
        const auto checkBox = CheckBox::create(
            "img/CheckBox_Normal.png", "img/CheckBox_Press.png",
            "img/CheckBox_Disable.png", "img/CheckBoxNode_Normal.png",
            "img/empty.png");
        checkBox->setName("select_check");
        checkBox->setContentSize(rightMenuSize);
        checkBox->setAnchorPoint(cocos2d::Vec2::ZERO);
        checkBox->setPosition(cocos2d::Vec2(rootSize.width - margin, -margin));

        // dir_icon panel
        const auto dirIcon = Widget::create();
        dirIcon->setName("dir_icon");
        dirIcon->setContentSize(rightMenuSize);
        dirIcon->setAnchorPoint(cocos2d::Vec2::ZERO);
        dirIcon->setPosition(cocos2d::Vec2(rootSize.width - margin, -margin));
        dirIcon->setOpacity(102);

        // 斜线宽高
        float lineLength = rootSize.height * 0.6f; // h * ceil(2 / 3)
        float lineThickness = lineLength * 0.3f;

        // 上斜线
        const auto arrow1 = Layout::create();
        arrow1->setName("TopArrowLine");
        arrow1->setContentSize(cocos2d::Size(lineLength, lineThickness));
        arrow1->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        arrow1->setBackGroundColor(cocos2d::Color3B(191, 191, 191));
        arrow1->setAnchorPoint(cocos2d::Vec2(1, 0.5f));
        arrow1->setPosition(
            cocos2d::Vec2(0, dirIcon->getContentSize().height / 2));
        arrow1->setRotation(-45);
        dirIcon->addChild(arrow1);

        // 下斜线
        const auto arrow2 = Layout::create();
        arrow2->setName("BottomArrowLine");
        arrow2->setContentSize(cocos2d::Size(lineLength, lineThickness));
        arrow2->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        arrow2->setBackGroundColor(cocos2d::Color3B(191, 191, 191));
        arrow2->setAnchorPoint(cocos2d::Vec2(1, 0.5f));
        arrow2->setPosition(
            cocos2d::Vec2(0, dirIcon->getContentSize().height / 2));
        arrow2->setRotation(45);
        dirIcon->addChild(arrow2);

        const auto rect = Layout::create();
        rect->setName("rect");
        rect->setContentSize(cocos2d::Size(lineThickness, lineThickness));
        rect->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        rect->setBackGroundColor(cocos2d::Color3B(191, 191, 191));
        rect->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
        rect->setPosition(
            cocos2d::Vec2(0, dirIcon->getContentSize().height / 2));
        rect->setRotation(45);
        dirIcon->addChild(rect);

        root->addChild(filename);
        root->addChild(underline);
        root->addChild(checkBox);
        root->addChild(dirIcon);
        root->addChild(highlight); // 最后添加按钮（确保在最上层）

        return root;
    }

    Widget *createBottomBarTextInput(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float pad = ctx.relWx(10, kCanvasBottomBarW);
        const float btnBarH = ctx.relHy(90, kCanvasBottomBarH);

        auto *panel4 = Layout::create();
        panel4->setContentSize(ctx.size);
        panel4->setAnchorPoint(Vec2::ZERO);
        panel4->setPosition(Vec2::ZERO);
        panel4->setTouchEnabled(true);

        const Size inputSize(ctx.w() - pad * 2, ctx.h() - btnBarH - pad);
        auto *panel14_9 = Layout::create();
        panel14_9->setContentSize(inputSize);
        panel14_9->setPosition(Vec2(pad, btnBarH));
        panel14_9->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel14_9->setBackGroundColor(Color3B(199, 199, 199));
        panel14_9->setTouchEnabled(true);
        panel4->addChild(panel14_9);

        auto *input = TextField::create("Touch to input", "Arial",
                                        ctx.fontSize(72, kCanvasBottomBarW));
        input->setName("input");
        input->setContentSize(inputSize);
        input->setMaxLengthEnabled(true);
        input->setMaxLength(10);
        input->setTextColor(Color4B::BLACK);
        input->setAnchorPoint(Vec2(0.5f, 0.5f));
        input->setPosition(
            Vec2(inputSize.width * 0.5f, inputSize.height * 0.5f));
        panel14_9->addChild(input);

        const Size btnSize(ctx.relW(0.36f), ctx.relHy(70, kCanvasBottomBarH));
        auto *cancelBtn =
            Button::create("img/gray.png", "img/white.png", "img/gray.png");
        cancelBtn->setName("cancel");
        cancelBtn->setTitleText("Cancel");
        cancelBtn->setTitleFontSize(ctx.fontSize(72, kCanvasBottomBarW));
        cancelBtn->setTitleColor(Color3B::BLACK);
        cancelBtn->setContentSize(btnSize);
        cancelBtn->setScale9Enabled(true);
        cancelBtn->setAnchorPoint(Vec2(0, 0.5f));
        cancelBtn->setPosition(Vec2(pad, btnBarH * 0.5f));
        panel4->addChild(cancelBtn);

        auto *okBtn =
            Button::create("img/gray.png", "img/white.png", "img/gray.png");
        okBtn->setName("ok");
        okBtn->setTitleText("OK");
        okBtn->setTitleFontSize(ctx.fontSize(72, kCanvasBottomBarW));
        okBtn->setTitleColor(Color3B::BLACK);
        okBtn->setContentSize(btnSize);
        okBtn->setScale9Enabled(true);
        okBtn->setAnchorPoint(Vec2(1, 0.5f));
        okBtn->setPosition(Vec2(ctx.w() - pad, btnBarH * 0.5f));
        panel4->addChild(okBtn);

        return panel4;
    }

    Widget *createTextPairInput(const cocos2d::Size &size, float scale) {

        LayoutCtx ctx{ size, scale };
        const auto root = Widget::create();
        root->setContentSize(ctx.size);

        const float pad = ctx.relWx(10, kCanvasTextPairW);
        const float btnY = ctx.h() - ctx.relHy(40, kCanvasTextPairH);
        const float btnSize = ctx.relWx(80, kCanvasTextPairW);
        const float input1H = ctx.relHy(80, kCanvasTextPairH);
        const float input2H =
            ctx.h() - input1H - ctx.relHy(100, kCanvasTextPairH);

        const auto panel4 = Layout::create();
        panel4->setContentSize(ctx.size);
        panel4->setTouchEnabled(true);
        panel4->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        panel4->setBackGroundColor(cocos2d::Color3B(150, 200, 255),
                                   cocos2d::Color3B(255, 255, 255));
        panel4->setPosition(cocos2d::Vec2::ZERO);
        root->addChild(panel4);

        const Size fieldW(ctx.w() - pad * 2, 0);

        const auto panel13 = Layout::create();
        panel13->setContentSize(Size(fieldW.width, input1H));
        panel13->setTouchEnabled(true);
        panel13->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        panel13->setBackGroundColor(Color3B(150, 200, 255),
                                    Color3B(255, 255, 255));
        panel13->setPosition(Vec2(pad, ctx.relHy(320, kCanvasTextPairH)));
        panel4->addChild(panel13);

        const auto input1 = TextField::create(
            "Touch to input", "Arial", ctx.fontSize(72, kCanvasTextPairW));
        input1->setName("input1");
        input1->setMaxLength(10);
        input1->setMaxLengthEnabled(true);
        input1->setTextColor(Color4B::BLACK);
        input1->setContentSize(panel13->getContentSize());
        input1->setPosition(Vec2(fieldW.width * 0.5f, input1H * 0.5f));
        input1->setTouchEnabled(true);
        panel13->addChild(input1);

        const auto panel14 = Layout::create();
        panel14->setContentSize(Size(fieldW.width, input2H));
        panel14->setTouchEnabled(true);
        panel14->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        panel14->setBackGroundColor(Color3B(150, 200, 255),
                                    Color3B(255, 255, 255));
        panel14->setPosition(Vec2(pad, pad));
        panel4->addChild(panel14);

        const auto input2 = TextField::create(
            "Touch to input", "Arial", ctx.fontSize(72, kCanvasTextPairW));
        input2->setName("input2");
        input2->setMaxLength(10);
        input2->setMaxLengthEnabled(true);
        input2->setTextColor(Color4B::BLACK);
        input2->setContentSize(panel14->getContentSize());
        input2->setPosition(Vec2(fieldW.width * 0.5f, input2H * 0.5f));
        input2->setTouchEnabled(true);
        panel14->addChild(input2);

        const auto cancelBtn =
            Button::create("img/Cancel_Normal.png", "img/Cancel_Press.png",
                           "img/CheckBox_Disable.png");
        cancelBtn->setName("cancel");
        cancelBtn->setContentSize(Size(btnSize, btnSize));
        cancelBtn->setPosition(Vec2(0, btnY));
        cancelBtn->setAnchorPoint(Vec2(0, 0.5f));
        cancelBtn->setTitleFontSize(ctx.fontSize(14, kCanvasTextPairW));
        cancelBtn->setTitleColor(Color3B(65, 65, 70));
        root->addChild(cancelBtn);

        const auto okBtn = Button::create("img/CheckBoxNode_Normal.png",
                                          "img/CheckBoxNode_Press.png",
                                          "img/CheckBox_Disable.png");
        okBtn->setName("ok");
        okBtn->setContentSize(Size(btnSize, btnSize));
        okBtn->setPosition(Vec2(ctx.w(), btnY));
        okBtn->setAnchorPoint(Vec2(1, 0.5f));
        okBtn->setTitleFontSize(ctx.fontSize(14, kCanvasTextPairW));
        okBtn->setTitleColor(Color3B(65, 65, 70));
        root->addChild(okBtn);
        return root;
    }

    Widget *createSelectList(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float btnY = ctx.h() - ctx.relHy(40, kCanvasSelectListH);
        const float btnSize = ctx.relWx(80, kCanvasSelectListW);
        const float contentH = ctx.relH(400.f / 480.f);
        const float pageW = ctx.relW(560.f / 720.f);
        const float pageX = ctx.relW(80.f / 720.f);

        const auto root = Widget::create();
        root->setContentSize(ctx.size);

        const auto panel4 = Layout::create();
        panel4->setContentSize(ctx.size);
        panel4->setAnchorPoint(cocos2d::Vec2::ZERO);
        panel4->setPosition(cocos2d::Vec2::ZERO);
        panel4->setTouchEnabled(true);

        // title
        const auto titleLabel =
            Text::create("Title", "fonts/NotoSansCJK-Regular.ttc", 48);
        titleLabel->setPosition(Vec2(ctx.w() * 0.5f, btnY));
        titleLabel->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
        titleLabel->enableOutline(cocos2d::Color4B(0, 0, 255, 255), 1);
        titleLabel->enableShadow(cocos2d::Color4B(110, 110, 110, 255),
                                 cocos2d::Size(2, -2));
        panel4->addChild(titleLabel);

        // Panel_5 - 内容面板
        const auto panel5 = Layout::create();
        panel5->setContentSize(Size(ctx.w(), contentH));
        panel5->setPosition(Vec2::ZERO);
        panel5->setTouchEnabled(true);
        panel5->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel5->setBackGroundColor(cocos2d::Color3B(255, 200, 150));
        panel5->setBackGroundColorOpacity(102);
        panel5->setClippingEnabled(true);

        // pageview 区域
        const auto pageViewPanel = Layout::create();
        pageViewPanel->setName("pageview");
        pageViewPanel->setContentSize(Size(pageW, contentH));
        pageViewPanel->setPosition(Vec2(pageX, 0));
        pageViewPanel->setTouchEnabled(true);
        pageViewPanel->setBackGroundColorType(
            Layout::BackGroundColorType::SOLID);
        pageViewPanel->setBackGroundColor(cocos2d::Color3B(255, 200, 150));
        pageViewPanel->setBackGroundColorOpacity(102);
        panel5->addChild(pageViewPanel);

        // Panel_17 - 输入或信息框
        const auto panel17 = Layout::create();
        panel17->setContentSize(cocos2d::Size(560, 96));
        panel17->setPosition(cocos2d::Vec2(80, 200));
        panel17->setAnchorPoint(cocos2d::Vec2(0, 0.5f));
        panel5->addChild(panel17);

        // dir_icon - 左箭头
        const auto dirIconLeft = Layout::create();
        dirIconLeft->setContentSize(cocos2d::Size(80, 80));
        dirIconLeft->setPosition(cocos2d::Vec2(80, 200));
        dirIconLeft->setAnchorPoint(cocos2d::Vec2(1.0f, 0.5f));

        // 两个箭头线段组成箭头图形（可替换为 Sprite）
        const auto leftArrowPart1 = Layout::create();
        leftArrowPart1->setContentSize(cocos2d::Size(45, 10));
        leftArrowPart1->setPosition(cocos2d::Vec2(70, 40));
        leftArrowPart1->setRotation(-135);
        dirIconLeft->addChild(leftArrowPart1);

        const auto leftArrowPart2 = Layout::create();
        leftArrowPart2->setContentSize(cocos2d::Size(45, 10));
        leftArrowPart2->setPosition(cocos2d::Vec2(70, 40));
        leftArrowPart2->setRotation(135);
        dirIconLeft->addChild(leftArrowPart2);

        panel5->addChild(dirIconLeft);

        // dir_icon_0 - 右箭头
        const auto dirIconRight = Layout::create();
        dirIconRight->setContentSize(cocos2d::Size(80, 80));
        dirIconRight->setPosition(
            cocos2d::Vec2(640 + 40, 200)); // 640 left margin + half width
        dirIconRight->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
        dirIconRight->setScaleX(-1); // 镜像反转

        // 同样添加箭头部件
        const auto rightArrowPart1 = Layout::create();
        rightArrowPart1->setContentSize(cocos2d::Size(45, 10));
        rightArrowPart1->setPosition(cocos2d::Vec2(70, 40));
        rightArrowPart1->setRotation(-135);
        dirIconRight->addChild(rightArrowPart1);

        const auto rightArrowPart2 = Layout::create();
        rightArrowPart2->setContentSize(cocos2d::Size(45, 10));
        rightArrowPart2->setPosition(cocos2d::Vec2(70, 40));
        rightArrowPart2->setRotation(135);
        dirIconRight->addChild(rightArrowPart2);

        panel5->addChild(dirIconRight);

        // Cancel 按钮
        const auto cancelButton =
            Button::create("img/Cancel_Normal.png", "img/Cancel_Press.png",
                           "img/CheckBox_Disable.png");
        cancelButton->setName("cancel");
        cancelButton->setContentSize(Size(btnSize, btnSize));
        cancelButton->setPosition(Vec2(0, btnY));
        cancelButton->setAnchorPoint(Vec2(0, 0.5f));
        panel4->addChild(cancelButton);

        const auto okButton = Button::create("img/CheckBoxNode_Normal.png",
                                             "img/CheckBoxNode_Press.png",
                                             "img/CheckBox_Disable.png");
        okButton->setName("ok");
        okButton->setContentSize(Size(btnSize, btnSize));
        okButton->setPosition(Vec2(ctx.w(), btnY));
        okButton->setAnchorPoint(Vec2(1.0f, 0.5f));
        panel4->addChild(okButton);

        // 将两个面板添加到根节点
        root->addChild(panel4);
        root->addChild(panel5);

        return root;
    }

    Widget *createListView(const cocos2d::Size &size, float scale) {
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
        listView->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        listView->setBackGroundColor(Color3B(42, 42, 42));
        listView->setItemsMargin(ctx.relHy(11, kCanvasListViewH));
        root->addChild(listView);
        return root;
    }


    Widget *createMessageBox(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const auto root = Widget::create();
        root->setContentSize(ctx.size);

        // Panel_1（背景 Panel）
        const auto panel1 = Layout::create();
        panel1->setTouchEnabled(true);
        panel1->setContentSize(ctx.size);
        panel1->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        panel1->setBackGroundColor(cocos2d::Color3B(150, 200, 255),
                                   cocos2d::Color3B(255, 255, 255));
        panel1->setBackGroundColorVector(cocos2d::Vec2(0, 1));
        panel1->setPosition(cocos2d::Vec2::ZERO);

        // Panel_2（对话框 Panel）
        const auto panel2 = Layout::create();
        panel2->setTouchEnabled(true);
        panel2->setContentSize(
            Size(ctx.relW(576.f / 720.f), ctx.relH(432.f / 960.f)));
        panel2->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        panel2->setBackGroundColor(cocos2d::Color3B(150, 200, 255),
                                   cocos2d::Color3B(255, 255, 255));
        panel2->setBackGroundColorVector(cocos2d::Vec2(0, 1));
        panel2->setPosition(
            Vec2(ctx.relW(72.f / 720.f), ctx.relH(264.f / 960.f))); // 居中显示
        panel1->addChild(panel2);

        // Panel_6（内部 Panel）
        const auto panel6 = Layout::create();
        panel6->setTouchEnabled(true);
        panel6->setContentSize(cocos2d::Size(570, 299));
        panel6->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel6->setBackGroundColor(cocos2d::Color3B(42, 42, 42));
        panel6->setPosition(cocos2d::Vec2(3, 130));
        panel2->addChild(panel6);

        // Panel_3（可能为顶部分隔条）
        const auto panel3 = Layout::create();
        panel3->setTouchEnabled(true);
        panel3->setContentSize(cocos2d::Size(576, 20));
        panel3->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel3->setBackGroundColor(cocos2d::Color3B(85, 85, 85));
        panel3->setPosition(cocos2d::Vec2(0, 317));
        panel2->addChild(panel3);

        // ScrollView text（用于文字显示）
        const auto scrollView = ScrollView::create();
        scrollView->setName("text");
        scrollView->setDirection(ScrollView::Direction::HORIZONTAL);
        scrollView->setTouchEnabled(true);
        scrollView->setContentSize(cocos2d::Size(566, 152));
        scrollView->setInnerContainerSize(cocos2d::Size(566, 222)); // 容器大小
        scrollView->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        scrollView->setBackGroundColor(cocos2d::Color3B(32, 32, 32));
        scrollView->setPosition(cocos2d::Vec2(5, 150));
        panel2->addChild(scrollView);

        // Text 内容
        const auto label =
            Text::create("Text cocos2d::Label", "NotoSansCJK-Regular.ttc", 56);
        label->setName("content");
        label->enableOutline(cocos2d::Color4B::BLUE, 1);
        label->enableShadow(cocos2d::Color4B(110, 110, 110, 255),
                            cocos2d::Size(2, -2));
        label->setAnchorPoint(cocos2d::Vec2(0, 1));
        label->setContentSize(cocos2d::Size(526, 64));
        label->setPosition(cocos2d::Vec2(20, 222));
        scrollView->addChild(label);

        // 标题 title
        const auto title =
            Text::create("Text cocos2d::Label", "NotoSansCJK-Regular.ttc", 64);
        title->setName("title");
        title->setTextColor(cocos2d::Color4B::WHITE);
        title->enableOutline(cocos2d::Color4B::BLUE, 1);
        title->enableShadow(cocos2d::Color4B(110, 110, 110, 255),
                            cocos2d::Size(2, -2));
        title->setAnchorPoint(cocos2d::Vec2(0.5, 1));
        title->setPosition(cocos2d::Vec2(288, 422));
        panel2->addChild(title);

        // btnList Panel
        const auto btnList = Layout::create();
        btnList->setTouchEnabled(true);
        btnList->setContentSize(cocos2d::Size(566, 120));
        btnList->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        btnList->setBackGroundColor(cocos2d::Color3B(32, 32, 32));
        btnList->setPosition(cocos2d::Vec2(5, 5));
        panel2->addChild(btnList);

        // btn（按钮容器）
        const auto btnPanel = Layout::create();
        btnPanel->setTouchEnabled(true);
        btnPanel->setContentSize(cocos2d::Size(250, 105));
        btnPanel->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        btnPanel->setBackGroundColor(cocos2d::Color3B(136, 136, 136),
                                     cocos2d::Color3B(68, 68, 68));
        btnPanel->setBackGroundColorVector(cocos2d::Vec2(0, 1));
        btnPanel->setPosition(cocos2d::Vec2(10, 5));
        btnPanel->setName("btn");
        btnList->addChild(btnPanel);

        // Panel_7 内嵌按钮背景
        const auto btnPanelInner = Layout::create();
        btnPanelInner->setTouchEnabled(true);
        btnPanelInner->setContentSize(cocos2d::Size(242, 97));
        btnPanelInner->setBackGroundColorType(
            Layout::BackGroundColorType::SOLID);
        btnPanelInner->setBackGroundColor(cocos2d::Color3B(153, 153, 153));
        btnPanelInner->setPosition(cocos2d::Vec2(4, 4));
        btnPanel->addChild(btnPanelInner);

        // Button 实体
        const auto button =
            Button::create("img/empty.png", "img/gray.png", "img/gray.png");
        button->setName("btnBody");
        button->setTitleFontName("NotoSansCJK-Regular.ttc");
        button->setTitleFontSize(64);
        button->setTitleColor(cocos2d::Color3B::BLACK);
        button->setScale9Enabled(true);
        button->setContentSize(cocos2d::Size(242, 97));
        button->setPosition(cocos2d::Vec2::ZERO);
        button->setAnchorPoint(cocos2d::Vec2::ZERO);
        button->setZoomScale(0.05f);
        btnPanelInner->addChild(button);
        root->addChild(panel1);

        return root;
    }

    Widget *createProgressBox(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const auto root = Widget::create();
        root->setContentSize(ctx.size);

        // Panel_1
        const auto panel1 = cocos2d::ui::Layout::create();
        panel1->setName("Panel_1");
        panel1->setContentSize(ctx.size);
        panel1->setBackGroundColorType(
            cocos2d::ui::Layout::BackGroundColorType::GRADIENT);
        panel1->setBackGroundColor(cocos2d::Color3B(150, 200, 255),
                                   cocos2d::Color3B(255, 255, 255));
        panel1->setBackGroundColorOpacity(102);
        panel1->setAnchorPoint(cocos2d::Vec2::ZERO);
        panel1->setPosition(cocos2d::Vec2::ZERO);
        root->addChild(panel1);

        // Panel_2
        const auto panel2 = cocos2d::ui::Layout::create();
        panel2->setName("Panel_2");
        panel2->setContentSize(
            Size(ctx.relW(576.f / 720.f), ctx.relH(600.f / 960.f)));
        panel2->setTouchEnabled(true);
        panel2->setAnchorPoint(Vec2(0.5f, 0.5f));
        panel2->setPosition(Vec2(ctx.w() * 0.5f, ctx.h() * 0.5f));
        panel2->setBackGroundColorType(
            cocos2d::ui::Layout::BackGroundColorType::GRADIENT);
        panel2->setBackGroundColor(cocos2d::Color3B(150, 200, 255),
                                   cocos2d::Color3B(255, 255, 255));
        panel2->setBackGroundColorOpacity(255);
        panel1->addChild(panel2);

        // Panel_6
        const auto panel6 = cocos2d::ui::Layout::create();
        panel6->setName("Panel_6");
        panel6->setContentSize(cocos2d::Size(570, 467));
        panel6->setTouchEnabled(true);
        panel6->setPosition(cocos2d::Vec2(3, 130));
        panel6->setBackGroundColorType(
            cocos2d::ui::Layout::BackGroundColorType::GRADIENT);
        panel6->setBackGroundColor(cocos2d::Color3B(150, 200, 255),
                                   cocos2d::Color3B(255, 255, 255));
        panel2->addChild(panel6);

        // LoadingBar progrss_1
        const auto loadingBar1 =
            cocos2d::ui::LoadingBar::create("img/white.png");
        loadingBar1->setName("progrss_1");
        loadingBar1->setContentSize(cocos2d::Size(536, 64));
        loadingBar1->setPercent(50);
        loadingBar1->setPosition(cocos2d::Vec2::ZERO);
        panel2->addChild(loadingBar1);

        // Text progress_text_1
        const auto label1 = cocos2d::ui::Text::create(
            "Text cocos2d::Label", "NotoSansCJK-Regular.ttc", 48);
        label1->setName("progress_text_1");
        label1->setPosition(cocos2d::Vec2(268, 32));
        label1->enableOutline(cocos2d::Color4B(77, 77, 77, 255), 3);
        label1->enableShadow(cocos2d::Color4B(110, 110, 110, 255),
                             cocos2d::Size(2, -2), 0);
        panel2->addChild(label1);

        return root;
    }

    Widget *createCheckListDialog(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const auto root = Widget::create();
        root->setContentSize(ctx.size);

        const auto panel_20 = Layout::create();
        panel_20->setContentSize(ctx.size);
        panel_20->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        panel_20->setBackGroundColor(
            cocos2d::Color3B(150, 200, 255),
            cocos2d::Color3B(255, 255, 255)); // FirstColor & EndColor
        panel_20->setBackGroundColorOpacity(38);
        panel_20->setAnchorPoint(cocos2d::Vec2::ZERO);
        panel_20->setPosition(cocos2d::Vec2::ZERO);
        panel_20->setTouchEnabled(true);

        // Panel_1 中心主面板
        const auto panel_1 = Layout::create();
        panel_1->setContentSize(Size(ctx.relW(0.9f), ctx.relH(0.9f)));
        panel_1->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        panel_1->setBackGroundColor(cocos2d::Color3B(150, 200, 255),
                                    cocos2d::Color3B(255, 255, 255));
        panel_1->setBackGroundColorOpacity(102);
        panel_1->setAnchorPoint(Vec2(0.5f, 0.5f));
        panel_1->setPosition(Vec2(ctx.w() * 0.5f, ctx.h() * 0.5f));
        panel_1->setTouchEnabled(true);
        panel_20->addChild(panel_1);

        // title文本
        const auto title =
            Text::create("Text cocos2d::Label", "NotoSansCJK-Regular.ttc", 64);
        title->setName("title");
        title->setPosition(cocos2d::Vec2(5, 643)); // 以Panel_1为坐标系
        title->setTextColor(cocos2d::Color4B::WHITE);
        title->enableOutline(cocos2d::Color4B(0, 0, 255, 255), 1);
        title->enableShadow(cocos2d::Color4B(110, 110, 110, 255),
                            cocos2d::Size(2, -2));
        panel_1->addChild(title);

        // Panel_2，左中部面板
        const auto panel_2 = Layout::create();
        panel_2->setContentSize(cocos2d::Size(1152, 430));
        panel_2->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        panel_2->setBackGroundColor(cocos2d::Color3B(150, 200, 255),
                                    cocos2d::Color3B(255, 255, 255));
        panel_2->setBackGroundColorOpacity(102);
        panel_2->setAnchorPoint(cocos2d::Vec2::ZERO);
        panel_2->setPosition(cocos2d::Vec2(0, 118));
        panel_2->setTouchEnabled(true);
        panel_1->addChild(panel_2);

        // Panel_4 左边子面板
        const auto panel_4 = Layout::create();
        panel_4->setContentSize(cocos2d::Size(576, 430));
        panel_4->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        panel_4->setBackGroundColor(cocos2d::Color3B(150, 200, 255),
                                    cocos2d::Color3B(255, 255, 255));
        panel_4->setBackGroundColorOpacity(102);
        panel_4->setAnchorPoint(cocos2d::Vec2::ZERO);
        panel_4->setPosition(cocos2d::Vec2::ZERO);
        panel_4->setTouchEnabled(true);
        panel_2->addChild(panel_4);

        // list_1 ListView
        const auto list_1 = ListView::create();
        list_1->setName("list_1");
        list_1->setContentSize(panel_4->getContentSize());
        list_1->setAnchorPoint(cocos2d::Vec2::ZERO);
        list_1->setPosition(cocos2d::Vec2::ZERO);
        list_1->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        list_1->setBackGroundColor(cocos2d::Color3B(42, 42, 42));
        list_1->setBounceEnabled(true);
        panel_4->addChild(list_1);

        // Panel_5 右边子面板
        const auto panel_5 = Layout::create();
        panel_5->setContentSize(cocos2d::Size(576, 430));
        panel_5->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        panel_5->setBackGroundColor(cocos2d::Color3B(150, 200, 255),
                                    cocos2d::Color3B(255, 255, 255));
        panel_5->setBackGroundColorOpacity(102);
        panel_5->setAnchorPoint(cocos2d::Vec2::ZERO);
        panel_5->setPosition(cocos2d::Vec2(576, 0));
        panel_5->setTouchEnabled(true);
        panel_2->addChild(panel_5);

        // list_2 ListView
        const auto list_2 = ListView::create();
        list_2->setName("list_2");
        list_2->setContentSize(Size(panel_5->getContentSize().width -
                                        ctx.relWx(8, kCanvasMessageBoxW),
                                    panel_5->getContentSize().height));
        list_2->setAnchorPoint(cocos2d::Vec2::ZERO);
        list_2->setPosition(cocos2d::Vec2(8, 0));
        list_2->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        list_2->setBackGroundColor(cocos2d::Color3B(42, 42, 42));
        list_2->setBounceEnabled(true);
        panel_5->addChild(list_2);

        // btn_list ScrollView 底部按钮栏
        const auto btn_list = ScrollView::create();
        btn_list->setName("btn_list");
        btn_list->setContentSize(Size(panel_1->getContentSize().width,
                                      ctx.relHy(105, kCanvasMessageBoxH)));
        btn_list->setAnchorPoint(cocos2d::Vec2(0.5f, 0));
        btn_list->setPosition(cocos2d::Vec2(576, 0));
        btn_list->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        btn_list->setBackGroundColor(cocos2d::Color3B(100, 150, 255),
                                     cocos2d::Color3B(255, 255, 255));
        btn_list->setBackGroundColorOpacity(102);
        btn_list->setDirection(ScrollView::Direction::BOTH);
        btn_list->setClippingEnabled(true);
        panel_1->addChild(btn_list);

        // btn_cell 按钮容器
        const auto btn_cell = Layout::create();
        btn_cell->setName("btn_cell");
        btn_cell->setContentSize(
            ctx.relSize(250, 105, kCanvasMessageBoxW, kCanvasMessageBoxH));
        btn_cell->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
        btn_cell->setPosition(cocos2d::Vec2(576, 52.5f));
        btn_cell->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        btn_cell->setBackGroundColor(cocos2d::Color3B(136, 136, 136),
                                     cocos2d::Color3B(68, 68, 68));
        btn_cell->setBackGroundColorOpacity(255);
        btn_cell->setTouchEnabled(true);
        btn_list->addChild(btn_cell);

        // Panel_7 按钮背景面板
        const auto panel_7 = Layout::create();
        panel_7->setContentSize(cocos2d::Size(242, 97));
        panel_7->setAnchorPoint(cocos2d::Vec2::ZERO);
        panel_7->setPosition(cocos2d::Vec2(4, 4));
        panel_7->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        panel_7->setBackGroundColor(cocos2d::Color3B(150, 200, 255),
                                    cocos2d::Color3B(255, 255, 255));
        panel_7->setBackGroundColorOpacity(255);
        btn_cell->addChild(panel_7);

        // btn 按钮
        const auto btn =
            Button::create("img/empty.png", "img/gray.png", "img/gray.png");
        btn->setName("btn");
        btn->setTitleFontSize(64);
        btn->setTitleColor(cocos2d::Color3B::BLACK);
        btn->setAnchorPoint(cocos2d::Vec2::ZERO);
        btn->setPosition(cocos2d::Vec2::ZERO);
        btn->setContentSize(cocos2d::Size(242, 97));
        btn->setScale9Enabled(true);
        btn->setTouchEnabled(true);
        panel_7->addChild(btn);

        root->addChild(panel_20);

        return root;
    }

    Widget *createMediaPlayerNavi(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float pad = ctx.relWx(20, kCanvasMediaNaviW);

        auto *root = Widget::create();
        root->setContentSize(ctx.size);

        auto *naviBar = Layout::create();
        naviBar->setName("NaviBar");
        naviBar->setContentSize(ctx.size);
        naviBar->setAnchorPoint(Vec2::ZERO);
        naviBar->setPosition(Vec2::ZERO);
        naviBar->setTouchEnabled(true);
        naviBar->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        naviBar->setBackGroundColor(Color3B(42, 42, 42));
        root->addChild(naviBar);

        auto *back =
            Button::create("img/back_btn_off.png", "img/back_btn_on.png",
                           "img/back_btn_on.png");
        back->setName("Back");
        back->setContentSize(
            ctx.relSize(80, 80, kCanvasMediaNaviW, kCanvasMediaNaviH));
        back->setAnchorPoint(Vec2(0, 0.5f));
        back->setPosition(
            Vec2(pad, ctx.h() - ctx.relHy(40, kCanvasMediaNaviH)));
        naviBar->addChild(back);

        auto *title = makeMediaText(
            ctx, "Title", 40,
            Vec2(ctx.w() * 0.5f, ctx.h() - ctx.relHy(20, kCanvasMediaNaviH)),
            Vec2(0.5f, 1.f));
        title->setContentSize(Size(ctx.w() - ctx.relWx(160, kCanvasMediaNaviW),
                                   ctx.relHy(52, kCanvasMediaNaviH)));
        naviBar->addChild(title);

        auto *timeline = Slider::create("Default/Slider_Back.png",
                                        "Default/SliderNode_Normal.png");
        timeline->loadProgressBarTexture("Default/Slider_PressBar.png");
        timeline->setName("Timeline");
        timeline->setContentSize(
            Size(ctx.w() - pad * 2, ctx.relHy(20, kCanvasMediaNaviH)));
        timeline->setAnchorPoint(Vec2(0.5f, 0.5f));
        timeline->setPosition(
            Vec2(ctx.w() * 0.5f, ctx.relHy(55, kCanvasMediaNaviH)));
        naviBar->addChild(timeline);

        auto *playTime = makeMediaText(
            ctx, "PlayTime", 32, Vec2(pad, ctx.relHy(5, kCanvasMediaNaviH)),
            Vec2(0, 0));
        naviBar->addChild(playTime);

        auto *remainTime = makeMediaText(
            ctx, "RemainTime", 32,
            Vec2(ctx.w() - pad, ctx.relHy(5, kCanvasMediaNaviH)), Vec2(1, 0));
        naviBar->addChild(remainTime);

        return root;
    }

    Widget *createMediaPlayerBody(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };

        auto *root = Widget::create();
        root->setContentSize(ctx.size);

        auto *overlay = Layout::create();
        overlay->setName("Overlay");
        overlay->setContentSize(ctx.size);
        overlay->setAnchorPoint(Vec2::ZERO);
        overlay->setPosition(Vec2::ZERO);
        overlay->setTouchEnabled(true);
        overlay->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        overlay->setBackGroundColor(Color3B::BLACK);
        root->addChild(overlay);

        const Size osdSize(ctx.relW(384.f / 960.f), ctx.relH(192.f / 640.f));
        auto *osd = Layout::create();
        osd->setName("OSD");
        osd->setContentSize(osdSize);
        osd->setAnchorPoint(Vec2(0.5f, 0.5f));
        osd->setPosition(Vec2(ctx.w() * 0.5f, ctx.h() * 0.5f));
        osd->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        osd->setBackGroundColor(Color3B(32, 32, 32));
        osd->setBackGroundColorOpacity(191);
        root->addChild(osd);

        auto *osdText = Text::create("Text Label", "NotoSansCJK-Regular.ttc",
                                     ctx.fontSize(64, kCanvasMediaBodyW));
        osdText->setName("OSDText");
        osdText->setAnchorPoint(Vec2(0.5f, 0.5f));
        osdText->setPosition(Vec2(osdSize.width * 0.5f, osdSize.height * 0.5f));
        osdText->setTextColor(Color4B::WHITE);
        osdText->enableOutline(Color4B(0, 0, 255, 255), 1);
        osdText->enableShadow(Color4B(110, 110, 110, 255), Size(2, -2));
        osd->addChild(osdText);

        return root;
    }

    Widget *createMediaPlayerFoot(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };

        auto *root = Widget::create();
        root->setContentSize(ctx.size);

        auto *controlBar = Layout::create();
        controlBar->setName("ControlBar");
        controlBar->setContentSize(ctx.size);
        controlBar->setAnchorPoint(Vec2::ZERO);
        controlBar->setPosition(Vec2::ZERO);
        controlBar->setTouchEnabled(true);
        controlBar->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        controlBar->setBackGroundColor(Color3B(42, 42, 42));
        root->addChild(controlBar);

        auto *playBtn = buildPlayBtn(ctx);
        playBtn->setPosition(Vec2(ctx.w() * 0.5f, ctx.h() * 0.5f));
        controlBar->addChild(playBtn);

        return root;
    }

    Widget *createAllTips(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };

        const auto root = Widget::create();
        root->setContentSize(ctx.size);

        const auto listView = ListView::create();
        listView->setName("tipslist");
        listView->setDirection(ScrollView::Direction::VERTICAL);
        listView->setBounceEnabled(true);
        listView->setTouchEnabled(true);
        listView->setContentSize(ctx.size);
        listView->setAnchorPoint(Vec2::ZERO);
        listView->setPosition(Vec2::ZERO);
        listView->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        listView->setBackGroundColor(Color3B(42, 42, 42));
        root->addChild(listView);

        const float touchH = ctx.relH(360.f / kCanvasAllTipsH);
        const float screenH = ctx.relH(180.f / kCanvasAllTipsH);

        auto *touchTip = Layout::create();
        touchTip->setName("touchTip");
        touchTip->setContentSize(Size(ctx.w(), touchH));
        LayoutCtx touchCtx{ touchTip->getContentSize(), scale };
        touchTip->addChild(buildTouchModeTips(touchCtx));
        listView->pushBackCustomItem(touchTip);

        auto *screenTip = Layout::create();
        screenTip->setName("screenTip");
        screenTip->setContentSize(Size(ctx.w(), screenH));
        LayoutCtx screenCtx{ screenTip->getContentSize(), scale };
        screenTip->addChild(buildScreenModeTips(screenCtx));
        listView->pushBackCustomItem(screenTip);

        const auto btnClose =
            Button::create("img/Cancel_Normal.png", "img/Cancel_Press.png",
                           "img/Cancel_Press.png");
        btnClose->setName("btn_close");
        btnClose->setAnchorPoint(Vec2(1, 1));
        btnClose->setPosition(Vec2(ctx.w(), ctx.h()));
        btnClose->setContentSize(
            ctx.relSize(80, 80, kCanvasAllTipsW, kCanvasAllTipsH));
        root->addChild(btnClose);

        return root;
    }

    Widget *createWinMgrOverlay(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float pad = ctx.relWx(20, kCanvasWinMgrW);
        const Size btnSize =
            ctx.relSize(64, 64, kCanvasWinMgrW, kCanvasWinMgrH);
        const Size topBtnSize =
            ctx.relSize(76, 76, kCanvasWinMgrW, kCanvasWinMgrH);

        auto *root = Widget::create();
        root->setContentSize(ctx.size);

        auto *dim = Layout::create();
        dim->setName("Panel_1");
        dim->setContentSize(ctx.size);
        dim->setAnchorPoint(Vec2(0.5f, 0.5f));
        dim->setPosition(Vec2(ctx.w() * 0.5f, ctx.h() * 0.5f));
        dim->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        dim->setBackGroundColor(Color3B::BLACK);
        dim->setBackGroundColorOpacity(51);
        dim->setTouchEnabled(true);
        root->addChild(dim);

        auto *left =
            Button::create("img/back_btn_off.png", "img/back_btn_on.png",
                           "img/back_btn_on.png");
        left->setName("left");
        left->setContentSize(btnSize);
        left->setAnchorPoint(Vec2(0, 0.5f));
        left->setPosition(Vec2(pad, ctx.h() * 0.5f));
        root->addChild(left);

        auto *right =
            Button::create("img/back_btn_off.png", "img/back_btn_on.png",
                           "img/back_btn_on.png");
        right->setName("right");
        right->setContentSize(btnSize);
        right->setScaleX(-1);
        right->setAnchorPoint(Vec2(0.5f, 0.5f));
        right->setPosition(
            Vec2(ctx.w() - ctx.relWx(52, kCanvasWinMgrW), ctx.h() * 0.5f));
        root->addChild(right);

        auto *ok =
            Button::create("img/CheckBox_Normal.png", "img/CheckBox_Press.png",
                           "img/CheckBox_Disable.png");
        ok->setName("ok");
        ok->setContentSize(topBtnSize);
        ok->setAnchorPoint(Vec2(0.5f, 0.5f));
        ok->setPosition(Vec2(ctx.w() - ctx.relWx(80, kCanvasWinMgrW),
                             ctx.h() - ctx.relHy(80, kCanvasWinMgrH)));
        root->addChild(ok);

        auto *fillscr =
            Button::create("img/CheckBox_Normal.png", "img/CheckBox_Press.png",
                           "img/CheckBox_Disable.png");
        fillscr->setName("fillscr");
        fillscr->setContentSize(topBtnSize);
        fillscr->setAnchorPoint(Vec2(0.5f, 0.5f));
        fillscr->setPosition(Vec2(ctx.relWx(80, kCanvasWinMgrW),
                                  ctx.h() - ctx.relHy(80, kCanvasWinMgrH)));
        root->addChild(fillscr);

        auto *fillIcon =
            makeImage("img/windows_icon.png",
                      ctx.relSize(32, 32, kCanvasWinMgrW, kCanvasWinMgrH),
                      Vec2(topBtnSize.width * 0.5f, topBtnSize.height * 0.5f));
        fillscr->addChild(fillIcon);

        return root;
    }
    /**
     * 创建一个横向分割线 Widget
     * @param width      分割线实际长度
     * @param lineHeight 线粗（像素）
     * @param color      线颜色
     */
    Widget *
    createSeperateItem(float width, float lineHeight = 2.0f,
                       const cocos2d::Color4F &color = cocos2d::Color4F::GRAY) {
        // 1. 根节点
        Widget *root = Widget::create();
        root->setContentSize(
            cocos2d::Size(width, lineHeight + 20)); // 上下各留 10 像素空隙

        // 2. 用 DrawNode 画线
        cocos2d::DrawNode *line = cocos2d::DrawNode::create();
        cocos2d::Vec2 from(0, 0);
        cocos2d::Vec2 to(width, 0);
        line->drawLine(from, to, color);

        // 3. 把线放在根节点中间
        line->setAnchorPoint(cocos2d::Vec2::ANCHOR_MIDDLE_LEFT);
        line->setPosition(cocos2d::Vec2(0, (lineHeight + 20) * 0.5f));

        root->addChild(line);
        return root;
    }


} // namespace Csd
