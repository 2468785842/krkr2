#include "MainScene.h"

#include <cocos2d.h>
#include <cocos-ext.h>

#include "tjsCommHead.h"
#include "StorageIntf.h"
#include "WindowImpl.h"
#include "LayerBitmapImpl.h"
#include "ui/BaseForm.h"
#include "ui/GameMainMenu.h"
#include "ui/UIHelper.h"
#include "TickCount.h"
#include "UtilStreams.h"
#include "vkdefine.h"
#include "ConfigManager/IndividualConfigManager.h"
#include "Platform.h"
#include "ui/ConsoleWindow.h"
#include "ui/FileSelectorForm.h"
#include "ui/DebugViewLayerForm.h"
#include "Application.h"
#include "ScriptMgnIntf.h"
#include "impl/TVPWindow.h"
#include "RenderManager.h"
#include "ui/UIButton.h"
#include "ui/csd/CsdUIFactory.h"

#include "CCKeyCodeConv.h"
#include "RenderUtils.h"

USING_NS_CC;

enum SCENE_ORDER {
    GAME_SCENE_ORDER,
    GAME_CONSOLE_ORDER,
    GAME_WINMGR_ORDER, // also for the virtual mouse cursor
    GAME_MENU_ORDER,
    UI_NODE_ORDER,
};

const float UI_CHANGE_DURATION = 0.3f;

class TVPWindowLayer;

class TVPWindowManagerOverlay;

static float _mouseCursorScale;
static cocos2d::Vec2 _mouseTouchPoint, _mouseBeginPoint;
static std::set<cocos2d::Touch *> _mouseTouches;
static bool _mouseMoved, _mouseClickedDown;

static tjs_uint16 _keymap[0x200];
static cocos2d::Label *_fpsLabel = nullptr;

#ifndef GL_UNPACK_ROW_LENGTH
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#endif

struct tTVPCursor {
    Node *RootNode;
    Action *Anim = nullptr;
};

#pragma pack(push)
#pragma pack(1)
enum eIconType { eIconTypeNone, eIconTypeICO, eIconTypeCUR };

struct ICONDIR {
    uint16_t idReserved; // must be 0
    uint16_t idType; // eIconType
    uint16_t idCount;
};

struct ICODIREntry {
    uint8_t bWidth; // 0 -> 256
    uint8_t bHeight; // 0 -> 256
    uint8_t bColorCount;
    uint8_t bReserved;
    union {
        struct {
            uint16_t wPlanes;
            uint16_t wBitCount;
        };
        struct {
            uint16_t wHotSpotX;
            uint16_t wHotSpotY;
        };
    };
    uint32_t dwBytesInRes;
    uint32_t dwImageOffset;
};
#pragma pack(pop)

Sprite *TVPLoadCursorCUR(tTJSBinaryStream *pStream) {
    ICONDIR header;
    pStream->ReadBuffer(&header, sizeof(header));
    if((header.idReserved != 0) || (header.idType != eIconTypeCUR) ||
       (header.idCount == 0)) {
        return nullptr;
    }

    std::vector<ICODIREntry> cur_dir;
    cur_dir.resize(header.idCount);
    pStream->ReadBuffer(&cur_dir[0], sizeof(ICODIREntry) * header.idCount);
    ICODIREntry bestentry = { 0 };
    for(auto entry : cur_dir) {
        if(entry.bHeight > bestentry.bHeight)
            bestentry = entry;
    }
    cur_dir.clear();
    cur_dir.emplace_back(bestentry);

    for(int i = 0; i < cur_dir.size(); ++i) {
        const ICODIREntry &entry = cur_dir[i];
        int bWidth = entry.bWidth;
        int bHeight = entry.bHeight;
        int bColorCount = entry.bColorCount;
        if(!bWidth)
            bWidth = 256;
        if(!bHeight)
            bHeight = 256;
        if(!bColorCount)
            bColorCount = 256;

        pStream->SetPosition(entry.dwImageOffset);
        TVPBITMAPINFOHEADER bmhdr;
        pStream->ReadBuffer(&bmhdr, sizeof(bmhdr));
        if(bmhdr.biSize != 40)
            continue;
        if(bmhdr.biCompression)
            continue;
        int bmpPitch, pad;
        switch(bmhdr.biBitCount) {
            case 1:
                bmpPitch = (bmhdr.biWidth + 7) >> 3;
                pad = (((bmpPitch) % 4) ? (4 - ((bmpPitch) % 4)) : 0);
                break;
            case 4:
                bmpPitch = (bmhdr.biWidth + 1) >> 1;
                pad = (((bmpPitch) % 4) ? (4 - ((bmpPitch) % 4)) : 0);
                break;
            case 8:
                bmpPitch = bmhdr.biWidth;
                pad = (((bmpPitch) % 4) ? (4 - ((bmpPitch) % 4)) : 0);
                break;
            case 32:
                break;
            default:
                continue;
        }
        bmhdr.biHeight /= 2;
        std::vector<unsigned char> pixbuf;
        pixbuf.resize(bmhdr.biWidth * bmhdr.biHeight * 4);
        tjs_uint32 palette[256];
        if(bmhdr.biBitCount <= 8) {
            if(bmhdr.biClrUsed == 0)
                bmhdr.biClrUsed = 1 << bmhdr.biBitCount;
            for(unsigned int j = 0; j < bmhdr.biClrUsed; ++j) {
                union {
                    tjs_uint32 u32;
                    tjs_uint8 u8[4];
                } clr{}, rclr{};
                pStream->ReadBuffer(&clr.u32, 4);
                rclr.u8[0] = clr.u8[2];
                rclr.u8[1] = clr.u8[1];
                rclr.u8[2] = clr.u8[0];
                rclr.u8[3] = clr.u8[3];
                palette[j] = rclr.u32;
            }
        }
        /* Read the surface pixels.  Note that the bmp image is upside
         * down */
        int pitch = bmhdr.biWidth * 4;
        tjs_uint8 *bits = (tjs_uint8 *)&pixbuf[0] + (bmhdr.biHeight * pitch);
        switch(bmhdr.biBitCount) {
            case 1:
            case 4:
            case 8:
                while(bits > (tjs_uint8 *)&pixbuf[0]) {
                    bits -= pitch;
                    tjs_uint8 pixel = 0;
                    int shift = (8 - bmhdr.biBitCount);
                    for(i = 0; i < bmhdr.biWidth; ++i) {
                        if(i % (8 / bmhdr.biBitCount) == 0) {
                            pStream->ReadBuffer(&pixel, 1);
                        }
                        *((tjs_uint32 *)bits + i) = (palette[pixel >> shift]);
                        pixel <<= bmhdr.biBitCount;
                    }
                }
                /* Skip padding bytes, ugh */
                if(pad) {
                    tjs_uint8 padbyte;
                    for(i = 0; i < pad; ++i) {
                        pStream->ReadBuffer(&padbyte, 1);
                    }
                }
                break;
            case 32:
                bmpPitch = bmhdr.biWidth * 4;
                pad = 0;
                while(bits > (tjs_uint8 *)&pixbuf[0]) {
                    bits -= pitch;
                    pStream->ReadBuffer(bits, pitch);
                }
                break;
            default:;
        }

        /* Read the mask pixels.  Note that the bmp image is upside
         * down */
        bits = (tjs_uint8 *)&pixbuf[0] + (bmhdr.biHeight * pitch);
        bmpPitch = (bmhdr.biWidth + 7) >> 3;
        pad = (((bmpPitch) % 4) ? (4 - ((bmpPitch) % 4)) : 0);
        while(bits > (tjs_uint8 *)&pixbuf[0]) {
            tjs_uint8 pixel = 0;
            int shift = (8 - 1);
            bits -= pitch;
            for(i = 0; i < bmhdr.biWidth; ++i) {
                if(i % (8 / 1) == 0) {
                    pStream->ReadBuffer(&pixel, 1);
                }
                *((tjs_uint32 *)bits + i) |=
                    ((pixel >> shift) ? 0 : 0xFF000000);
                pixel <<= 1;
            }
            /* Skip padding bytes, ugh */
            if(pad) {
                tjs_uint8 padbyte;
                for(i = 0; i < pad; ++i) {
                    pStream->ReadBuffer(&padbyte, 1);
                }
            }
        }
        cocos2d::Image *surface = new cocos2d::Image;
        surface->initWithRawData(&pixbuf[0], pixbuf.size(), bmhdr.biWidth,
                                 bmhdr.biHeight,
                                 (int)Texture2D::PixelFormat::RGBA8888, false);
        Texture2D *tex = new Texture2D();
        tex->initWithImage(surface);
        Sprite *sprite = Sprite::create();
        sprite->setTexture(tex);
        sprite->setTextureRect(
            cocos2d::Rect(0, 0, bmhdr.biWidth, bmhdr.biHeight));
        sprite->setAnchorPoint(
            Vec2((float)entry.wHotSpotX / bmhdr.biWidth,
                 1.f - (float)entry.wHotSpotY / bmhdr.biHeight));
        float scale = sqrtf(Device::getDPI() / 150.f);
        sprite->setScale(scale);
        return sprite; // only the first one
    }
    return nullptr;
}

