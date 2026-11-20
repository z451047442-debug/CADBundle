#include "CADTable.h"
#include "CADAnnotationCommon.h"

#include <gfx.h>
#include <maxapi.h>
#include <object.h>
#include <tchar.h>

#include <algorithm>

namespace
{
constexpr int kMaxGridSegments = 128;

int ClampInt(int value, int low, int high)
{
    return value < low ? low : (value > high ? high : value);
}

constexpr float kTableTextScale = 0.6f;
constexpr int kTableTextMinPx = 6;
constexpr int kTableTextMaxPx = 48;

float ResolveTablePointSize(float cellHeight)
{
    return static_cast<float>(ClampInt(
        static_cast<int>(cellHeight * kTableTextScale + 0.5f),
        kTableTextMinPx, kTableTextMaxPx));
}

void EnsureFont(CADBezFontCache& cache)
{
    if (!cache.IsReady())
    {
        cache.SetFont(MSTR(_T("")));
    }
}

MSTR FitCellText(CADBezFontCache& cache, const MSTR& text, float maxPixels,
                 float pixelHeight)
{
    if (text.Length() <= 16)
    {
        return text;
    }

    const float widthPx =
        CADMeasureText(cache, text.data(), pixelHeight).widthPx;
    if (widthPx <= maxPixels)
    {
        return text;
    }

    MSTR result = text.Substr(0, 24);
    result += _T("...");
    return result;
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

int BuildTableGridScreen(GraphicsWindow* gw, const Matrix3& objectTM,
                         const Point3& insertPos, int rows, int cols,
                         float cellWidthWorld, float cellHeightWorld,
                         IPoint2* segA, IPoint2* segB, int maxSegments)
{
    int count = 0;
    for (int row = 0; row <= rows; ++row)
    {
        const Point3 a = insertPos + Point3(0.0f, row * cellHeightWorld, 0.0f);
        const Point3 b = insertPos +
                         Point3(cols * cellWidthWorld, row * cellHeightWorld,
                                0.0f);
        IPoint2 sa;
        IPoint2 sb;
        if (CADWorldToScreen(gw, a * objectTM, sa) &&
            CADWorldToScreen(gw, b * objectTM, sb))
        {
            AddSegment(sa, sb, segA, segB, count, maxSegments);
        }
    }

    for (int col = 0; col <= cols; ++col)
    {
        const Point3 a = insertPos + Point3(col * cellWidthWorld, 0.0f, 0.0f);
        const Point3 b = insertPos +
                         Point3(col * cellWidthWorld, rows * cellHeightWorld,
                                0.0f);
        IPoint2 sa;
        IPoint2 sb;
        if (CADWorldToScreen(gw, a * objectTM, sa) &&
            CADWorldToScreen(gw, b * objectTM, sb))
        {
            AddSegment(sa, sb, segA, segB, count, maxSegments);
        }
    }
    return count;
}
}

void CADTable::GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt,
                                Box3& box)
{
    box.Init();
    Point3 insertPos;
    int rows = 2;
    int cols = 2;
    float cellWidth = 60.0f;
    float cellHeight = 20.0f;
    Color lineColor;
    Color textColor;
    if (!GetParams(t, insertPos, rows, cols, cellWidth, cellHeight,
                   lineColor, textColor))
    {
        return;
    }

    const Matrix3 objectTM = inode ? inode->GetObjectTM(t) : Matrix3();
    const float screenScale = vpt ? CADScreenScale(vpt, objectTM) : 1.0f;
    const float width = cols * cellWidth * screenScale;
    const float height = rows * cellHeight * screenScale;
    box += insertPos;
    box += insertPos + Point3(width, height, 0.0f);

    const float pointSize = ResolveTablePointSize(cellHeight);
    EnsureFont(mFontCache);
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            const size_t index = static_cast<size_t>(row) * cols + col;
            if (index >= mCells.size() || mCells[index].Length() == 0)
            {
                continue;
            }
            const CADTextMetrics metrics =
                CADMeasureText(mFontCache, mCells[index].data(), pointSize);
            const float textWidth = metrics.widthPx * screenScale;
            const float textHeight = metrics.heightPx * screenScale;
            const Point3 cellOrigin =
                insertPos + Point3(col * cellWidth * screenScale,
                                   row * cellHeight * screenScale, 0.0f);
            box += cellOrigin;
            box += cellOrigin + Point3(textWidth, textHeight, 0.0f);
        }
    }

    box = CADExpandBox(box, std::max(width, height) * 0.05f);
}

