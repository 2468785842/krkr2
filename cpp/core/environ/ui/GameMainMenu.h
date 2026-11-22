#pragma once
#include "BaseForm.h"

class TVPGameMainMenu : public cocos2d::Node {
public:
    TVPGameMainMenu(GLubyte opa);
    static TVPGameMainMenu *create(GLubyte opa);

    bool init() override;

#if KRKR2_USE_COCOS_UI
    void setMouseIcon(bool bMouse);
#endif

    void shrink();
    void expand();
    void toggle();
    void shrinkWithTime(float dur);
    bool isShrinked();

private:
    bool onHandlerTouchBegan(cocos2d::Touch *touch,
                             cocos2d::Event *unusedEvent);
    void onHandlerTouchMoved(cocos2d::Touch *touch,
                             cocos2d::Event *unusedEvent);
    void onHandlerTouchEnded(cocos2d::Touch *touch,
                             cocos2d::Event *unusedEvent);
    void onHandlerTouchCancelled(cocos2d::Touch *touch,
                                 cocos2d::Event *unusedEvent);

    bool onBackgroundTouchBegan(cocos2d::Touch *touch,
                                cocos2d::Event *unusedEvent);
    void onBackgroundTouchMoved(cocos2d::Touch *touch,
                                cocos2d::Event *unusedEvent);
    void onBackgroundTouchEnded(cocos2d::Touch *touch,
                                cocos2d::Event *unusedEvent);
    void onBackgroundTouchCancelled(cocos2d::Touch *touch,
                                    cocos2d::Event *unusedEvent);

    GLubyte _handler_inactive_opacity;
    bool _hitted;
    bool _shrinked;
    bool _draggingX, _draggingY;

    cocos2d::Node *_root;
    cocos2d::Node *_handler;
#if KRKR2_USE_COCOS_UI
    cocos2d::Node *_icon_touch;
    cocos2d::Node *_icon_mouse;
#endif

    cocos2d::Vec2 _touchBeganPosition;
    cocos2d::Vec2 _touchMovePosition;
    cocos2d::Vec2 _touchEndPosition;

    unsigned int _touchBeganTime;
};