tTVPCursor *TVPLoadCursorANI(tTJSBinaryStream *pStream) {
    // TODO http://www.gdgsoft.com/anituner/help/aniformat.htm
    return nullptr;
}

tTVPCursor *TVPLoadCursor(tTJSBinaryStream *stream) {
    if(!stream)
        return nullptr;
    unsigned char sig[4];
    stream->Read(sig, 4);
    stream->SetPosition(0);
    if(memcmp(sig, "RIFF", 4) == 0) { // ani format
        return TVPLoadCursorANI(stream);
    } else { // cur format
        Sprite *cur = TVPLoadCursorCUR(stream);
        tTVPCursor *ret = nullptr;
        if(cur) {
            ret = new tTVPCursor;
            ret->RootNode = cur;
            cur->retain();
        }
        return ret;
    }
}

class TVPWindowManagerOverlay : public iTVPBaseForm {
public:
    static TVPWindowManagerOverlay *create() {
        auto *ret = new TVPWindowManagerOverlay();
        ret->autorelease();
        ret->initFromFile(nullptr, Csd::createWinMgrOverlay(), nullptr);
        return ret;
    }

    void rearrangeLayout() override {
        cocos2d::Size sceneSize =
            TVPMainScene::GetInstance()->getGameNodeSize();
        setContentSize(sceneSize);
        RootNode->setContentSize(sceneSize);
        ui::Helper::doLayout(RootNode);
    }

    void bindHeaderController(const Node *allNodes) override {}

    void bindBodyController(const Node *allNodes) override {
        _left = allNodes->getChildByName<ui::Button *>("left");
        _right = allNodes->getChildByName<ui::Button *>("right");
        _ok = allNodes->getChildByName<ui::Button *>("ok");

        auto funcUpdate = [this] { updateButtons(); };

        _left->addClickEventListener([=](Ref *) {
            if(!TVPWindowLayer::_currentWindowLayer ||
               !TVPWindowLayer::_currentWindowLayer->_prevWindow)
                return;
            // TVPWindowLayer::_currentWindowLayer->_prevWindow->setVisible(true);
            cocos2d::Size size =
                TVPWindowLayer::_currentWindowLayer->_prevWindow->getViewSize();
            TVPWindowLayer::_currentWindowLayer->_prevWindow->setPosition(
                -size.width, 0);
            TVPWindowLayer::_currentWindowLayer->_prevWindow->runAction(
                EaseQuadraticActionOut::create(
                    MoveTo::create(UI_CHANGE_DURATION, Vec2::ZERO)));
            TVPWindowLayer::_currentWindowLayer->runAction(
                Sequence::createWithTwoActions(
                    EaseQuadraticActionOut::create(MoveTo::create(
                        UI_CHANGE_DURATION, Vec2(size.width, 0))),
                    Sequence::createWithTwoActions(
                        Hide::create(), CallFunc::create(funcUpdate))));
            TVPWindowLayer::_currentWindowLayer =
                TVPWindowLayer::_currentWindowLayer->_prevWindow;
            _left->setVisible(false);
            _right->setVisible(false);
        });

        _right->addClickEventListener([=](Ref *) {
            if(!TVPWindowLayer::_currentWindowLayer ||
               !TVPWindowLayer::_currentWindowLayer->_nextWindow)
                return;
            // TVPWindowLayer::_currentWindowLayer->_nextWindow->setVisible(true);
            cocos2d::Size size =
                TVPWindowLayer::_currentWindowLayer->_nextWindow->getViewSize();
            TVPWindowLayer::_currentWindowLayer->_nextWindow->setPosition(
                size.width, 0);
            TVPWindowLayer::_currentWindowLayer->_nextWindow->runAction(
                EaseQuadraticActionOut::create(
                    MoveTo::create(UI_CHANGE_DURATION, Vec2::ZERO)));
            TVPWindowLayer::_currentWindowLayer->runAction(
                Sequence::createWithTwoActions(
                    EaseQuadraticActionOut::create(MoveTo::create(
                        UI_CHANGE_DURATION, Vec2(-size.width, 0))),
                    Sequence::createWithTwoActions(
                        Hide::create(), CallFunc::create(funcUpdate))));
            TVPWindowLayer::_currentWindowLayer =
                TVPWindowLayer::_currentWindowLayer->_nextWindow;
            _left->setVisible(false);
            _right->setVisible(false);
        });

        _ok->addClickEventListener([](Ref *) {
            TVPMainScene::GetInstance()->showWindowManagerOverlay(false);
        });

        ui::Button *fillscr =
            (allNodes->getChildByName<cocos2d::ui::Button *>("fillscr"));
        fillscr->addClickEventListener([](Ref *) {
            if(!TVPWindowLayer::_currentWindowLayer)
                return;
            TVPWindowLayer::_currentWindowLayer->toggleFillScale();
        });

        updateButtons();
    }

    void bindFooterController(const Node *allNodes) override {}

    void updateButtons() const {
        if(!TVPWindowLayer::_currentWindowLayer)
            return;
        if(_left) {
            TVPWindowLayer *pLay =
                TVPWindowLayer::_currentWindowLayer->_prevWindow;
            while(pLay && !pLay->Visible) {
                pLay = pLay->_prevWindow;
            }
            _left->setVisible(pLay && pLay->Visible);
        }
        if(_right) {
            TVPWindowLayer *pLay =
                TVPWindowLayer::_currentWindowLayer->_nextWindow;
            while(pLay && !pLay->Visible) {
                pLay = pLay->_nextWindow;
            }
            _right->setVisible(pLay && pLay->Visible);
        }
    }

private:
    ui::Button *_left, *_right, *_ok;
};

