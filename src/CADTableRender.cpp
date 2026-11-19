#include "CADTable.h"
#include "CADAnnotationCommon.h"

#include <gfx.h>
#include <maxapi.h>
#include <object.h>
#include <tchar.h>

#include <algorithm>

namespace
{
MSTR FitCellText(GraphicsWindow* gw, const MSTR& text, float maxPixels)
{
    if (text.Length() <= 16)
    {
        return text;
    }

    SIZE extent = {};
    gw->getTextExtents(text.data(), &extent);
    if (extent.cx <= static_cast<LONG>(maxPixels))
    {
        return text;
    }

    MSTR result = text.Substr(0, 24);
    result += _T("...");
    return result;
}

void DrawTableGrid(GraphicsWindow* gw, const Point3& insertPos, int rows,
                   int cols, float cellWidth, float cellHeight,
                   const Point3& color)
{
    for (int row = 0; row <= rows; ++row)
    {
        const Point3 line[2] = {
            insertPos + Point3(0.0f, row * cellHeight, 0.0f),
            insertPos + Point3(cols * cellWidth, row * cellHeight, 0.0f),
        };
        CADDrawPolyline(gw, line, 2, color);
    }

    for (int col = 0; col <= cols; ++col)
    {
        const Point3 line[2] = {
            insertPos + Point3(col * cellWidth, 0.0f, 0.0f),
            insertPos + Point3(col * cellWidth, rows * cellHeight, 0.0f),
        };
        CADDrawPolyline(gw, line, 2, color);
    }

    const Point3 outline[4] = {
        insertPos,
        insertPos + Point3(cols * cellWidth, 0.0f, 0.0f),
        insertPos + Point3(cols * cellWidth, rows * cellHeight, 0.0f),
        insertPos + Point3(0.0f, rows * cellHeight, 0.0f),
    };
    CADDrawClosedPolyline(gw, outline, 4, color);
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
    gw->setTransform(objectTM);
    const float screenScale = CADScreenScale(vpt, objectTM);
    const float width = cellWidth * screenScale;
    const float height = cellHeight * screenScale;
    const Point3 line(lineColor.r, lineColor.g, lineColor.b);
    const Point3 text(textColor.r, textColor.g, textColor.b);

    DrawTableGrid(gw, insertPos, rows, cols, width, height, line);
    gw->setColor(TEXT_COLOR, text);
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            const size_t index = static_cast<size_t>(row) * cols + col;
            if (index >= mCells.size() || mCells[index].Length() == 0)
            {
                continue;
            }

            const MSTR display = FitCellText(gw, mCells[index], width);
            const Point3 position = insertPos +
                Point3(col * width + width * 0.1f,
                       row * height + height * 0.7f, 0.0f);
            gw->text(&position, display.data());
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
    DrawTableGrid(gw, insertPos, rows, cols, cellWidth * screenScale,
                  cellHeight * screenScale, line);
    return gw->checkHitCode();
}
