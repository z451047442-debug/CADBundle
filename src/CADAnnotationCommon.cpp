#include "CADAnnotationCommon.h"

#include <cmath>
#include <tchar.h>

namespace
{
constexpr float kMinVectorLengthSquared = 1.0e-20f;
constexpr int kMaxTextPrecision = 6;
}

float CADScreenScale(ViewExp* vpt, const Matrix3& objectTM)
{
    if (!vpt || !vpt->IsAlive())
    {
        return 1.0f;
    }

    const float nonScaling = vpt->NonScalingObjectSize();
    const float worldWidth = vpt->GetVPWorldWidth(objectTM.GetTrans());
    const float scale = nonScaling * worldWidth / 360.0f;
    if (!std::isfinite(scale) || scale <= 0.0f)
    {
        return 1.0f;
    }
    return scale;
}

bool CADNormalizeSafe(const Point3& value, Point3& normalized)
{
    const float lengthSquared = value.LengthSquared();
    if (!std::isfinite(lengthSquared) || lengthSquared <= kMinVectorLengthSquared)
    {
        normalized = Point3(0.0f, 0.0f, 0.0f);
        return false;
    }

    normalized = value / std::sqrt(lengthSquared);
    return CADIsFinite(normalized);
}

bool CADIsFinite(float value)
{
    return std::isfinite(value) != 0;
}

bool CADIsFinite(const Point3& value)
{
    return CADIsFinite(value.x) && CADIsFinite(value.y) && CADIsFinite(value.z);
}

Box3 CADBoxFromPoints(const Point3* points, int count)
{
    Box3 box;
    box.Init();
    if (!points || count <= 0)
    {
        return box;
    }

    for (int i = 0; i < count; ++i)
    {
        if (CADIsFinite(points[i]))
        {
            box += points[i];
        }
    }
    return box;
}

Box3 CADExpandBox(const Box3& box, float margin)
{
    if (!CADIsFinite(margin) || margin <= 0.0f)
    {
        return box;
    }
    if (box.pmin.x > box.pmax.x || box.pmin.y > box.pmax.y ||
        box.pmin.z > box.pmax.z)
    {
        return box;
    }

    Box3 result = box;
    const Point3 delta(margin, margin, margin);
    result.pmin -= delta;
    result.pmax += delta;
    return result;
}

float CADPointSegmentDistance(const Point3& point, const Point3& a, const Point3& b)
{
    const Point3 segment = b - a;
    const float lengthSquared = segment.LengthSquared();
    if (!std::isfinite(lengthSquared) || lengthSquared <= kMinVectorLengthSquared)
    {
        return Length(point - a);
    }

    float t = (point - a).DotProd(segment) / lengthSquared;
    if (t < 0.0f)
    {
        t = 0.0f;
    }
    else if (t > 1.0f)
    {
        t = 1.0f;
    }

    const Point3 closest = a + segment * t;
    return Length(point - closest);
}

MSTR CADFormatFixed(float value, int precision)
{
    if (!CADIsFinite(value))
    {
        return MSTR(_T("nan"));
    }

    if (precision < 0)
    {
        precision = 0;
    }
    else if (precision > kMaxTextPrecision)
    {
        precision = kMaxTextPrecision;
    }

    TCHAR buffer[64] = {};
    _stprintf_s(buffer, _countof(buffer), _T("%.*f"), precision, value);
    for (TCHAR* p = buffer; *p != _T('\0'); ++p)
    {
        if (*p == _T(','))
        {
            *p = _T('.');
        }
    }
    return MSTR(buffer);
}

void CADDrawPolyline(GraphicsWindow* gw, const Point3* points, int count,
                     const Point3& color)
{
    if (!gw || !points || count < 2)
    {
        return;
    }

    gw->setColor(LINE_COLOR, color);
    DrawLineProc proc(gw);
    proc.proc(const_cast<Point3*>(points), count);
}

void CADDrawClosedPolyline(GraphicsWindow* gw, const Point3* points, int count,
                           const Point3& color)
{
    if (!gw || !points || count < 3)
    {
        return;
    }

    gw->setColor(LINE_COLOR, color);
    gw->polyline(count, const_cast<Point3*>(points), nullptr, nullptr, 1, nullptr);
}

void CADDrawArrowHead(GraphicsWindow* gw, const Point3& tip,
                      const Point3& baseDir, float length, float width,
                      const Point3& planeNormal, const Point3& color)
{
    if (!gw || !CADIsFinite(tip) || !CADIsFinite(length) ||
        !CADIsFinite(width) || length <= 0.0f || width <= 0.0f)
    {
        return;
    }

    Point3 direction;
    if (!CADNormalizeSafe(baseDir, direction))
    {
        return;
    }

    Point3 normal;
    if (!CADNormalizeSafe(planeNormal, normal))
    {
        normal = Point3(0.0f, 0.0f, 1.0f);
    }

    Point3 perpendicular;
    if (!CADNormalizeSafe(normal.CrossProd(direction), perpendicular))
    {
        return;
    }

    const Point3 baseCenter = tip + direction * length;
    const Point3 halfWidth = perpendicular * (width * 0.5f);
    const Point3 triangle[3] = {
        tip,
        baseCenter + halfWidth,
        baseCenter - halfWidth,
    };
    CADDrawClosedPolyline(gw, triangle, 3, color);
}
