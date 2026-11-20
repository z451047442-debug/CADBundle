#pragma once

#include "CADBundle.h"

#include <GraphicsWindow.h>
#include <gfx.h>
#include <maxapi.h>
#include <object.h>

// Shared, object-agnostic helpers for the CAD annotation helpers.
//
// Coordinate conventions used throughout this module:
// - Geometry points are in object space unless a function explicitly says
//   "world".
// - ScreenScale() converts a viewport-independent size (the values stored in
//   the parameter blocks) into world units for the current viewport.
// - DrawPolyline() and DrawClosedPolyline() receive points already in the
//   coordinate system currently set on the GraphicsWindow.

// U4: s = NonScalingObjectSize() * GetVPWorldWidth(objectTM.GetTrans()) / 360.
// The translation of the object TM is the fixed reference point.
float CADScreenScale(ViewExp* vpt, const Matrix3& objectTM);

// Returns false for NaN/Inf or a zero-length vector.
bool CADNormalizeSafe(const Point3& value, Point3& normalized);
bool CADIsFinite(float value);
bool CADIsFinite(const Point3& value);

// Bounding boxes and simple hit-testing helpers.
Box3 CADBoxFromPoints(const Point3* points, int count);
Box3 CADExpandBox(const Box3& box, float margin);
float CADPointSegmentDistance(const Point3& point, const Point3& a, const Point3& b);

// Region-independent fixed-point formatting for distance labels.
// Precision is clamped to 0..6. A comma decimal separator is normalized to '.'.
MSTR CADFormatFixed(float value, int precision);

// Draws a connected polyline through DrawLineProc. This is the default path
// for all continuous annotation lines.
void CADDrawPolyline(GraphicsWindow* gw, const Point3* points, int count,
                     const Point3& color);

// Draws a closed polyline. This exists because DrawLineProc::proc()
// hardcodes closed=0 in the 3ds Max 2027 SDK; arrows are the documented
// exception that may use GraphicsWindow::polyline directly with closed=1.
void CADDrawClosedPolyline(GraphicsWindow* gw, const Point3* points, int count,
                           const Point3& color);

// Draws a closed triangular arrow outline. tip is the arrow point, baseDir is
// a unit direction pointing from the tip toward the base, length is the
// triangle length, and width is the full base width. The triangle lies in the
// plane whose normal is planeNormal.
void CADDrawArrowHead(GraphicsWindow* gw, const Point3& tip,
                      const Point3& baseDir, float length, float width,
                      const Point3& planeNormal, const Point3& color);

// Restores the graphics-window render limits on scope exit.
class GWRndLimitsGuard
{
public:
    explicit GWRndLimitsGuard(GraphicsWindow* gw)
        : mGw(gw), mOldLimits(gw ? gw->getRndLimits() : 0)
    {
    }

    ~GWRndLimitsGuard()
    {
        if (mGw)
        {
            mGw->setRndLimits(mOldLimits);
        }
    }

    GWRndLimitsGuard(const GWRndLimitsGuard&) = delete;
    GWRndLimitsGuard& operator=(const GWRndLimitsGuard&) = delete;

private:
    GraphicsWindow* mGw = nullptr;
    DWORD mOldLimits = 0;
};

// Restores the graphics-window transform on scope exit.
class GWTransformGuard
{
public:
    explicit GWTransformGuard(GraphicsWindow* gw)
        : mGw(gw), mOldTransform(gw ? gw->getTransform() : Matrix3())
    {
    }

    ~GWTransformGuard()
    {
        if (mGw)
        {
            mGw->setTransform(mOldTransform);
        }
    }

    GWTransformGuard(const GWTransformGuard&) = delete;
    GWTransformGuard& operator=(const GWTransformGuard&) = delete;

private:
    GraphicsWindow* mGw = nullptr;
    Matrix3 mOldTransform;
};

// Screen-space drawing helpers (U13). Callers must hold a GWScreenGuard so
// the GraphicsWindow transform is identity while wTransPoint/wPolyline/wText
// run. Values produced by the world-space geometry (sizes already multiplied
// by CADScreenScale) are projected here and drawn as pixels.
class GWScreenGuard
{
public:
    explicit GWScreenGuard(GraphicsWindow* gw)
        : mGw(gw),
          mOldTransform(gw ? gw->getTransform() : Matrix3()),
          mOldTextPoint(gw ? gw->GetTextPointSize() : 0.0f)
    {
        if (mGw)
        {
            mGw->setTransform(Matrix3());
        }
    }

    ~GWScreenGuard()
    {
        if (mGw)
        {
            mGw->SetTextPointSize(mOldTextPoint);
            mGw->setTransform(mOldTransform);
        }
    }

    GWScreenGuard(const GWScreenGuard&) = delete;
    GWScreenGuard& operator=(const GWScreenGuard&) = delete;

private:
    GraphicsWindow* mGw = nullptr;
    Matrix3 mOldTransform;
    float mOldTextPoint = 0.0f;
};

// Projects a world point with wTransPoint (perspective clipping aware).
bool CADWorldToScreen(GraphicsWindow* gw, const Point3& world, IPoint2& screen);

void CADDrawScreenSegment(GraphicsWindow* gw, const IPoint2& a,
                          const IPoint2& b, const Point3& color);

void CADDrawScreenPolyline(GraphicsWindow* gw, const IPoint2* points, int count,
                           bool closed, const Point3& color);

// Draws a closed triangular arrow in screen space. tip is the arrow point and
// toward is any point on the shaft side; length/width are pixels.
void CADDrawScreenArrow(GraphicsWindow* gw, const IPoint2& tip,
                        const IPoint2& toward, int lengthPx, int widthPx,
                        const Point3& color);

// Applies 3ds Max color-managed system colors to Win32 dialog messages
// (WM_CTLCOLORDLG/STATIC/BTN/EDIT/LISTBOX). Returns the brush for the
// message, or 0 when the message is not a color request.
INT_PTR CADApplyThemeColors(UINT msg, WPARAM wParam);

void CADDrawScreenText(GraphicsWindow* gw, const IPoint2& anchor,
                       const MCHAR* text, float pointSize, const Point3& color);

// Screen-space hit test against a set of pixel segments.
bool CADHitScreenSegments(const HitRegion& region, const IPoint2* a,
                          const IPoint2* b, int count);
