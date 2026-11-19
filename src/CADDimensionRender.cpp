#include "CADDimension.h"
#include "CADAnnotationCommon.h"
#include "CADBezFontText.h"

#include <gfx.h>
#include <maxapi.h>
#include <object.h>
#include <tchar.h>

#include <cmath>

namespace
{
constexpr float kDegenerateEpsilon = 1.0e-6f;

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

void DrawPointMarkers(GraphicsWindow* gw, const Point3* points, int count,
                      float screenScale, const Point3& color)
{
    if (!gw || !points || count <= 0)
    {
        return;
    }

    const float offset = screenScale > 0.0f ? screenScale * 2.0f : 2.0f;
    for (int i = 0; i < count; ++i)
    {
        if (!CADIsFinite(points[i]))
        {
            continue;
        }
        const Point3 horizontal[2] = {
            points[i] + Point3(-offset, 0.0f, 0.0f),
            points[i] + Point3(offset, 0.0f, 0.0f),
        };
        const Point3 vertical[2] = {
            points[i] + Point3(0.0f, -offset, 0.0f),
            points[i] + Point3(0.0f, offset, 0.0f),
        };
        CADDrawPolyline(gw, horizontal, 2, color);
        CADDrawPolyline(gw, vertical, 2, color);
    }
}

void DrawDimensionLines(GraphicsWindow* gw, const DimGeometry& geometry,
                        float screenScale, float arrowSize,
                        float extensionOffset, const Point3& color)
{
    if (!gw || !geometry.valid || screenScale <= 0.0f)
    {
        return;
    }

    const float extension = extensionOffset * screenScale;
    const Point3 ext0[2] = {
        geometry.p0,
        geometry.dimP0 + geometry.normal * extension,
    };
    const Point3 ext1[2] = {
        geometry.p1,
        geometry.dimP1 + geometry.normal * extension,
    };
    CADDrawPolyline(gw, ext0, 2, color);
    CADDrawPolyline(gw, ext1, 2, color);

    if (Length(geometry.dimP1 - geometry.dimP0) <= kDegenerateEpsilon)
    {
        return;
    }

    const Point3 dimensionLine[2] = { geometry.dimP0, geometry.dimP1 };
    CADDrawPolyline(gw, dimensionLine, 2, color);

    const float arrowLength = arrowSize * screenScale;
    const float arrowWidth = arrowLength * 0.35f;
    if (arrowLength > 0.0f)
    {
        CADDrawArrowHead(gw, geometry.dimP0, geometry.dir,
                         arrowLength, arrowWidth, geometry.normal, color);
        const Point3 reverse(-geometry.dir.x, -geometry.dir.y, -geometry.dir.z);
        CADDrawArrowHead(gw, geometry.dimP1, reverse,
                         arrowLength, arrowWidth, geometry.normal, color);
    }
}

bool DrawCenteredText(GraphicsWindow* gw, ViewExp* vpt, const Matrix3& objectTM,
                      const DimGeometry& geometry, const MSTR& text,
                      float heightWorld, const Point3& color,
                      CADBezFontCache& fontCache)
{
    if (!gw || !vpt || !vpt->IsAlive() || !geometry.valid ||
        text.Length() == 0 || heightWorld <= 0.0f)
    {
        return false;
    }

    if (!fontCache.IsReady() && !fontCache.SetFont(MSTR(_T(""))))
    {
        return false;
    }

    Point3 right;
    Point3 up;
    Point3 normal;
    if (!CADGetBillboardAxes(vpt, right, up, normal))
    {
        return false;
    }
    (void)up;
    (void)normal;

    const float textWidth = fontCache.TextWidth(text.data()) *
                            heightWorld / 1000.0f;
    const Point3 objectAnchor = (geometry.dimP0 + geometry.dimP1) * 0.5f +
                                geometry.normal * (heightWorld * 0.6f);
    const Point3 worldAnchor = objectAnchor * objectTM +
                               right * (-textWidth * 0.5f);
    Point3 localAnchor = worldAnchor * Inverse(objectTM);
    if (!CADIsFinite(localAnchor))
    {
        localAnchor = objectAnchor;
    }

    return CADDrawBezFontText(gw, vpt, objectTM, localAnchor, text.data(),
                              heightWorld, color, fontCache);
}

void DrawCoordinateLabels(GraphicsWindow* gw, ViewExp* vpt,
                          const Matrix3& objectTM, const DimGeometry& geometry,
                          float textHeight, int precision,
                          const Point3& color, CADBezFontCache& fontCache)
{
    if (!geometry.valid || textHeight <= 0.0f)
    {
        return;
    }

    const Point3 points[3] = { geometry.p0, geometry.p1, geometry.p2 };
    const MCHAR* names[3] = { _T("P0"), _T("P1"), _T("P2") };
    for (int i = 0; i < 3; ++i)
    {
        const MSTR x = CADFormatFixed(points[i].x, precision);
        const MSTR y = CADFormatFixed(points[i].y, precision);
        const MSTR z = CADFormatFixed(points[i].z, precision);
        TCHAR buffer[256] = {};
        _stprintf_s(buffer, _countof(buffer), _T("%s (%s, %s, %s)"),
                    names[i], x.data(), y.data(), z.data());
        const Point3 anchor = points[i] + geometry.normal * (textHeight * 0.8f);
        CADDrawBezFontText(gw, vpt, objectTM, anchor, buffer,
                           textHeight * 0.6f, color, fontCache);
    }
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
    gw->setTransform(objectTM);
    const float screenScale = CADScreenScale(vpt, objectTM);
    const DimGeometry geometry = BuildGeometry(points[0], points[1], points[2]);
    const Point3 line = ColorToPoint3(lineColor);
    if (!geometry.valid)
    {
        DrawPointMarkers(gw, points, 3, screenScale, line);
        return 0;
    }

    const Point3 text = ColorToPoint3(textColor);
    DrawDimensionLines(gw, geometry, screenScale, arrowSize,
                       extensionOffset, line);

    const float heightWorld = textHeight * screenScale;
    if (heightWorld > 0.0f)
    {
        const MSTR distanceText =
            CADFormatFixed(Length(points[1] - points[0]), precision);
        DrawCenteredText(gw, vpt, objectTM, geometry, distanceText,
                         heightWorld, text, mFontCache);
    }

    if (showXYZ)
    {
        DrawCoordinateLabels(gw, vpt, objectTM, geometry, heightWorld,
                             precision, text, mFontCache);
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

    GWRndLimitsGuard limits(gw);
    const DWORD savedLimits = gw->getRndLimits();
    gw->setRndLimits((savedLimits | GW_PICK) & ~GW_ILLUM);

    HitRegion hitRegion;
    MakeHitRegion(hitRegion, type, crossing, 4, p);
    gw->setHitRegion(&hitRegion);
    gw->clearHitCode();

    const Matrix3 objectTM = inode->GetObjectTM(t);
    gw->setTransform(objectTM);
    const float screenScale = CADScreenScale(vpt, objectTM);
    const DimGeometry geometry = BuildGeometry(points[0], points[1], points[2]);
    const Point3 line = ColorToPoint3(lineColor);
    if (!geometry.valid)
    {
        DrawPointMarkers(gw, points, 3, screenScale, line);
        return gw->checkHitCode();
    }

    DrawDimensionLines(gw, geometry, screenScale, arrowSize,
                       extensionOffset, line);

    if (geometry.valid && textHeight > 0.0f)
    {
        const MSTR distanceText =
            CADFormatFixed(Length(points[1] - points[0]), precision);
        if (!mFontCache.IsReady())
        {
            mFontCache.SetFont(MSTR(_T("")));
        }
        const float heightWorld = textHeight * screenScale;
        const float widthWorld = mFontCache.TextWidth(distanceText.data()) *
                                 heightWorld / 1000.0f;
        const Point3 center = (geometry.dimP0 + geometry.dimP1) * 0.5f +
                              geometry.normal * (heightWorld * 0.6f);
        const Point3 halfRight = geometry.dir * (widthWorld * 0.5f);
        const Point3 halfUp = geometry.normal * (heightWorld * 0.5f);
        const Point3 corners[4] = {
            center - halfRight - halfUp,
            center + halfRight - halfUp,
            center + halfRight + halfUp,
            center - halfRight + halfUp,
        };
        CADDrawClosedPolyline(gw, corners, 4, line);
    }

    return gw->checkHitCode();
}
