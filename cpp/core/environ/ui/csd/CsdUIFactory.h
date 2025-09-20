//
// Created by lidong on 25-6-19.
//
#pragma once

#include <ui/CocosGUI.h>

#include <2d/CCLayer.h>
#include <renderer/CCGLProgram.h>

#include <spdlog/spdlog.h>
namespace Csd {
#define LOGGER spdlog::get("core")

    using namespace cocos2d::ui;

    using NodeBuilderFn =
        std::function<Widget *(const cocos2d::Size &size, float scale)>;

    static Widget *createEmpty(const cocos2d::Size &, float) { return nullptr; }

    static Widget *createMainFileSelector(const cocos2d::Size &size,
                                          float scale) {

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

    static Widget *createTableView(const cocos2d::Size &size, float) {
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

    static Widget *createNaviBarWithMenu(const cocos2d::Size &size, float) {

        constexpr int bothSizesPadding = 13;
        const cocos2d::Size leftBtnSize(80, 80);
        const cocos2d::Size &rightBtnSize = leftBtnSize;
        const cocos2d::Size titleSize(
            size.width - leftBtnSize.width - rightBtnSize.width, size.height);

        const float yOffset = size.height / 2 - bothSizesPadding;

        // 创建根节点：容器层
        const auto root = Widget::create();
        root->setAnchorPoint(cocos2d::Vec2::ZERO);
        root->setContentSize(size);

        // 创建 背景
        const auto background = Layout::create();
        background->setName("background");
        background->setContentSize(size);
        background->setTouchEnabled(true);
        background->setAnchorPoint(cocos2d::Vec2(0, 0));
        background->setPosition(cocos2d::Vec2(0, bothSizesPadding));
        background->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        background->setBackGroundColor(cocos2d::Color3B(42, 42, 42));
        background->setBackGroundColorOpacity(255);

        // 添加左按钮
        const auto leftBtn =
            Button::create("img/back_btn_off.png", "img/back_btn_on.png",
                           "img/back_btn_on.png");
        leftBtn->setName("left");
        leftBtn->setTouchEnabled(true);
        leftBtn->setContentSize(leftBtnSize);
        leftBtn->setPosition(cocos2d::Vec2(bothSizesPadding, yOffset));
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
        titleBtn->setPosition(cocos2d::Vec2(leftBtnSize.width, yOffset));
        titleBtn->setAnchorPoint(cocos2d::Vec2(0, 0.5));
        titleBtn->setTitleFontSize(32);
        titleBtn->setTitleAlignment(cocos2d::TextHAlignment::CENTER,
                                    cocos2d::TextVAlignment::CENTER);
        titleBtn->setTouchEnabled(true);
        titleBtn->setTitleColor(cocos2d::Color3B(199, 199, 199));

        // 右侧按钮
        const auto rightBtn = Button::create(
            "img/menu_icon.png", "img/menu_press.png", "img/menu_icon.png");
        rightBtn->setName("right");
        rightBtn->setTouchEnabled(true);
        rightBtn->setContentSize(rightBtnSize);
        rightBtn->setPosition(
            cocos2d::Vec2(size.width - bothSizesPadding, yOffset));
        rightBtn->setAnchorPoint(cocos2d::Vec2(1, 0.5));

        background->addChild(leftBtn);
        background->addChild(titleBtn);
        background->addChild(rightBtn);

        root->addChild(background);

        return root;
    }

    static Widget *createFileItem(const cocos2d::Size &size, float) {
        constexpr int margin = 12;
        const cocos2d::Size rootSize(size.width - margin * 2,
                                     size.height - margin * 2);
        const cocos2d::Size &highlightFocusSize = size;
        const cocos2d::Size rightMenuSize(80, 80);

        const cocos2d::Size underlineSize(rootSize.width, 4);
        const cocos2d::Size filenameSize(rootSize.width - rightMenuSize.width,
                                         rootSize.height -
                                             underlineSize.height);

        const auto root = Widget::create();
        root->setAnchorPoint(cocos2d::Vec2::ZERO);
        root->setPosition(cocos2d::Vec2(margin, margin));
        root->setContentSize(rootSize);

        // filename label
        const auto filename =
            Text::create("", "NotoSansCJK-Regular.ttc", rootSize.height);
        filename->setName("filename");
        filename->setContentSize(filenameSize);
        filename->setSwallowTouches(false);
        filename->setAnchorPoint(cocos2d::Vec2::ZERO);
        filename->setPosition(cocos2d::Vec2(margin, margin));
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

    static Widget *createNaviBar() {
        // 创建根节点
        const auto root = Widget::create();
        root->setContentSize(cocos2d::Size(720, 120));

        // Panel_1 背景面板（含渐变色）
        const auto panel1 = Layout::create();
        panel1->setName("Panel_1");
        panel1->setContentSize(cocos2d::Size(720, 120));
        panel1->setAnchorPoint(cocos2d::Vec2::ZERO);
        panel1->setPosition(cocos2d::Vec2::ZERO);
        panel1->setTouchEnabled(true);

        // 渐变背景
        const auto gradient = cocos2d::LayerGradient::create(
            cocos2d::Color4B(150, 200, 255, 255), // FirstColor
            cocos2d::Color4B(255, 255, 255, 255), // EndColor
            cocos2d::Vec2(0, 1) // ColorVector (Y向上)
        );
        gradient->setContentSize(panel1->getContentSize());
        gradient->setAnchorPoint(cocos2d::Vec2::ZERO);
        gradient->setPosition(cocos2d::Vec2::ZERO);
        panel1->addChild(gradient, -1);

        // 左侧按钮
        const auto left =
            Button::create("img/back_btn_off.png", "img/back_btn_on.png",
                           "img/back_btn_on.png");
        left->setName("left");
        left->setTouchEnabled(true);
        left->setContentSize(cocos2d::Size(100, 100));
        left->setAnchorPoint(cocos2d::Vec2(0, 0.5f));
        left->setPosition(cocos2d::Vec2(20, 60));
        panel1->addChild(left);

        // 中间 Panel_2（裁剪区域）
        const auto panel2 = Layout::create();
        panel2->setName("Panel_2");
        panel2->setContentSize(cocos2d::Size(500, 120));
        panel2->setAnchorPoint(cocos2d::Vec2::ZERO);
        panel2->setPosition(cocos2d::Vec2(110, 0));
        panel2->setClippingEnabled(true);
        panel2->setTouchEnabled(true);

        // 中间渐变背景（可选）
        const auto panel2Bg = cocos2d::LayerGradient::create(
            cocos2d::Color4B(150, 200, 255, 255),
            cocos2d::Color4B(255, 255, 255, 255), cocos2d::Vec2(0, 1));
        panel2Bg->setContentSize(panel2->getContentSize());
        panel2Bg->setPosition(cocos2d::Vec2::ZERO);
        panel2->addChild(panel2Bg, -1);

        // 标题按钮
        const auto title =
            Button::create("img/empty.png", "img/gray.png", "img/empty.png");
        title->setName("title");
        title->setTouchEnabled(true);
        title->setContentSize(cocos2d::Size(500, 120));
        title->setAnchorPoint(cocos2d::Vec2(0, 0.5f));
        title->setPosition(cocos2d::Vec2(0, 60));
        title->setTitleFontSize(64);
        title->setTitleColor(cocos2d::Color3B(199, 199, 199));
        title->setTitleText("标题");
        panel2->addChild(title);

        panel1->addChild(panel2);

        // 右侧按钮（空 Panel）
        const auto right = Layout::create();
        right->setName("right");
        right->setContentSize(cocos2d::Size(100, 100));
        right->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
        right->setPosition(cocos2d::Vec2(660, 60));
        right->setTouchEnabled(true);

        // 可选：添加背景渐变色
        const auto rightBg = cocos2d::LayerGradient::create(
            cocos2d::Color4B(150, 200, 255, 255),
            cocos2d::Color4B(255, 255, 255, 255), cocos2d::Vec2(0, 1));
        rightBg->setContentSize(right->getContentSize());
        rightBg->setPosition(cocos2d::Vec2::ZERO);
        right->addChild(rightBg, -1);

        panel1->addChild(right);

        // 添加所有到 root
        root->addChild(panel1);

        return root;
    }

    static Widget *createBottomBarTextInput() {
        const auto root = Widget::create();
        root->setContentSize(cocos2d::Size(720, 340));

        // Panel_4
        const auto panel4 = Layout::create();
        panel4->setContentSize(cocos2d::Size(720, 340));
        panel4->setAnchorPoint(cocos2d::Vec2::ZERO);
        panel4->setPosition(cocos2d::Vec2::ZERO);
        panel4->setTouchEnabled(true);
        root->addChild(panel4);

        // Panel_14_9
        const auto panel14_9 = Layout::create();
        panel14_9->setContentSize(cocos2d::Size(700, 240));
        panel14_9->setPosition(cocos2d::Vec2(10, 90));
        panel14_9->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel14_9->setBackGroundColor(cocos2d::Color3B(199, 199, 199));
        panel14_9->setTouchEnabled(true);
        panel4->addChild(panel14_9);

        // TextField: input
        const auto input = TextField::create("Touch to input", "Arial", 72);
        input->setContentSize(cocos2d::Size(700, 240));
        input->setMaxLengthEnabled(true);
        input->setMaxLength(10);
        input->setTextColor(cocos2d::Color4B::BLACK);
        input->setAnchorPoint(cocos2d::Vec2(0.5, 0.5));
        input->setPosition(cocos2d::Vec2(350, 120));
        panel14_9->addChild(input);

        // Cancel Button
        const auto cancelBtn =
            Button::create("img/gray.png", "img/white.png", "img/gray.png");
        cancelBtn->setTitleText("Cancel");
        cancelBtn->setTitleFontSize(72);
        cancelBtn->setTitleColor(cocos2d::Color3B::BLACK);
        cancelBtn->setContentSize(cocos2d::Size(260, 70));
        cancelBtn->setScale9Enabled(true);
        cancelBtn->setAnchorPoint(cocos2d::Vec2(0, 0.5));
        cancelBtn->setPosition(cocos2d::Vec2(20, 45));
        panel4->addChild(cancelBtn);

        // OK Button
        const auto okBtn =
            Button::create("img/gray.png", "img/white.png", "img/gray.png");
        okBtn->setTitleText("OK");
        okBtn->setTitleFontSize(72);
        okBtn->setTitleColor(cocos2d::Color3B::BLACK);
        okBtn->setContentSize(cocos2d::Size(260, 70));
        okBtn->setScale9Enabled(true);
        okBtn->setAnchorPoint(cocos2d::Vec2(1, 0.5));
        okBtn->setPosition(cocos2d::Vec2(700, 45));
        panel4->addChild(okBtn);

        return root;
    }

    static Widget *createTextPairInput() {

        const auto root = Widget::create();
        root->setContentSize(cocos2d::Size(720, 480));

        // Panel_4
        const auto panel4 = Layout::create();
        panel4->setContentSize(cocos2d::Size(720, 480));
        panel4->setTouchEnabled(true);
        panel4->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        panel4->setBackGroundColor(cocos2d::Color3B(150, 200, 255),
                                   cocos2d::Color3B(255, 255, 255));
        panel4->setPosition(cocos2d::Vec2::ZERO);
        root->addChild(panel4);

        // Panel_13 (input1 container)
        const auto panel13 = Layout::create();
        panel13->setContentSize(cocos2d::Size(700, 80));
        panel13->setTouchEnabled(true);
        panel13->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        panel13->setBackGroundColor(cocos2d::Color3B(150, 200, 255),
                                    cocos2d::Color3B(255, 255, 255));
        panel13->setPosition(cocos2d::Vec2(10, 320));
        panel4->addChild(panel13);

        // input1
        const auto input1 = TextField::create("Touch to input", "Arial", 72);
        input1->setMaxLength(10);
        input1->setMaxLengthEnabled(true);
        input1->setTextColor(cocos2d::Color4B::BLACK);
        input1->setContentSize(cocos2d::Size(700, 80));
        input1->setPosition(cocos2d::Vec2(350, 40));
        input1->setTouchEnabled(true);
        panel13->addChild(input1);

        // Panel_14 (input2 container)
        const auto panel14 = Layout::create();
        panel14->setContentSize(cocos2d::Size(700, 290));
        panel14->setTouchEnabled(true);
        panel14->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        panel14->setBackGroundColor(cocos2d::Color3B(150, 200, 255),
                                    cocos2d::Color3B(255, 255, 255));
        panel14->setPosition(cocos2d::Vec2(10, 10));
        panel4->addChild(panel14);

        // input2
        const auto input2 = TextField::create("Touch to input", "Arial", 72);
        input2->setMaxLength(10);
        input2->setMaxLengthEnabled(true);
        input2->setTextColor(cocos2d::Color4B::BLACK);
        input2->setContentSize(cocos2d::Size(700, 290));
        input2->setPosition(cocos2d::Vec2(350, 145));
        input2->setTouchEnabled(true);
        panel14->addChild(input2);

        // Cancel Button
        const auto cancelBtn =
            Button::create("img/Cancel_Normal.png", "img/Cancel_Press.png",
                           "img/CheckBox_Disable.png");
        cancelBtn->setContentSize(cocos2d::Size(80, 80));
        cancelBtn->setPosition(cocos2d::Vec2(0, 440));
        cancelBtn->setAnchorPoint(cocos2d::Vec2(0, 0.5));
        cancelBtn->setTitleFontSize(14);
        cancelBtn->setTitleColor(cocos2d::Color3B(65, 65, 70));
        root->addChild(cancelBtn);

        // OK Button
        const auto okBtn = Button::create("img/CheckBoxNode_Normal.png",
                                          "img/CheckBoxNode_Press.png",
                                          "img/CheckBox_Disable.png");
        okBtn->setContentSize(cocos2d::Size(80, 80));
        okBtn->setPosition(cocos2d::Vec2(720, 440));
        okBtn->setAnchorPoint(cocos2d::Vec2(1, 0.5));
        okBtn->setTitleFontSize(14);
        okBtn->setTitleColor(cocos2d::Color3B(65, 65, 70));
        root->addChild(okBtn);
        return root;
    }

    static Widget *createSelectList() {
        const auto root = Widget::create();

        // Panel_4 - 背景面板
        const auto panel4 = Layout::create();
        panel4->setContentSize(cocos2d::Size(720, 480));
        panel4->setAnchorPoint(cocos2d::Vec2::ZERO);
        panel4->setPosition(cocos2d::Vec2::ZERO);
        panel4->setTouchEnabled(true);

        // title
        const auto titleLabel =
            Text::create("Title", "fonts/NotoSansCJK-Regular.ttc", 48);
        titleLabel->setPosition(cocos2d::Vec2(360, 441.5f));
        titleLabel->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
        titleLabel->enableOutline(cocos2d::Color4B(0, 0, 255, 255), 1);
        titleLabel->enableShadow(cocos2d::Color4B(110, 110, 110, 255),
                                 cocos2d::Size(2, -2));
        panel4->addChild(titleLabel);

        // Panel_5 - 内容面板
        const auto panel5 = Layout::create();
        panel5->setContentSize(cocos2d::Size(720, 400));
        panel5->setPosition(cocos2d::Vec2::ZERO);
        panel5->setTouchEnabled(true);
        panel5->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel5->setBackGroundColor(cocos2d::Color3B(255, 200, 150));
        panel5->setBackGroundColorOpacity(102);
        panel5->setClippingEnabled(true);

        // pageview 区域
        const auto pageViewPanel = Layout::create();
        pageViewPanel->setContentSize(cocos2d::Size(560, 400));
        pageViewPanel->setPosition(cocos2d::Vec2(80, 0));
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
        cancelButton->setPosition(cocos2d::Vec2(0, 440));
        cancelButton->setAnchorPoint(cocos2d::Vec2(0, 0.5f));
        panel4->addChild(cancelButton);

        // OK 按钮
        const auto okButton = Button::create("img/CheckBoxNode_Normal.png",
                                             "img/CheckBoxNode_Press.png",
                                             "img/CheckBox_Disable.png");
        okButton->setPosition(cocos2d::Vec2(720, 440));
        okButton->setAnchorPoint(cocos2d::Vec2(1.0f, 0.5f));
        panel4->addChild(okButton);

        // 将两个面板添加到根节点
        root->addChild(panel4);
        root->addChild(panel5);

        return root;
    }

    static Widget *createListView() {
        const auto root = Widget::create();

        // 设置 layer 尺寸
        root->setContentSize(cocos2d::Size(720, 960));

        // 创建 ListView
        const auto listView = ListView::create();
        listView->setDirection(ListView::Direction::VERTICAL);
        listView->setBounceEnabled(true);
        listView->setTouchEnabled(true);
        listView->setContentSize(cocos2d::Size(720, 960));
        listView->setAnchorPoint(cocos2d::Vec2::ZERO);
        listView->setPosition(cocos2d::Vec2::ZERO);
        listView->setItemsMargin(11);

        // 可选：设置背景颜色或图片
        // listView->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        // listView->setBackGroundColor(cocos2d::Color3B(150, 150, 255),
        // cocos2d::Color3B(255, 255, 255));

        root->addChild(listView);
        return root;
    }

    static Widget *createMessageBox() {
        const auto root = Widget::create();

        root->setContentSize(cocos2d::Size(720, 960));

        // Panel_1（背景 Panel）
        const auto panel1 = Layout::create();
        panel1->setTouchEnabled(true);
        panel1->setContentSize(cocos2d::Size(720, 960));
        panel1->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        panel1->setBackGroundColor(cocos2d::Color3B(150, 200, 255),
                                   cocos2d::Color3B(255, 255, 255));
        panel1->setBackGroundColorVector(cocos2d::Vec2(0, 1));
        panel1->setPosition(cocos2d::Vec2::ZERO);

        // Panel_2（对话框 Panel）
        const auto panel2 = Layout::create();
        panel2->setTouchEnabled(true);
        panel2->setContentSize(cocos2d::Size(576, 432));
        panel2->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        panel2->setBackGroundColor(cocos2d::Color3B(150, 200, 255),
                                   cocos2d::Color3B(255, 255, 255));
        panel2->setBackGroundColorVector(cocos2d::Vec2(0, 1));
        panel2->setPosition(cocos2d::Vec2(72, 264)); // 居中显示
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
        label->setTextColor(cocos2d::Color4B::WHITE);
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
        button->setTitleText("Button");
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

    static Widget *createProgressBox() {
        const auto root = Widget::create();
        root->setContentSize(cocos2d::Size(720, 960));

        // Panel_1
        const auto panel1 = cocos2d::ui::Layout::create();
        panel1->setName("Panel_1");
        panel1->setContentSize(cocos2d::Size(720, 960));
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
        panel2->setContentSize(cocos2d::Size(576, 600));
        panel2->setTouchEnabled(true);
        panel2->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
        panel2->setPosition(cocos2d::Vec2(360, 480));
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

    static Widget *createCheckListDialog() {
        const auto root = Widget::create();

        // 最外层Panel_20（半透明背景）
        const auto panel_20 = Layout::create();
        panel_20->setContentSize(cocos2d::Size(1280, 720));
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
        panel_1->setContentSize(cocos2d::Size(1152, 648));
        panel_1->setBackGroundColorType(Layout::BackGroundColorType::GRADIENT);
        panel_1->setBackGroundColor(cocos2d::Color3B(150, 200, 255),
                                    cocos2d::Color3B(255, 255, 255));
        panel_1->setBackGroundColorOpacity(102);
        panel_1->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
        panel_1->setPosition(cocos2d::Vec2(640, 360));
        panel_1->setTouchEnabled(true);
        panel_20->addChild(panel_1);

        // title文本
        const auto title =
            Text::create("Text cocos2d::Label", "NotoSansCJK-Regular.ttc", 64);
        title->setAnchorPoint(cocos2d::Vec2(0, 1));
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
        list_1->setContentSize(cocos2d::Size(568, 430));
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
        list_2->setContentSize(cocos2d::Size(568, 430));
        list_2->setAnchorPoint(cocos2d::Vec2::ZERO);
        list_2->setPosition(cocos2d::Vec2(8, 0));
        list_2->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        list_2->setBackGroundColor(cocos2d::Color3B(42, 42, 42));
        list_2->setBounceEnabled(true);
        panel_5->addChild(list_2);

        // btn_list ScrollView 底部按钮栏
        const auto btn_list = ScrollView::create();
        btn_list->setContentSize(cocos2d::Size(1152, 105));
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
        btn_cell->setContentSize(cocos2d::Size(250, 105));
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
        btn->setTitleText("Button");
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

    static Widget *createMediaPlayerNavi() {
        LOGGER->warn("createMediaPlayerNaviLayer");
        return nullptr;
    }

    static Widget *createMediaPlayerBody() {
        LOGGER->warn("createMediaPlayerBodyLayer");
        return nullptr;
    }

    static Widget *createMediaPlayerFoot() {
        LOGGER->warn("createMediaPlayerFootLayer");
        return nullptr;
    }

    static Widget *createAllTips() {
        // 创建根节点
        const auto root = Widget::create();
        root->setContentSize(cocos2d::Size(960, 480));

        // 创建 ListView
        const auto listView = ListView::create();
        listView->setDirection(ScrollView::Direction::VERTICAL);
        listView->setBounceEnabled(true);
        listView->setTouchEnabled(true);
        listView->setContentSize(cocos2d::Size(960, 480));
        listView->setAnchorPoint(cocos2d::Vec2::ZERO);
        listView->setPosition(cocos2d::Vec2::ZERO);
        listView->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        listView->setBackGroundColor(cocos2d::Color3B(42, 42, 42));
        root->addChild(listView);

        // 创建 touchTip 面板
        const auto touchTip = Layout::create();
        touchTip->setContentSize(cocos2d::Size(960, 360));
        touchTip->setAnchorPoint(cocos2d::Vec2::ZERO);
        touchTip->setPosition(cocos2d::Vec2(0, 180));
        touchTip->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        touchTip->setBackGroundColor(cocos2d::Color3B(150, 200, 255));

        // 创建 FileNode_1 (这里用空节点代替)
        const auto fileNode1 = Widget::create();
        fileNode1->setContentSize(cocos2d::Size(960, 360));
        fileNode1->setAnchorPoint(cocos2d::Vec2::ZERO);
        fileNode1->setPosition(cocos2d::Vec2::ZERO);
        touchTip->addChild(fileNode1);

        // 创建 screenTip 面板
        const auto screenTip = Layout::create();
        screenTip->setContentSize(cocos2d::Size(960, 180));
        screenTip->setAnchorPoint(cocos2d::Vec2::ZERO);
        screenTip->setPosition(cocos2d::Vec2(0, 0));
        screenTip->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        screenTip->setBackGroundColor(cocos2d::Color3B(150, 200, 255));
        screenTip->setLocalZOrder(1);

        // 创建 FileNode_2 (这里用空节点代替)
        const auto fileNode2 = Widget::create();
        fileNode2->setContentSize(cocos2d::Size(960, 180));
        fileNode2->setAnchorPoint(cocos2d::Vec2::ZERO);
        fileNode2->setPosition(cocos2d::Vec2::ZERO);
        screenTip->addChild(fileNode2);

        // 将两个面板添加到 ListView
        listView->addChild(touchTip);
        listView->addChild(screenTip);

        // 创建关闭按钮
        const auto btnClose =
            Button::create("img/Cancel_Normal.png", "img/Cancel_Press.png",
                           "img/Cancel_Press.png");
        btnClose->setAnchorPoint(cocos2d::Vec2(1, 1));
        btnClose->setPosition(cocos2d::Vec2(960, 480));
        btnClose->setContentSize(cocos2d::Size(80, 80));
        btnClose->setTitleText("");
        btnClose->setTitleFontSize(14);
        btnClose->setTitleColor(cocos2d::Color3B(65, 65, 70));
        root->addChild(btnClose);

        return root;
    }

    static Widget *createWinMgrOverlay() {
        LOGGER->warn("createWinMgrOverlayLayer");
        return nullptr;
    }
    /**
     * 创建一个横向分割线 Widget
     * @param width      分割线实际长度
     * @param lineHeight 线粗（像素）
     * @param color      线颜色
     */
    static Widget *
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
