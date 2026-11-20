#include "CADBezFontText.h"
#include "CADAnnotationCommon.h"

#include <maxchar.h>

#include <cmath>
#include <vector>

namespace
{
constexpr float kBezFontReferenceHeight = 1000.0f;
constexpr float kMissingGlyphAdvance = 300.0f;
constexpr int kGlyphSamplesPerSegment = 8;
constexpr float kMinFontHeight = 0.0f;

bool BuildGlyph(BEZFONTHANDLE handle, UINT code, CADBezGlyph& glyph)
{
    glyph.width = 0.0f;
    glyph.valid = false;

    const auto tryBuild = [&](UINT buildCode, bool native) -> bool
    {
        glyph.shape.Init();
        glyph.width = 0.0f;
        return theBezFontManager.BuildCharacterEx(
            handle, buildCode, kBezFontReferenceHeight,
            glyph.shape, glyph.width, 1, native) != FALSE;
    };

    bool valid = false;
    if (code > 0x7fU)
    {
        valid = tryBuild(code | 0x10000000U, true);
        if (!valid)
        {
            valid = tryBuild(code, true);
        }
    }
    else
    {
        // Character codes must use the native/Unicode path. With
        // ggo_native=false the manager treats the value as a glyph index,
        // which renders ASCII 'V03' as glyph indices for 'sMP'.
        valid = tryBuild(code, true);
    }

    if (!valid)
    {
        glyph.width = 0.0f;
        glyph.shape.Init();
    }
    glyph.valid = valid;
    return valid;
}

void DrawGlyphOutline(GraphicsWindow* gw, CADBezGlyph& glyph,
                      const Point3& origin, const Point3& right,
                      const Point3& up, const Point3& normal,
                      float penOffset, float scale, const Point3& color)
{
    if (!glyph.valid)
    {
        return;
    }

    const Point3 glyphOrigin = origin + right * penOffset;
    for (int splineIndex = 0; splineIndex < glyph.shape.splineCount; ++splineIndex)
    {
        Spline3D* spline = glyph.shape.splines[splineIndex];
        if (!spline || spline->KnotCount() < 2 || spline->Segments() <= 0)
        {
            continue;
        }

        for (int segment = 0; segment < spline->Segments(); ++segment)
        {
            Point3 points[kGlyphSamplesPerSegment + 1];
            bool validSegment = true;
            for (int sample = 0; sample <= kGlyphSamplesPerSegment; ++sample)
            {
                const float t = static_cast<float>(sample) /
                                static_cast<float>(kGlyphSamplesPerSegment);
                const Point3 glyphPoint =
                    spline->InterpBezier3D(segment, t, SPLINE_INTERP_SIMPLE);
                if (!CADIsFinite(glyphPoint))
                {
                    validSegment = false;
                    break;
                }

                points[sample] = glyphOrigin +
                    (right * glyphPoint.x + up * glyphPoint.y +
                     normal * glyphPoint.z) * scale;
            }

            if (validSegment)
            {
                CADDrawPolyline(gw, points,
                                kGlyphSamplesPerSegment + 1, color);
            }
        }
    }
}
}

CADBezFontCache::~CADBezFontCache()
{
    Close();
}

bool CADBezFontCache::SetFont(const MSTR& requestedName, bool* usedFallback)
{
    if (usedFallback)
    {
        *usedFallback = false;
    }

    if (mHandle && (requestedName.Length() == 0 || requestedName == mFontName))
    {
        if (usedFallback)
        {
            *usedFallback = mFallback;
        }
        return true;
    }

    Close();
    const bool opened = OpenFirstAvailable(requestedName);
    if (usedFallback)
    {
        *usedFallback = mFallback;
    }
    return opened;
}

void CADBezFontCache::Close()
{
    if (mCjkFallbackHandle && mCjkFallbackHandle != mHandle)
    {
        theBezFontManager.CloseFont(mCjkFallbackHandle);
    }
    if (mHandle)
    {
        theBezFontManager.CloseFont(mHandle);
    }
    mHandle = 0;
    mCjkFallbackHandle = 0;
    mFontName = _T("");
    mCjkFallbackFontName = _T("");
    mFallback = false;
    mMissingGlyph = false;
    mGlyphs.clear();
}

void CADBezFontCache::ClearGlyphs()
{
    mGlyphs.clear();
    mMissingGlyph = false;
}

