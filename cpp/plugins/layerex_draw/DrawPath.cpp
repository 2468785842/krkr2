#include "LayerExDraw.hpp"

extern void getPoints(const tTJSVariant &var, std::vector<PointFClass> &points);

extern void getRects(const tTJSVariant &var, std::vector<RectFClass> &rects);

/**
 * 現在の図形を閉じずに次の図形を開始します
 */
void DrawPath::startFigure() { GdipStartPathFigure(this->path); }

/**
 * 現在の図形を閉じます
 */
void DrawPath::closeFigure() { GdipClosePathFigure(this->path); }

/**
 * 円弧の描画
 * @param x 左上座標
 * @param y 左上座標
 * @param width 横幅
 * @param height 縦幅
 * @param startAngle 時計方向円弧開始位置
 * @param sweepAngle 描画角度
 */
void DrawPath::drawArc(REAL x, REAL y, REAL width, REAL height, REAL startAngle,
                       REAL sweepAngle) {
    GdipAddPathArc(path, x, y, width, height, startAngle, sweepAngle);
}

/**
 * ベジェ曲線の描画
 * @param app アピアランス
 * @param x1
 * @param y1
 * @param x2
 * @param y2
 * @param x3
 * @param y3
 * @param x4
 * @param y4
 */
void DrawPath::drawBezier(REAL x1, REAL y1, REAL x2, REAL y2, REAL x3, REAL y3,
                          REAL x4, REAL y4) {
    GdipAddPathBezier(path, x1, y1, x2, y2, x3, y3, x4, y4);
}

/**
 * 連続ベジェ曲線の描画
 * @param app アピアランス
 * @param points 点の配列
 */
void DrawPath::drawBeziers(tTJSVariant points) {
    std::vector<PointFClass> ps;
    getPoints(points, ps);
    GdipAddPathBeziers(path, &ps[0], (int)ps.size());
}

/**
 * Closed cardinal spline の描画
 * @param app アピアランス
 * @param points 点の配列
 */
void DrawPath::drawClosedCurve(tTJSVariant points) {
    std::vector<PointFClass> ps;
    getPoints(points, ps);
    GdipAddPathClosedCurve(path, &ps[0], (int)ps.size());
}

/**
 * Closed cardinal spline の描画
 * @param app アピアランス
 * @param points 点の配列
 * @pram tension tension
 */
void DrawPath::drawClosedCurve2(tTJSVariant points, REAL tension) {
    std::vector<PointFClass> ps;
    getPoints(points, ps);
    GdipAddPathClosedCurve2(path, &ps[0], (int)ps.size(), tension);
}

/**
 * cardinal spline の描画
 * @param app アピアランス
 * @param points 点の配列
 */
void DrawPath::drawCurve(tTJSVariant points) {
    std::vector<PointFClass> ps;
    getPoints(points, ps);
    GdipAddPathCurve(path, &ps[0], (int)ps.size());
}

/**
 * cardinal spline の描画
 * @param app アピアランス
 * @param points 点の配列
 * @parma tension tension
 */
void DrawPath::drawCurve2(tTJSVariant points, REAL tension) {
    std::vector<PointFClass> ps;
    getPoints(points, ps);
    GdipAddPathCurve2(path, &ps[0], (int)ps.size(), tension);
}

/**
 * cardinal spline の描画
 * @param app アピアランス
 * @param points 点の配列
 * @param offset
 * @param numberOfSegments
 * @param tension tension
 */
void DrawPath::drawCurve3(tTJSVariant points, int offset, int numberOfSegments,
                          REAL tension) {
    std::vector<PointFClass> ps;
    getPoints(points, ps);
    GdipAddPathCurve3(path, &ps[0], (int)ps.size(), offset, numberOfSegments,
                      tension);
}

/**
 * 円錐の描画
 * @param x 左上座標
 * @param y 左上座標
 * @param width 横幅
 * @param height 縦幅
 * @param startAngle 時計方向円弧開始位置
 * @param sweepAngle 描画角度
 */
void DrawPath::drawPie(REAL x, REAL y, REAL width, REAL height, REAL startAngle,
                       REAL sweepAngle) {
    GdipAddPathPie(path, x, y, width, height, startAngle, sweepAngle);
}

/**
 * 楕円の描画
 * @param app アピアランス
 * @param x
 * @param y
 * @param width
 * @param height
 */
void DrawPath::drawEllipse(REAL x, REAL y, REAL width, REAL height) {
    GdipAddPathEllipse(path, x, y, width, height);
}

/**
 * 線分の描画
 * @param app アピアランス
 * @param x1 始点X座標
 * @param y1 始点Y座標
 * @param x2 終点X座標
 * @param y2 終点Y座標
 */
void DrawPath::drawLine(REAL x1, REAL y1, REAL x2, REAL y2) {
    GdipAddPathLine(path, x1, y1, x2, y2);
}

/**
 * 連続線分の描画
 * @param app アピアランス
 * @param points 点の配列
 */
void DrawPath::drawLines(tTJSVariant points) {
    std::vector<PointFClass> ps;
    getPoints(points, ps);
    GdipAddPathLine2(path, &ps[0], (int)ps.size());
}

/**
 * 多角形の描画
 * @param app アピアランス
 * @param points 点の配列

 */
void DrawPath::drawPolygon(tTJSVariant points) {
    std::vector<PointFClass> ps;
    getPoints(points, ps);
    GdipAddPathPolygon(path, &ps[0], (int)ps.size());
}

/**
 * 矩形の描画
 * @param app アピアランス
 * @param x
 * @param y
 * @param width
 * @param height
 */
void DrawPath::drawRectangle(REAL x, REAL y, REAL width, REAL height) {
    GdipAddPathRectangle(path, x, y, width, height);
}

/**
 * 複数矩形の描画
 * @param app アピアランス
 * @param rects 矩形情報の配列
 */
void DrawPath::drawRectangles(tTJSVariant rects) {
    std::vector<RectFClass> rs;
    getRects(rects, rs);
    GdipAddPathRectangles(path, &rs[0], (int)rs.size());
}
