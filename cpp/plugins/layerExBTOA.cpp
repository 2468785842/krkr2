// layerExBTOA API compatibility; see docs/reference/plugins.md for provenance.
#include "LayerImpl.h"
#include "MsgIntf.h"
#include "ncbind.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

#define NCB_MODULE_NAME TJS_W("layerExBTOA.dll")

namespace {
    tTJSNI_Layer *RequireLayer(iTJSDispatch2 *object) {
        tTJSNI_Layer *layer = nullptr;
        if(!object ||
           TJS_FAILED(object->NativeInstanceSupport(
               TJS_NIS_GETINSTANCE, tTJSNC_Layer::ClassID,
               reinterpret_cast<iTJSNativeInstance **>(&layer))) ||
           !layer || !layer->GetHasImage())
            TVPThrowExceptionMessage(
                TJS_W("clipAlphaRect requires a Layer with an image"));
        return layer;
    }

    tjs_uint32 MultiplyAlpha(tjs_uint32 value, tjs_uint32 alpha) {
        const auto product = value * alpha;
        return (product + (product >> 7)) >> 8;
    }

    tjs_error ClipAlphaRect(tTJSVariant *, tjs_int count, tTJSVariant **args,
                            iTJSDispatch2 *object) {
        if(count < 7)
            return TJS_E_BADPARAMCOUNT;
        // Public coordinates are signed 32-bit; intermediates must not overflow
        // when a rectangle is far outside either image.
        const tjs_int64 dx = static_cast<tjs_int32>(args[0]->AsInteger());
        const tjs_int64 dy = static_cast<tjs_int32>(args[1]->AsInteger());
        const tjs_int64 sx = static_cast<tjs_int32>(args[3]->AsInteger());
        const tjs_int64 sy = static_cast<tjs_int32>(args[4]->AsInteger());
        const tjs_int64 width = static_cast<tjs_int32>(args[5]->AsInteger());
        const tjs_int64 height = static_cast<tjs_int32>(args[6]->AsInteger());
        if(width <= 0 || height <= 0)
            return TJS_E_INVALIDPARAM;
        const auto clear = count >= 8 && args[7]->Type() != tvtVoid
            ? args[7]->AsInteger()
            : -1;
        const bool hasClear = clear >= 0 && clear <= 255;
        auto *dest = RequireLayer(object);
        auto *src = RequireLayer(args[2]->AsObjectNoAddRef());

        const tjs_int64 left = std::max<tjs_int64>(0, dest->GetClipLeft());
        const tjs_int64 top = std::max<tjs_int64>(0, dest->GetClipTop());
        const tjs_int64 right = std::min<tjs_int64>(
            dest->GetImageWidth(),
            static_cast<tjs_int64>(dest->GetClipLeft()) + dest->GetClipWidth());
        const tjs_int64 bottom = std::min<tjs_int64>(
            dest->GetImageHeight(),
            static_cast<tjs_int64>(dest->GetClipTop()) + dest->GetClipHeight());
        if(left >= right || top >= bottom)
            return TJS_S_OK;

        const auto x0 = std::max({ left, dx, dx - sx });
        const auto y0 = std::max({ top, dy, dy - sy });
        const auto x1 = std::min(
            { right, dx + width,
              dx + static_cast<tjs_int64>(src->GetImageWidth()) - sx });
        const auto y1 = std::min(
            { bottom, dy + height,
              dy + static_cast<tjs_int64>(src->GetImageHeight()) - sy });
        const bool overlaps = x0 < x1 && y0 < y1;
        if(!overlaps && !hasClear)
            return TJS_S_OK;

        // Snapshot the mask before requesting writable storage. This handles
        // overlapping self-copies and assignImages copy-on-write on both GPU
        // and software textures, without retaining an invalidated read pointer.
        std::vector<tjs_uint8> mask;
        if(overlaps) {
            mask.resize(static_cast<size_t>(x1 - x0) * (y1 - y0));
            const auto *pixels =
                static_cast<const tjs_uint8 *>(src->GetMainImagePixelBuffer());
            if(!pixels)
                TVPThrowExceptionMessage(
                    TJS_W("clipAlphaRect cannot read source image"));
            const auto pitch = src->GetMainImagePixelBufferPitch();
            size_t index = 0;
            for(auto y = y0; y < y1; ++y) {
                const auto *row = reinterpret_cast<const tjs_uint32 *>(
                    pixels + static_cast<ptrdiff_t>(sy + y - dy) * pitch);
                for(auto x = x0; x < x1; ++x)
                    mask[index++] = row[sx + x - dx] >> 24;
            }
        }

        auto *pixels =
            static_cast<tjs_uint8 *>(dest->GetMainImagePixelBufferForWrite());
        if(!pixels)
            TVPThrowExceptionMessage(
                TJS_W("clipAlphaRect cannot write destination image"));
        const auto pitch = dest->GetMainImagePixelBufferPitch();
        const bool additive = dest->GetType() == ltAddAlpha;
        const auto drawLeft = hasClear ? left : x0;
        const auto drawTop = hasClear ? top : y0;
        const auto drawRight = hasClear ? right : x1;
        const auto drawBottom = hasClear ? bottom : y1;
        for(auto y = drawTop; y < drawBottom; ++y) {
            auto *row = reinterpret_cast<tjs_uint32 *>(
                pixels + static_cast<ptrdiff_t>(y) * pitch);
            for(auto x = drawLeft; x < drawRight; ++x) {
                const bool inside =
                    overlaps && x >= x0 && x < x1 && y >= y0 && y < y1;
                const tjs_uint32 alpha = inside
                    ? mask[static_cast<size_t>(y - y0) * (x1 - x0) + x - x0]
                    : static_cast<tjs_uint32>(clear);
                const auto pixel = row[x];
                if(additive) {
                    // Premultiplied color must follow alpha, including clear.
                    tjs_uint32 value = 0;
                    for(int shift = 0; shift < 32; shift += 8)
                        value |= MultiplyAlpha((pixel >> shift) & 255, alpha)
                            << shift;
                    row[x] = value;
                } else {
                    const auto value =
                        inside ? MultiplyAlpha(pixel >> 24, alpha) : alpha;
                    row[x] = (pixel & 0xffffff) | (value << 24);
                }
            }
        }
        dest->Update(
            tTVPRect(static_cast<tjs_int>(drawLeft) + dest->GetImageLeft(),
                     static_cast<tjs_int>(drawTop) + dest->GetImageTop(),
                     static_cast<tjs_int>(drawRight) + dest->GetImageLeft(),
                     static_cast<tjs_int>(drawBottom) + dest->GetImageTop()));
        return TJS_S_OK;
    }
} // namespace

NCB_ATTACH_FUNCTION(clipAlphaRect, Layer, ClipAlphaRect);