bool CADBezFontCache::OpenFirstAvailable(const MSTR& requestedName)
{
    std::vector<MSTR> candidates;
    if (requestedName.Length() > 0)
    {
        candidates.push_back(requestedName);
    }

    const MCHAR* fallbackNames[] = {
        _T("Microsoft YaHei"),
        _T("SimSun"),
        _T("Arial"),
        _T("Segoe UI"),
    };
    for (const MCHAR* name : fallbackNames)
    {
        const MSTR candidate(name);
        bool duplicate = false;
        for (const MSTR& existing : candidates)
        {
            if (existing == candidate)
            {
                duplicate = true;
                break;
            }
        }
        if (!duplicate)
        {
            candidates.push_back(candidate);
        }
    }

    for (const MSTR& candidate : candidates)
    {
        if (candidate.Length() == 0)
        {
            continue;
        }

        const BEZFONTHANDLE handle = theBezFontManager.OpenFont(candidate, 0);
        if (handle)
        {
            mHandle = handle;
            mFontName = candidate;
            mFallback = requestedName.Length() > 0 &&
                        candidate != requestedName;

            // If the selected font is not one of the CJK candidates, open a
            // secondary CJK font for characters missing from the primary.
            const MCHAR* cjkNames[] = {
                _T("Microsoft YaHei"),
                _T("SimSun"),
            };
            for (const MCHAR* cjkName : cjkNames)
            {
                const MSTR cjkCandidate(cjkName);
                if (cjkCandidate == mFontName)
                {
                    break;
                }
                const BEZFONTHANDLE cjkHandle =
                    theBezFontManager.OpenFont(cjkCandidate, 0);
                if (cjkHandle)
                {
                    mCjkFallbackHandle = cjkHandle;
                    mCjkFallbackFontName = cjkCandidate;
                    break;
                }
            }
            return true;
        }
    }

    mHandle = 0;
    mFontName = requestedName;
    mFallback = false;
    return false;
}

CADBezGlyph* CADBezFontCache::GetGlyph(UINT code)
{
    if (mHandle == 0)
    {
        return nullptr;
    }

    const auto found = mGlyphs.find(code);
    if (found != mGlyphs.end())
    {
        return found->second.get();
    }

    auto glyph = std::make_unique<CADBezGlyph>();
    glyph->width = 0.0f;
    glyph->valid = false;

    glyph->valid = BuildGlyph(mHandle, code, *glyph);
    if (!glyph->valid && mCjkFallbackHandle != 0 &&
        mCjkFallbackHandle != mHandle)
    {
        glyph->valid = BuildGlyph(mCjkFallbackHandle, code, *glyph);
    }

    if (!glyph->valid)
    {
        mMissingGlyph = true;
        glyph->width = 0.0f;
    }

    CADBezGlyph* result = glyph.get();
    mGlyphs.emplace(code, std::move(glyph));
    return result;
}

float CADBezFontCache::TextWidth(const MCHAR* text)
{
    if (!text || !*text || mHandle == 0)
    {
        return 0.0f;
    }

    float width = 0.0f;
    MaxSDK::Util::CharIterator<const TCHAR> it(text);
    for (; !it.AtEnd(); ++it)
    {
        const MaxSDK::Util::Char character = it.Get();
        const CADBezGlyph* glyph = GetGlyph(character.CharCode());
        width += (glyph && glyph->valid) ? glyph->width : kMissingGlyphAdvance;
    }
    return width;
}

bool CADGetBillboardAxes(ViewExp* vpt, Point3& right, Point3& up,
                         Point3& normal)
{
    if (!vpt || !vpt->IsAlive())
    {
        return false;
    }

    Matrix3 affine;
    vpt->GetAffineTM(affine);
    const Matrix3 viewToWorld = Inverse(affine);

    return CADNormalizeSafe(viewToWorld.GetRow(0), right) &&
           CADNormalizeSafe(viewToWorld.GetRow(1), up) &&
           CADNormalizeSafe(viewToWorld.GetRow(2), normal);
}