void CADTable::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt,
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

int CADTable::Display(TimeValue t, INode* inode, ViewExp* vpt, int flags)
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

    Point3 insertPos;
    int rows = 2;
    int cols = 2;
    float cellWidth = 60.0f;
    float cellHeight = 20.0f;
    Color lineColor;
    Color textColor;
    if (!GetParams(t, insertPos, rows, cols, cellWidth, cellHeight,
                   lineColor, textColor))
    {
        return 0;
    }

    const Matrix3 objectTM = inode->GetObjectTM(t);
    const float screenScale = CADScreenScale(vpt, objectTM);
    const float widthWorld = cellWidth * screenScale;
    const float heightWorld = cellHeight * screenScale;
    const Point3 line(lineColor.r, lineColor.g, lineColor.b);
    const Point3 text(textColor.r, textColor.g, textColor.b);

    GWScreenGuard screenGuard(gw);

    IPoint2 segA[kMaxGridSegments] = {};
    IPoint2 segB[kMaxGridSegments] = {};
    const int segmentCount =
        BuildTableGridScreen(gw, objectTM, insertPos, rows, cols, widthWorld,
                             heightWorld, segA, segB, kMaxGridSegments);
    for (int i = 0; i < segmentCount; ++i)
    {
        CADDrawScreenSegment(gw, segA[i], segB[i], line);
    }

    const float pointSize = ResolveTablePointSize(cellHeight);
    EnsureFont(mFontCache);
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            const size_t index = static_cast<size_t>(row) * cols + col;
            if (index >= mCells.size() || mCells[index].Length() == 0)
            {
                continue;
            }

            const Point3 center = insertPos +
                Point3((col + 0.1f) * widthWorld,
                       (row + 0.7f) * heightWorld, 0.0f);
            IPoint2 p;
            if (!CADWorldToScreen(gw, center * objectTM, p))
            {
                continue;
            }

            const MSTR display =
                FitCellText(mFontCache, mCells[index], cellWidth, pointSize);
            CADDrawScreenBezFontText(gw, p, display.data(), pointSize, text,
                                     mFontCache);
        }
    }
    return 0;
}

int CADTable::HitTest(TimeValue t, INode* inode, int type, int crossing,
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

    Point3 insertPos;
    int rows = 2;
    int cols = 2;
    float cellWidth = 60.0f;
    float cellHeight = 20.0f;
    Color lineColor;
    Color textColor;
    if (!GetParams(t, insertPos, rows, cols, cellWidth, cellHeight,
                   lineColor, textColor))
    {
        return 0;
    }

    HitRegion hitRegion;
    MakeHitRegion(hitRegion, type, crossing, 4, p);

    const Matrix3 objectTM = inode->GetObjectTM(t);
    const float screenScale = CADScreenScale(vpt, objectTM);

    GWScreenGuard screenGuard(gw);

    IPoint2 segA[kMaxGridSegments] = {};
    IPoint2 segB[kMaxGridSegments] = {};
    const int segmentCount =
        BuildTableGridScreen(gw, objectTM, insertPos, rows, cols,
                             cellWidth * screenScale, cellHeight * screenScale,
                             segA, segB, kMaxGridSegments);

    return CADHitScreenSegments(hitRegion, segA, segB, segmentCount) ? 1 : 0;
}
