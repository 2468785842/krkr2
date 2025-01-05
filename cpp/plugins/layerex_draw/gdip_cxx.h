//
// Created by lidong on 2025/1/5.
//

#ifndef KRKR2_GDIP_CXX_H
#define KRKR2_GDIP_CXX_H

#include <win32_dt.h>
#include <gdiplus-private.h>
#include <bitmap-private.h>
#include <graphics-private.h>
#include <graphics-path-private.h>
#include <brush-private.h>
#include <customlinecap-private.h>
#include <matrix-private.h>
#include <image-private.h>
#include <pen-private.h>
#include <lineargradientbrush-private.h>
#include <customlinecap-private.h>

class PointFClass : public PointF {
public:
    PointFClass() = default;

    PointFClass(PointF pf) : PointFClass{pf.X, pf.Y} {}

    PointFClass(float x, float y) : PointF{x, y} {}

    [[nodiscard]] bool Equals(const PointFClass &p) {
        return p.X == this->X && p.Y == this->Y;
    }

};

class RectFClass : public RectF {
public:
    RectFClass() = default;

    RectFClass(RectF rf) : RectF{rf.X, rf.Y, rf.Width, rf.Height} {}

    RectFClass(float x, float y, float w, float h) : RectF{x, y, w, h} {}

    [[nodiscard]] bool Equals(const RectFClass &p) const {
        return p.X == this->X && p.Y == this->Y
               && p.Width == this->Width
               && p.Height == this->Height;
    }

    [[nodiscard]] float GetLeft() const {
        return this->X;
    }

    [[nodiscard]] float GetTop() const {
        return this->Y;
    }

    [[nodiscard]] float GetRight() const {
        return this->X + this->Width;
    }

    [[nodiscard]] float GetBottom() const {
        return this->Y + this->Height;
    }

    [[nodiscard]] bool IntersectsWith(const RectFClass &rect) const {
        return this->GetRight() > rect.GetLeft()
               && this->GetLeft() < rect.GetRight()
               && this->GetBottom() > rect.GetTop()
               && this->GetTop() < rect.GetBottom();
    }

    [[nodiscard]] bool IsEmptyArea() const {
        return this->Width <= 0 || this->Height <= 0;
    }

    void Offset(const PointFClass &point) {
        Offset(point.X, point.Y);
    }

    void Offset(float dx, float dy) {
        this->X += dx;
        this->Y += dy;
    }

    static bool Union(RectFClass &c, const RectFClass &a, const RectFClass &b) {
        float minX = min(a.X, b.X);
        float minY = min(a.Y, b.Y);
        float maxX = max(a.GetRight(), b.GetRight());
        float maxY = max(a.GetBottom(), b.GetBottom());

        float width = maxX - minX;
        float height = maxY - minY;

        c = RectFClass{minX, minY, width, height};
        return !(a.IsEmptyArea() && b.IsEmptyArea());
    }

    void GetLocation(PointF *point) const {
        *point = PointF{this->X, this->Y};
    }

    void GetBounds(RectFClass *rfc) const {
        *rfc = *this;
    }

    void Inflate(const PointFClass &point) {
        this->Inflate(point.X, point.Y);
    }

    void Inflate(float dx, float dy) {
        this->X -= dx;
        this->Y -= dy;
        this->Width += dx * 2;
        this->Height += dy * 2;
    }

    [[nodiscard]] RectFClass *Clone() const {
        return new RectFClass{*this};
    }

    ~RectFClass() = default;

};

class MatrixClass {

public:
    MatrixClass() {
        this->mGpStatus = GdipCreateMatrix(&mGpMatrix);
    }

    MatrixClass(const GpRectF &rect, const GpPointF &point) {
        this->mGpStatus = GdipCreateMatrix3(&rect, &point, &mGpMatrix);
    }

    MatrixClass(float m11, float m12, float m21, float m22, float dx, float dy) {
        this->mGpStatus = GdipCreateMatrix2(m11, m12, m21, m22, dx, dy, &mGpMatrix);
    }

