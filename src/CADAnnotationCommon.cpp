#include "CADAnnotationCommon.h"

#include <icolorman.h>

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

namespace
{
bool ClipScreenSegment(float& x0, float& y0, float& x1, float& y1,
                       float xmin, float ymin, float xmax, float ymax)
{
    float t0 = 0.0f;
    float t1 = 1.0f;
    const float dx = x1 - x0;
    const float dy = y1 - y0;

    auto edge = [&](float p, float q) -> bool
    {
        if (p == 0.0f)
        {
            return q >= 0.0f;
        }
        const float r = q / p;
        if (p < 0.0f)
        {
            if (r > t1) return false;
            if (r > t0) t0 = r;
        }
        else
        {
            if (r < t0) return false;
            if (r < t1) t1 = r;
        }
        return true;
    };

    if (!edge(-dx, x0 - xmin)) return false;
    if (!edge(dx, xmax - x0)) return false;
    if (!edge(-dy, y0 - ymin)) return false;
    if (!edge(dy, ymax - y0)) return false;
    if (t0 > t1) return false;

    const float ax = x0;
    const float ay = y0;
    const float bx = x1;
    const float by = y1;
    x0 = ax + (bx - ax) * t0;
    y0 = ay + (by - ay) * t0;
    x1 = ax + (bx - ax) * t1;
    y1 = ay + (by - ay) * t1;
    return true;
}

float DistPointSegment2D(float px, float py, float ax, float ay,
                         float bx, float by)
{
    const float abx = bx - ax;
    const float aby = by - ay;
    const float lengthSquared = abx * abx + aby * aby;
    if (lengthSquared <= 1.0e-6f)
    {
        const float dx = px - ax;
        const float dy = py - ay;
        return std::sqrt(dx * dx + dy * dy);
    }

    float t = ((px - ax) * abx + (py - ay) * aby) / lengthSquared;
    if (t < 0.0f) t = 0.0f;
    else if (t > 1.0f) t = 1.0f;
    const float cx = ax + abx * t;
    const float cy = ay + aby * t;
    const float dx = px - cx;
    const float dy = py - cy;
    return std::sqrt(dx * dx + dy * dy);
}

bool PointInRect2D(float x, float y, const RECT& r)
{
    return x >= static_cast<float>(r.left) && x <= static_cast<float>(r.right) &&
           y >= static_cast<float>(r.top) && y <= static_cast<float>(r.bottom);
}
}

bool CADWorldToScreen(GraphicsWindow* gw, const Point3& world, IPoint2& screen)
{
    screen = IPoint2(0, 0);
    if (!gw || !CADIsFinite(world))
    {
        return false;
    }

    IPoint3 p(0, 0, 0);
    const DWORD clip = gw->wTransPoint(&world, &p);
    const bool perspective = gw->isPerspectiveView() != 0;
    if (perspective && (clip & (GW_FRONT_PLANE | GW_BACK_PLANE)) != 0)
    {
        return false;
    }

    screen = IPoint2(p.x, p.y);
    return true;
}

void CADDrawScreenSegment(GraphicsWindow* gw, const IPoint2& a,
                          const IPoint2& b, const Point3& color)
{
    if (!gw)
    {
        return;
    }

    const float width = static_cast<float>(gw->getWinSizeX());
    const float height = static_cast<float>(gw->getWinSizeY());
    if (width <= 4.0f || height <= 4.0f)
    {
        return;
    }

    float x0 = static_cast<float>(a.x);
    float y0 = static_cast<float>(a.y);
    float x1 = static_cast<float>(b.x);
    float y1 = static_cast<float>(b.y);
    if (!ClipScreenSegment(x0, y0, x1, y1, 1.0f, 1.0f, width - 1.0f,
                           height - 1.0f))
    {
        return;
    }

    IPoint3 pts[2] = {
        IPoint3(static_cast<int>(x0 + 0.5f), static_cast<int>(y0 + 0.5f), 0),
        IPoint3(static_cast<int>(x1 + 0.5f), static_cast<int>(y1 + 0.5f), 0),
    };
    gw->setColor(LINE_COLOR, color);
    gw->wPolyline(2, pts, nullptr, FALSE, nullptr);
}

void CADDrawScreenPolyline(GraphicsWindow* gw, const IPoint2* points, int count,
                           bool closed, const Point3& color)
{
    if (!gw || !points || count < 2)
    {
        return;
    }

    for (int i = 0; i + 1 < count; ++i)
    {
        CADDrawScreenSegment(gw, points[i], points[i + 1], color);
    }
    if (closed && count > 2)
    {
        CADDrawScreenSegment(gw, points[count - 1], points[0], color);
    }
}