bool CADDrawBezFontText(GraphicsWindow* gw, ViewExp* vpt,
                        const Matrix3& objectTM, const Point3& objectPos,
                        const MCHAR* text, float heightWorld,
                        const Point3& color, CADBezFontCache& cache)
{
    if (!gw || !vpt || !vpt->IsAlive() || !text || !*text ||
        !CADIsFinite(heightWorld) || heightWorld <= kMinFontHeight)
    {
        return false;
    }

    if (!cache.IsReady() && !cache.SetFont(MSTR(_T(""))))
    {
        return false;
    }

    Point3 right;
    Point3 up;
    Point3 normal;
    if (!CADGetBillboardAxes(vpt, right, up, normal))
    {
        return false;
    }

    const Point3 origin = objectPos * objectTM;
    const float scale = heightWorld / kBezFontReferenceHeight;

    GWTransformGuard transformGuard(gw);
    gw->setTransform(Matrix3());

    float penOffset = 0.0f;
    MaxSDK::Util::CharIterator<const TCHAR> it(text);
    for (; !it.AtEnd(); ++it)
    {
        const MaxSDK::Util::Char character = it.Get();
        const unsigned int code = character.CharCode();
        CADBezGlyph* glyph = cache.GetGlyph(code);
        if (!glyph || !glyph->valid)
        {
            penOffset += kMissingGlyphAdvance;
            continue;
        }

        DrawGlyphOutline(gw, *glyph, origin,
                         right, up, normal, penOffset * scale, scale, color);
        penOffset += glyph->width;
    }

    return true;
}

bool CADDrawScreenBezFontText(GraphicsWindow* gw, const IPoint2& anchor,
                              const MCHAR* text, float pixelHeight,
                              const Point3& color, CADBezFontCache& cache)
{
    if (!gw || !text || !*text || !CADIsFinite(pixelHeight) ||
        pixelHeight <= kMinFontHeight)
    {
        return false;
    }

    if (!cache.IsReady() && !cache.SetFont(MSTR(_T(""))))
    {
        return false;
    }

    const float scale = pixelHeight / kBezFontReferenceHeight;
    const Point3 right(1.0f, 0.0f, 0.0f);
    const Point3 up(0.0f, -1.0f, 0.0f);
    const Point3 normal(0.0f, 0.0f, 1.0f);
    const Point3 origin(static_cast<float>(anchor.x),
                        static_cast<float>(anchor.y) + pixelHeight, 0.0f);

    GWTransformGuard transformGuard(gw);
    gw->setTransform(Matrix3());

    float penOffset = 0.0f;
    MaxSDK::Util::CharIterator<const TCHAR> it(text);
    for (; !it.AtEnd(); ++it)
    {
        const MaxSDK::Util::Char character = it.Get();
        CADBezGlyph* glyph = cache.GetGlyph(character.CharCode());
        if (!glyph || !glyph->valid)
        {
            penOffset += kMissingGlyphAdvance;
            continue;
        }

        const Point3 glyphOrigin = origin + right * (penOffset * scale);
        for (int splineIndex = 0; splineIndex < glyph->shape.splineCount;
             ++splineIndex)
        {
            Spline3D* spline = glyph->shape.splines[splineIndex];
            if (!spline || spline->KnotCount() < 2 || spline->Segments() <= 0)
            {
                continue;
            }

            for (int segment = 0; segment < spline->Segments(); ++segment)
            {
                Point3 points[kGlyphSamplesPerSegment + 1];
                bool validSegment = true;
                for (int sample = 0; sample <= kGlyphSamplesPerSegment;
                     ++sample)
                {
                    const float t =
                        static_cast<float>(sample) /
                        static_cast<float>(kGlyphSamplesPerSegment);
                    const Point3 glyphPoint = spline->InterpBezier3D(
                        segment, t, SPLINE_INTERP_SIMPLE);
                    if (!CADIsFinite(glyphPoint))
                    {
                        validSegment = false;
                        break;
                    }
                    points[sample] =
                        glyphOrigin +
                        (right * glyphPoint.x + up * glyphPoint.y +
                         normal * glyphPoint.z) * scale;
                }

                if (!validSegment)
                {
                    continue;
                }

                for (int sample = 0; sample < kGlyphSamplesPerSegment; ++sample)
                {
                    const IPoint2 a(
                        static_cast<int>(points[sample].x + 0.5f),
                        static_cast<int>(points[sample].y + 0.5f));
                    const IPoint2 b(
                        static_cast<int>(points[sample + 1].x + 0.5f),
                        static_cast<int>(points[sample + 1].y + 0.5f));
                    CADDrawScreenSegment(gw, a, b, color);
                }
            }
        }

        penOffset += glyph->width;
    }

    return true;
}
