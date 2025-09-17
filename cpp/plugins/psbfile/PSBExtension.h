//
// Created by lidong on 25-3-18.
//
#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>
#include <boost/locale.hpp>
#include "tjs.h"

#include "PSBEnums.h"

namespace PSB::Extension {

    static int getSize(std::uint32_t i) {
        int n = 0;
        do {
            i >>= 8;
            n++;
        } while(i != 0);
        return n;
    }

    static void readAndUnzip(TJS::tTJSBinaryStream *stream, std::uint8_t size,
                             std::vector<std::uint8_t> &data,
                             bool usigned = false) {
        stream->Read(data.data(), size);

        std::uint8_t fill = 0x0;
        if(!usigned && data[size - 1] >= 0b10000000) { // negative
            fill = 0xFF;
        }

        for(int i = 0; i < data.size(); i++) {
            data[i] = i < size ? data[i] : fill;
        }
    }

    static std::string readStringZeroTrim(TJS::tTJSBinaryStream *stream) {
        std::string name;

        while(true) {
            constexpr size_t CHUNK_SIZE = 256;
            char temp[CHUNK_SIZE]; // default encoding: UTF-8
            const size_t bytes_read = stream->Read(temp, CHUNK_SIZE);
            if(bytes_read == 0)
                break;

            for(size_t j = 0; j < bytes_read; ++j) {
                unsigned char uc = temp[j];
                if(uc == '\0') {
                    name.append(temp, j);
                    return std::move(name);
                }
            }
            name.append(temp, bytes_read);
        }

        return "";
    }

    struct RectangleF {
        float X;
        float Y;
        float Width;
        float Height;

        // 构造函数
        RectangleF(float x = 0.0f, float y = 0.0f, float width = 0.0f,
                   float height = 0.0f) :
            X(x), Y(y), Width(width), Height(height) {}

        // 根据左、上、右、下边界创建矩形
        static RectangleF FromLTRB(float left, float top, float right,
                                   float bottom) {
            return { left, top, right - left, bottom - top };
        }

        // 获取边界
        [[nodiscard]] float Left() const { return X; }
        [[nodiscard]] float Right() const { return X + Width; }
        [[nodiscard]] float Top() const { return Y; }
        [[nodiscard]] float Bottom() const { return Y + Height; }

        // 判断点是否在矩形内
        [[nodiscard]] bool Contains(float x, float y) const {
            float effectiveLeft = std::min(X, X + Width);
            float effectiveRight = std::max(X, X + Width);
            float effectiveTop = std::min(Y, Y + Height);
            float effectiveBottom = std::max(Y, Y + Height);

            return (x >= effectiveLeft) && (x < effectiveRight) &&
                (y >= effectiveTop) && (y < effectiveBottom);
        }

        // 判断矩形是否完全在当前矩形内
        [[nodiscard]] bool Contains(const RectangleF &rect) const {
            float currLeft = std::min(X, X + Width);
            float currRight = std::max(X, X + Width);
            float currTop = std::min(Y, Y + Height);
            float currBottom = std::max(Y, Y + Height);

            float rectLeft = std::min(rect.X, rect.X + rect.Width);
            float rectRight = std::max(rect.X, rect.X + rect.Width);
            float rectTop = std::min(rect.Y, rect.Y + rect.Height);
            float rectBottom = std::max(rect.Y, rect.Y + rect.Height);

            return (rectLeft >= currLeft) && (rectRight <= currRight) &&
                (rectTop >= currTop) && (rectBottom <= currBottom);
        }

        // 判断是否与另一矩形相交
        [[nodiscard]] bool IntersectsWith(const RectangleF &rect) const {
            return (rect.Left() < Right() && rect.Right() > Left() &&
                    rect.Top() < Bottom() && rect.Bottom() > Top());
        }

        // 修改当前矩形为与另一矩形的交集
        void Intersect(const RectangleF &rect) {
            float interLeft = std::max(Left(), rect.Left());
            float interRight = std::min(Right(), rect.Right());
            float interTop = std::max(Top(), rect.Top());
            float interBottom = std::min(Bottom(), rect.Bottom());

            if(interLeft >= interRight || interTop >= interBottom) {
                X = Y = Width = Height = 0.0f; // 设为空矩形
            } else {
                X = interLeft;
                Y = interTop;
                Width = interRight - interLeft;
                Height = interBottom - interTop;
            }
        }

        // 静态方法：返回两个矩形的交集
        static RectangleF Intersect(const RectangleF &a, const RectangleF &b) {
            float interLeft = std::max(a.Left(), b.Left());
            float interRight = std::min(a.Right(), b.Right());
            float interTop = std::max(a.Top(), b.Top());
            float interBottom = std::min(a.Bottom(), b.Bottom());

            if(interLeft >= interRight || interTop >= interBottom) {
                return {}; // 空矩形
            }
            return { interLeft, interTop, interRight - interLeft,
                     interBottom - interTop };
        }

        // 静态方法：返回两个矩形的并集
        static RectangleF Union(const RectangleF &a, const RectangleF &b) {
            float unionLeft = std::min(a.Left(), b.Left());
            float unionRight = std::max(a.Right(), b.Right());
            float unionTop = std::min(a.Top(), b.Top());
            float unionBottom = std::max(a.Bottom(), b.Bottom());

            return { unionLeft, unionTop, unionRight - unionLeft,
                     unionBottom - unionTop };
        }

        // 判断矩形是否为空（Width 和 Height 均为 0）
        [[nodiscard]] bool IsEmpty() const {
            return Width == 0.0f && Height == 0.0f;
        }

