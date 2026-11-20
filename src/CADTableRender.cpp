#include "CADTable.h"
#include "CADAnnotationCommon.h"

#include <GetCOREInterface.h>
#include <gfx.h>
#include <log.h>
#include <maxapi.h>
#include <object.h>
#include <tchar.h>

#include <algorithm>
#include <cmath>

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

float ResolveTablePointSize(float cellHeight, float fontSize)
{
    const float requested =
        fontSize > 0.0f ? fontSize : cellHeight * kTableTextScale;
    return static_cast<float>(ClampInt(static_cast<int>(requested + 0.5f),
                                       kTableTextMinPx, kTableTextMaxPx));
}

float ResolveTableWorldTextHeight(float cellHeight, float fontSize)
{
    const float requested =
        fontSize > 0.0f ? fontSize : cellHeight * kTableTextScale;
    return std::min(std::max(requested, 0.01f), 10000.0f);
}

int EstimateVisibleCells(GraphicsWindow* gw, const Matrix3& objectTM,
                         const Point3& insertPos, int rows, int cols,
                         float widthWorld, float heightWorld,
                         float cellWidthPx, float cellHeightPx)
{
    if (!gw || rows <= 0 || cols <= 0)
    {
        return 0;
    }

    const int viewWidth = gw->getWinSizeX();
    const int viewHeight = gw->getWinSizeY();
    if (viewWidth <= 1 || viewHeight <= 1)
    {
        return 0;
    }

    const Point3 corners[4] = {
        insertPos,
        insertPos + Point3(cols * widthWorld, 0.0f, 0.0f),
        insertPos + Point3(cols * widthWorld, rows * heightWorld, 0.0f),
        insertPos + Point3(0.0f, rows * heightWorld, 0.0f),
    };
    IPoint2 screen[4] = {};
    for (int i = 0; i < 4; ++i)
    {
        if (!CADWorldToScreen(gw, corners[i] * objectTM, screen[i]))
        {
            return 0;
        }
    }

    int minX = screen[0].x;
    int maxX = screen[0].x;
    int minY = screen[0].y;
    int maxY = screen[0].y;
    for (int i = 1; i < 4; ++i)
    {
        minX = std::min(minX, screen[i].x);
        maxX = std::max(maxX, screen[i].x);
        minY = std::min(minY, screen[i].y);
        maxY = std::max(maxY, screen[i].y);
    }
    minX = std::max(minX, 0);
    minY = std::max(minY, 0);
    maxX = std::min(maxX, viewWidth - 1);
    maxY = std::min(maxY, viewHeight - 1);
    if (maxX <= minX || maxY <= minY)
    {
        return 0;
    }

    if (cellWidthPx <= 0.0f || cellHeightPx <= 0.0f)
    {
        return rows * cols;
    }

    int visibleCols = static_cast<int>(
        std::ceil(static_cast<float>(maxX - minX + 1) / cellWidthPx));
    int visibleRows = static_cast<int>(
        std::ceil(static_cast<float>(maxY - minY + 1) / cellHeightPx));
    visibleCols = std::max(1, std::min(cols, visibleCols));
    visibleRows = std::max(1, std::min(rows, visibleRows));
    return visibleCols * visibleRows;
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
    MSTR fontName;
    float fontSize = 0.0f;
    int sizeMode = 0;
    if (!GetParams(t, insertPos, rows, cols, cellWidth, cellHeight,
                   lineColor, textColor, fontName, fontSize, sizeMode))
    {
        return;
    }

    mFontCache.EnsureFont(fontName);

    const Matrix3 objectTM = inode ? inode->GetObjectTM(t) : Matrix3();
    const float screenScale = vpt ? CADScreenScale(vpt, objectTM) : 1.0f;
    const float widthWorld = sizeMode == 1 ? cellWidth : cellWidth * screenScale;
    const float heightWorld =
        sizeMode == 1 ? cellHeight : cellHeight * screenScale;
    const float width = cols * widthWorld;
    const float height = rows * heightWorld;
    box += insertPos;
    box += insertPos + Point3(width, height, 0.0f);

    const float maxCellPixels =
        sizeMode == 1
            ? (screenScale > 0.0f ? cellWidth / screenScale : cellWidth)
            : cellWidth;
    EnsureFont(mFontCache);
    if (rows * cols <= 400)
    {
        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                const size_t index = static_cast<size_t>(row) * cols + col;
                if (index >= mCells.size() || mCells[index].Length() == 0)
                {
                    continue;
                }
                const Point3 textAnchor =
                    insertPos + Point3((col + 0.1f) * widthWorld,
                                       (row + 0.7f) * heightWorld, 0.0f);
                if (sizeMode == 1)
                {
                    const float worldTextHeight =
                        ResolveTableWorldTextHeight(cellHeight, fontSize);
                    const float measureHeight =
                        screenScale > 0.0f ? worldTextHeight / screenScale
                                           : worldTextHeight;
                    const MSTR display = FitCellText(
                        mFontCache, mCells[index], maxCellPixels,
                        measureHeight);
                    const CADTextMetrics metrics = CADMeasureText(
                        mFontCache, display.data(), measureHeight);
                    const float textWidthWorld = metrics.widthPx * screenScale;
                    CADAddWorldTextBox(vpt, objectTM, textAnchor, 0.0f, 0.0f,
                                       textWidthWorld, worldTextHeight, box);
                }
                else
                {
                    const float pointSize =
                        ResolveTablePointSize(cellHeight, fontSize);
                    const MSTR display = FitCellText(
                        mFontCache, mCells[index], maxCellPixels, pointSize);
                    const CADTextMetrics metrics = CADMeasureText(
                        mFontCache, display.data(), pointSize);
                    CADAddScreenTextBox(vpt, objectTM, textAnchor, 0.0f, 0.0f,
                                        metrics.widthPx, metrics.heightPx,
                                        screenScale, box);
                }
            }
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
    MSTR fontName;
    float fontSize = 0.0f;
    int sizeMode = 0;
    if (!GetParams(t, insertPos, rows, cols, cellWidth, cellHeight,
                   lineColor, textColor, fontName, fontSize, sizeMode))
    {
        return 0;
    }

    mFontCache.EnsureFont(fontName);

    const Matrix3 objectTM = inode->GetObjectTM(t);
    const float screenScale = CADScreenScale(vpt, objectTM);
    const float widthWorld = sizeMode == 1 ? cellWidth : cellWidth * screenScale;
    const float heightWorld =
        sizeMode == 1 ? cellHeight : cellHeight * screenScale;
    const Point3 line(lineColor.r, lineColor.g, lineColor.b);
    const Point3 text(textColor.r, textColor.g, textColor.b);

    GWScreenGuard screenGuard(gw);

    const float cellWidthPx =
        sizeMode == 1
            ? (screenScale > 0.0f ? cellWidth / screenScale : cellWidth)
            : cellWidth;
    const float cellHeightPx =
        sizeMode == 1
            ? (screenScale > 0.0f ? cellHeight / screenScale : cellHeight)
            : cellHeight;
    const int visibleCells = EstimateVisibleCells(
        gw, objectTM, insertPos, rows, cols, widthWorld, heightWorld,
        cellWidthPx, cellHeightPx);
    const float pixelHeightAtInsert =
        sizeMode == 1
            ? CADWorldHeightToPixels(
                  vpt, insertPos * objectTM,
                  ResolveTableWorldTextHeight(cellHeight, fontSize))
            : ResolveTablePointSize(cellHeight, fontSize);
    const bool wasFallback = mUseFontFallback;
    if (!mUseFontFallback)
    {
        if (visibleCells > 480 || pixelHeightAtInsert < 8.0f)
        {
            mUseFontFallback = true;
            ++mFallbackEntries;
        }
    }
    else if (visibleCells <= 360 && pixelHeightAtInsert >= 10.0f)
    {
        mUseFontFallback = false;
    }
    mLastVisibleCells = visibleCells;
    if (wasFallback != mUseFontFallback)
    {
        Interface* ip = GetCOREInterface();
        LogSys* log = ip ? ip->Log() : nullptr;
        if (log)
        {
            log->LogEntry(
                SYSLOG_INFO, FALSE, MSTR(_T("CADTable")),
                _T("font path=%s visible=%d px=%.1f entries=%d"),
                mUseFontFallback ? _T("fallback") : _T("bezfont"),
                visibleCells, pixelHeightAtInsert, mFallbackEntries);
        }
    }

    IPoint2 segA[kMaxGridSegments] = {};
    IPoint2 segB[kMaxGridSegments] = {};
    const int segmentCount =
        BuildTableGridScreen(gw, objectTM, insertPos, rows, cols, widthWorld,
                             heightWorld, segA, segB, kMaxGridSegments);
    for (int i = 0; i < segmentCount; ++i)
    {
        CADDrawScreenSegment(gw, segA[i], segB[i], line);
    }

    const float maxCellPixels =
        sizeMode == 1
            ? (screenScale > 0.0f ? cellWidth / screenScale : cellWidth)
            : cellWidth;
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

            float cellPixelHeight = pixelHeightAtInsert;
            if (sizeMode == 1 && !mUseFontFallback)
            {
                cellPixelHeight = CADWorldHeightToPixels(
                    vpt, center * objectTM,
                    ResolveTableWorldTextHeight(cellHeight, fontSize));
            }

            if (mUseFontFallback)
            {
                const float fallbackPx = static_cast<float>(
                    ClampInt(static_cast<int>(cellPixelHeight + 0.5f), 6, 48));
                const MSTR display = FitCellText(mFontCache, mCells[index],
                                                 maxCellPixels, fallbackPx);
                CADDrawScreenText(gw, p, display.data(), fallbackPx, text);
            }
            else
            {
                const MSTR display = FitCellText(mFontCache, mCells[index],
                                                 maxCellPixels,
                                                 cellPixelHeight);
                CADDrawScreenBezFontText(gw, p, display.data(),
                                         cellPixelHeight, text, mFontCache);
            }
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
    MSTR fontName;
    float fontSize = 0.0f;
    int sizeMode = 0;
    if (!GetParams(t, insertPos, rows, cols, cellWidth, cellHeight,
                   lineColor, textColor, fontName, fontSize, sizeMode))
    {
        return 0;
    }

    HitRegion hitRegion;
    MakeHitRegion(hitRegion, type, crossing, 4, p);

    mFontCache.EnsureFont(fontName);

    const Matrix3 objectTM = inode->GetObjectTM(t);
    const float screenScale = CADScreenScale(vpt, objectTM);

    GWScreenGuard screenGuard(gw);

    IPoint2 segA[kMaxGridSegments] = {};
    IPoint2 segB[kMaxGridSegments] = {};
    const float widthWorld = sizeMode == 1 ? cellWidth : cellWidth * screenScale;
    const float heightWorld =
        sizeMode == 1 ? cellHeight : cellHeight * screenScale;
    const int segmentCount =
        BuildTableGridScreen(gw, objectTM, insertPos, rows, cols, widthWorld,
                             heightWorld, segA, segB, kMaxGridSegments);

    return CADHitScreenSegments(hitRegion, segA, segB, segmentCount) ? 1 : 0;
}