static std::function<bool(Touch *, Event *)> _func_mask_layer_touchbegan;

class MaskLayer : public LayerColor {
public:
    static LayerColor *create(const Color4B &color, GLfloat width,
                              GLfloat height) {
        LayerColor *layer = LayerColor::create(color, width, height);
        auto listener = EventListenerTouchOneByOne::create();
        listener->setSwallowTouches(false);
        if(_func_mask_layer_touchbegan) {
            listener->onTouchBegan = _func_mask_layer_touchbegan;
            _func_mask_layer_touchbegan = nullptr;
        } else {
            listener->onTouchBegan = [](Touch *, Event *) -> bool {
                return true;
            };
        }
        listener->onTouchMoved = [](Touch *, Event *) {};
        listener->onTouchEnded = [](Touch *, Event *) {};
        listener->onTouchCancelled = [](Touch *, Event *) {};

        Director::getInstance()
            ->getEventDispatcher()
            ->addEventListenerWithSceneGraphPriority(listener, layer);

        return layer;
    }
};

static TVPMainScene *_instance = nullptr;

TVPMainScene *TVPMainScene::GetInstance() { return _instance; }

TVPMainScene *TVPMainScene::CreateInstance() {
    _instance = create();
    return _instance;
}

void TVPMainScene::initialize() {
    auto glview = cocos2d::Director::getInstance()->getOpenGLView();
    cocos2d::Size screenSize = glview->getFrameSize();
    cocos2d::Size designSize = glview->getDesignResolutionSize();
    ScreenRatio = screenSize.height / designSize.height;
    designSize.width = designSize.height * screenSize.width / screenSize.height;
    initWithSize(designSize);
    addChild(LayerColor::create(Color4B::BLACK, designSize.width,
                                designSize.height));
    GameNode = cocos2d::Node::create();
    // horizontal
    // std::swap(designSize.width, designSize.height);
    GameNode->setContentSize(designSize);
    UINode = cocos2d::Node::create();
    UINode->setContentSize(designSize);
    UINode->setAnchorPoint(Vec2::ZERO);
    UINode->setPosition(Vec2::ZERO);
    UISize = designSize;
    addChild(UINode, UI_NODE_ORDER);
    GameNode->setAnchorPoint(Vec2(0, 0));
    // GameNode->setRotation(-90);
    // GameNode->setPosition(getContentSize() / 2);
    addChild(GameNode, GAME_SCENE_ORDER);

    EventListenerKeyboard *keylistener = EventListenerKeyboard::create();
    keylistener->onKeyPressed = CC_CALLBACK_2(TVPMainScene::onKeyPressed, this);
    keylistener->onKeyReleased =
        CC_CALLBACK_2(TVPMainScene::onKeyReleased, this);
    _eventDispatcher->addEventListenerWithFixedPriority(keylistener, 1);

    _touchListener = EventListenerTouchOneByOne::create();
    _touchListener->onTouchBegan =
        CC_CALLBACK_2(TVPMainScene::onTouchBegan, this);
    _touchListener->onTouchMoved =
        CC_CALLBACK_2(TVPMainScene::onTouchMoved, this);
    _touchListener->onTouchEnded =
        CC_CALLBACK_2(TVPMainScene::onTouchEnded, this);
    _touchListener->onTouchCancelled =
        CC_CALLBACK_2(TVPMainScene::onTouchCancelled, this);

    _eventDispatcher->addEventListenerWithSceneGraphPriority(_touchListener,
                                                             this);

    EventListenerController *ctrllistener = EventListenerController::create();
    ctrllistener->onAxisEvent = CC_CALLBACK_3(TVPMainScene::onAxisEvent, this);
    ctrllistener->onKeyDown = CC_CALLBACK_3(TVPMainScene::onPadKeyDown, this);
    ctrllistener->onKeyUp = CC_CALLBACK_3(TVPMainScene::onPadKeyUp, this);
    ctrllistener->onKeyRepeat =
        CC_CALLBACK_3(TVPMainScene::onPadKeyRepeat, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(ctrllistener,
                                                             this);
    cocos2d::Controller::startDiscoveryController(); // for win32 &
                                                     // iOS
}

TVPMainScene *TVPMainScene::create() {
    TVPMainScene *ret = new TVPMainScene;
    ret->initialize();
    ret->autorelease();

    TVPWindowLayer::_touchMoveThresholdSq = cocos2d::Device::getDPI() / 10.0f;
    TVPWindowLayer::_touchMoveThresholdSq *=
        TVPWindowLayer::_touchMoveThresholdSq;
    return ret;
}

void TVPMainScene::pushUIForm(cocos2d::Node *ui, eEnterAni ani) {
#if defined(TVP_DEBUG) || defined(TVP_DEBUG_UI) || defined(_DEBUG)
    CCLOG("TVPMainScene::pushUIForm: ui=%p, ani=%d", ui, ani);
    CCLOG("ui initial pos: %f, %f", ui->getPosition().x, ui->getPosition().y);
    CCLOG("ui contentSize: %f x %f", ui->getContentSize().width,
          ui->getContentSize().height);
#endif
    if(!ui) {
        CCLOGERROR("TVPMainScene::pushUIForm: ui is nullptr");
        return;
    }
    TVPControlAdDialog(0x10002, 1, 0);
    int n = UINode->getChildrenCount();
    if(ani == eEnterAniNone) {
        UINode->addChild(ui);
    } else if(ani == eEnterAniOverFromRight) {
        if(n > 0) {
            cocos2d::Size size = UINode->getContentSize();
            cocos2d::Node *lastui = UINode->getChildren().back();
            lastui->runAction(EaseQuadraticActionOut::create(
                MoveTo::create(UI_CHANGE_DURATION, Vec2(size.width / -5, 0))));
            cocos2d::Node *ColorMask =
                MaskLayer::create(Color4B(0, 0, 0, 0), size.width, size.height);
            ColorMask->setPosition(Vec2(-size.width, 0));
            ui->addChild(ColorMask);
            ColorMask->runAction(FadeTo::create(UI_CHANGE_DURATION, 128));
            ui->setPosition(size.width, 0);
            ui->runAction(EaseQuadraticActionOut::create(
                MoveTo::create(UI_CHANGE_DURATION, Vec2::ZERO)));
            runAction(Sequence::createWithTwoActions(
                DelayTime::create(UI_CHANGE_DURATION),
                CallFunc::create([=]() { ColorMask->removeFromParent(); })));
        }
        UINode->addChild(ui);
    } else if(ani == eEnterFromBottom) {
        cocos2d::Size size = UINode->getContentSize();
        cocos2d::Node *ColorMask =
            MaskLayer::create(Color4B(0, 0, 0, 0), size.width, size.height);
        ColorMask->runAction(FadeTo::create(UI_CHANGE_DURATION, 128));
        ui->setPositionY(-ui->getContentSize().height);
        ColorMask->addChild(ui);
        UINode->addChild(ColorMask);
        ui->runAction(EaseQuadraticActionOut::create(
            MoveTo::create(UI_CHANGE_DURATION, Vec2::ZERO)));
    }
}

void TVPMainScene::popUIForm(cocos2d::Node *form, eLeaveAni ani) {
    int n = UINode->getChildrenCount();
    if(n <= 0)
        return;
    if(n == 1) {
        TVPControlAdDialog(0x10002, 0, 0);
    }
    auto children = UINode->getChildren();
    if(ani == eLeaveAniNone) {
        if(n > 1) {
            Node *lastui = children.at(n - 2);
            lastui->setPosition(0, 0);
        }
        Node *ui = children.back();
        if(form)
            CCAssert(form == ui, "must be the same form");
        ui->removeFromParent();
    } else if(ani == eLeaveAniLeaveFromLeft) {
        Node *ui = children.back();
        if(form)
            CCAssert(form == ui, "must be the same form");
        cocos2d::Size size = UINode->getContentSize();
        if(n > 1) {
            Node *lastui = children.at(n - 2);
            lastui->setPosition(size.width / -5, 0);
            lastui->runAction(EaseQuadraticActionOut::create(
                MoveTo::create(UI_CHANGE_DURATION, Vec2::ZERO)));
        }
        cocos2d::Node *ColorMask =
            MaskLayer::create(Color4B(0, 0, 0, 128), size.width, size.height);
        ColorMask->setPosition(Vec2(-size.width, 0));
        ui->addChild(ColorMask);
        ColorMask->runAction(FadeOut::create(UI_CHANGE_DURATION));
        ui->runAction(EaseQuadraticActionOut::create(
            MoveTo::create(UI_CHANGE_DURATION, Vec2(size.width, 0))));
        this->runAction(Sequence::createWithTwoActions(
            DelayTime::create(UI_CHANGE_DURATION),
            CallFunc::create([ui] { ui->removeFromParent(); })));
    } else if(ani == eLeaveToBottom) {
        cocos2d::Node *ColorMask = children.back();
        ColorMask->runAction(FadeOut::create(UI_CHANGE_DURATION));
        Node *ui = ColorMask->getChildren().at(0);
        if(form)
            CCAssert(form == ui, "must be the same form");
        ui->runAction(EaseQuadraticActionIn::create(MoveTo::create(
            UI_CHANGE_DURATION, Vec2(0, -ui->getContentSize().height))));
        runAction(Sequence::createWithTwoActions(
            DelayTime::create(UI_CHANGE_DURATION),
            CallFunc::create([=]() { ColorMask->removeFromParent(); })));
    }
}

bool TVPMainScene::startupFrom(const std::string &path) {
    // startup from dir
    if(!TVPCheckStartupPath(path)) {
        return false;
    }

    IndividualConfigManager *pGlobalCfgMgr =
        IndividualConfigManager::GetInstance();
    pGlobalCfgMgr->UsePreferenceAt(
        TVPBaseFileSelectorForm::pathSplit(path).first);
    if(UINode->getChildrenCount()) {
        popUIForm(nullptr);
    }

    if(GlobalConfigManager::GetInstance()->GetValue<bool>("keep_screen_alive",
                                                          true)) {
        Device::setKeepScreenOn(true);
    }

    for(int i = 0; i < std::size(_keymap); ++i) {
        _keymap[i] = i;
    }

    const auto &keymap = pGlobalCfgMgr->GetKeyMap();
    for(const auto &it : keymap) {
        if(!it.second)
            continue;
        _keymap[it.first] = _keymap[it.second];
    }
    // 	if (pGlobalCfgMgr->GetValueBool("rot_screen_180", false)) {
    // 		GameNode->setRotation(90);
    // 	}

    scheduleOnce([this, path](float delay) { doStartup(delay, path); }, 0,
                 "startup");

    return true;
}

void TVPMainScene::doStartup(float dt, std::string path) {
    unschedule("startup");
    IndividualConfigManager *pGlobalCfgMgr =
        IndividualConfigManager::GetInstance();
    TVPWindowLayer::_consoleWin = TVPConsoleWindow::create(24, nullptr);

    auto glview = cocos2d::Director::getInstance()->getOpenGLView();
    cocos2d::Size screenSize = glview->getFrameSize();
    float scale = screenSize.height / getContentSize().height;
    TVPWindowLayer::_consoleWin->setScale(1 / scale);
    TVPWindowLayer::_consoleWin->setContentSize(getContentSize() * scale);
    GameNode->addChild(TVPWindowLayer::_consoleWin, GAME_CONSOLE_ORDER);
    ::Application->StartApplication(path);
    // update one frame
    update(0);
    //_ResotreGLStatues(); // already in update()
    GLubyte handlerOpacity =
        pGlobalCfgMgr->GetValue<float>("menu_handler_opa", 0.15f) * 255;
    _gameMenu = TVPGameMainMenu::create(handlerOpacity);
    GameNode->addChild(_gameMenu, GAME_MENU_ORDER);
    _gameMenu->shrinkWithTime(1);
    if(TVPWindowLayer::_consoleWin) {
        TVPWindowLayer::_consoleWin->removeFromParent();
        TVPWindowLayer::_consoleWin = nullptr;
        scheduleUpdate();

        cocos2d::Director::getInstance()->purgeCachedData();
        TVPControlAdDialog(0x10002, 0,
                           0); // ensure to close banner ad
    }

    TVPWindowLayer *pWin = TVPWindowLayer::_lastWindowLayer;
    while(pWin) {
        pWin->setVisible(true);
        pWin = pWin->_prevWindow;
    }

    if(pGlobalCfgMgr->GetValue<bool>("showfps", false)) {
        _fpsLabel =
            cocos2d::Label::createWithTTF("", "NotoSansCJK-Regular.ttc", 16);
        _fpsLabel->setAnchorPoint(Vec2(0, 1));
        _fpsLabel->setPosition(Vec2(0, GameNode->getContentSize().height));
        _fpsLabel->setColor(Color3B::WHITE);
        _fpsLabel->enableOutline(Color4B::BLACK, 1);
        GameNode->addChild(_fpsLabel, GAME_MENU_ORDER);
    }
    int fps = pGlobalCfgMgr->GetValue<int>("fps_limit", 60);
    cocos2d::Director::getInstance()->setAnimationInterval(1.0f / fps);
}

extern ttstr TVPGetErrorDialogTitle();

void TVPOnError();

tjs_uint TVPGetGraphicCacheTotalBytes();

void TVPMainScene::update(float delta) {
    ::Application->Run();
    //	if (TVPWindowLayer::_currentWindowLayer)
    //TVPWindowLayer::_currentWindowLayer->UpdateOverlay();
    iTVPTexture2D::RecycleProcess();
    //_ResotreGLStatues();
    if(_postUpdate)
        _postUpdate();
    if(_fpsLabel) {
        unsigned int drawCount;
        uint64_t vmemsize;
        TVPGetRenderManager()->GetRenderStat(drawCount, vmemsize);
        static timeval _lastUpdate;
        // static int _lastUpdateReq = gettimeofday(&_lastUpdate,
        // nullptr);
        struct timeval now;
        gettimeofday(&now, nullptr);
        float _deltaTime = (now.tv_sec - _lastUpdate.tv_sec) +
            (now.tv_usec - _lastUpdate.tv_usec) / 1000000.0f;
        _lastUpdate = now;

        static float prevDeltaTime = 0.016f; // 60FPS
        static const float FPS_FILTER = 0.10f;
        static float _accumDt = 0;
        static unsigned int prevDrawCount = 0;
        _accumDt += _deltaTime;
        float dt = _deltaTime * FPS_FILTER + (1 - FPS_FILTER) * prevDeltaTime;
        prevDeltaTime = dt;
        if(drawCount > prevDrawCount)
            prevDrawCount = drawCount;

        char buffer[30];
        if(_accumDt > 0.1f) {
            sprintf(buffer, "%.1f (%d draws)", 1 / dt, drawCount);
            _fpsLabel->setString(buffer);
            // #ifdef _MSC_VER
            std::string msg = buffer;
            msg += "\n";
            // sprintf(buffer, "%.2f MB",
            // TVPGetGraphicCacheTotalBytes() / (float)(1024 * 1024));
            vmemsize >>= 10;
            sprintf(buffer, "%d MB(%.2f MB) %d MB\n", TVPGetSelfUsedMemory(),
                    (float)vmemsize / 1024.f, TVPGetSystemFreeMemory());
            msg += buffer;
            _fpsLabel->setString(msg);
            // #endif
            _accumDt = 0;
            prevDrawCount = 0;
        }
    }
}

cocos2d::Size TVPMainScene::getUINodeSize() { return UINode->getContentSize(); }

void TVPMainScene::addLayer(TVPWindowLayer *lay) {
    GameNode->addChild(lay, GAME_SCENE_ORDER);
    lay->setViewSize(GameNode->getContentSize());
    lay->setContentSize(lay->getViewSize());
    // 	if (TVPWindowLayer::_currentWindowLayer) {
    // 		TVPWindowLayer::_currentWindowLayer->setVisible(false);
    // 	}
    // 	TVPWindowLayer::_currentWindowLayer = lay;
}

void TVPMainScene::rotateUI() {
    float rot = UINode->getRotation();
    if(rot < 1) {
        UINode->setRotation(90);
        UINode->setContentSize(cocos2d::Size{ UISize.height, UISize.width });
    } else {
        UINode->setRotation(0);
        UINode->setContentSize(UISize);
    }
    for(Node *ui : UINode->getChildren()) {
        static_cast<iTVPBaseForm *>(ui)->rearrangeLayout();
    }
}

void TVPMainScene::setMaskLayTouchBegain(
    const std::function<bool(cocos2d::Touch *, cocos2d::Event *)> &func) {
    _func_mask_layer_touchbegan = func;
}

static float _getUIScale() {
    auto glview = Director::getInstance()->getOpenGLView();
    float factor = (glview->getScaleX() + glview->getScaleY()) / 2;
    factor /= Device::getDPI(); // inch per pixel
    cocos2d::Size screenSize = glview->getFrameSize();
    cocos2d::Size designSize = glview->getDesignResolutionSize();
    designSize.width = designSize.height * screenSize.width / screenSize.height;
    screenSize.width = factor * designSize.width;
#ifdef _WIN32
    // if (screenSize.width > 3.5433) return 0.35f; // 7 inch @ 16:9
    // device
    return 0.35f;
#endif
    // 	char tmp[128];
    // 	sprintf(tmp, "screenSize.width = %f",
    // (float)screenSize.width); 	TVPPrintLog(tmp); #if CC_PLATFORM_IOS
    // == CC_TARGET_PLATFORM 	return /*sqrtf*/(0.0005f / factor) *
    // screenSize.width; #else
    return /*sqrtf*/ (0.0005f / factor) * screenSize.width;
    // #endif
}

float TVPMainScene::getUIScale() {
    static float uiscale = _getUIScale();
    return uiscale;
}

void TVPMainScene::onKeyPressed(EventKeyboard::KeyCode keyCode, Event *event) {
    Vector<Node *> &uiChild = UINode->getChildren();
    if(!uiChild.empty()) {
        iTVPBaseForm *uiform = static_cast<iTVPBaseForm *>(uiChild.back());
        if(uiform)
            uiform->onKeyPressed(keyCode, event);
        return;
    }
    switch(keyCode) {
        case EventKeyboard::KeyCode::KEY_MENU:
            if(UINode->getChildren().empty()) {
                if(_gameMenu)
                    _gameMenu->toggle();
            }
            return;
            break;
        case EventKeyboard::KeyCode::KEY_BACK:
            if(!UINode->getChildren().empty()) {
                return;
            }
            if(_gameMenu && !_gameMenu->isShrinked()) {
                _gameMenu->shrink();
                return;
            }
            keyCode = EventKeyboard::KeyCode::KEY_ESCAPE;
            break;
#ifdef _DEBUG
        case EventKeyboard::KeyCode::KEY_PAUSE:
            GameNode->addChild(DebugViewLayerForm::create());
            return;
        case EventKeyboard::KeyCode::KEY_F12:
            if(TVPGetCurrentShiftKeyState() & ssShift) {
                std::vector<ttstr> btns({ "OK", "Cancel" });
                ttstr text;
                tTJSVariant result;
                if(TVPShowSimpleInputBox(text, "console command", "", btns) ==
                   0) {
                    try {
                        TVPExecuteExpression(text, &result);
                    } catch(...) {
                        ;
                    }
                }
                result = text;
            }
            break;
#endif
        default:
            break;
    }
    unsigned int code = TVPConvertKeyCodeToVKCode(keyCode);
    if(!code || code >= 0x200)
        return;
    code = _keymap[code];

    TVPWindowLayer::_scancode[code] = 0x11;
    if(TVPWindowLayer::_currentWindowLayer) {
        TVPWindowLayer::_currentWindowLayer->InternalKeyDown(
            code, TVPGetCurrentShiftKeyState());
    }
}

void TVPMainScene::onKeyReleased(EventKeyboard::KeyCode keyCode, Event *event) {
#ifdef _DEBUG
    if(keyCode == EventKeyboard::KeyCode::KEY_PAUSE)
        return;
#endif
    if(keyCode == EventKeyboard::KeyCode::KEY_MENU)
        return;
    if(keyCode == EventKeyboard::KeyCode::KEY_BACK)
        keyCode = EventKeyboard::KeyCode::KEY_ESCAPE;
    if(keyCode == EventKeyboard::KeyCode::KEY_PLAY) { // auto play
        iTJSDispatch2 *global = TVPGetScriptDispatch();
        tTJSVariant var;
        if(global->PropGet(0, TJS_W("kag"), nullptr, &var, global) ==
               TJS_S_OK &&
           var.Type() == tvtObject) {
            iTJSDispatch2 *kag = var.AsObjectNoAddRef();
            if(kag->PropGet(0, TJS_W("autoMode"), nullptr, &var, kag) ==
               TJS_S_OK) {
                if(var.operator bool()) {
                    if(kag->PropGet(0, TJS_W("cancelAutoMode"), nullptr, &var,
                                    kag) == TJS_S_OK &&
                       var.Type() == tvtObject) {
                        iTJSDispatch2 *fn = var.AsObjectNoAddRef();
                        if(fn->IsInstanceOf(0, 0, 0, TJS_W("Function"), fn)) {
                            tTJSVariant *args = nullptr;
                            fn->FuncCall(0, nullptr, nullptr, nullptr, 0, &args,
                                         kag);
                            return;
                        }
                    }
                } else {
                    if(kag->PropGet(0, TJS_W("enterAutoMode"), nullptr, &var,
                                    kag) == TJS_S_OK &&
                       var.Type() == tvtObject) {
                        iTJSDispatch2 *fn = var.AsObjectNoAddRef();
                        if(fn->IsInstanceOf(0, 0, 0, TJS_W("Function"), fn)) {
                            tTJSVariant *args = nullptr;
                            fn->FuncCall(0, nullptr, nullptr, nullptr, 0, &args,
                                         kag);
                            return;
                        }
                    }
                }
            }
        }
        global->Release();
    }
    unsigned int code = TVPConvertKeyCodeToVKCode(keyCode);
    if(!code || code >= 0x200)
        return;
    code = _keymap[code];
    bool isPressed = TVPWindowLayer::_scancode[code] & 1;
    TVPWindowLayer::_scancode[code] &= 0x10;
    if(isPressed && TVPWindowLayer::TVPWindowLayer::_currentWindowLayer) {
        TVPWindowLayer::TVPWindowLayer::_currentWindowLayer->OnKeyUp(
            code, TVPGetCurrentShiftKeyState());
    }
}

TVPMainScene::TVPMainScene() {
    _gameMenu = nullptr;
    TVPWindowLayer::_windowMgrOverlay = nullptr;
}

void TVPMainScene::showWindowManagerOverlay(bool bVisible) {
    if(bVisible) {
        if(!TVPWindowLayer::_windowMgrOverlay) {
            if(TVPWindowLayer::TVPWindowLayer::_currentWindowLayer &&
               TVPWindowLayer::TVPWindowLayer::_currentWindowLayer->in_mode_)
                return;
            TVPWindowLayer::_windowMgrOverlay =
                TVPWindowManagerOverlay::create();
            GameNode->addChild(TVPWindowLayer::_windowMgrOverlay,
                               GAME_WINMGR_ORDER);
            _gameMenu->setVisible(false);
        }
    } else {
        if(TVPWindowLayer::_windowMgrOverlay) {
            TVPWindowLayer::_windowMgrOverlay->removeFromParent();
            TVPWindowLayer::_windowMgrOverlay = nullptr;
            _gameMenu->setVisible(true);
        }
    }
}

void TVPMainScene::popAllUIForm() {
    TVPControlAdDialog(0x10002, 0, 0);
    auto children = UINode->getChildren();
    for(auto ui : children) {
        cocos2d::Size size = getContentSize();
        cocos2d::Node *ColorMask =
            MaskLayer::create(Color4B(0, 0, 0, 128), size.width, size.height);
        ColorMask->setPosition(Vec2(-size.width, 0));
        ui->addChild(ColorMask);
        ColorMask->runAction(FadeOut::create(UI_CHANGE_DURATION));
        ui->runAction(EaseQuadraticActionOut::create(
            MoveTo::create(UI_CHANGE_DURATION, Vec2(size.width, 0))));
        runAction(Sequence::createWithTwoActions(
            DelayTime::create(UI_CHANGE_DURATION),
            CallFunc::create([=]() { ui->removeFromParent(); })));
    }
}

void TVPMainScene::toggleVirtualMouseCursor() {
    showVirtualMouseCursor(
        !TVPWindowLayer::TVPWindowLayer::_mouseCursor ||
        !TVPWindowLayer::TVPWindowLayer::_mouseCursor->isVisible());
}

Sprite *TVPCreateCUR() {
    std::string fullPath =
        FileUtils::getInstance()->fullPathForFilename("default.cur");
    Data buf = FileUtils::getInstance()->getDataFromFile(fullPath);

    tTVPMemoryStream stream(buf.getBytes(), buf.getSize());
    return TVPLoadCursorCUR(&stream);
}

void TVPMainScene::showVirtualMouseCursor(bool bVisible) {
    if(!bVisible) {
        if(TVPWindowLayer::TVPWindowLayer::_mouseCursor)
            TVPWindowLayer::TVPWindowLayer::_mouseCursor->setVisible(false);
        TVPWindowLayer::_virutalMouseMode = bVisible;
        return;
    }
    if(!TVPWindowLayer::TVPWindowLayer::_mouseCursor) {
        TVPWindowLayer::TVPWindowLayer::_mouseCursor = TVPCreateCUR();
        if(!TVPWindowLayer::TVPWindowLayer::_mouseCursor)
            return;

        _mouseCursorScale =
            TVPWindowLayer::TVPWindowLayer::_mouseCursor->getScale() *
            convertCursorScale(
                IndividualConfigManager::GetInstance()->GetValue<float>(
                    "vcursor_scale", 0.5f));
        TVPWindowLayer::TVPWindowLayer::_mouseCursor->setScale(
            _mouseCursorScale);
        GameNode->addChild(TVPWindowLayer::TVPWindowLayer::_mouseCursor,
                           GAME_WINMGR_ORDER);
        TVPWindowLayer::TVPWindowLayer::_mouseCursor->setPosition(
            GameNode->getContentSize() / 2);
    }
    TVPWindowLayer::TVPWindowLayer::_mouseCursor->setVisible(true);
    TVPWindowLayer::_virutalMouseMode = bVisible;
}

bool TVPMainScene::isVirtualMouseMode() const {
    return TVPWindowLayer::TVPWindowLayer::_mouseCursor &&
        TVPWindowLayer::TVPWindowLayer::_mouseCursor->isVisible();
}

bool TVPMainScene::onTouchBegan(cocos2d::Touch *touch, cocos2d::Event *event) {
    if(UINode->getChildrenCount())
        return false;
    if(!TVPWindowLayer::_currentWindowLayer)
        return false;
    if(!TVPWindowLayer::_virutalMouseMode || TVPWindowLayer::_windowMgrOverlay)
        return TVPWindowLayer::_currentWindowLayer->onTouchBegan(touch, event);
    _mouseTouches.insert(touch);
    switch(_mouseTouches.size()) {
        case 1:
            _mouseTouchPoint =
                GameNode->convertToNodeSpace(touch->getLocation());
            _mouseBeginPoint = TVPWindowLayer::_mouseCursor->getPosition();
            TVPWindowLayer::_touchBeginTick = TVPGetRoughTickCount32();
            _mouseMoved = false;
            _mouseClickedDown = false;
            _mouseBtn = ::mbLeft;
            TVPWindowLayer::_mouseCursor->stopAllActions();
            TVPWindowLayer::_mouseCursor->setOpacity(255);
            TVPWindowLayer::_mouseCursor->setScale(_mouseCursorScale);
            TVPWindowLayer::_mouseCursor->runAction(
                Sequence::createWithTwoActions(
                    DelayTime::create(1), CallFuncN::create([this](Node *p) {
                        p->setScale(_mouseCursorScale * 0.8f);
                        TVPWindowLayer::TVPWindowLayer::_currentWindowLayer
                            ->onMouseMove(GameNode->convertToWorldSpace(
                                _mouseBeginPoint));
                        TVPWindowLayer::_currentWindowLayer->onMouseDown(
                            GameNode->convertToWorldSpace(_mouseBeginPoint));
                        _mouseClickedDown = true;
                        _mouseMoved = true;
                    })));
            break;
        case 2:
            _mouseBtn = ::mbRight;
            _mouseTouchPoint =
                (_mouseTouchPoint +
                 GameNode->convertToNodeSpace(touch->getLocation())) /
                2;
            break;
        default:
            break;
    }
    return true;
}

void TVPMainScene::onTouchMoved(cocos2d::Touch *touch,
                                cocos2d::Event *event) const {
    if(!TVPWindowLayer::_currentWindowLayer)
        return;
    if(!TVPWindowLayer::_virutalMouseMode || TVPWindowLayer::_windowMgrOverlay)
        return TVPWindowLayer::_currentWindowLayer->onTouchMoved(touch, event);
    if(_mouseTouches.size()) {
        Vec2 pt, newpt;
        if(_mouseTouches.size() == 1) {
            pt = touch->getLocation();
        } else if(_mouseTouches.size() == 2) {
            auto it = _mouseTouches.begin();
            pt = (*it)->getLocation();
            pt = (pt + (*++it)->getLocation()) / 2;
        }
        Vec2 moveDistance, newPoint;
        newPoint = GameNode->convertToNodeSpace(pt);
        moveDistance = newPoint - _mouseTouchPoint;
        pt = _mouseBeginPoint + moveDistance;
        cocos2d::Size size = GameNode->getContentSize();
        newpt = pt;
        if(pt.x < 0)
            newpt.x = 0;
        else if(pt.x > size.width)
            newpt.x = size.width;
        if(pt.y < 0)
            newpt.y = 0;
        else if(pt.y > size.height)
            newpt.y = size.height;
        _mouseBeginPoint += newpt - pt;
        TVPWindowLayer::_mouseCursor->setPosition(newpt);
        TVPWindowLayer::_currentWindowLayer->onMouseMove(
            GameNode->convertToWorldSpace(newpt));
        if(!_mouseMoved) {
            if(TVPGetRoughTickCount32() - TVPWindowLayer::_touchBeginTick >
               1000) {
                TVPWindowLayer::_currentWindowLayer->onMouseDown(
                    GameNode->convertToWorldSpace(newpt));
                _mouseClickedDown = true;
                _mouseMoved = true;
            } else if(moveDistance.getLengthSq() >
                      TVPWindowLayer::_touchMoveThresholdSq) {
                TVPWindowLayer::_mouseCursor->stopAllActions();
                _mouseMoved = true;
            }
        }
    }
}

void TVPMainScene::onTouchEnded(cocos2d::Touch *touch,
                                cocos2d::Event *event) const {
    if(!TVPWindowLayer::_currentWindowLayer)
        return;
    if(!TVPWindowLayer::_virutalMouseMode || TVPWindowLayer::_windowMgrOverlay)
        return TVPWindowLayer::_currentWindowLayer->onTouchEnded(touch, event);
    if(_mouseTouches.size() == 1) {
        Vec2 pt = TVPWindowLayer::_mouseCursor->getPosition();
        if(!_mouseClickedDown &&
           TVPGetRoughTickCount32() - TVPWindowLayer::_touchBeginTick < 150) {
            TVPWindowLayer::_currentWindowLayer->onMouseClick(
                GameNode->convertToWorldSpace(pt));
        } else if(_mouseClickedDown) {
            TVPWindowLayer::_currentWindowLayer->onMouseUp(
                GameNode->convertToWorldSpace(pt));
        }
    }
    _mouseTouches.erase(touch);
    if(_mouseTouches.size() == 0) {
        _mouseMoved = false;
        _mouseClickedDown = false;
        TVPWindowLayer::_refadeMouseCursor();
        TVPWindowLayer::_mouseCursor->setScale(_mouseCursorScale);
        TVPWindowLayer::_mouseCursor->stopAllActions();
    }
}

void TVPMainScene::onTouchCancelled(cocos2d::Touch *touch,
                                    cocos2d::Event *event) {
    if(!TVPWindowLayer::_currentWindowLayer)
        return;
    if(!TVPWindowLayer::_virutalMouseMode || TVPWindowLayer::_windowMgrOverlay)
        return TVPWindowLayer::_currentWindowLayer->onTouchCancelled(touch,
                                                                     event);
    _mouseTouches.erase(touch);
    _mouseMoved = false;
    TVPWindowLayer::_refadeMouseCursor();
}

bool TVPMainScene::attachWithIME() {
    bool ret = IMEDelegate::attachWithIME();
    if(ret) {
        // open keyboard
        auto pGlView = Director::getInstance()->getOpenGLView();
        if(pGlView) {
#if (CC_TARGET_PLATFORM != CC_PLATFORM_WP8 &&                                  \
     CC_TARGET_PLATFORM != CC_PLATFORM_WINRT)
            pGlView->setIMEKeyboardState(true);
#else
            pGlView->setIMEKeyboardState(true, "");
#endif
        }
    }
    return ret;
}

