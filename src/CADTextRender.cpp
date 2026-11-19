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

void DrawTextMarker(GraphicsWindow* gw, const Point3& point, float screenScale,
                    const Point3& color)
{
    const float offset = screenScale > 0.0f ? screenScale * 2.0f : 2.0f;
    const Point3 horizontal[2] = {
        point + Point3(-offset, 0.0f, 0.0f),
        point + Point3(offset, 0.0f, 0.0f),
    };
    const Point3 vertical[2] = {
        point + Point3(0.0f, -offset, 0.0f),
        point + Point3(0.0f, offset, 0.0f),
    };
    CADDrawPolyline(gw, horizontal, 2, color);
    CADDrawPolyline(gw, vertical, 2, color);
}

void DrawTextBox(GraphicsWindow* gw, const Point3& position, float width,
                 float height, const Point3& color)
{
    const Point3 corners[4] = {
        position,
        position + Point3(width, 0.0f, 0.0f),
        position + Point3(width, height, 0.0f),
        position + Point3(0.0f, height, 0.0f),
    };
    CADDrawClosedPolyline(gw, corners, 4, color);
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
    if (!mFontCache.IsReady())
    {
        mFontCache.SetFont(fontName);
    }
    const float width = mFontCache.TextWidth(SanitizeText(text).data()) *
                        height / 1000.0f;
    const float boxWidth = width > 0.0f ? width : height * 0.5f;
    box += position;
    box += position + Point3(boxWidth, height, 0.0f);
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
    gw->setTransform(objectTM);
    const Point3 textColor(color.r, color.g, color.b);
    const float height = fontSize * screenScale;
    mFontCache.SetFont(fontName);

    const MSTR displayText = SanitizeText(text);
    if (displayText.Length() == 0)
    {
        DrawTextMarker(gw, position, screenScale, textColor);
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
    const Point3 textColor(color.r, color.g, color.b);
    const float height = fontSize * screenScale;
    mFontCache.SetFont(fontName);
    const MSTR displayText = SanitizeText(text);

    if (displayText.Length() == 0)
    {
        DrawTextMarker(gw, position, screenScale, textColor);
    }
    else
    {
        const float width = mFontCache.TextWidth(displayText.data()) *
                            height / 1000.0f;
        DrawTextBox(gw, position, width > 0.0f ? width : height * 0.5f,
                    height, textColor);
    }

    return gw->checkHitCode();
}
