#pragma once

#include <cocos/ui/CocosGUI.h>

namespace Csd {

    /** CSD 画布宽高，仅作比例分母，运行时尺寸一律来自父节点 size */
    constexpr float kCanvasMenuListW = 720.f;
    constexpr float kCanvasMenuListH = 600.f;
    constexpr float kCanvasNaviBarW = 720.f;
    constexpr float kCanvasNaviBarH = 120.f;
    constexpr float kCanvasComctrlW = 640.f;
    constexpr float kCanvasListViewW = 720.f;
    constexpr float kCanvasListViewH = 960.f;
    constexpr float kCanvasListItemW = 720.f;
    constexpr float kCanvasListItemH = 96.f;
    constexpr float kCanvasSliderIconW = 1280.f;
    constexpr float kCanvasSliderIconH = 240.f;
    constexpr float kCanvasSliderTextW = 640.f;
    constexpr float kCanvasSliderTextH = 240.f;
    constexpr float kCanvasFileManageMenuW = 720.f;
    constexpr float kCanvasFileManageMenuH = 1000.f;
    constexpr float kCanvasAllTipsW = 960.f;
    constexpr float kCanvasAllTipsH = 480.f;
    constexpr float kCanvasGameMenuW = 720.f;
    constexpr float kCanvasGameMenuH = 130.f;
    constexpr float kCanvasFileItemW = 640.f;
    constexpr float kCanvasFileItemH = 96.f;
    constexpr float kCanvasBottomBarW = 720.f;
    constexpr float kCanvasBottomBarH = 200.f;
    constexpr float kCanvasTextPairW = 720.f;
    constexpr float kCanvasTextPairH = 480.f;
    constexpr float kCanvasSelectListW = 720.f;
    constexpr float kCanvasSelectListH = 480.f;
    constexpr float kCanvasMessageBoxW = 1152.f;
    constexpr float kCanvasMessageBoxH = 480.f;
    constexpr float kCanvasMediaNaviW = 960.f;
    constexpr float kCanvasMediaNaviH = 120.f;
    constexpr float kCanvasMediaBodyW = 960.f;
    constexpr float kCanvasMediaBodyH = 640.f;
    constexpr float kCanvasMediaFootW = 960.f;
    constexpr float kCanvasMediaFootH = 200.f;
    constexpr float kCanvasWinMgrW = 960.f;
    constexpr float kCanvasWinMgrH = 640.f;
    constexpr float kCanvasRecentItemW = 720.f;
    constexpr float kCanvasRecentItemH = 120.f;

    /**
     * 布局上下文：size 为父级分配区域，scale 为 UI 缩放。
     * 坐标与尺寸用 CSD 数值 / 画布尺寸 → relW/relH，不用固定像素布局。
     */
    struct LayoutCtx {
        cocos2d::Size size;
        float scale;

        float w() const { return size.width; }
        float h() const { return size.height; }

        float relW(float ratio) const { return size.width * ratio; }
        float relH(float ratio) const { return size.height * ratio; }

        /** designX / canvasW → 相对当前节点宽度 */
        float relWx(float designX, float canvasW) const {
            return relW(designX / canvasW);
        }

        /** designY / canvasH → 相对当前节点高度 */
        float relHy(float designY, float canvasH) const {
            return relH(designY / canvasH);
        }

        cocos2d::Size relSize(float designW, float designH, float canvasW,
                              float canvasH) const {
            return { relWx(designW, canvasW), relHy(designH, canvasH) };
        }

        /** 字号：相对宽度比例后再乘 scale */
        int fontSize(float designFont, float canvasW) const {
            return static_cast<int>(relWx(designFont, canvasW) + 0.5f);
        }

        /**
         * 列表行高：父级只给宽度时，按 CSD 高/宽比推算；
         * 父级已分配高度则直接用。
         */
        float rowH(float designRowH, float canvasW = kCanvasComctrlW) const {
            if(size.height > 0.f)
                return size.height;
            return relWx(designRowH, canvasW);
        }
    };

} // namespace Csd
