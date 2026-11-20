#include "CADLeader.h"
#include "CADAnnotationCommon.h"

#include <gfx.h>
#include <maxapi.h>
#include <object.h>
#include <tchar.h>

#include <cmath>

namespace
{
constexpr int kMaxHitSegments = 32;

int ClampInt(int value, int low, int high)
{
    return value < low ? low : (value > high ? high : value);
}

bool ProjectPoint(GraphicsWindow* gw, const Matrix3& objectTM,
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

int NoteWidthPixels(const MSTR& note, float textHeight)
{
    return ClampInt(static_cast<int>(note.Length() * 0.6f * textHeight + 0.5f),
                    4, 4000);
}

void AppendTextBox(const IPoint2& anchor, int widthPx, int heightPx,
                   IPoint2* segA, IPoint2* segB, int& count, int maxSegments)
{
    if (widthPx <= 0 || heightPx <= 0)
    {
        return;
    }
    const IPoint2 c0(anchor.x, anchor.y - heightPx);
    const IPoint2 c1(anchor.x + widthPx, anchor.y - heightPx);
    const IPoint2 c2(anchor.x + widthPx, anchor.y);
    const IPoint2 c3(anchor.x, anchor.y);
    AddSegment(c0, c1, segA, segB, count, maxSegments);
    AddSegment(c1, c2, segA, segB, count, maxSegments);
    AddSegment(c2, c3, segA, segB, count, maxSegments);
    AddSegment(c3, c0, segA, segB, count, maxSegments);
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
    const Point3 line(lineColor.r, lineColor.g, lineColor.b);
    const Point3 text(textColor.r, textColor.g, textColor.b);

    GWScreenGuard screenGuard(gw);

    IPoint2 ps;
    IPoint2 pe;
    IPoint2 pt;
    if (!ProjectPoint(gw, objectTM, start, ps) ||
        !ProjectPoint(gw, objectTM, elbow, pe) ||
        !ProjectPoint(gw, objectTM, textPos, pt))
    {
        return 0;
    }

    CADDrawScreenSegment(gw, ps, pe, line);
    CADDrawScreenSegment(gw, pe, pt, line);

    const int arrowPx = ClampInt(static_cast<int>(arrowSize + 0.5f), 2, 400);
    CADDrawScreenArrow(gw, ps, pe, arrowPx, ClampInt(arrowPx / 3, 1, 200), line);

    if (note.Length() > 0 && textHeight > 0.0f)
    {
        CADDrawScreenBezFontText(gw, pt, note.data(), textHeight, text,
                                 mFontCache);
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

    HitRegion hitRegion;
    MakeHitRegion(hitRegion, type, crossing, 4, p);

    const Matrix3 objectTM = inode->GetObjectTM(t);
    GWScreenGuard screenGuard(gw);

    IPoint2 ps;
    IPoint2 pe;
    IPoint2 pt;
    if (!ProjectPoint(gw, objectTM, start, ps) ||
        !ProjectPoint(gw, objectTM, elbow, pe) ||
        !ProjectPoint(gw, objectTM, textPos, pt))
    {
        return 0;
    }

    IPoint2 segA[kMaxHitSegments] = {};
    IPoint2 segB[kMaxHitSegments] = {};
    int segmentCount = 0;
    AddSegment(ps, pe, segA, segB, segmentCount, kMaxHitSegments);
    AddSegment(pe, pt, segA, segB, segmentCount, kMaxHitSegments);

    const int arrowPx = ClampInt(static_cast<int>(arrowSize + 0.5f), 2, 400);
    AppendArrowOutline(ps, pe, arrowPx, ClampInt(arrowPx / 3, 1, 200), segA,
                       segB, segmentCount, kMaxHitSegments);

    if (note.Length() > 0 && textHeight > 0.0f)
    {
        AppendTextBox(pt, NoteWidthPixels(note, textHeight),
                      static_cast<int>(textHeight + 0.5f), segA, segB,
                      segmentCount, kMaxHitSegments);
    }

    return CADHitScreenSegments(hitRegion, segA, segB, segmentCount) ? 1 : 0;
}
