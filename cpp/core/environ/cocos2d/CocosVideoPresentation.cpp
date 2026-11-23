#include "CocosVideoPresentation.h"

#include "YUVSprite.h"

namespace krkr::video {
    namespace {
        class CocosVideoPresentation final : public Presentation {
            cocos2d::Node &root;
            TVPYUVSprite *sprite = nullptr;

        public:
            explicit CocosVideoPresentation(cocos2d::Node &node) : root(node) {
                root.retain();
            }

            ~CocosVideoPresentation() override {
                if(root.getParent())
                    root.removeFromParent();
                else
                    root.cleanup();
                root.release();
            }

            void SetVisible(bool visible) override { root.setVisible(visible); }

            void SetPictureVisible(bool visible) override {
                if(sprite)
                    sprite->setVisible(visible);
            }

            void Present(const Yuv420Frame &frame,
                         const Bounds &bounds) override {
                auto *parent = root.getParent();
                if(!parent)
                    return;

                if(!sprite) {
                    sprite = TVPYUVSprite::create();
                    sprite->setAnchorPoint(cocos2d::Vec2(0, 1));
                    root.addChild(sprite);
                }
                const int chromaWidth = (frame.width + 1) / 2;
                const int chromaHeight = (frame.height + 1) / 2;
                sprite->updateTextureData(frame.y, frame.width, frame.height,
                                          frame.u, chromaWidth, chromaHeight,
                                          frame.v, chromaWidth, chromaHeight);
                const float scaleX =
                    bounds.width / static_cast<float>(frame.width);
                const float scaleY =
                    bounds.height / static_cast<float>(frame.height);
                if(scaleX != sprite->getScaleX())
                    sprite->setScaleX(scaleX);
                if(scaleY != sprite->getScaleY())
                    sprite->setScaleY(scaleY);

                const auto position = sprite->getPosition();
                const int top = parent->getContentSize().height - bounds.top;
                if(static_cast<int>(position.x) != bounds.left ||
                   static_cast<int>(position.y) != top)
                    sprite->setPosition(bounds.left, top);
            }
        };
    } // namespace

    std::unique_ptr<Presentation>
    CreateCocosVideoPresentation(cocos2d::Node &root) {
        return std::make_unique<CocosVideoPresentation>(root);
    }
} // namespace krkr::video
