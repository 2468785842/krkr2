//
// Created by lidong on 2025/1/7.
//

#ifndef KRKR2_GDIP_CXX_BRUSH_H
#define KRKR2_GDIP_CXX_BRUSH_H

#include <gdiplus-private.h>
#include <brush-private.h>
#include <solidbrush-private.h>
#include <hatchbrush-private.h>
#include <texturebrush-private.h>
#include <pathgradientbrush-private.h>
#include <lineargradientbrush-private.h>
#include <stdexcept>

class [[nodiscard]] BrushBase {
public:

    [[nodiscard]] virtual explicit operator Brush *() const = 0;

    [[nodiscard]] virtual BrushBase *Clone() const = 0;

    [[nodiscard]] virtual GpStatus GetLastStatus() const {
        return this->gpStatus;
    }

    [[nodiscard]] virtual GpBrushType GetType() const {
        GpBrushType type{};
        this->gpStatus = GdipGetBrushType(
                (Brush *) this, &type
        );
        return type;
    }

    virtual ~BrushBase() {
        GdipDeleteBrush((Brush *) this);
    }

protected:
    mutable GpStatus gpStatus{};
};

class SolidBrush : public BrushBase {
public:
    SolidBrush(Brush *brush) {
        if (((BrushClass *) brush)->type != BrushTypeSolidColor) {
            throw std::invalid_argument{
                    "can't init SolidBrush Object with not BrushTypeSolidColor type brush ptr"
            };
        }

        this->_gpSolidFill = reinterpret_cast<SolidFill *>(brush);
    }

    SolidBrush(const Color &color) {
        GdipCreateSolidFill(*(ARGB *) &color, &this->_gpSolidFill);
    }

    SolidBrush(const SolidBrush &brush) = delete;

    explicit operator Brush *() const override {
        return (Brush *) this->_gpSolidFill;
    }

    [[nodiscard]] BrushBase *Clone() const override {
        Brush *tmp{nullptr};
        GdipCloneBrush((Brush *) this, &tmp);
        return new SolidBrush{tmp};
    }

    GpStatus GetColor(Color *color) const {
        this->gpStatus = GdipGetSolidFillColor(
                this->_gpSolidFill, (ARGB *) color
        );
        return this->gpStatus;
    }

    GpStatus SetColor(const Color &color) {
        this->gpStatus = GdipSetSolidFillColor(
                this->_gpSolidFill, *(ARGB *) &color
        );
        return this->gpStatus;
    }

private:
    GpSolidFill *_gpSolidFill{nullptr};
};

class HatchBrush : public BrushBase {
public:
    HatchBrush(Brush *brush) {
        if (((BrushClass *) brush)->type != BrushTypeHatchFill) {
            throw std::invalid_argument{
                    "can't init HatchBrush Object with not BrushTypeHatchFill type brush ptr"
            };
        }

        this->_gpHatch = reinterpret_cast<GpHatch *>(brush);
    }

    HatchBrush(HatchStyle hatchStyle,
               const Color &foreColor,
               const Color &backColor
    ) {
        GdipCreateHatchBrush(
                hatchStyle, *(ARGB *) &foreColor,
                *(ARGB *) &backColor, &this->_gpHatch
        );
    }

    HatchBrush(const HatchBrush &) = delete;

    explicit operator Brush *() const override {
        return (Brush *) this->_gpHatch;
    }

    [[nodiscard]] BrushBase *Clone() const override {
        Brush *tmp{nullptr};
        GdipCloneBrush((Brush *) this, &tmp);
        return new HatchBrush{tmp};
    }

    HatchStyle GetHatchStyle() const {
        HatchStyle style{};
        this->gpStatus = GdipGetHatchStyle(this->_gpHatch, &style);
        return style;
    }

    GpStatus GetBackgroundColor(Color *color) const {
        this->gpStatus = GdipGetHatchBackgroundColor(
                this->_gpHatch, (ARGB *) &color
        );
        return this->gpStatus;
    }

