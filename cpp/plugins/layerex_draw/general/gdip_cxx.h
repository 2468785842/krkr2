//
// Created by lidong on 2025/1/5.
// more:
// https://learn.microsoft.com/en-us/windows/win32/api/gdiplusheaders
//

#ifndef KRKR2_GDIP_CXX_H
#define KRKR2_GDIP_CXX_H
#include <cassert>

extern "C" {
#include <libgdiplus/gdiplus-private.h>
#include <libgdiplus/gdipenums.h>
#include <libgdiplus/bitmap-private.h>
#include <libgdiplus/graphics-private.h>
#include <libgdiplus/graphics-path-private.h>
#include <libgdiplus/customlinecap-private.h>
#include <libgdiplus/matrix-private.h>
#include <libgdiplus/image-private.h>
#include <libgdiplus/pen-private.h>
#include <libgdiplus/customlinecap-private.h>
#include <libgdiplus/fontfamily-private.h>
#include <libgdiplus/fontcollection-private.h>
#include <libgdiplus/font-private.h>
#include <libgdiplus/adjustablearrowcap-private.h>
}

#include <win32_dt.h>

class PointFClass : public PointF {
public:
    PointFClass() = default;

    PointFClass(PointF pf) : PointFClass{ pf.X, pf.Y } {}

    PointFClass(float x, float y) : PointF{ x, y } {}

    [[nodiscard]] bool Equals(const PointFClass &p) {
        return p.X == this->X && p.Y == this->Y;
    }

    PointFClass operator-(const PointF &point) {
        return PointFClass{ this->X - point.X, this->Y - point.Y };
    }

    PointFClass operator+(const PointF &point) {
        return PointFClass{ this->X + point.X, this->Y + point.Y };
    }

    PointFClass *operator+=(const PointF &point) {
        this->X = this->X + point.X;
        this->Y = this->Y + point.Y;
        return this;
    }

    template <typename T>
    PointFClass operator/(T number) {
        return PointFClass{ this->X / number, this->Y / number };
    }

    template <typename T>
    PointFClass operator*(T number) {
        return PointFClass{ this->X * number, this->Y * number };
    }

    template <typename T>
    PointFClass *operator*=(T number) {
        this->X = this->X * number;
        this->Y = this->Y * number;
        return this;
    }
};

class RectFClass : public RectF {
public:
    RectFClass() = default;

    RectFClass(RectF rf) : RectF{ rf.X, rf.Y, rf.Width, rf.Height } {}

    RectFClass(float x, float y, float w, float h) : RectF{ x, y, w, h } {}

    [[nodiscard]] bool Equals(const RectFClass &p) const {
        return p.X == this->X && p.Y == this->Y && p.Width == this->Width &&
            p.Height == this->Height;
    }

    [[nodiscard]] float GetLeft() const { return this->X; }

    [[nodiscard]] float GetTop() const { return this->Y; }

    [[nodiscard]] float GetRight() const { return this->X + this->Width; }

    [[nodiscard]] float GetBottom() const { return this->Y + this->Height; }

    [[nodiscard]] bool IntersectsWith(const RectFClass &rect) const {
        return this->GetRight() > rect.GetLeft() &&
            this->GetLeft() < rect.GetRight() &&
            this->GetBottom() > rect.GetTop() &&
            this->GetTop() < rect.GetBottom();
    }

    [[nodiscard]] bool IsEmptyArea() const {
        return this->Width <= 0 || this->Height <= 0;
    }

    void Offset(const PointFClass &point) { this->Offset(point.X, point.Y); }

    void Offset(float dx, float dy) {
        this->X += dx;
        this->Y += dy;
    }

    static bool Union(RectFClass &c, const RectFClass &a, const RectFClass &b) {
        if(a.IsEmptyArea() && b.IsEmptyArea()) {
            // 如果两个矩形都为空，结果也为空
            c = RectFClass{};
            return false;
        }

        if(a.IsEmptyArea()) {
            // 如果 a 为空，结果是 b
            c = b;
            return true;
        }

        if(b.IsEmptyArea()) {
            // 如果 b 为空，结果是 a
            c = a;
            return true;
        }

        // 正常计算并集
        float minX = min(a.X, b.X);
        float minY = min(a.Y, b.Y);
        float maxX = max(a.GetRight(), b.GetRight());
        float maxY = max(a.GetBottom(), b.GetBottom());

        float width = maxX - minX;
        float height = maxY - minY;

        c = RectFClass{ minX, minY, width, height };
        return true;
    }

    void GetLocation(PointF *point) const {
        *point = PointF{ this->X, this->Y };
    }

    void GetBounds(RectFClass *rfc) const { *rfc = *this; }

    void Inflate(const PointFClass &point) { this->Inflate(point.X, point.Y); }