bool TVPMainScene::detachWithIME() {
    bool ret = IMEDelegate::detachWithIME();
    if(ret) {
        // close keyboard
        auto glView = Director::getInstance()->getOpenGLView();
        if(glView) {
#if (CC_TARGET_PLATFORM != CC_PLATFORM_WP8 &&                                  \
     CC_TARGET_PLATFORM != CC_PLATFORM_WINRT)
            glView->setIMEKeyboardState(false);
#else
            glView->setIMEKeyboardState(false, "");
#endif
        }
    }
    return ret;
}

bool TVPMainScene::canAttachWithIME() { return true; }

bool TVPMainScene::canDetachWithIME() { return true; }

void TVPMainScene::deleteBackward() {
#ifndef _WIN32
    if(TVPWindowLayer::_currentWindowLayer) {
        TVPWindowLayer::_currentWindowLayer->InternalKeyDown(
            VK_BACK, TVPGetCurrentShiftKeyState());
        TVPWindowLayer::_currentWindowLayer->OnKeyUp(
            VK_BACK, TVPGetCurrentShiftKeyState());
    }
#endif
}

void TVPMainScene::insertText(const char *text, size_t len) {
    std::string utf8(text, len);
    onTextInput(utf8);
}

void TVPMainScene::onCharInput(int keyCode) {
    TVPWindowLayer::_currentWindowLayer->OnKeyPress((tjs_char)keyCode, 0, false,
                                                    false);
}

