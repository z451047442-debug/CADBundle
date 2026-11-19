#include "CADDimension.h"
#include "CADAnnotationCommon.h"

#include <gfx.h>
#include <maxapi.h>
#include <object.h>
#include <tchar.h>

#include <cmath>

namespace
{
constexpr float kDegenerateEpsilon = 1.0e-6f;
constexpr int kMaxHitSegments = 96;

struct DimGeometry
{
    Point3 p0;
    Point3 p1;
    Point3 p2;
    Point3 dimP0;
    Point3 dimP1;
    Point3 dir;
    Point3 normal;
    float offset = 0.0f;
    bool valid = false;
};

DimGeometry BuildGeometry(const Point3& p0, const Point3& p1, const Point3& p2)
{
    DimGeometry geometry;
    geometry.p0 = p0;
    geometry.p1 = p1;
    geometry.p2 = p2;

    if (!CADIsFinite(p0) || !CADIsFinite(p1) || !CADIsFinite(p2))
    {
        return geometry;
    }

    Point3 direction;
    if (!CADNormalizeSafe(p1 - p0, direction))
    {
        return geometry;
    }

    Point3 normal;
    if (!CADNormalizeSafe(direction.CrossProd(Point3(0.0f, 0.0f, 1.0f)), normal) &&
        !CADNormalizeSafe(direction.CrossProd(Point3(1.0f, 0.0f, 0.0f)), normal))
    {
        return geometry;
    }

    geometry.dir = direction;
    geometry.normal = normal;
    geometry.offset = (p2 - p0).DotProd(normal);
    geometry.dimP0 = p0 + normal * geometry.offset;
    geometry.dimP1 = p1 + normal * geometry.offset;
    geometry.valid = true;
    return geometry;
}

Point3 ColorToPoint3(const Color& color)
{
    return Point3(color.r, color.g, color.b);
}

bool ProjectObjectPoint(GraphicsWindow* gw, const Matrix3& objectTM,
                        const Point3& local, IPoint2& screen)
{
    return CADWorldToScreen(gw, local * objectTM, screen);
}

void AddSegment(const IPoint2& a, const IPoint2& b, IPoint2* segA, IPoint2* segB,
                int& count, int maxSegments)
{
    if (count >= maxSegments)
    {
        return;
    }
    segA[count] = a;
    segB[count] = b;
    ++count;
}

int ClampInt(int value, int low, int high)
{
    return value < low ? low : (value > high ? high : value);
}

int BuildDimensionScreen(GraphicsWindow* gw, const Matrix3& objectTM,
                         const DimGeometry& geometry, float screenScale,
                         float extensionOffset, IPoint2* segA, IPoint2* segB,
                         int maxSegments, int& segmentCount)
{
    segmentCount = 0;
    if (!geometry.valid)
    {
        return 0;
    }

    IPoint2 p0;
    IPoint2 p1;
    IPoint2 d0;
    IPoint2 d1;
    if (!ProjectObjectPoint(gw, objectTM, geometry.p0, p0) ||
        !ProjectObjectPoint(gw, objectTM, geometry.p1, p1) ||
        !ProjectObjectPoint(gw, objectTM, geometry.dimP0, d0) ||
        !ProjectObjectPoint(gw, objectTM, geometry.dimP1, d1))
    {
        return 0;
    }

    const float extension = extensionOffset * screenScale;
    const Point3 ext0 = geometry.dimP0 + geometry.normal * extension;
    const Point3 ext1 = geometry.dimP1 + geometry.normal * extension;
    IPoint2 e0;
    IPoint2 e1;
    if (ProjectObjectPoint(gw, objectTM, ext0, e0) &&
        ProjectObjectPoint(gw, objectTM, ext1, e1))
    {
        AddSegment(p0, e0, segA, segB, segmentCount, maxSegments);
        AddSegment(p1, e1, segA, segB, segmentCount, maxSegments);
    }

    AddSegment(d0, d1, segA, segB, segmentCount, maxSegments);
    return segmentCount;
}

void AppendArrowOutline(const IPoint2& tip, const IPoint2& toward,
                        int lengthPx, int widthPx, IPoint2* segA, IPoint2* segB,
                        int& count, int maxSegments)
{
    if (lengthPx <= 0 || widthPx <= 0)
    {
        return;
    }

    float dx = static_cast<float>(toward.x - tip.x);
    float dy = static_cast<float>(toward.y - tip.y);
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length <= 1.0e-3f)
    {
        return;
    }
    dx /= length;
    dy /= length;

