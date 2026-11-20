#include "CADDimension.h"
#include "CADAnnotationCommon.h"
#include "CADParamBlockHost.h"

#include <GetCOREInterface.h>
#include <commctrl.h>
#include <iparamm2.h>
#include <maxapi.h>
#include <mouseman.h>
#include <object.h>
#include <tchar.h>

#include <cmath>

namespace
{
enum DimParam
{
    dim_p0,
    dim_p1,
    dim_p2,
    dim_showxyz,
    dim_arrow,
    dim_texth,
    dim_extoff,
    dim_linecol,
    dim_textcol,
    dim_prec,
    dim_font_name,
};

constexpr BlockID kDimParams = 0x1001;
constexpr USHORT kDimChunkVersion = 0x0F01;
constexpr USHORT kDimChunkPoints = 0x0F02;
constexpr DWORD kDimChunkVersionValue = 1;
constexpr int kPrecisionMin = 0;
constexpr int kPrecisionMax = 6;
}

class CADDimensionClassDesc : public ClassDesc2
{
public:
    int IsPublic() override { return TRUE; }
    void* Create(BOOL) override { return new CADDimension(); }
    const TCHAR* ClassName() override { return GetString(IDS_DIM_CLASS); }
    const TCHAR* NonLocalizedClassName() override { return _T("CADDimension"); }
    SClass_ID SuperClassID() override { return HELPER_CLASS_ID; }
    Class_ID ClassID() override { return kCADDimensionClassID; }
    const TCHAR* Category() override { return GetString(IDS_CATEGORY); }
    const TCHAR* InternalName() override { return _T("CADDimension"); }
    HINSTANCE HInstance() override { return hInstance; }
};

static CADDimensionClassDesc gDimDesc;

ClassDesc2* GetCADDimensionDesc()
{
    return &gDimDesc;
}

static ParamBlockDesc2 gDimParamBlock(
    kDimParams, _T("CADDimensionParams"), 0, &gDimDesc,
    P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, 1, 0,
    IDD_CAD_DIM, IDS_PARAMS, 0, 0, nullptr,

    dim_p0, _T("p0"), TYPE_POINT3, 0, IDS_DIM_P0,
        p_default, Point3(0.0f, 0.0f, 0.0f),
        p_range, float(-1.0e30), float(1.0e30),
        p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE,
            IDC_DIM_P0X, IDC_DIM_P0XSPIN,
            IDC_DIM_P0Y, IDC_DIM_P0YSPIN,
            IDC_DIM_P0Z, IDC_DIM_P0ZSPIN, SPIN_AUTOSCALE,
        p_end,

    dim_p1, _T("p1"), TYPE_POINT3, 0, IDS_DIM_P1,
        p_default, Point3(100.0f, 0.0f, 0.0f),
        p_range, float(-1.0e30), float(1.0e30),
        p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE,
            IDC_DIM_P1X, IDC_DIM_P1XSPIN,
            IDC_DIM_P1Y, IDC_DIM_P1YSPIN,
            IDC_DIM_P1Z, IDC_DIM_P1ZSPIN, SPIN_AUTOSCALE,
        p_end,

    dim_p2, _T("p2"), TYPE_POINT3, 0, IDS_DIM_P2,
        p_default, Point3(100.0f, 50.0f, 0.0f),
        p_range, float(-1.0e30), float(1.0e30),
        p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE,
            IDC_DIM_P2X, IDC_DIM_P2XSPIN,
            IDC_DIM_P2Y, IDC_DIM_P2YSPIN,
            IDC_DIM_P2Z, IDC_DIM_P2ZSPIN, SPIN_AUTOSCALE,
        p_end,

    dim_showxyz, _T("showXYZ"), TYPE_BOOL, 0, IDS_DIM_SHOWXYZ,
        p_default, FALSE,
        p_ui, TYPE_SINGLECHECKBOX, IDC_DIM_SHOWXYZ,
        p_end,

    dim_arrow, _T("arrowSize"), TYPE_FLOAT, 0, IDS_DIM_ARROW,
        p_default, 8.0f,
        p_range, 0.1f, 1000.0f,
        p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_DIM_ARROW, IDC_DIM_ARROWSPIN, 0.1f,
        p_end,

    dim_texth, _T("textHeight"), TYPE_FLOAT, 0, IDS_DIM_TEXTH,
        p_default, 12.0f,
        p_range, 1.0f, 1000.0f,
        p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_DIM_TEXTH, IDC_DIM_TEXTHSPIN, 0.1f,
        p_end,

    dim_extoff, _T("extensionOffset"), TYPE_FLOAT, 0, IDS_DIM_EXTOFF,
        p_default, 5.0f,
        p_range, 0.0f, 1000.0f,
        p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_DIM_EXTOFF, IDC_DIM_EXTOFFSPIN, 0.1f,
        p_end,

    dim_linecol, _T("lineColor"), TYPE_RGBA, 0, IDS_DIM_LINECOL,
        p_default, Color(0.1f, 0.8f, 0.1f),
        p_ui, TYPE_COLORSWATCH, IDC_DIM_LINECOL,
        p_end,

    dim_textcol, _T("textColor"), TYPE_RGBA, 0, IDS_DIM_TEXTCOL,
        p_default, Color(1.0f, 1.0f, 1.0f),
        p_ui, TYPE_COLORSWATCH, IDC_DIM_TEXTCOL,
        p_end,

    dim_prec, _T("textPrecision"), TYPE_INT, 0, IDS_DIM_PREC,
        p_default, 2,
        p_range, kPrecisionMin, kPrecisionMax,
        p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_DIM_PREC, IDC_DIM_PRECSPIN, 0.1f,
        p_end,

    dim_font_name, _T("fontName"), TYPE_STRING, 0, IDS_DIM_FONT,
        p_default, _T("Microsoft YaHei"),
        p_end,

    p_end);