void TVPMainScene::onTextInput(const std::string &text) {
    std::u16string buf;
    if(StringUtils::UTF8ToUTF16(text, buf)) {
        for(int i = 0; i < buf.size(); ++i) {
            TVPWindowLayer::_currentWindowLayer->OnKeyPress(buf[i], 0, false,
                                                            false);
        }
    }
}

void TVPMainScene::onAxisEvent(cocos2d::Controller *ctrl, int keyCode,
                               cocos2d::Event *e) {
    if(!TVPWindowLayer::_currentWindowLayer ||
       !TVPWindowLayer::_currentWindowLayer->PrimaryLayerArea)
        return;
    if(!TVPWindowLayer::_virutalMouseMode || TVPWindowLayer::_windowMgrOverlay)
        return;
    const float threashold = 0.1f;
    const cocos2d::Controller::KeyStatus &keyStatus =
        ctrl->getKeyStatus(keyCode);
    //	CCLOG("Axis KeyCode:%d Axis Value:%f", keyCode,
    // keyStatus.value);
    if(std::abs(keyStatus.value) < threashold) {
        return;
    }
    float offv = keyStatus.value;
    if(offv > 0)
        offv = (offv - threashold) / (1 - threashold);
    else
        offv = (offv + threashold) / (1 - threashold);
    Vec2 pt = Vec2(TVPWindowLayer::_currentWindowLayer->_LastMouseX,
                   TVPWindowLayer::_currentWindowLayer->_LastMouseY);
    float *pValue = nullptr;
    switch(keyCode) {
        case cocos2d::Controller::JOYSTICK_LEFT_X:
            pValue = &pt.x;
            break;
        case cocos2d::Controller::JOYSTICK_LEFT_Y:
            pValue = &pt.y;
            break;
        default:
            return;
    }
    *pValue += offv * 16;
    pt = TVPWindowLayer::_currentWindowLayer->PrimaryLayerArea
             ->convertToWorldSpace(pt);
    pt = GameNode->convertToNodeSpace(pt);
    Vec2 newpt = pt;
    cocos2d::Size size = GameNode->getContentSize();
    if(pt.x < 0)
        newpt.x = 0;
    else if(pt.x > size.width)
        newpt.x = size.width;
    if(pt.y < 0)
        newpt.y = 0;
    else if(pt.y > size.height)
        newpt.y = size.height;
    TVPWindowLayer::_mouseCursor->setPosition(newpt);
    TVPWindowLayer::_currentWindowLayer->onMouseMove(
        GameNode->convertToWorldSpace(newpt));
}