    GpStatus GetForegroundColor(Color *color) const {
        this->gpStatus = GdipGetHatchForegroundColor(
                this->_gpHatch, (ARGB *) &color
        );
        return this->gpStatus;
    }

private:
    GpHatch *_gpHatch{nullptr};
};

class TextureBrush : public BrushBase {
public:
    TextureBrush(Brush *brush) {
        if (((BrushClass *) brush)->type != BrushTypeTextureFill) {
            throw std::invalid_argument{
                    "can't init TextureBrush Object with not BrushTypeTextureFill type brush ptr"
            };
        }

        this->_gpTexture = reinterpret_cast<GpTexture *>(brush);
    }

    TextureBrush(ImageClass *image, WrapMode mode) {
        GdipCreateTexture((GpImage *) image, mode, &this->_gpTexture);
    }

    TextureBrush(ImageClass *image, WrapMode mode, const RectF &rectF) {
        GdipCreateTexture2(
                (GpImage *) image, mode,
                rectF.X, rectF.Y, rectF.Width, rectF.Height,
                &this->_gpTexture
        );
    }

    TextureBrush(const TextureBrush &) = delete;

    explicit operator Brush *() const override {
        return (Brush *) this->_gpTexture;
    }

    [[nodiscard]] BrushBase *Clone() const override {
        Brush *tmp{nullptr};
        GdipCloneBrush((Brush *) this, &tmp);
        return new TextureBrush{tmp};
    }

private:
    GpTexture *_gpTexture;
};

class PathGradientBrush : public BrushBase {
public:
    PathGradientBrush(Brush *brush) {
        if (((BrushClass *) brush)->type != BrushTypePathGradient) {
            throw std::invalid_argument{
                    "can't init PathGradientBrush Object with not BrushTypePathGradient type brush ptr"
            };
        }

        this->_gpPathG = reinterpret_cast<GpPathGradient *>(brush);
    }

    PathGradientBrush(const PointF *points, int count, WrapMode wrapMode) {
        GdipCreatePathGradient(points, count, wrapMode, &this->_gpPathG);
    }

    PathGradientBrush(const PathGradientBrush &) = delete;

    explicit operator Brush *() const override {
        return (Brush *) this->_gpPathG;
    }

    [[nodiscard]] BrushBase *Clone() const override {
        Brush *tmp{nullptr};
        GdipCloneBrush((Brush *) this, &tmp);
        return new PathGradientBrush{tmp};
    }

    GpStatus SetCenterColor(const Color &color) {
        this->gpStatus = GdipSetPathGradientCenterColor(
                this->_gpPathG, *(ARGB *) &color
        );
        return this->gpStatus;
    }

    GpStatus SetCenterPoint(const Point &point) {
        return this->SetCenterPoint(
                PointF{(float) point.X, (float) point.Y}
        );
    }

    GpStatus SetCenterPoint(const PointF &point) {
        this->gpStatus = GdipSetPathGradientCenterPoint(
                this->_gpPathG, &point
        );
        return this->gpStatus;
    }

    GpStatus SetFocusScales(float xScale, float yScale) {
        this->gpStatus = GdipSetPathGradientFocusScales(
                this->_gpPathG, xScale, yScale
        );
        return this->gpStatus;
    }

    GpStatus SetSurroundColors(const Color *colors, int *count) {
        this->gpStatus = GdipSetPathGradientSurroundColorsWithCount(
                this->_gpPathG, (ARGB *) &colors, count
        );
        return this->gpStatus;
    }


    GpStatus SetBlend(
            const float *blendFactors,
            const float *blendPositions, int count
    ) {
        this->gpStatus = GdipSetPathGradientBlend(
                this->_gpPathG, blendFactors, blendPositions, count
        );
        return this->gpStatus;
    }

    GpStatus SetBlendBellShape(float focus, float scale) {
        this->gpStatus = GdipSetPathGradientSigmaBlend(this->_gpPathG, focus, scale);
        return this->gpStatus;
    }