    void Inflate(float dx, float dy) {
        this->X -= dx;
        this->Y -= dy;
        this->Width += dx * 2;
        this->Height += dy * 2;
    }

    [[nodiscard]] RectFClass *Clone() const { return new RectFClass{ *this }; }

    ~RectFClass() = default;
};

class MatrixClass {

public:
    MatrixClass(GpMatrix *matrix) : _gpMatrix(matrix) {}

    MatrixClass() { this->_gpStatus = GdipCreateMatrix(&_gpMatrix); }

    MatrixClass(const GpRectF &rect, const GpPointF &point) {
        this->_gpStatus = GdipCreateMatrix3(&rect, &point, &_gpMatrix);
    }

    MatrixClass(float m11, float m12, float m21, float m22, float dx,
                float dy) {
        this->_gpStatus =
            GdipCreateMatrix2(m11, m12, m21, m22, dx, dy, &_gpMatrix);
    }

    [[nodiscard]] float OffsetX() const {
        return static_cast<float>(_gpMatrix->x0);
    }

    [[nodiscard]] float OffsetY() const {
        return static_cast<float>(_gpMatrix->y0);
    }

    [[nodiscard]] bool Equals(MatrixClass *matrix) const {
        return this->_gpMatrix->xx == matrix->_gpMatrix->xx &&
            this->_gpMatrix->yx == matrix->_gpMatrix->yx &&
            this->_gpMatrix->xy == matrix->_gpMatrix->xy &&
            this->_gpMatrix->yy == matrix->_gpMatrix->yy &&
            this->_gpMatrix->x0 == matrix->_gpMatrix->x0 &&
            this->_gpMatrix->y0 == matrix->_gpMatrix->y0;
    }

    GpStatus SetElements(float m11, float m12, float m21, float m22, float dx,
                         float dy) {
        this->_gpStatus =
            GdipSetMatrixElements(this->_gpMatrix, m11, m12, m21, m22, dx, dy);
        return this->_gpStatus;
    }

    [[nodiscard]] GpStatus GetLastStatus() const { return this->_gpStatus; }

    [[nodiscard]] bool IsInvertible() const {
        BOOL r = false;
        this->_gpStatus = GdipIsMatrixInvertible(this->_gpMatrix, &r);
        return r;
    }

    GpStatus Invert() {
        this->_gpStatus = GdipInvertMatrix(_gpMatrix);
        return this->_gpStatus;
    }

    [[nodiscard]] bool IsIdentity() const {
        BOOL r = false;
        this->_gpStatus = GdipIsMatrixIdentity(_gpMatrix, &r);
        return r;
    }

    GpStatus Multiply(MatrixClass *matrix,
                      MatrixOrder order = MatrixOrderPrepend) {
        this->_gpStatus =
            GdipMultiplyMatrix(this->_gpMatrix, matrix->_gpMatrix, order);
        return this->_gpStatus;
    }

    GpStatus Reset() {
        if(!this->_gpMatrix) {
            this->_gpStatus = InvalidParameter;
            return this->_gpStatus;
        }

        this->_gpMatrix->xx = 1.0; // 缩放因子 x
        this->_gpMatrix->xy = 0.0; // 倾斜因子 x
        this->_gpMatrix->yx = 0.0; // 倾斜因子 y
        this->_gpMatrix->yy = 1.0; // 缩放因子 y
        this->_gpMatrix->x0 = 0.0; // 平移量 x
        this->_gpMatrix->y0 = 0.0; // 平移量 y

        this->_gpStatus = Ok;
        return this->_gpStatus;
    }

    GpStatus Rotate(float angle, MatrixOrder order) {
        this->_gpStatus = GdipRotateMatrix(this->_gpMatrix, angle, order);
        return this->_gpStatus;
    }

    GpStatus Translate(float offsetX, float offsetY, MatrixOrder order) {
        this->_gpStatus =
            GdipTranslateMatrix(this->_gpMatrix, offsetX, offsetY, order);
        return this->_gpStatus;
    }

    GpStatus RotateAt(float angle, const PointFClass &center,
                      MatrixOrder order) {
        this->Translate(-center.X, -center.Y, order);
        this->Rotate(angle, order);
        this->Translate(center.X, center.Y, order);
        return this->_gpStatus;
    }

    GpStatus Scale(float scaleX, float scaleY, MatrixOrder order) {
        this->_gpStatus =
            GdipScaleMatrix(this->_gpMatrix, scaleX, scaleY, order);
        return this->_gpStatus;
    }

    GpStatus Shear(float shearX, float shearY, MatrixOrder order) {
        this->_gpStatus =
            GdipShearMatrix(this->_gpMatrix, shearX, shearY, order);
        return this->_gpStatus;
    }

    GpStatus TransformPoints(PointFClass *pts, int count) {
        this->_gpStatus =
            GdipTransformMatrixPoints(this->_gpMatrix, pts, count);
        return this->_gpStatus;
    }