void TVPMainScene::onPadKeyDown(cocos2d::Controller *ctrl, int keyCode,
                                cocos2d::Event *e) {
    if(!UINode->getChildren().empty())
        return;
    unsigned int code = TVPConvertPadKeyCodeToVKCode(keyCode);
    if(!code || code >= 0x200)
        return;
    code = _keymap[code];
    TVPWindowLayer::_scancode[code] = 0x11;
    if(TVPWindowLayer::_currentWindowLayer) {
        TVPWindowLayer::_currentWindowLayer->InternalKeyDown(
            code, TVPGetCurrentShiftKeyState());
    }
}

void TVPMainScene::onPadKeyUp(cocos2d::Controller *ctrl, int keyCode,
                              cocos2d::Event *e) {
    unsigned int code = TVPConvertPadKeyCodeToVKCode(keyCode);
    if(!code || code >= 0x200)
        return;
    code = _keymap[code];
    bool isPressed = TVPWindowLayer::_scancode[code] & 1;
    TVPWindowLayer::_scancode[code] &= 0x10;
    if(isPressed && TVPWindowLayer::_currentWindowLayer) {
        TVPWindowLayer::_currentWindowLayer->OnKeyUp(
            code, TVPGetCurrentShiftKeyState());
    }
}

void TVPMainScene::onPadKeyRepeat(cocos2d::Controller *ctrl, int code,
                                  cocos2d::Event *e) {}