void CADDrawScreenArrow(GraphicsWindow* gw, const IPoint2& tip,
                        const IPoint2& toward, int lengthPx, int widthPx,
                        const Point3& color)
{
    if (!gw || lengthPx <= 0 || widthPx <= 0)
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

    IPoint3 pts[3] = {
        IPoint3(tip.x, tip.y, 0),
        IPoint3(static_cast<int>(baseX + px + 0.5f),
                static_cast<int>(baseY + py + 0.5f), 0),
        IPoint3(static_cast<int>(baseX - px + 0.5f),
                static_cast<int>(baseY - py + 0.5f), 0),
    };
    gw->setColor(LINE_COLOR, color);
    gw->wPolyline(3, pts, nullptr, TRUE, nullptr);
}

void CADDrawScreenText(GraphicsWindow* gw, const IPoint2& anchor,
                       const MCHAR* text, float pointSize, const Point3& color)
{
    if (!gw || !text || text[0] == 0)
    {
        return;
    }

    gw->setColor(TEXT_COLOR, color);
    if (pointSize > 0.0f)
    {
        gw->SetTextPointSize(pointSize);
    }
    IPoint3 p(anchor.x, anchor.y, 0);
    gw->wText(&p, text);
}

bool CADHitScreenSegments(const HitRegion& region, const IPoint2* a,
                          const IPoint2* b, int count)
{    if (!a || !b || count <= 0)
    {
        return false;
    }

    if (region.type == POINT_RGN)
    {
        const float epsilon =
            region.epsilon > 0 ? static_cast<float>(region.epsilon) : 4.0f;
        const float px = static_cast<float>(region.pt.x);
        const float py = static_cast<float>(region.pt.y);
        for (int i = 0; i < count; ++i)
        {
            if (DistPointSegment2D(px, py, static_cast<float>(a[i].x),
                                   static_cast<float>(a[i].y),
                                   static_cast<float>(b[i].x),
                                   static_cast<float>(b[i].y)) <= epsilon)
            {
                return true;
            }
        }
        return false;
    }

    if (region.type == RECT_RGN)
    {
        RECT r = region.rect;
        if (r.left > r.right)
        {
            const int tmp = r.left;
            r.left = r.right;
            r.right = tmp;
        }
        if (r.top > r.bottom)
        {
            const int tmp = r.top;
            r.top = r.bottom;
            r.bottom = tmp;
        }

        if (region.crossing)
        {
            for (int i = 0; i < count; ++i)
            {
                float x0 = static_cast<float>(a[i].x);
                float y0 = static_cast<float>(a[i].y);
                float x1 = static_cast<float>(b[i].x);
                float y1 = static_cast<float>(b[i].y);
                if (ClipScreenSegment(x0, y0, x1, y1,
                                      static_cast<float>(r.left),
                                      static_cast<float>(r.top),
                                      static_cast<float>(r.right),
                                      static_cast<float>(r.bottom)))
                {
                    return true;
                }
            }
            return false;
        }

        for (int i = 0; i < count; ++i)
        {
            if (!PointInRect2D(static_cast<float>(a[i].x),
                               static_cast<float>(a[i].y), r) ||
                !PointInRect2D(static_cast<float>(b[i].x),
                               static_cast<float>(b[i].y), r))
            {
                return false;
            }
        }
        return true;
    }

    if (region.type == CIRCLE_RGN)
    {
        const float cx = static_cast<float>(region.circle.x);
        const float cy = static_cast<float>(region.circle.y);
        const float radius = static_cast<float>(region.circle.r);

        if (region.crossing)
        {
            for (int i = 0; i < count; ++i)
            {
                if (DistPointSegment2D(cx, cy, static_cast<float>(a[i].x),
                                       static_cast<float>(a[i].y),
                                       static_cast<float>(b[i].x),
                                       static_cast<float>(b[i].y)) <= radius)
                {
                    return true;
                }
            }
            return false;
        }

        for (int i = 0; i < count; ++i)
        {
            const float adx = static_cast<float>(a[i].x) - cx;
            const float ady = static_cast<float>(a[i].y) - cy;
            const float bdx = static_cast<float>(b[i].x) - cx;
            const float bdy = static_cast<float>(b[i].y) - cy;
            if (std::sqrt(adx * adx + ady * ady) > radius ||
                std::sqrt(bdx * bdx + bdy * bdy) > radius)
            {
                return false;
            }
        }
        return true;
    }

    return false;
}

INT_PTR CADApplyThemeColors(UINT msg, WPARAM wParam)
{
    IColorManager* colorManager = GetColorManager();
    if (!colorManager)
    {
        return 0;
    }

    switch (msg)
    {
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, colorManager->CustSysColor(COLOR_BTNTEXT));
        SetBkColor(hdc, colorManager->CustSysColor(COLOR_BTNFACE));
        return reinterpret_cast<INT_PTR>(
            colorManager->CustSysColorBrush(COLOR_BTNFACE));
    }

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, colorManager->CustSysColor(COLOR_WINDOWTEXT));
        SetBkColor(hdc, colorManager->CustSysColor(COLOR_WINDOW));
        return reinterpret_cast<INT_PTR>(
            colorManager->CustSysColorBrush(COLOR_WINDOW));
    }

    default:
        return 0;
    }
}
