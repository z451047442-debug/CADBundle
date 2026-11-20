#include "CADText.h"
#include "CADAnnotationCommon.h"
#include "CADBezFontText.h"

#include <gfx.h>
#include <maxapi.h>
#include <object.h>
#include <tchar.h>

#include <cmath>

namespace
{
constexpr int kMaxTextCharacters = 1024;
constexpr int kMaxHitSegments = 16;

MSTR SanitizeText(const MSTR& source)
{
    TCHAR buffer[kMaxTextCharacters + 1] = {};
    int count = 0;
    const MCHAR* p = source.data();
    for (; p && *p && count < kMaxTextCharacters; ++p)
    {
        const MCHAR ch = *p;
        if (ch == _T('\r') || ch == _T('\n') || ch == _T('\t'))
        {
            buffer[count++] = _T(' ');
        }
        else
        {
            buffer[count++] = ch;
        }
    }
    buffer[count] = _T('\0');
    return MSTR(buffer);
}

void DrawScreenMarker(GraphicsWindow* gw, const IPoint2& p, int radius,
                      const Point3& color)
{
    CADDrawScreenSegment(gw, IPoint2(p.x - radius, p.y),
                         IPoint2(p.x + radius, p.y), color);
    CADDrawScreenSegment(gw, IPoint2(p.x, p.y - radius),
                         IPoint2(p.x, p.y + radius), color);
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
}

void CADText::GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt,
                               Box3& box)
{
    box.Init();
    Point3 position;
    MSTR text;
    MSTR fontName;
    float fontSize = 12.0f;
    Color color;
    if (!GetParams(t, position, text, fontSize, fontName, color))
    {
        return;
    }

    const Matrix3 objectTM = inode ? inode->GetObjectTM(t) : Matrix3();
    const float screenScale = vpt ? CADScreenScale(vpt, objectTM) : 1.0f;
    const float height = fontSize * screenScale;
    mFontCache.EnsureFont(fontName);
    const MSTR displayText = SanitizeText(text);
    const float pixelHeight =
        screenScale > 0.0f ? height / screenScale : height;
    const CADTextMetrics metrics =
        CADMeasureText(mFontCache, displayText.data(), pixelHeight);
    const float pixelWidth = metrics.widthPx > 0.0f
                                 ? metrics.widthPx
                                 : pixelHeight * 0.5f;
    box += position;
    CADAddScreenTextBox(vpt, objectTM, position, 0.0f, -metrics.heightPx,
                        pixelWidth, 0.0f, screenScale, box);
    box = CADExpandBox(box, height * 0.2f);
}

void CADText::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt,
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

int CADText::Display(TimeValue t, INode* inode, ViewExp* vpt, int flags)
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

    Point3 position;
    MSTR text;
    MSTR fontName;
    float fontSize = 12.0f;
    Color color;
    if (!GetParams(t, position, text, fontSize, fontName, color))
    {
        return 0;
    }

    const Matrix3 objectTM = inode->GetObjectTM(t);
    const float screenScale = CADScreenScale(vpt, objectTM);
    const Point3 textColor(color.r, color.g, color.b);
    const float height = fontSize * screenScale;
    mFontCache.EnsureFont(fontName);

    const MSTR displayText = SanitizeText(text);
    if (displayText.Length() == 0)
    {
        GWScreenGuard screenGuard(gw);
        IPoint2 p;
        if (CADWorldToScreen(gw, position * objectTM, p))
        {
            DrawScreenMarker(gw, p, 3, textColor);
        }
        return 0;
    }

    CADDrawBezFontText(gw, vpt, objectTM, position, displayText.data(),
                       height, textColor, mFontCache);
    return 0;
}

int CADText::HitTest(TimeValue t, INode* inode, int type, int crossing,
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

    Point3 position;
    MSTR text;
    MSTR fontName;
    float fontSize = 12.0f;
    Color color;
    if (!GetParams(t, position, text, fontSize, fontName, color))
    {
        return 0;
    }

    HitRegion hitRegion;
    MakeHitRegion(hitRegion, type, crossing, 4, p);

    const Matrix3 objectTM = inode->GetObjectTM(t);
    const float screenScale = CADScreenScale(vpt, objectTM);
    const float height = fontSize * screenScale;
    mFontCache.EnsureFont(fontName);
    const MSTR displayText = SanitizeText(text);

    GWScreenGuard screenGuard(gw);

    IPoint2 segA[kMaxHitSegments] = {};
    IPoint2 segB[kMaxHitSegments] = {};
    int segmentCount = 0;

    if (displayText.Length() == 0)
    {
        IPoint2 sp;
        if (!CADWorldToScreen(gw, position * objectTM, sp))
        {
            return 0;
        }
        AddSegment(IPoint2(sp.x - 3, sp.y), IPoint2(sp.x + 3, sp.y), segA, segB,
                   segmentCount, kMaxHitSegments);
        AddSegment(IPoint2(sp.x, sp.y - 3), IPoint2(sp.x, sp.y + 3), segA, segB,
                   segmentCount, kMaxHitSegments);
        return CADHitScreenSegments(hitRegion, segA, segB, segmentCount) ? 1 : 0;
    }

    Point3 right;
    Point3 up;
    Point3 normal;
    if (!CADGetBillboardAxes(vpt, right, up, normal))
    {
        return 0;
    }
    (void)normal;

    const float width =
        CADMeasureText(mFontCache, displayText.data(), height).widthPx;
    const Point3 origin = position * objectTM;
    const Point3 corners[4] = {
        origin,
        origin + right * width,
        origin + right * width + up * height,
        origin + up * height,
    };

    IPoint2 projected[4] = {};
    for (int i = 0; i < 4; ++i)
    {
        if (!CADWorldToScreen(gw, corners[i], projected[i]))
        {
            return 0;
        }
    }

    int minX = projected[0].x;
    int maxX = projected[0].x;
    int minY = projected[0].y;
    int maxY = projected[0].y;
    for (int i = 1; i < 4; ++i)
    {
        if (projected[i].x < minX) minX = projected[i].x;
        if (projected[i].x > maxX) maxX = projected[i].x;
        if (projected[i].y < minY) minY = projected[i].y;
        if (projected[i].y > maxY) maxY = projected[i].y;
    }

    const IPoint2 c0(minX, minY);
    const IPoint2 c1(maxX, minY);
    const IPoint2 c2(maxX, maxY);
    const IPoint2 c3(minX, maxY);
    AddSegment(c0, c1, segA, segB, segmentCount, kMaxHitSegments);
    AddSegment(c1, c2, segA, segB, segmentCount, kMaxHitSegments);
    AddSegment(c2, c3, segA, segB, segmentCount, kMaxHitSegments);
    AddSegment(c3, c0, segA, segB, segmentCount, kMaxHitSegments);

    return CADHitScreenSegments(hitRegion, segA, segB, segmentCount) ? 1 : 0;
}
