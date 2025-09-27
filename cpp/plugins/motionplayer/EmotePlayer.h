//
// Created by LiDon on 2025/9/15.
//
#pragma once

#include <cstdint>
#include "ResourceManager.h"
#include "tjsArray.h"

namespace emote {
    enum class LayerType : std::uint8_t {
        Obj,
        Shape,
        Layout,
        Motion,
        Particle,
        Camera,
    };

    enum class ShapeType {
        Point,
        Circle,
        Rect,
        Quad,
    };

    enum class CoordinateRecutangular : std::uint8_t { XY, XZ };

    enum class TransformOrderType : std::uint8_t {
        Flip,
        Angle,
        Zoom,
        Slant,
    };

    enum class PlayFlagForceType : std::uint8_t {
        Force = 1,
        Chain = 1 << 1,
        AsCan = 1 << 2,
        Join = 1 << 3,
        Stealth = 1 << 4,
    };

    enum class ResumeModeType : std::uint8_t {
        None,
        ImplicitReload,
        ExplicitReload
    };

    enum class MaskModeType : std::uint8_t { Stencil, Alpha };

    class EmotePlayer {
    public:
        enum class TimelinePlayFlagsType : std::uint8_t {
            Parallel = 1,
            Difference = 1 << 1
        };

        enum class TransformOrderMaskType : std::uint32_t {
            PositionTranslateToScale = 1 << 0,
            PositionScaleToTranslate = 1 << 1,
            PhysicsTranslateToScale = 1 << 8,
            PhysicsScaleToTranslate = 1 << 9,

            TranslateToScale =
                PositionTranslateToScale | PhysicsTranslateToScale,
            ScaleToTranslate =
                PositionScaleToTranslate | PhysicsScaleToTranslate,
            Default = PositionTranslateToScale | PhysicsScaleToTranslate
        };

        explicit EmotePlayer(ResourceManager resManager);

        void initPhysics(tTJSVariant rule);

        void play(tTJSVariant) {}

        void draw(tTJSVariant layer) {}

        tTJSVariant getTimelineLabelList(const std::function<bool(int)> &diffFilter) const;

        tTJSVariant getDiffTimelineLabelList() const;

        tTJSVariant getMainTimelineLabelList() const;

        void setCompletionType(tTVPBBStretchType completionType) {
            this->_completionType = completionType;
        }

        [[nodiscard]] tTVPBBStretchType getCompletionType() const {
            return this->_completionType;
        }

        void setMaskMode(MaskModeType maskMode) { this->_maskMode = maskMode; }
        [[nodiscard]] MaskModeType getMaskMode() const { return this->_maskMode; }

        void setChara(ttstr chara) { this->_chara = chara; }
        [[nodiscard]] ttstr getChara() const { return _chara; }

        void setMotionKey(ttstr motionKey) { this->_motionKey = motionKey; }
        [[nodiscard]] ttstr getMotionKey() { return this->_motionKey; }

        void setUseD3D(bool useD3D) { this->_useD3D = useD3D; }
        [[nodiscard]] bool getUseD3D() const { return this->_useD3D; }

    private:
        ttstr _chara;
        ResourceManager _resManager;
        ttstr _motionKey; // psb file path
        bool _useD3D;
        MaskModeType _maskMode;
        tTVPBBStretchType _completionType;
    };

} // namespace emote