    const float baseX = tip.x + dx * lengthPx;
    const float baseY = tip.y + dy * lengthPx;
    const float halfW = widthPx * 0.5f;
    const float px = -dy * halfW;
    const float py = dx * halfW;

    const IPoint2 b0(static_cast<int>(baseX + px + 0.5f),
                     static_cast<int>(baseY + py + 0.5f));
    const IPoint2 b1(static_cast<int>(baseX - px + 0.5f),
                     static_cast<int>(baseY - py + 0.5f));
    AddSegment(tip, b0, segA, segB, count, maxSegments);
    AddSegment(b0, b1, segA, segB, count, maxSegments);
    AddSegment(b1, tip, segA, segB, count, maxSegments);
}

void AppendTextBox(const IPoint2& center, int widthPx, int heightPx,
                   IPoint2* segA, IPoint2* segB, int& count, int maxSegments)
{
    if (widthPx <= 0 || heightPx <= 0)
    {
        return;
    }
    const int halfW = widthPx / 2;
    const int halfH = heightPx / 2;
    const IPoint2 c0(center.x - halfW, center.y - halfH);
    const IPoint2 c1(center.x + halfW, center.y - halfH);
    const IPoint2 c2(center.x + halfW, center.y + halfH);
    const IPoint2 c3(center.x - halfW, center.y + halfH);
    AddSegment(c0, c1, segA, segB, count, maxSegments);
    AddSegment(c1, c2, segA, segB, count, maxSegments);
    AddSegment(c2, c3, segA, segB, count, maxSegments);
    AddSegment(c3, c0, segA, segB, count, maxSegments);
}

void DrawScreenMarker(GraphicsWindow* gw, const IPoint2& p, int radius,
                      const Point3& color)
{
    CADDrawScreenSegment(gw, IPoint2(p.x - radius, p.y),
                         IPoint2(p.x + radius, p.y), color);
    CADDrawScreenSegment(gw, IPoint2(p.x, p.y - radius),
                         IPoint2(p.x, p.y + radius), color);
}

IPoint2 ScreenTextAnchor(GraphicsWindow* gw, const MCHAR* text,
                         const IPoint2& center, int offsetX, int offsetY)
{
    SIZE extent = {0, 0};
    if (gw && text && text[0] != 0)
    {
        gw->getTextExtents(text, &extent);
    }
    return IPoint2(center.x - extent.cx / 2 + offsetX,
                   center.y - extent.cy / 2 + offsetY);
}
}

void CADDimension::GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt,
                                    Box3& box)
{
    box.Init();
    Point3 points[3] = {};
    if (!GetPoints(t, points))
    {
        return;
    }

    const Matrix3 objectTM = inode ? inode->GetObjectTM(t) : Matrix3();
    const float screenScale = vpt ? CADScreenScale(vpt, objectTM) : 1.0f;

    bool showXYZ = false;
    float arrowSize = 8.0f;
    float textHeight = 12.0f;
    float extensionOffset = 5.0f;
    Color lineColor;
    Color textColor;
    int precision = 2;
    GetParams(t, showXYZ, arrowSize, textHeight, extensionOffset,
              lineColor, textColor, precision);
    (void)showXYZ;
    (void)lineColor;
    (void)textColor;
    (void)precision;

    const float margin = (arrowSize + extensionOffset + textHeight * 2.0f) *
                         screenScale;
    box = CADExpandBox(CADBoxFromPoints(points, 3), margin);
    if (box.pmin.x > box.pmax.x)
    {
        box += Point3(0.0f, 0.0f, 0.0f);
    }
}

void CADDimension::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt,
                                    Box3& box)
{
    Box3 localBox;
    GetLocalBoundBox(t, inode, vpt, localBox);
    box.Init();
    if (!inode || localBox.pmin.x > localBox.pmax.x)
    {
        box = localBox;
        return;
    }

    const Matrix3 objectTM = inode->GetObjectTM(t);
    for (int x = 0; x < 2; ++x)
    {
        for (int y = 0; y < 2; ++y)
        {
            for (int z = 0; z < 2; ++z)
            {
                const Point3 corner(x ? localBox.pmax.x : localBox.pmin.x,
                                    y ? localBox.pmax.y : localBox.pmin.y,
                                    z ? localBox.pmax.z : localBox.pmin.z);
                box += corner * objectTM;
            }
        }
    }
}