        // 运算符重载
        bool operator==(const RectangleF &other) const {
            return (X == other.X && Y == other.Y && Width == other.Width &&
                    Height == other.Height);
        }

        bool operator!=(const RectangleF &other) const {
            return !(*this == other);
        }
    };

    /// Whether the PSBSpec should use PostProcessing.TileTexture
    static bool useTile(PSBSpec spec) {
        switch(spec) {
            case PSBSpec::PS4:
            case PSBSpec::Revo:
                return true;
            default:
                return false;
        }
    }

    /// Whether the PSBSpec should use BigEndian
    static bool useBigEndian(PSBSpec spec) {
        switch(spec) {
            case PSBSpec::Common:
            case PSBSpec::Ems:
            case PSBSpec::Vita: // TODO: is vita or psp BigEndian?
            case PSBSpec::PSP:
            case PSBSpec::PS3: // TODO: is ps3 BigEndian?
                return true;
            default:
                return false;
        }
    }

    static PSBPixelFormat toPSBPixelFormat(const std::string &typeStr,
                                           PSBSpec spec) {
        if(typeStr.empty()) {
            return PSBPixelFormat::None;
        }

        bool isUseTile = useTile(spec);

        const auto &tmp = boost::locale::to_upper(typeStr);
        if(tmp == "CI4") {
            return spec == PSBSpec::Revo ? PSBPixelFormat::TileCI4
                                         : PSBPixelFormat::CI4_SW_PSP;
        }

        if(tmp == "CI8") {
            switch(spec) {
                // TODO: PS4?
                case PSBSpec::PSP:
                    return PSBPixelFormat::CI8_SW_PSP;
                case PSBSpec::Revo:
                    return PSBPixelFormat::TileCI8;
                default:
                    return PSBPixelFormat::CI8;
            }
        }
        if(tmp == "CI4_SW") {
            return PSBPixelFormat::CI4_SW;
        }
        if(tmp == "CI8_SW") {
            return PSBPixelFormat::CI8_SW;
        }
        if(tmp == "DXT1") {
            return PSBPixelFormat::DXT1;
        }
        if(tmp == "DXT5") {
            return PSBPixelFormat::DXT5;
        }
        if(tmp == "RGBA4444") {
            if(useBigEndian(spec)) {
                return PSBPixelFormat::BeRGBA4444;
            }
            return PSBPixelFormat::LeRGBA4444;
        }
        if(tmp == "RGBA4444_SW") {
            // TODO: BeRGBA4444_SW?
            return isUseTile ? PSBPixelFormat::TileLeRGBA4444_SW
                             : PSBPixelFormat::LeRGBA4444_SW;
        }
        if(tmp == "RGBA8") {
            if(spec == PSBSpec::Revo) {
                return PSBPixelFormat::TileBeRGBA8_Rvl;
            }
            // TODO: I'm not sure if psv and psp always use BE, so
            // for now just set for RGBA8
            // psv #95
            if(useBigEndian(spec)) {
                return PSBPixelFormat::BeRGBA8;
            }
            return PSBPixelFormat::LeRGBA8;
        }
        if(tmp == "RGBA8_SW" || tmp == "RGBX8_SW") {
            if(isUseTile) {
                if(useBigEndian(spec)) {
                    return PSBPixelFormat::TileBeRGBA8_SW;
                }
                return PSBPixelFormat::TileLeRGBA8_SW;
            }

            if(useBigEndian(spec)) {
                if(spec == PSBSpec::PS3) {
                    return PSBPixelFormat::FlipBeRGBA8_SW;
                }
                return PSBPixelFormat::BeRGBA8_SW;
            }

            if(spec == PSBSpec::PS3) {
                return PSBPixelFormat::FlipLeRGBA8_SW;
            }
            return PSBPixelFormat::LeRGBA8_SW;
        }
        if(tmp == "A8_SW") {
            return isUseTile ? PSBPixelFormat::TileA8_SW
                             : PSBPixelFormat::A8_SW;
        }
        if(tmp == "L8_SW") {
            return isUseTile ? PSBPixelFormat::TileL8_SW
                             : PSBPixelFormat::L8_SW;
        }
        if(tmp == "A8L8") {
            return PSBPixelFormat::A8L8;
        }
        if(tmp == "A8L8_SW") {
            return isUseTile ? PSBPixelFormat::TileA8L8_SW
                             : PSBPixelFormat::A8L8_SW;
        }
        if(tmp == "RGBA5650") {
            return PSBPixelFormat::RGBA5650;
        }
        if(tmp == "RGBA5650_SW") {
            return isUseTile ? PSBPixelFormat::TileRGBA5650_SW
                             : PSBPixelFormat::RGBA5650_SW;
        }
        if(tmp == "ASTC_8BPP") {
            return PSBPixelFormat::ASTC_8BPP;
        }
        return tryParseToPSBPixelFormatEnum(typeStr, true);
    }


    static PSBPixelFormat defaultPalettePixelFormat(PSBSpec spec) {
        switch(spec) {
            case PSBSpec::Common:
            case PSBSpec::Ems:
            case PSBSpec::Vita:
            case PSBSpec::PSP:
                return PSBPixelFormat::BeRGBA8;
            case PSBSpec::NX:
            case PSBSpec::PS4:
            case PSBSpec::Krkr:
            case PSBSpec::Win:
                return PSBPixelFormat::LeRGBA8;
            case PSBSpec::Revo:
                return PSBPixelFormat::RGB5A3;
            default:
                return PSBPixelFormat::None;
        }
    }

} // namespace PSB::Extension