    [[nodiscard]] explicit operator GpMatrix *() const {
        return this->_gpMatrix;
    }

    ~MatrixClass() { GdipDeleteMatrix(_gpMatrix); }

private:
    GpMatrix *_gpMatrix{ nullptr };
    mutable GpStatus _gpStatus;
};

class ImageClass {
public:
    ImageClass(GpImage *gpImage) { this->_gpImage = gpImage; }

    static ImageClass *FromFile(const WCHAR *filename,
                                bool useEmbeddedColorManagement) {
        return new ImageClass{ filename, useEmbeddedColorManagement };
    }

    ImageClass(const WCHAR *stream, bool useEmbeddedColorManagement = false) {
        if(useEmbeddedColorManagement)
            this->_gpStatus = GdipLoadImageFromFileICM(stream, &this->_gpImage);
        else
            this->_gpStatus = GdipLoadImageFromFile(stream, &this->_gpImage);
    }

    [[nodiscard]] GpStatus GetLastStatus() const { return this->_gpStatus; }

    GpStatus GetBounds(RectFClass *srcRect, Unit *srcUnit) const {
        this->_gpStatus = GdipGetImageBounds(this->_gpImage, srcRect, srcUnit);
        return this->_gpStatus;
    }

    [[nodiscard]] float GetHorizontalResolution() const {
        float hr;
        this->_gpStatus = GdipGetImageHorizontalResolution(this->_gpImage, &hr);
        return hr;
    }

    [[nodiscard]] float GetVerticalResolution() const {
        float vr;
        this->_gpStatus = GdipGetImageVerticalResolution(this->_gpImage, &vr);
        return vr;
    }

    [[nodiscard]] PixelFormat GetPixelFormat() const {
        PixelFormat pf;
        this->_gpStatus = GdipGetImagePixelFormat(this->_gpImage, &pf);
        return pf;
    }

    [[nodiscard]] ImageType GetType() const {
        ImageType it;
        this->_gpStatus = GdipGetImageType(this->_gpImage, &it);
        return it;
    }

    [[nodiscard]] uint GetFlags() const {
        uint f;
        this->_gpStatus = GdipGetImageFlags(this->_gpImage, &f);
        return f;
    }

    [[nodiscard]] uint GetHeight() const {
        uint h;
        this->_gpStatus = GdipGetImageHeight(this->_gpImage, &h);
        return h;
    }

    [[nodiscard]] uint GetWidth() const {
        uint w;
        this->_gpStatus = GdipGetImageWidth(this->_gpImage, &w);
        return w;
    }

    [[nodiscard]] ImageClass *Clone() const {
        GpImage *image{ nullptr };
        this->_gpStatus = GdipCloneImage(this->_gpImage, &image);
        return new ImageClass{ image };
    }

    [[nodiscard]] explicit operator GpImage *() const { return this->_gpImage; }

    GpStatus RotateFlip(RotateFlipType rotateFlipType) {
        this->_gpStatus = GdipImageRotateFlip(this->_gpImage, rotateFlipType);
        return this->_gpStatus;
    }

    ~ImageClass() { GdipDisposeImage(this->_gpImage); }

private:
    GpImage *_gpImage{ nullptr };
    mutable GpStatus _gpStatus;
};

class PrivateFontCollection {
public:
    PrivateFontCollection() { GdipNewPrivateFontCollection(&_gpFC); };

    void AddFontFile(const WCHAR *filename) {
        this->status = GdipPrivateAddFontFile(this->_gpFC, filename);
        assert(this->status == Ok && "add font file failed!");
    }

    // android mkstemp无法正常使用
    //    void AddMemoryFont(const void *memory, size_t length) {
    //        this->status = GdipPrivateAddMemoryFont(this->_gpFC,
    //        memory, static_cast<int>(length)); g_assert(status == Ok
    //        && "add memory font failed");
    //    }

    [[nodiscard]] GpFontCollection *getFontCollection() { return this->_gpFC; }

    ~PrivateFontCollection() { GdipDeletePrivateFontCollection(&_gpFC); }

private:
    GpFontCollection *_gpFC{ nullptr };
    GpStatus status;
};

typedef enum {
    EncoderValueColorTypeCMYK = 0,
    EncoderValueColorTypeYCCK = 1,
    EncoderValueCompressionLZW = 2,
    EncoderValueCompressionCCITT3 = 3,
    EncoderValueCompressionCCITT4 = 4,
    EncoderValueCompressionRle = 5,
    EncoderValueCompressionNone = 6,
    EncoderValueScanMethodInterlaced = 7,
    EncoderValueScanMethodNonInterlaced = 8,
    EncoderValueVersionGif87 = 9,
    EncoderValueVersionGif89 = 10,
    EncoderValueRenderProgressive = 11,
    EncoderValueRenderNonProgressive = 12,
    EncoderValueTransformRotate90 = 13,
    EncoderValueTransformRotate180 = 14,
    EncoderValueTransformRotate270 = 15,
    EncoderValueTransformFlipHorizontal = 16,
    EncoderValueTransformFlipVertical = 17
} EncoderValue;