CADDimension::CADDimension()
{
    GetCADDimensionDesc()->MakeAutoParamBlocks(this);
}

void CADDimension::SetReference(int index, RefTargetHandle target)
{
    if (index == 0)
    {
        CADSetPBlockRef(mPBlock, target);
    }
}

RefResult CADDimension::NotifyRefChanged(const Interval&, RefTargetHandle hTarget,
                                         PartID& partID, RefMessage message,
                                         BOOL propagate)
{
    if (message == REFMSG_CHANGE)
    {
        mFontCache.ClearGlyphs();
        IParamMap2* map = mPBlock ? mPBlock->GetMap(0) : nullptr;
        HWND hDlg = map ? map->GetHWnd() : nullptr;
        if (hDlg)
        {
            CADSyncFontComboSelection(hDlg, IDC_DIM_FONT, mPBlock,
                                      dim_font_name);
        }
    }
    return CADNotifyPBlockChange(this, mPBlock, hTarget, partID, message,
                                 propagate, GetCOREInterface(), nullptr);
}

IParamBlock2* CADDimension::GetParamBlockByID(BlockID id)
{
    return (mPBlock && id == kDimParams) ? mPBlock : nullptr;
}

bool CADDimension::GetPoints(TimeValue t, Point3 out[3])
{
    if (!mPBlock)
    {
        return false;
    }

    Interval validity;
    const BOOL ok = mPBlock->GetValue(dim_p0, t, out[0], validity) &&
                    mPBlock->GetValue(dim_p1, t, out[1], validity) &&
                    mPBlock->GetValue(dim_p2, t, out[2], validity);
    return ok && CADIsFinite(out[0]) && CADIsFinite(out[1]) &&
           CADIsFinite(out[2]);
}

bool CADDimension::GetParams(TimeValue t, bool& showXYZ, float& arrowSize,
                             float& textHeight, float& extensionOffset,
                             Color& lineColor, Color& textColor, int& precision,
                             MSTR& fontName)
{
    if (!mPBlock)
    {
        return false;
    }

    Interval validity;
    BOOL showValue = FALSE;
    mPBlock->GetValue(dim_showxyz, t, showValue, validity);
    mPBlock->GetValue(dim_arrow, t, arrowSize, validity);
    mPBlock->GetValue(dim_texth, t, textHeight, validity);
    mPBlock->GetValue(dim_extoff, t, extensionOffset, validity);
    mPBlock->GetValue(dim_prec, t, precision, validity);
    lineColor = mPBlock->GetColor(dim_linecol, t, validity);
    textColor = mPBlock->GetColor(dim_textcol, t, validity);
    const MCHAR* fontValue = mPBlock->GetStr(dim_font_name, t, validity);
    fontName = fontValue ? fontValue : _T("");
    showXYZ = showValue != FALSE;

    if (precision < kPrecisionMin)
    {
        precision = kPrecisionMin;
    }
    else if (precision > kPrecisionMax)
    {
        precision = kPrecisionMax;
    }
    return true;
}

