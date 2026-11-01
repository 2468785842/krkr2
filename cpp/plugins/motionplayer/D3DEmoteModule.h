//
// Reverse-engineered from libkrkr2.so D3DEmoteModule class
// Top-level class registered under emoteplayer.dll
//
#pragma once

#include "TVPLog.h"

namespace motion {

    class D3DEmoteModule {
    public:
        D3DEmoteModule() = default;

        static void setMaskMode(int v) { _maskMode = v; }
        static int getMaskMode() { return _maskMode; }

        static void setMaskRegionClipping(bool v) { _maskRegionClipping = v; }
        static bool getMaskRegionClipping() { return _maskRegionClipping; }

        static void setMipMapEnabled(bool v) { _mipMapEnabled = v; }
        static bool getMipMapEnabled() { return _mipMapEnabled; }

        static void setAlphaOp(int v) { _alphaOp = v; }
        static int getAlphaOp() { return _alphaOp; }

        static void setProtectTranslucentTextureColor(bool v) {
            _protectTranslucentTextureColor = v;
        }
        static bool getProtectTranslucentTextureColor() {
            return _protectTranslucentTextureColor;
        }

        static void setPixelateDivision(int v) { _pixelateDivision = v; }
        static int getPixelateDivision() { return _pixelateDivision; }

        static void setMaxTextureSize(int w, int h) {
            G_PluginLog.warn(
                "D3DEmoteModule::setMaxTextureSize({}, {}) stub called", w, h);
        }

    private:
        inline static int _maskMode = 1; // MaskModeAlpha
        inline static bool _maskRegionClipping = false;
        inline static bool _mipMapEnabled = false;
        inline static int _alphaOp = 0;
        inline static bool _protectTranslucentTextureColor = false;
        inline static int _pixelateDivision = 1;
    };

} // namespace motion