float TVPMainScene::convertCursorScale(float val /*0 ~ 1*/) {
    if(val <= 0.5f) {
        return 0.25f + (val * 2) * 0.75f;
    } else {
        return 1.f + (val - 0.5f) * 2.f;
    }
}

iWindowLayer *TVPCreateAndAddWindow(tTJSNI_Window *w) {
    TVPWindowLayer *ret = TVPWindowLayer::create(w);
    TVPMainScene::GetInstance()->addLayer(ret);
    if(TVPWindowLayer::TVPWindowLayer::_consoleWin)
        ret->setVisible(false);
    return ret;
}

void TVPRemoveWindowLayer(iWindowLayer *lay) {
    static_cast<TVPWindowLayer *>(lay)->removeFromParent();
}

void TVPConsoleLog(const ttstr &l, bool important) {
    static bool TVPLoggingToConsole =
        IndividualConfigManager::GetInstance()->GetValue<bool>("outputlog",
                                                               true);
    if(!TVPLoggingToConsole)
        return;
    if(TVPWindowLayer::TVPWindowLayer::_consoleWin) {
        TVPWindowLayer::TVPWindowLayer::_consoleWin->addLine(
            l, important ? Color3B::YELLOW : Color3B::GRAY);
        TVPDrawSceneOnce(100); // force update in 10fps
    }
    spdlog::get("tjs2")->info("{}", l.AsStdString());
}