void CADDimension::SetCreationPoint(int index, const Point3& localPoint)
{
    if (!mPBlock || !CADIsFinite(localPoint))
    {
        return;
    }

    if (index <= 0)
    {
        mPBlock->SetValue(dim_p0, 0, localPoint);
        mPBlock->SetValue(dim_p1, 0, localPoint);
        mPBlock->SetValue(dim_p2, 0, localPoint);
    }
    else if (index == 1)
    {
        mPBlock->SetValue(dim_p1, 0, localPoint);
        mPBlock->SetValue(dim_p2, 0, localPoint);
    }
    else
    {
        mPBlock->SetValue(dim_p2, 0, localPoint);
    }

    NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
    Interface* ip = GetCOREInterface();
    if (ip)
    {
        ip->RedrawViews(ip->GetTime(), REDRAW_INTERACTIVE);
    }
}

class CADDimensionCreateCallBack : public CreateMouseCallBack
{
public:
    int proc(ViewExp* vpt, int msg, int point, int flags, IPoint2 m,
             Matrix3& mat) override
    {
        (void)flags;
        if (!vpt || !vpt->IsAlive() || !mObj)
        {
            return CREATE_ABORT;
        }

        if (msg == MOUSE_FREEMOVE)
        {
            vpt->SnapPreview(m, m, nullptr, SNAP_IN_3D);
            return TRUE;
        }
        if (msg == MOUSE_ABORT)
        {
            return CREATE_ABORT;
        }
        if (msg != MOUSE_POINT && msg != MOUSE_MOVE)
        {
            return TRUE;
        }

        const Point3 world = vpt->SnapPoint(m, m, nullptr, SNAP_IN_3D);
        const Point3 local = world * Inverse(mat);
        if (point <= 0)
        {
            mObj->SetCreationPoint(0, local);
            return CREATE_CONTINUE;
        }
        if (point == 1)
        {
            mObj->SetCreationPoint(1, local);
            return CREATE_CONTINUE;
        }

        mObj->SetCreationPoint(2, local);
        return msg == MOUSE_POINT ? CREATE_STOP : CREATE_CONTINUE;
    }

    int override(int mode) override
    {
        (void)mode;
        return CLICK_MOVE_CLICK;
    }

    void SetObj(CADDimension* obj) { mObj = obj; }

private:
    CADDimension* mObj = nullptr;
};

static CADDimensionCreateCallBack gDimensionCreateCB;

CreateMouseCallBack* CADDimension::GetCreateMouseCallBack()
{
    gDimensionCreateCB.SetObj(this);
    return &gDimensionCreateCB;
}

namespace
{
constexpr UINT_PTR kDimensionDialogSubclassId = 0x43414444;

LRESULT CALLBACK DimensionDialogSubclass(HWND hWnd, UINT msg, WPARAM wParam,
                                         LPARAM lParam, UINT_PTR,
                                         DWORD_PTR refData)
{
    if (msg == WM_CTLCOLORDLG || msg == WM_CTLCOLORSTATIC ||
        msg == WM_CTLCOLORBTN || msg == WM_CTLCOLOREDIT ||
        msg == WM_CTLCOLORLISTBOX)
    {
        const INT_PTR brush = CADApplyThemeColors(msg, wParam);
        if (brush)
        {
            return brush;
        }
    }
    else if (msg == WM_COMMAND && HIWORD(wParam) == CBN_SELCHANGE &&
             LOWORD(wParam) == IDC_DIM_FONT)
    {
        CADDimension* dimension = reinterpret_cast<CADDimension*>(refData);
        CADApplyFontCombo(hWnd, IDC_DIM_FONT,
                          dimension ? dimension->GetParamBlock(0) : nullptr,
                          dim_font_name, dimension, IDS_DIM_FONT);
        return 0;
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}
}

void CADDimension::BeginEditParams(IObjParam* ip, ULONG flags, Animatable* prev)
{
    CADBeginAutoUI(GetCADDimensionDesc(), ip, this, flags, prev);
    IParamMap2* map = mPBlock ? mPBlock->GetMap(0) : nullptr;
    HWND hDlg = map ? map->GetHWnd() : nullptr;
    if (hDlg)
    {
        RemoveWindowSubclass(hDlg, DimensionDialogSubclass,
                             kDimensionDialogSubclassId);
        SetWindowSubclass(hDlg, DimensionDialogSubclass,
                          kDimensionDialogSubclassId,
                          reinterpret_cast<DWORD_PTR>(this));
        CADPopulateFontCombo(hDlg, IDC_DIM_FONT, mPBlock, dim_font_name);
    }
}

void CADDimension::EndEditParams(IObjParam* ip, ULONG flags, Animatable* next)
{
    IParamMap2* map = mPBlock ? mPBlock->GetMap(0) : nullptr;
    HWND hDlg = map ? map->GetHWnd() : nullptr;
    if (hDlg)
    {
        RemoveWindowSubclass(hDlg, DimensionDialogSubclass,
                             kDimensionDialogSubclassId);
    }
    CADEndAutoUI(GetCADDimensionDesc(), ip, this, flags, next);
}

class CADDimensionPostLoad : public PostLoadCallback
{
public:
    CADDimensionPostLoad(CADDimension* obj, const Point3 points[3]) : mObj(obj)
    {
        for (int i = 0; i < 3; ++i)
        {
            mPoints[i] = points[i];
        }
    }

