#pragma once

#include "2d/CCNode.h"
#include <unordered_map>

#include "csd/CsdUIFactory.h"
#include "ui/UIWidget.h"
#include "extensions/GUI/CCScrollView/CCTableViewCell.h"

namespace cocos2d {
    class Ref;

    class Node;
    namespace ui {
        class Widget;

        class Button;

        class Text;

        class ListView;

        class CheckBox;

        class PageView;

        class TextField;

        class Slider;

        class ScrollView;

        class LoadingBar;
    } // namespace ui
    namespace extension {
        class TableView;
    }
} // namespace cocos2d

namespace cocostudio::timeline {
    class ActionTimeline;
}

class NodeMap : public std::unordered_map<std::string, cocos2d::Node *> {
protected:
    const char *FileName;

    void onLoadError(const std::string &name) const;

public:
    NodeMap();

    NodeMap(const char *filename, cocos2d::Node *node);

    template <typename T = cocos2d::Node>
    [[nodiscard]] T *findController(const std::string &name,
                                    bool notice = true) const {
        auto *node = findController<cocos2d::Node>(name, notice);
        if(node) {
            return dynamic_cast<T *>(node);
        }
        return nullptr;
    }

    [[nodiscard]] cocos2d::ui::Widget *findWidget(const std::string &name,
                                                  bool notice = true) const {
        return findController<cocos2d::ui::Widget>(name, notice);
    }

    void initFromNode(cocos2d::Node *node);
};

template <>
[[nodiscard]] cocos2d::Node *
NodeMap::findController<cocos2d::Node>(const std::string &name,
                                       bool notice) const;

class CSBReader : public NodeMap {
public:
    cocos2d::Node *Load(const char *filename);
};

class iTVPBaseForm : public cocos2d::Node {
public:
    iTVPBaseForm() : RootNode(nullptr) {}

    ~iTVPBaseForm() override;

    void Show();

    virtual void rearrangeLayout();

    virtual void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode,
                              cocos2d::Event *event);

protected:
    bool initFromFile(const Csd::NodeBuilderFn &naviBarCall,
                      const Csd::NodeBuilderFn &bodyCall,
                      const Csd::NodeBuilderFn &bottomBarCall,
                      Node *parent = nullptr);

    bool initFromFile(Node *naviBarCall, Node *bodyCall, Node *bottomBarCall,
                      Node *parent = nullptr) {
        return true;
    }

    bool initFromFile(Node *body) {
        return initFromFile(nullptr, body, nullptr);
    }

    bool initFromFile(const Csd::NodeBuilderFn &body) {
        return initFromFile(nullptr, body, nullptr);
    }

    // Screen Size 10%
    static cocos2d::Size rearrangeHeaderSize(const Node *parent) {
        const auto &pSize = parent->getContentSize();
        return { pSize.width, pSize.height * 0.1f };
    }

    // Screen Size 80%
    static cocos2d::Size rearrangeBodySize(const Node *parent) {
        const auto &pSize = parent->getContentSize();
        return { pSize.width, pSize.height * 0.8f };
    }

    // Screen Size 10%
    static cocos2d::Size rearrangeFooterSize(const Node *parent) {
        const auto &pSize = parent->getContentSize();
        return { pSize.width, pSize.height * 0.1f };
    }

    virtual void bindHeaderController(const Node *allNodes) = 0;
    virtual void bindBodyController(const Node *allNodes) = 0;
    virtual void bindFooterController(const Node *allNodes) = 0;

    cocos2d::ui::Widget *RootNode;

    struct {
        // cocos2d::ui::Button *Title;
        cocos2d::ui::Button *Left;
        cocos2d::ui::Button *Right;
        cocos2d::Node *Root;
    } NaviBar{};

    struct {
        // cocos2d::ui::ListView *Panel;
        cocos2d::Node *Root;
    } BottomBar{};
};

class TTouchEventRouter : public cocos2d::ui::Widget {
public:
    typedef std::function<void(cocos2d::ui::Widget::TouchEventType event,
                               cocos2d::ui::Widget *sender,
                               cocos2d::Touch *touch)>
        EventFunc;

    static TTouchEventRouter *create() {
        auto *ret = new TTouchEventRouter;
        ret->init();
        ret->autorelease();
        return ret;
    }

    void setEventFunc(const EventFunc &func) { _func = func; }

    void interceptTouchEvent(cocos2d::ui::Widget::TouchEventType event,
                             cocos2d::ui::Widget *sender,
                             cocos2d::Touch *touch) override {
        if(_func)
            _func(event, sender, touch);
    }

private:
    EventFunc _func;
};

class TCommonTableCell : public cocos2d::extension::TableViewCell {
    typedef cocos2d::extension::TableViewCell inherit;

protected: // must be inherited
    TCommonTableCell() : _router(nullptr) {}

public:
    ~TCommonTableCell() override {
        if(_router)
            _router->release();
    }

    void setContentSize(const cocos2d::Size &contentSize) override {
        inherit::setContentSize(contentSize);
        if(_router)
            _router->setContentSize(contentSize);
    }

    bool init() override {
        bool ret = inherit::init();
        _router = TTouchEventRouter::create();
        return ret;
    }

protected:
    TTouchEventRouter *_router;
};

class iTVPFloatForm : public iTVPBaseForm {
public:
    void rearrangeLayout() override;

    void bindHeaderController(const Node *allNodes) override {}
    void bindBodyController(const Node *allNodes) override {}
    void bindFooterController(const Node *allNodes) override {}
};

void ReloadTableViewAndKeepPos(cocos2d::extension::TableView *pTableView);
