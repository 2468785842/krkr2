//
// Created by lidong on 2025/1/7.
//

#ifndef KRKR2_GDIP_CXX_BRUSH_H
#define KRKR2_GDIP_CXX_BRUSH_H

#include "gdip_cxx.h"

extern "C" {
#include <libgdiplus/gdiplus-private.h>
#include <libgdiplus/brush-private.h>
#include <libgdiplus/solidbrush-private.h>
#include <libgdiplus/hatchbrush-private.h>
#include <libgdiplus/texturebrush-private.h>
#include <libgdiplus/pathgradientbrush-private.h>
#include <libgdiplus/lineargradientbrush-private.h>
}

class [[nodiscard]] BrushBase {
public:
    [[nodiscard]] explicit virtual operator GpBrush *() const = 0;

    [[nodiscard]] virtual BrushBase *Clone() const = 0;

    [[nodiscard]] virtual GpStatus GetLastStatus() const {
        return this->gpStatus;
    }

    virtual ~BrushBase() = default;

protected:
    mutable GpStatus gpStatus{};
};

class SolidBrush : public BrushBase {
public:
    SolidBrush(SolidFill *brush) : _gpSolidFill(brush) {}

    SolidBrush(const Color &color) {
        GdipCreateSolidFill(*(ARGB *)&color, &this->_gpSolidFill);
    }

    SolidBrush(const SolidBrush &brush) = delete;

    [[nodiscard]] explicit operator GpBrush *() const override {
        return (GpBrush *)this->_gpSolidFill;
    }

    [[nodiscard]] BrushBase *Clone() const override {
        SolidFill *tmp{};
        GdipCloneBrush((GpBrush *)this->_gpSolidFill, (GpBrush **)&tmp);
        return new SolidBrush{ tmp };
    }

    GpStatus GetColor(Color *color) const {
        this->gpStatus =
            GdipGetSolidFillColor(this->_gpSolidFill, (ARGB *)color);
        return this->gpStatus;
    }

    GpStatus SetColor(const Color &color) {
        this->gpStatus =
            GdipSetSolidFillColor(this->_gpSolidFill, *(ARGB *)&color);
        return this->gpStatus;
    }

    ~SolidBrush() override { GdipDeleteBrush((GpBrush *)this->_gpSolidFill); }

private:
    GpSolidFill *_gpSolidFill{ nullptr };
};

class HatchBrush : public BrushBase {
public:
    HatchBrush(GpHatch *brush) : _gpHatch(brush) {}

    HatchBrush(HatchStyle hatchStyle, const Color &foreColor,
               const Color &backColor) {
        GdipCreateHatchBrush(hatchStyle, *(ARGB *)&foreColor,
                             *(ARGB *)&backColor, &this->_gpHatch);
    }

    HatchBrush(const HatchBrush &) = delete;

    [[nodiscard]] explicit operator GpBrush *() const override {
        return (GpBrush *)this->_gpHatch;
    }

    [[nodiscard]] BrushBase *Clone() const override {
        GpHatch *tmp{};
        GdipCloneBrush((GpBrush *)this->_gpHatch, (GpBrush **)&tmp);
        return new HatchBrush{ tmp };
    }

    HatchStyle GetHatchStyle() const {
        HatchStyle style{};
        this->gpStatus = GdipGetHatchStyle(this->_gpHatch, &style);
        return style;
    }

    GpStatus GetBackgroundColor(Color *color) const {
        this->gpStatus =
            GdipGetHatchBackgroundColor(this->_gpHatch, (ARGB *)&color);
        return this->gpStatus;
    }

    GpStatus GetForegroundColor(Color *color) const {
        this->gpStatus =
            GdipGetHatchForegroundColor(this->_gpHatch, (ARGB *)&color);
        return this->gpStatus;
    }

    ~HatchBrush() override { GdipDeleteBrush((GpBrush *)this->_gpHatch); }

private:
    GpHatch *_gpHatch{ nullptr };
};

class TextureBrush : public BrushBase {
public:
    TextureBrush(GpTexture *brush) : _gpTexture(brush) {}

    TextureBrush(ImageClass *image, WrapMode mode) {
        GdipCreateTexture((GpImage *)image, mode, &this->_gpTexture);
    }

    TextureBrush(ImageClass *image, WrapMode mode, const RectF &rectF) {
        GdipCreateTexture2((GpImage *)image, mode, rectF.X, rectF.Y,
                           rectF.Width, rectF.Height, &this->_gpTexture);
    }

    TextureBrush(const TextureBrush &) = delete;

    [[nodiscard]] explicit operator GpBrush *() const override {
        return (GpBrush *)this->_gpTexture;
    }

    [[nodiscard]] BrushBase *Clone() const override {
        GpTexture *tmp{};
        GdipCloneBrush((GpBrush *)this->_gpTexture, (GpBrush **)&tmp);
        return new TextureBrush{ tmp };
    }

    ~TextureBrush() override { GdipDeleteBrush((GpBrush *)this->_gpTexture); }

private:
    GpTexture *_gpTexture;
};

class PathGradientBrush : public BrushBase {
public:
    PathGradientBrush(GpPathGradient *brush) : _gpPathG(brush) {}

    PathGradientBrush(const PointF *points, int count, WrapMode wrapMode) {
        GdipCreatePathGradient(points, count, wrapMode, &this->_gpPathG);
    }

    PathGradientBrush(const PathGradientBrush &) = delete;

    [[nodiscard]] explicit operator GpBrush *() const override {
        return (GpBrush *)this->_gpPathG;
    }