    void proc(ILoad*) override
    {
        if (mObj)
        {
            IParamBlock2* pblock = mObj->GetParamBlockByID(kDimParams);
            if (pblock)
            {
                pblock->SetValue(dim_p0, 0, mPoints[0]);
                pblock->SetValue(dim_p1, 0, mPoints[1]);
                pblock->SetValue(dim_p2, 0, mPoints[2]);
            }
            mObj->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
        }
        delete this;
    }

private:
    CADDimension* mObj = nullptr;
    Point3 mPoints[3];
};

IOResult CADDimension::Save(ISave* isave)
{
    if (!isave || !mPBlock)
    {
        return IO_ERROR;
    }

    Point3 points[3] = {};
    if (!GetPoints(0, points))
    {
        return IO_ERROR;
    }

    ULONG written = 0;
    isave->BeginChunk(kDimChunkVersion);
    const DWORD version = kDimChunkVersionValue;
    IOResult result = isave->Write(&version, sizeof(version), &written);
    isave->EndChunk();
    if (result != IO_OK)
    {
        return result;
    }

    isave->BeginChunk(kDimChunkPoints);
    const DWORD count = 3;
    result = isave->Write(&count, sizeof(count), &written);
    if (result == IO_OK)
    {
        result = isave->Write(points, static_cast<ULONG>(sizeof(Point3) * count),
                              &written);
    }
    isave->EndChunk();
    return result;
}

IOResult CADDimension::Load(ILoad* iload)
{
    if (!iload || !mPBlock)
    {
        return IO_ERROR;
    }

    IOResult result = IO_OK;
    Point3 loadedPoints[3] = {};
    bool havePoints = false;

    while (IO_OK == (result = iload->OpenChunk()))
    {
        if (iload->CurChunkID() == kDimChunkVersion)
        {
            DWORD version = 0;
            ULONG read = 0;
            result = iload->Read(&version, sizeof(version), &read);
            if (result == IO_OK && version > kDimChunkVersionValue)
            {
                result = IO_ERROR;
            }
        }
        else if (iload->CurChunkID() == kDimChunkPoints)
        {
            DWORD count = 0;
            ULONG read = 0;
            result = iload->Read(&count, sizeof(count), &read);
            if (result == IO_OK && count != 3)
            {
                result = IO_ERROR;
            }
            if (result == IO_OK)
            {
                result = iload->Read(
                    loadedPoints,
                    static_cast<ULONG>(sizeof(loadedPoints)), &read);
            }
            if (result == IO_OK &&
                (!CADIsFinite(loadedPoints[0]) ||
                 !CADIsFinite(loadedPoints[1]) ||
                 !CADIsFinite(loadedPoints[2])))
            {
                result = IO_ERROR;
            }
            if (result == IO_OK)
            {
                havePoints = true;
            }
        }

        const IOResult closeResult = iload->CloseChunk();
        if (result == IO_OK)
        {
            result = closeResult;
        }
        if (result != IO_OK)
        {
            return result;
        }
    }

    if (result != IO_OK && result != IO_END)
    {
        return result;
    }

    if (havePoints)
    {
        // The pblock mirror is restored by the reference system at an
        // unspecified point relative to Load(); defer the chunk override until
        // every reference is in place so the chunk stays authoritative
        // (B-12/B7).
        iload->RegisterPostLoadCallback(
            new CADDimensionPostLoad(this, loadedPoints));
    }
    return IO_OK;
}

RefTargetHandle CADDimension::Clone(RemapDir& remap)
{
    CADDimension* clone = new CADDimension();
    clone->ReplaceReference(0, CADClonePBlockRef(mPBlock, remap));
    BaseClone(this, clone, remap);
    return clone;
}
