// Opt-in component-test input. Not compiled into normal runtime builds.
#include "tjsCommHead.h"
#include <cocos2d.h>
#include <ui/CocosGUI.h>
#include "launch/DesktopLaunch.h"
#include "SysInitIntf.h"
#include "tjsString.h"
#include "HostLifecycle.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <sstream>

namespace {
    namespace fs = std::filesystem;
    using namespace cocos2d;

    bool visible(Node *node) {
        for(auto *parent = node; parent; parent = parent->getParent())
            if(!parent->isVisible())
                return false;
        return true;
    }

    void collect(Node *node, const std::string &selector, bool text,
                 std::vector<Node *> &matches) {
        if(!visible(node))
            return;
        bool match = !text && node->getName() == selector;
        if(text) {
            if(auto *label = dynamic_cast<ui::Text *>(node))
                match = label->getString() == selector;
            if(auto *button = dynamic_cast<ui::Button *>(node))
                match = button->getTitleText() == selector;
        }
        if(match)
            matches.push_back(node);
        for(auto *child : node->getChildren())
            collect(child, selector, text, matches);
    }

    void dump(Node *node, std::ostream &out, int depth = 0) {
        if(!visible(node))
            return;
        out << std::string(depth, ' ') << node->getName();
        if(auto *label = dynamic_cast<ui::Text *>(node))
            out << " text=" << label->getString();
        if(auto *field = dynamic_cast<ui::TextField *>(node))
            out << " field=" << field->getString();
        if(auto *button = dynamic_cast<ui::Button *>(node))
            out << " button=" << button->getTitleText();
        const auto &pos = node->getPosition();
        const auto &anchor = node->getAnchorPoint();
        const auto &size = node->getContentSize();
        out << " pos=" << pos.x << "," << pos.y << " anchor=" << anchor.x << ","
            << anchor.y << " size=" << size.width << "," << size.height
            << " scale=" << node->getScaleX() << "," << node->getScaleY();
        const auto center = node->convertToWorldSpace(
            Vec2(node->getContentSize().width * 0.5f,
                 node->getContentSize().height * 0.5f));
        out << " center=" << center.x << "," << center.y << '\n';
        for(auto *child : node->getChildren())
            dump(child, out, depth + 1);
    }
} // namespace

void TVPStartUIContractDriver(cocos2d::Scene *scene) {
    tTJSVariant enabled;
    if(!TVPGetCommandLine(TJS_W("-ui-contract-driver"), &enabled) ||
       ttstr(enabled) != TJS_W("yes"))
        return;
    const auto root = fs::u8path(krkr::launch::DesktopTarget());
    if(!fs::is_directory(root) ||
       !fs::is_regular_file(root / "interaction-fixture.json"))
        throw std::runtime_error(
            "UI driver requires an isolated interaction fixture");

    auto sequence = std::make_shared<std::string>();
    scene->schedule(
        [root, sequence, scene](float) {
            std::ifstream request(root / "ui-action.txt");
            std::string id, action, selector;
            if(!(request >> id >> action) || id == *sequence)
                return;
            *sequence = id;
            std::getline(request, selector);
            if(!selector.empty() && selector.front() == ' ')
                selector.erase(0, 1);
            std::ofstream log(root / "ui-driver-events.txt", std::ios::app);
            try {
                if(action == "tree") {
                    std::ofstream tree(root / "ui-tree.txt");
                    dump(scene, tree);
                } else if(action == "capture") {
                    const auto path = (root / "ui-frame.png").u8string();
                    utils::captureScreen(
                        [root, id](bool saved, const std::string &) {
                            std::ofstream result(root / "ui-driver-events.txt",
                                                 std::ios::app);
                            result << id
                                   << (saved ? " capture-saved\n"
                                             : " error:capture\n");
                        },
                        path);
                } else if(action == "activate" || action == "deactivate") {
                    if(action == "activate")
                        TVPOnHostForeground();
                    else
                        TVPOnHostBackground();
                } else if(action == "key-down" || action == "key-up") {
                    if(selector != "A")
                        throw std::runtime_error(
                            "Only the A-key probe is supported");
                    EventKeyboard event(EventKeyboard::KeyCode::KEY_A,
                                        action == "key-down");
                    Director::getInstance()
                        ->getEventDispatcher()
                        ->dispatchEvent(&event);
                } else if(action == "click-name" || action == "click-text") {
                    std::vector<Node *> matches;
                    collect(scene, selector, action == "click-text", matches);
                    // A button and its label may expose the same caption.
                    // Keep the deepest matching node; unrelated matches remain
                    // ambiguous and are rejected.
                    const auto candidates = matches;
                    matches.erase(
                        std::remove_if(
                            matches.begin(), matches.end(),
                            [&candidates](Node *candidate) {
                                for(auto *other : candidates)
                                    for(auto *parent = other->getParent();
                                        parent; parent = parent->getParent())
                                        if(parent == candidate)
                                            return true;
                                return false;
                            }),
                        matches.end());
                    if(matches.size() != 1)
                        throw std::runtime_error(
                            "Expected one visible target: " + selector +
                            "; found " + std::to_string(matches.size()));
                    auto *target = matches.front();
                    auto *director = Director::getInstance();
                    auto *view = director->getOpenGLView();
                    const auto point =
                        director->convertToUI(target->convertToWorldSpace(
                            Vec2(target->getContentSize().width * 0.5f,
                                 target->getContentSize().height * 0.5f)));
                    const auto viewport = view->getViewPortRect();
                    float x = point.x * view->getScaleX() + viewport.origin.x;
                    float y = point.y * view->getScaleY() + viewport.origin.y;
                    const auto frame = view->getFrameSize();
                    if(x < 0 || y < 0 || x >= frame.width || y >= frame.height)
                        throw std::runtime_error(
                            "UI target center is outside the window: " +
                            selector);
                    intptr_t touch = 0x4b524b52;
                    // Enter through the same Cocos input route as platform
                    // touches.
                    view->handleTouchesBegin(1, &touch, &x, &y);
                    view->handleTouchesEnd(1, &touch, &x, &y);
                    log << id << " touch " << selector << ' ' << x << ',' << y
                        << '\n';
                } else {
                    throw std::runtime_error("Unknown UI test action: " +
                                             action);
                }
                log << id << " complete " << action << '\n';
            } catch(const std::exception &error) {
                log << id << " error:" << error.what() << '\n';
            }
        },
        0.2f, "ui-contract-driver");
}