namespace TJS {
    void TVPConsoleLog(const ttstr &str) {
        spdlog::get("tjs2")->info("{}", str.AsStdString());
    }

    template <typename... Args>
    void TVPConsoleLog(spdlog::format_string_t<Args...> fmt, Args &&...args) {
        spdlog::get("tjs2")->info(fmt, std::forward<Args>(args)...);
    }
} // namespace TJS

bool TVPGetScreenSize(tjs_int idx, tjs_int &w, tjs_int &h) {
    if(idx != 0)
        return false;
    const cocos2d::Size &size =
        cocos2d::Director::getInstance()->getOpenGLView()->getFrameSize();
    // w = size.height; h = size.width;
    w = 2048;
    h = w * (size.height / size.width);
    return true;
}

ttstr TVPGetDataPath() {
    std::string path = cocos2d::FileUtils::getInstance()->getWritablePath();
    return path;
}

#include "StorageImpl.h"

static std::string _TVPGetInternalPreferencePath() {
    std::string path = cocos2d::FileUtils::getInstance()->getWritablePath();
    path += ".preference";
    if(!TVPCheckExistentLocalFolder(path)) {
        TVPCreateFolders(path);
    }
    path += "/";
    return path;
}

const std::string &TVPGetInternalPreferencePath() {
    static std::string ret = _TVPGetInternalPreferencePath();
    return ret;
}

tjs_uint32 TVPGetCurrentShiftKeyState() {
    tjs_uint32 f = 0;

    if(TVPWindowLayer::_scancode[VK_SHIFT] & 1)
        f |= ssShift;
    if(TVPWindowLayer::_scancode[VK_MENU] & 1)
        f |= ssAlt;
    if(TVPWindowLayer::_scancode[VK_CONTROL] & 1)
        f |= ssCtrl;
    if(TVPWindowLayer::_scancode[VK_LBUTTON] & 1)
        f |= ssLeft;
    if(TVPWindowLayer::_scancode[VK_RBUTTON] & 1)
        f |= ssRight;
    // if (_scancode[VK_MBUTTON] & 1) f |= TVP_SS_MIDDLE;

    return f;
}

ttstr TVPGetPlatformName() {
    switch(cocos2d::Application::getInstance()->getTargetPlatform()) {
        case ApplicationProtocol::Platform::OS_WINDOWS:
            return "Win32";
        case ApplicationProtocol::Platform::OS_LINUX:
            return "Linux";
        case ApplicationProtocol::Platform::OS_MAC:
            return "MacOS";
        case ApplicationProtocol::Platform::OS_ANDROID:
            return "Android";
        case ApplicationProtocol::Platform::OS_IPHONE:
            return "iPhone";
        case ApplicationProtocol::Platform::OS_IPAD:
            return "iPad";
        case ApplicationProtocol::Platform::OS_BLACKBERRY:
            return "BlackBerry";
        case ApplicationProtocol::Platform::OS_NACL:
            return "Nacl";
        case ApplicationProtocol::Platform::OS_TIZEN:
            return "Tizen";
        case ApplicationProtocol::Platform::OS_WINRT:
            return "WinRT";
        case ApplicationProtocol::Platform::OS_WP8:
            return "WinPhone8";
        default:
            return "Unknown";
    }
}

ttstr TVPGetOSName() { return TVPGetPlatformName(); }