    [[nodiscard]] float OffsetX() {
        return static_cast<float>(mGpMatrix->x0);
    }

    [[nodiscard]] float OffsetY() {
        return static_cast<float>(mGpMatrix->y0);
    }

    [[nodiscard]] bool Equals(MatrixClass *matrix) const {
        return this->mGpMatrix->xx == matrix->mGpMatrix->xx
               && this->mGpMatrix->yx == matrix->mGpMatrix->yx
               && this->mGpMatrix->xy == matrix->mGpMatrix->xy
               && this->mGpMatrix->yy == matrix->mGpMatrix->yy
               && this->mGpMatrix->x0 == matrix->mGpMatrix->x0
               && this->mGpMatrix->y0 == matrix->mGpMatrix->y0;
    }

    GpStatus SetElements(float m11, float m12, float m21, float m22, float dx, float dy) {
        this->mGpStatus = GdipSetMatrixElements(this->mGpMatrix, m11, m12, m21, m22, dx, dy);
        return this->mGpStatus;
    }

    [[nodiscard]] GpStatus GetLastStatus() const {
        return this->mGpStatus;
    }

    [[nodiscard]] bool IsInvertible() {
        BOOL r = false;
        this->mGpStatus = GdipIsMatrixInvertible(this->mGpMatrix, &r);
        return r;
    }

    GpStatus Invert() {
        this->mGpStatus = GdipInvertMatrix(mGpMatrix);
        return this->mGpStatus;
    }

    [[nodiscard]] bool IsIdentity() {
        BOOL r = false;
        this->mGpStatus = GdipIsMatrixIdentity(mGpMatrix, &r);
        return r;
    }

    GpStatus Multiply(MatrixClass *matrix, MatrixOrder order) {
        this->mGpStatus = GdipMultiplyMatrix(this->mGpMatrix, matrix->mGpMatrix, order);
        return this->mGpStatus;
    }

    GpStatus Reset() {
        if (!this->mGpMatrix) {
            this->mGpStatus = InvalidParameter;
            return this->mGpStatus;
        }

        this->mGpMatrix->xx = 1.0;  // 缩放因子 x
        this->mGpMatrix->xy = 0.0;  // 倾斜因子 x
        this->mGpMatrix->yx = 0.0;  // 倾斜因子 y
        this->mGpMatrix->yy = 1.0;  // 缩放因子 y
        this->mGpMatrix->x0 = 0.0;  // 平移量 x
        this->mGpMatrix->y0 = 0.0;  // 平移量 y

        this->mGpStatus = Ok;
        return this->mGpStatus;
    }

    GpStatus Rotate(float angle, MatrixOrder order) {
        this->mGpStatus = GdipRotateMatrix(this->mGpMatrix, angle, order);
        return this->mGpStatus;
    }

    GpStatus Translate(float offsetX, float offsetY, MatrixOrder order) {
        this->mGpStatus = GdipTranslateMatrix(this->mGpMatrix, offsetX, offsetY, order);
        return this->mGpStatus;
    }

    GpStatus RotateAt(float angle, const PointFClass &center, MatrixOrder order) {
        this->Translate(-center.X, -center.Y, order);
        this->Rotate(angle, order);
        this->Translate(center.X, center.Y, order);
        return this->mGpStatus;
    }

    GpStatus Scale(float scaleX, float scaleY, MatrixOrder order) {
        this->mGpStatus = GdipScaleMatrix(this->mGpMatrix, scaleX, scaleY, order);
        return this->mGpStatus;
    }

    GpStatus Shear(float shearX, float shearY, MatrixOrder order) {
        this->mGpStatus = GdipShearMatrix(this->mGpMatrix, shearX, shearY, order);
        return this->mGpStatus;
    }

    ~MatrixClass() {
        GdipDeleteMatrix(mGpMatrix);
    }

private:
    GpMatrix *mGpMatrix{nullptr};
    GpStatus mGpStatus;
};

class ImageClass : public GpImage {

};

#endif //KRKR2_GDIP_CXX_H
