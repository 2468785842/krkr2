//
// Created by LiDon on 2025/9/15.
//
#pragma once

#include "ResourceManager.h"

namespace motion {

    enum class MaskMode { MaskModeAlpha };

    class EmotePlayer {
    public:
        explicit EmotePlayer(ResourceManager rm) {}

        void initPhysics() {}

        void setUseD3D(bool useD3D) { this->_useD3D = useD3D; }

        [[nodiscard]] bool getUseD3D() const { return this->_useD3D; }

    private:
        bool _useD3D;
        MaskMode _maskMode;
    };

} // namespace motion