    [[nodiscard]] BrushBase *Clone() const override {
        GpPathGradient *tmp{};
        GdipCloneBrush((GpBrush *)this->_gpPathG, (GpBrush **)&tmp);
        return new PathGradientBrush{ tmp };
    }

    GpStatus SetCenterColor(const Color &color) {
        this->gpStatus =
            GdipSetPathGradientCenterColor(this->_gpPathG, *(ARGB *)&color);
        return this->gpStatus;
    }

    GpStatus SetCenterPoint(const Point &point) {
        return this->SetCenterPoint(PointF{ (float)point.X, (float)point.Y });
    }

    GpStatus SetCenterPoint(const PointF &point) {
        this->gpStatus = GdipSetPathGradientCenterPoint(this->_gpPathG, &point);
        return this->gpStatus;
    }

    GpStatus SetFocusScales(float xScale, float yScale) {
        this->gpStatus =
            GdipSetPathGradientFocusScales(this->_gpPathG, xScale, yScale);
        return this->gpStatus;
    }

    GpStatus SetSurroundColors(const Color *colors, int *count) {
        this->gpStatus = GdipSetPathGradientSurroundColorsWithCount(
            this->_gpPathG, (ARGB *)&colors, count);
        return this->gpStatus;
    }

    GpStatus SetBlend(const float *blendFactors, const float *blendPositions,
                      int count) {
        this->gpStatus = GdipSetPathGradientBlend(this->_gpPathG, blendFactors,
                                                  blendPositions, count);
        return this->gpStatus;
    }

    GpStatus SetBlendBellShape(float focus, float scale) {
        this->gpStatus =
            GdipSetPathGradientSigmaBlend(this->_gpPathG, focus, scale);
        return this->gpStatus;
    }

    GpStatus SetBlendTriangularShape(float focus, float scale = 1) {
        this->gpStatus =
            GdipSetPathGradientLinearBlend(this->_gpPathG, focus, scale);
        return this->gpStatus;
    }

    GpStatus SetGammaCorrection(bool /*useGammaCorrection*/) {
        return this->gpStatus = Ok;
    }

    GpStatus SetInterpolationColors(const Color *presetColors,
                                    const float *blendPositions, int count) {
        this->gpStatus = GdipSetPathGradientPresetBlend(
            this->_gpPathG, (const ARGB *)presetColors, blendPositions, count);
        return this->gpStatus;
    }

    ~PathGradientBrush() override {
        GdipDeleteBrush((GpBrush *)this->_gpPathG);
    }

private:
    GpPathGradient *_gpPathG{ nullptr };
};

class LinearGradientBrush : public BrushBase {
public:
    LinearGradientBrush(GpLineGradient *brush) : _gpLG(brush) {}

    LinearGradientBrush(const PointF &point1, const PointF &point2,
                        const Color &color1, const Color &color2) {
        GdipCreateLineBrush(&point1, &point2, *(ARGB *)&color1,
                            *(ARGB *)&color2, WrapModeTile, &this->_gpLG);
    }

    LinearGradientBrush(const RectF &rect, const Color &color1,
                        const Color &color2, float angle,
                        bool isAngleScalable) {
        GdipCreateLineBrushFromRectWithAngle(
            &rect, *(ARGB *)&color1, *(ARGB *)&color2, angle, isAngleScalable,
            WrapModeTile, &this->_gpLG);
    }

    LinearGradientBrush(const RectF &rect, const Color &color1,
                        const Color &color2, LinearGradientMode mode) {
        GdipCreateLineBrushFromRect(&rect, *(ARGB *)&color1, *(ARGB *)&color2,
                                    mode, WrapModeTile, &this->_gpLG);
    }

    LinearGradientBrush(const LinearGradientBrush &) = delete;

    [[nodiscard]] explicit operator GpBrush *() const override {
        return (GpBrush *)this->_gpLG;
    }

    [[nodiscard]] BrushBase *Clone() const override {
        GpLineGradient *tmp{};
        GdipCloneBrush((GpBrush *)this->_gpLG, (GpBrush **)&tmp);
        return new LinearGradientBrush{ tmp };
    }

    GpStatus SetWrapMode(WrapMode wrapMode) {
        this->gpStatus = GdipSetLineWrapMode(this->_gpLG, wrapMode);
        return this->gpStatus;
    }

    GpStatus SetBlend(const float *blendFactors, const float *blendPositions,
                      int count) {
        this->gpStatus =
            GdipSetLineBlend(this->_gpLG, blendFactors, blendPositions, count);
        return this->gpStatus;
    }

    GpStatus SetBlendBellShape(float focus, float scale = 1) {
        this->gpStatus = GdipSetLineSigmaBlend(this->_gpLG, focus, scale);
        return this->gpStatus;
    }

    GpStatus SetBlendTriangularShape(float focus, float scale = 1) {
        this->gpStatus = GdipSetLineLinearBlend(this->_gpLG, focus, scale);
        return this->gpStatus;
    }

    GpStatus SetGammaCorrection(bool useGammaCorrection) {
        this->gpStatus =
            GdipSetLineGammaCorrection(this->_gpLG, useGammaCorrection);
        return this->gpStatus;
    }

    GpStatus SetInterpolationColors(const Color *presetColors,
                                    const float *blendPositions, int count) {
        this->gpStatus = GdipSetLinePresetBlend(
            this->_gpLG, (const ARGB *)presetColors, blendPositions, count);
        return this->gpStatus;
    }

    ~LinearGradientBrush() override { GdipDeleteBrush((GpBrush *)this->_gpLG); }

private:
    GpLineGradient *_gpLG{ nullptr };
};

#endif // KRKR2_GDIP_CXX_BRUSH_H