int CADDimension::Display(TimeValue t, INode* inode, ViewExp* vpt, int flags)
{
    (void)flags;
    if (!vpt || !vpt->IsAlive() || !inode || !mPBlock)
    {
        return 0;
    }

    GraphicsWindow* gw = vpt->getGW();
    if (!gw)
    {
        return 0;
    }

    Point3 points[3] = {};
    if (!GetPoints(t, points))
    {
        return 0;
    }

    bool showXYZ = false;
    float arrowSize = 8.0f;
    float textHeight = 12.0f;
    float extensionOffset = 5.0f;
    Color lineColor;
    Color textColor;
    int precision = 2;
    if (!GetParams(t, showXYZ, arrowSize, textHeight, extensionOffset,
                   lineColor, textColor, precision))
    {
        return 0;
    }

    const Matrix3 objectTM = inode->GetObjectTM(t);
    const float screenScale = CADScreenScale(vpt, objectTM);
    const DimGeometry geometry = BuildGeometry(points[0], points[1], points[2]);
    const Point3 line = ColorToPoint3(lineColor);
    const Point3 text = ColorToPoint3(textColor);

    GWScreenGuard screenGuard(gw);

    if (!geometry.valid)
    {
        for (int i = 0; i < 3; ++i)
        {
            IPoint2 p;
            if (ProjectObjectPoint(gw, objectTM, points[i], p))
            {
                DrawScreenMarker(gw, p, 3, line);
            }
        }
        return 0;
    }

    IPoint2 segA[kMaxHitSegments] = {};
    IPoint2 segB[kMaxHitSegments] = {};
    int segmentCount = 0;
    BuildDimensionScreen(gw, objectTM, geometry, screenScale, extensionOffset,
                         segA, segB, kMaxHitSegments, segmentCount);
    for (int i = 0; i < segmentCount; ++i)
    {
        CADDrawScreenSegment(gw, segA[i], segB[i], line);
    }

    IPoint2 d0;
    IPoint2 d1;
    if (ProjectObjectPoint(gw, objectTM, geometry.dimP0, d0) &&
        ProjectObjectPoint(gw, objectTM, geometry.dimP1, d1) &&
        Length(geometry.dimP1 - geometry.dimP0) > kDegenerateEpsilon)
    {
        const int arrowPx = ClampInt(static_cast<int>(arrowSize + 0.5f), 2, 400);
        const int arrowWidthPx = ClampInt(arrowPx / 3, 1, 200);
        CADDrawScreenArrow(gw, d0, d1, arrowPx, arrowWidthPx, line);
        CADDrawScreenArrow(gw, d1, d0, arrowPx, arrowWidthPx, line);

        const float offsetWorld = textHeight * screenScale * 0.6f;
        const Point3 anchorWorld = (geometry.dimP0 + geometry.dimP1) * 0.5f +
                                   geometry.normal * offsetWorld;
        IPoint2 anchor;
        if (ProjectObjectPoint(gw, objectTM, anchorWorld, anchor))
        {
            const MSTR distanceText =
                CADFormatFixed(Length(points[1] - points[0]), precision);
            const IPoint2 textAnchor =
                ScreenTextAnchor(gw, distanceText.data(), anchor, 0, -8);
            CADDrawScreenText(gw, textAnchor, distanceText.data(), textHeight,
                              text);
        }
    }

    if (showXYZ)
    {
        const MCHAR* names[3] = {_T("P0"), _T("P1"), _T("P2")};
        for (int i = 0; i < 3; ++i)
        {
            IPoint2 p;
            if (!ProjectObjectPoint(gw, objectTM, points[i], p))
            {
                continue;
            }
            const MSTR x = CADFormatFixed(points[i].x, precision);
            const MSTR y = CADFormatFixed(points[i].y, precision);
            const MSTR z = CADFormatFixed(points[i].z, precision);
            TCHAR buffer[256] = {};
            _stprintf_s(buffer, _countof(buffer), _T("%s (%s, %s, %s)"),
                        names[i], x.data(), y.data(), z.data());
            const IPoint2 textAnchor =
                ScreenTextAnchor(gw, buffer, p, 8, -8);
            CADDrawScreenText(gw, textAnchor, buffer, textHeight * 0.8f, text);
        }
    }

    return 0;
}

