#include "CADLeader.h"
#include "CADAnnotationCommon.h"
#include "CADBezFontText.h"

#include <gfx.h>
#include <maxapi.h>
#include <object.h>
#include <tchar.h>

namespace
{
bool LeaderDirection(const Point3& start, const Point3& elbow,
                     const Point3& textPos, Point3& direction)
{
    return CADNormalizeSafe(elbow - start, direction) ||
           CADNormalizeSafe(textPos - start, direction);
}

void DrawLeader(GraphicsWindow* gw, const Point3& start, const Point3& elbow,
                const Point3& textPos, float screenScale, float arrowSize,
                const Point3& color)
{
    const Point3 polyline[3] = { start, elbow, textPos };
    CADDrawPolyline(gw, polyline, 3, color);

    Point3 direction;
    if (!LeaderDirection(start, elbow, textPos, direction))
    {
        return;
    }

    const float arrowLength = arrowSize * screenScale;
    if (arrowLength <= 0.0f)
    {
        return;
    }

    Point3 normal;
    if (!CADNormalizeSafe(direction.CrossProd(Point3(0.0f, 0.0f, 1.0f)), normal) &&
        !CADNormalizeSafe(direction.CrossProd(Point3(1.0f, 0.0f, 0.0f)), normal))
    {
        return;
    }

    CADDrawArrowHead(gw, start, direction, arrowLength,
                     arrowLength * 0.35f, normal, color);
}

void DrawTextHitBox(GraphicsWindow* gw, const Point3& position,
                    const Point3& direction, float width, float height,
                    const Point3& color)
{
    Point3 right;
    if (!CADNormalizeSafe(direction, right))
    {
        right = Point3(1.0f, 0.0f, 0.0f);
    }
    const Point3 up(0.0f, 1.0f, 0.0f);
    const Point3 corners[4] = {
        position,
        position + right * width,
        position + up * height,
        position + right * width + up * height,
    };
    CADDrawClosedPolyline(gw, corners, 4, color);
}
}

void CADLeader::GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt,
                                 Box3& box)
{
    box.Init();
    Point3 start;
    Point3 elbow;
    Point3 textPos;
    MSTR note;
    float arrowSize = 8.0f;
    float textHeight = 12.0f;
    Color lineColor;
    Color textColor;
    if (!GetParams(t, start, elbow, textPos, note, arrowSize, textHeight,
                   lineColor, textColor))
    {
        return;
    }

    const Matrix3 objectTM = inode ? inode->GetObjectTM(t) : Matrix3();
    const float screenScale = vpt ? CADScreenScale(vpt, objectTM) : 1.0f;
    const Point3 points[3] = { start, elbow, textPos };
    const float margin = (arrowSize + textHeight * 2.0f) * screenScale;
    box = CADExpandBox(CADBoxFromPoints(points, 3), margin);
    if (box.pmin.x > box.pmax.x)
    {
        box += Point3(0.0f, 0.0f, 0.0f);
    }
}

void CADLeader::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt,
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

int CADLeader::Display(TimeValue t, INode* inode, ViewExp* vpt, int flags)
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

    Point3 start;
    Point3 elbow;
    Point3 textPos;
    MSTR note;
    float arrowSize = 8.0f;
    float textHeight = 12.0f;
    Color lineColor;
    Color textColor;
    if (!GetParams(t, start, elbow, textPos, note, arrowSize, textHeight,
                   lineColor, textColor))
    {
        return 0;
    }

    const Matrix3 objectTM = inode->GetObjectTM(t);
    gw->setTransform(objectTM);
    const float screenScale = CADScreenScale(vpt, objectTM);
    const Point3 line(lineColor.r, lineColor.g, lineColor.b);
    const Point3 text(textColor.r, textColor.g, textColor.b);
    DrawLeader(gw, start, elbow, textPos, screenScale, arrowSize, line);

    if (note.Length() > 0 && textHeight > 0.0f)
    {
        CADDrawBezFontText(gw, vpt, objectTM, textPos, note.data(),
                           textHeight * screenScale, text, mFontCache);
    }
    return 0;
}

int CADLeader::HitTest(TimeValue t, INode* inode, int type, int crossing,
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

    Point3 start;
    Point3 elbow;
    Point3 textPos;
    MSTR note;
    float arrowSize = 8.0f;
    float textHeight = 12.0f;
    Color lineColor;
    Color textColor;
    if (!GetParams(t, start, elbow, textPos, note, arrowSize, textHeight,
                   lineColor, textColor))
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
    const Point3 line(lineColor.r, lineColor.g, lineColor.b);
    const Point3 text(textColor.r, textColor.g, textColor.b);
    DrawLeader(gw, start, elbow, textPos, screenScale, arrowSize, line);

    if (note.Length() > 0 && textHeight > 0.0f)
    {
        if (!mFontCache.IsReady())
        {
            mFontCache.SetFont(MSTR(_T("")));
        }
        Point3 direction;
        LeaderDirection(start, elbow, textPos, direction);
        const float width = mFontCache.TextWidth(note.data()) *
                            (textHeight * screenScale) / 1000.0f;
        DrawTextHitBox(gw, textPos, direction, width,
                       textHeight * screenScale, text);
    }

    return gw->checkHitCode();
}