    GpStatus SetBlendTriangularShape(float focus, float scale = 1) {
        this->gpStatus = GdipSetPathGradientLinearBlend(this->_gpPathG, focus, scale);
        return this->gpStatus;
    }

    GpStatus SetGammaCorrection(bool /*useGammaCorrection*/) {
        // 5.6.1 GdipGetPathGradientGammaCorrection doesn't impl
        // 6.x.x GdipGetPathGradientGammaCorrection is available
        // this->gpStatus = GdipSetPathGradientGammaCorrection(this->_gpPathG, useGammaCorrection);
        // you can ignore it
        // printf("warning PathGradientBrush SetGammaCorrection current not impl");
        return this->gpStatus = Ok;
    }

    GpStatus SetInterpolationColors(
            const Color *presetColors,
            const float *blendPositions,
            int count
    ) {
        this->gpStatus = GdipSetPathGradientPresetBlend(
                this->_gpPathG, (const ARGB *) presetColors,
                blendPositions, count
        );
        return this->gpStatus;
    }

private:
    GpPathGradient *_gpPathG{nullptr};
};

class LinearGradientBrush : public BrushBase {
public:

    LinearGradientBrush(Brush *brush) {
        if (((BrushClass *) brush)->type != BrushTypeLinearGradient) {
            throw std::invalid_argument{
                    "can't init LinearGradientBrush Object with not BrushTypeLinearGradient type brush ptr"
            };
        }

        this->_gpLG = reinterpret_cast<GpLineGradient *>(brush);
    }

    LinearGradientBrush(
            const PointF &point1, const PointF &point2,
            const Color &color1, const Color &color2
    ) {
        GdipCreateLineBrush(
                &point1, &point2,
                *(ARGB *) &color1, *(ARGB *) &color2,
                WrapModeTile, &this->_gpLG
        );
    }

    LinearGradientBrush(
            const RectF &rect,
            const Color &color1, const Color &color2,
            float angle, bool isAngleScalable
    ) {
        GdipCreateLineBrushFromRectWithAngle(
                &rect,
                *(ARGB *) &color1, *(ARGB *) &color2,
                angle, isAngleScalable, WrapModeTile,
                &this->_gpLG
        );
    }

    LinearGradientBrush(
            const RectF &rect,
            const Color &color1,
            const Color &color2,
            LinearGradientMode mode
    ) {
        GdipCreateLineBrushFromRect(
                &rect,
                *(ARGB *) &color1, *(ARGB *) &color2,
                mode, WrapModeTile, &this->_gpLG
        );
    }

    LinearGradientBrush(const LinearGradientBrush &) = delete;

    explicit operator Brush *() const override {
        return (Brush *) this->_gpLG;
    }

    [[nodiscard]] BrushBase *Clone() const override {
        Brush *tmp{nullptr};
        GdipCloneBrush((Brush *) this, &tmp);
        return new LinearGradientBrush{tmp};
    }

    GpStatus SetWrapMode(WrapMode wrapMode) {
        this->gpStatus = GdipSetLineWrapMode(this->_gpLG, wrapMode);
        return this->gpStatus;
    }

    GpStatus SetBlend(
            const float *blendFactors,
            const float *blendPositions, int count
    ) {
        this->gpStatus = GdipSetLineBlend(
                this->_gpLG, blendFactors, blendPositions, count
        );
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
        this->gpStatus = GdipSetLineGammaCorrection(
                this->_gpLG, useGammaCorrection
        );
        return this->gpStatus;
    }

    GpStatus SetInterpolationColors(
            const Color *presetColors,
            const float *blendPositions,
            int count
    ) {
        this->gpStatus = GdipSetLinePresetBlend(
                this->_gpLG, (const ARGB *) presetColors,
                blendPositions, count
        );
        return this->gpStatus;
    }

private:
    GpLineGradient *_gpLG{nullptr};
};

#endif //KRKR2_GDIP_CXX_BRUSH_H