int CADDimension::HitTest(TimeValue t, INode* inode, int type, int crossing,
                          int flags, IPoint2* p, ViewExp* vpt)
{
    (void)flags;
    if (!vpt || !vpt->IsAlive() || !inode || !p || !mPBlock)
    {
        return 0;
    }

    GraphicsWindow* gw = vpt->getGW();
    if (!gw)
    {
        return 0;
    }

    Point3 points[3] = {};
    if (!GetPoints(t, points))
    {
        return 0;
    }

    bool showXYZ = false;
    float arrowSize = 8.0f;
    float textHeight = 12.0f;
    float extensionOffset = 5.0f;
    Color lineColor;
    Color textColor;
    int precision = 2;
    if (!GetParams(t, showXYZ, arrowSize, textHeight, extensionOffset,
                   lineColor, textColor, precision))
    {
        return 0;
    }
    (void)showXYZ;

    HitRegion hitRegion;
    MakeHitRegion(hitRegion, type, crossing, 4, p);

    const Matrix3 objectTM = inode->GetObjectTM(t);
    const float screenScale = CADScreenScale(vpt, objectTM);
    const DimGeometry geometry = BuildGeometry(points[0], points[1], points[2]);

    GWScreenGuard screenGuard(gw);

    IPoint2 segA[kMaxHitSegments] = {};
    IPoint2 segB[kMaxHitSegments] = {};
    int segmentCount = 0;

    if (!geometry.valid)
    {
        for (int i = 0; i < 3; ++i)
        {
            IPoint2 sp;
            if (ProjectObjectPoint(gw, objectTM, points[i], sp))
            {
                AddSegment(IPoint2(sp.x - 3, sp.y), IPoint2(sp.x + 3, sp.y),
                           segA, segB, segmentCount, kMaxHitSegments);
                AddSegment(IPoint2(sp.x, sp.y - 3), IPoint2(sp.x, sp.y + 3),
                           segA, segB, segmentCount, kMaxHitSegments);
            }
        }
        return CADHitScreenSegments(hitRegion, segA, segB, segmentCount) ? 1 : 0;
    }

    BuildDimensionScreen(gw, objectTM, geometry, screenScale, extensionOffset,
                         segA, segB, kMaxHitSegments, segmentCount);

    IPoint2 d0;
    IPoint2 d1;
    if (ProjectObjectPoint(gw, objectTM, geometry.dimP0, d0) &&
        ProjectObjectPoint(gw, objectTM, geometry.dimP1, d1) &&
        Length(geometry.dimP1 - geometry.dimP0) > kDegenerateEpsilon)
    {
        const int arrowPx = ClampInt(static_cast<int>(arrowSize + 0.5f), 2, 400);
        const int arrowWidthPx = ClampInt(arrowPx / 3, 1, 200);
        AppendArrowOutline(d0, d1, arrowPx, arrowWidthPx, segA, segB,
                           segmentCount, kMaxHitSegments);
        AppendArrowOutline(d1, d0, arrowPx, arrowWidthPx, segA, segB,
                           segmentCount, kMaxHitSegments);
    }

    if (textHeight > 0.0f)
    {
        const float offsetWorld = textHeight * screenScale * 0.6f;
        const Point3 anchorWorld = (geometry.dimP0 + geometry.dimP1) * 0.5f +
                                   geometry.normal * offsetWorld;
        IPoint2 anchor;
        if (ProjectObjectPoint(gw, objectTM, anchorWorld, anchor))
        {
            const MSTR distanceText =
                CADFormatFixed(Length(points[1] - points[0]), precision);
            const int textWidthPx = ClampInt(
                static_cast<int>(distanceText.Length() * 0.6f * textHeight +
                                 0.5f),
                4, 4000);
            AppendTextBox(IPoint2(anchor.x, anchor.y - 8), textWidthPx,
                          static_cast<int>(textHeight + 0.5f), segA, segB,
                          segmentCount, kMaxHitSegments);
        }
    }

    return CADHitScreenSegments(hitRegion, segA, segB, segmentCount) ? 1 : 0;
}
