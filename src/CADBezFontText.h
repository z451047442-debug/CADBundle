#pragma once

#include "CADBundle.h"

#include <bezfont.h>
#include <shape.h>
#include <GraphicsWindow.h>
#include <maxapi.h>

#include <memory>
#include <unordered_map>

// One glyph outline built at the BezFont reference height of 1000 units.
struct CADBezGlyph
{
    BezierShape shape;
    float width = 0.0f;
    bool valid = false;
};

// Per-object BezFont handle and glyph cache.
//
// The glyph outlines are always built at height 1000; callers scale them by
// (fontSize * screenScale / 1000). Changing the font closes the old handle and
// discards the outline cache. The destructor closes the handle so a scene
// object does not leak a font reference.
class CADBezFontCache
{
public:
    CADBezFontCache() = default;
    ~CADBezFontCache();

    CADBezFontCache(const CADBezFontCache&) = delete;
    CADBezFontCache& operator=(const CADBezFontCache&) = delete;

    // Opens requestedName, falling back to a default/CJK font when it is not
    // available. Returns false only when no candidate font can be opened.
    bool SetFont(const MSTR& requestedName, bool* usedFallback = nullptr);

    // Reopens the font only when requestedName differs from the last request.
    // This is required because mFontName stores the *resolved* name, which can
    // differ from the request when a fallback font handled a missing font.
    void EnsureFont(const MSTR& requestedName);

    void Close();
    void ClearGlyphs();

    bool IsReady() const { return mHandle != 0; }
    bool IsFallback() const { return mFallback; }
    bool HasMissingGlyph() const { return mMissingGlyph; }
    const MSTR& FontName() const { return mFontName; }

    // Returns a cached glyph; an invalid glyph is returned for a missing
    // character and is not drawn. Width is in reference-height (1000) units.
    CADBezGlyph* GetGlyph(UINT code);

    // Sum of glyph advance widths in reference-height (1000) units.
    float TextWidth(const MCHAR* text);

private:
    bool OpenFirstAvailable(const MSTR& requestedName);

    BEZFONTHANDLE mHandle = 0;
    BEZFONTHANDLE mCjkFallbackHandle = 0;
    MSTR mRequestedName;
    MSTR mFontName;
    MSTR mCjkFallbackFontName;
    bool mFallback = false;
    bool mMissingGlyph = false;
    std::unordered_map<UINT, std::unique_ptr<CADBezGlyph>> mGlyphs;
};

// Measures text at a requested pixel height.
//
// TextWidth() returns advances in BezFont reference-height (1000) units, so the
// returned width is already converted to pixels: widthPx = TextWidth * heightPx
// / 1000. Callers must not apply the /1000 conversion a second time.
struct CADTextMetrics
{
    float widthPx = 0.0f;
    float heightPx = 0.0f;
};

CADTextMetrics CADMeasureText(CADBezFontCache& cache, const MCHAR* text,
                              float pixelHeight);

// Converts a world-space height at worldPos into pixels for the current view.
// Uses the billboard "up" axis and projects p / p + up * worldHeight, so the
// result is correct for both perspective and orthographic views. When the
// viewport is null/dead (for example in 3dsmaxbatch), this falls back to the
// same 1.0 scale used by CADScreenScale so bbox and hit tests stay stable.
float CADWorldHeightToPixels(ViewExp* vpt, const Point3& worldPos,
                             float worldHeight);

// Expands a local-space box with a screen-space text rectangle.
// The rectangle is in pixels relative to the projected anchor:
// left/right along screen +X and top/bottom along screen +Y (down).
// When the viewport is null/dead, a conservative axis-aligned local box is
// used so bbox and hit tests remain stable in batch mode.
void CADAddScreenTextBox(ViewExp* vpt, const Matrix3& objectTM,
                         const Point3& anchorLocal, float leftPx, float topPx,
                         float rightPx, float bottomPx, float screenScale,
                         Box3& box);

// Same rectangle in world units. Use this for model-space text, where the
// size must not depend on perspective or the object's distance from the eye.
void CADAddWorldTextBox(ViewExp* vpt, const Matrix3& objectTM,
                        const Point3& anchorLocal, float leftWorld,
                        float topWorld, float rightWorld, float bottomWorld,
                        Box3& box);

// Returns the screen-aligned axes (right, up, view normal) in world space.
bool CADGetBillboardAxes(ViewExp* vpt, Point3& right, Point3& up,
                         Point3& normal);

// Draws text as BezFont glyph outlines, billboarded toward the current view.
//
// objectTM and objectPos are both in object space: objectPos is transformed by
// objectTM exactly once. heightWorld is the already-scaled text height
// (fontSize * screenScale for the owning object). The function temporarily
// switches the GraphicsWindow to identity, draws world-space points, and
// restores the previous transform on exit.
bool CADDrawBezFontText(GraphicsWindow* gw, ViewExp* vpt,
                        const Matrix3& objectTM, const Point3& objectPos,
                        const MCHAR* text, float heightWorld,
                        const Point3& color, CADBezFontCache& cache);

// Draws text as BezFont glyph outlines directly in screen space (pixels).
// anchor is a top-left screen anchor; the glyph baseline is placed at
// anchor.y + pixelHeight. The function sets the GraphicsWindow transform to
// identity for the duration of the draw and restores it on exit.
bool CADDrawScreenBezFontText(GraphicsWindow* gw, const IPoint2& anchor,
                              const MCHAR* text, float pixelHeight,
                              const Point3& color, CADBezFontCache& cache);