static constexpr auto FW_BOLD = 700;
static constexpr auto FW_REGULAR = 400;

// https://learn.microsoft.com/en-us/openspecs/office_file_formats/ms-one/64e2db6e-6eeb-443c-9ccf-0f72b37ba411
typedef enum { ANSI_CHARSET = 0, DEFAULT_CHARSET = 1 } Charset;

/*
 * encoder param guids
 */
static const GUID GdipEncoderScanMethod = { 0x3A4E2661,
                                            0x3109,
                                            0x4E56,
                                            { 0x85, 0x36, 0x42, 0xC1, 0x56,
                                              0xE7, 0xDC, 0xFA } };

static const GUID GdipEncoderVersion = { 0x24D18C76,
                                         0x814A,
                                         0x41A4,
                                         { 0xBF, 0x53, 0x1C, 0x21, 0x9C, 0xCC,
                                           0xF7, 0x97 } };

static const GUID GdipEncoderRenderMethod = { 0x6D42C53A,
                                              0x229A,
                                              0x4825,
                                              { 0x8B, 0xB7, 0x5C, 0x99, 0xE2,
                                                0xB9, 0xA8, 0xB8 } };

static const GUID GdipEncoderCompression = { 0x0E09D739DU,
                                             0x0CCD4U,
                                             0x44EEU,
                                             { 0x8E, 0x0BA, 0x3F, 0x0BF, 0x8B,
                                               0x0E4, 0x0FC, 0x58 } };

static const GUID GdipEncoderColorDepth = { 0x66087055U,
                                            0x0AD66U,
                                            0x4C7CU,
                                            { 0x9A, 0x18, 0x38, 0x0A2, 0x31,
                                              0x0B, 0x83, 0x37 } };

static const GUID GdipEncoderSaveFlag = { 0x292266FCU,
                                          0x0AC40U,
                                          0x47BFU,
                                          { 0x8C, 0x0FC, 0x0A8, 0x5B, 0x89,
                                            0x0A6, 0x55, 0x0DE } };

static const GUID GdipEncoderSaveAsCMYK = { 0x0A219BBC9U,
                                            0x0A9DU,
                                            0x4005U,
                                            { 0x0A3, 0x0EE, 0x3A, 0x42, 0x1B,
                                              0x8B, 0x0B0, 0x6C } };

static const GUID GdipEncoderImageItems = { 0x63875E13U,
                                            0x1F1DU,
                                            0x45ABU,
                                            { 0x91, 0x95, 0x0A2, 0x9B, 0x60,
                                              0x66, 0x0A6, 0x50 } };

static const GUID GdipEncoderTransformation = { 0x8D0EB2D1U,
                                                0x0A58EU,
                                                0x4EA8U,
                                                { 0x0AA, 0x14, 0x10, 0x80, 0x74,
                                                  0x0B7, 0x0B6, 0x0F9 } };
static const GUID GdipEncoderQuality = { 0x1D5BE4B5U,
                                         0x0FA4AU,
                                         0x452DU,
                                         { 0x9C, 0x0DD, 0x5D, 0x0B3, 0x51, 0x5,
                                           0x0E7, 0x0EB } };

static const GUID GdipEncoderLuminanceTable = { 0x0EDB33BCEU,
                                                0x266U,
                                                0x4A77U,
                                                { 0x0B9, 0x4, 0x27, 0x21, 0x60,
                                                  0x99, 0x0E7, 0x17 } };

static const GUID GdipEncoderChrominanceTable = { 0x0F2E455DCU,
                                                  0x9B3U,
                                                  0x4316U,
                                                  { 0x82, 0x60, 0x67, 0x6A,
                                                    0x0DA, 0x32, 0x48, 0x1C } };

static const CLSID bmpEncoderClsid = {
    0x557cf400, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e }
};
static const CLSID tifEncoderClsid = {
    0x557cf405, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e }
};
static const CLSID gifEncoderClsid = {
    0x557cf402, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e }
};
static const CLSID pngEncoderClsid = {
    0x557cf406, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e }
};
static const CLSID jpegEncoderClsid = {
    0x557cf401, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e }
};
static const CLSID icoEncoderClsid = {
    0x557cf407, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e }
};
static const CLSID wmfEncoderClsid = {
    0x557cf404, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e }
};
static const CLSID emfEncoderClsid = {
    0x557cf403, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e }
};

#endif // KRKR2_GDIP_CXX_H
