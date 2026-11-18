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
    MSTR mFontName;
    MSTR mCjkFallbackFontName;
    bool mFallback = false;
    bool mMissingGlyph = false;
    std::unordered_map<UINT, std::unique_ptr<CADBezGlyph>> mGlyphs;
};

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
