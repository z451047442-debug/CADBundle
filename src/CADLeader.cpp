#include "CADLeader.h"
#include "CADAnnotationCommon.h"
#include "CADParamBlockHost.h"

#include <GetCOREInterface.h>
#include <commctrl.h>
#include <hold.h>
#include <iparamm2.h>
#include <maxapi.h>
#include <mouseman.h>
#include <object.h>
#include <tchar.h>

namespace
{
enum LeaderParam
{
    leader_start,
    leader_elbow,
    leader_textpos,
    leader_note,
    leader_arrow,
    leader_texth,
    leader_linecol,
    leader_textcol,
    leader_font_name,
};

constexpr BlockID kLeaderParams = 0x1003;
constexpr USHORT kLeaderChunkVersion = 0x0F03;
constexpr USHORT kLeaderChunkPoints = 0x0F04;
constexpr DWORD kLeaderChunkVersionValue = 1;
}

class CADLeaderClassDesc : public ClassDesc2
{
public:
    int IsPublic() override { return TRUE; }
    void* Create(BOOL) override { return new CADLeader(); }
    const TCHAR* ClassName() override { return GetString(IDS_LEADER_CLASS); }
    const TCHAR* NonLocalizedClassName() override { return _T("CADLeader"); }
    SClass_ID SuperClassID() override { return HELPER_CLASS_ID; }
    Class_ID ClassID() override { return kCADLeaderClassID; }
    const TCHAR* Category() override { return GetString(IDS_CATEGORY); }
    const TCHAR* InternalName() override { return _T("CADLeader"); }
    HINSTANCE HInstance() override { return hInstance; }
};

static CADLeaderClassDesc gLeaderDesc;

ClassDesc2* GetCADLeaderDesc()
{
    return &gLeaderDesc;
}

static ParamBlockDesc2 gLeaderParamBlock(
    kLeaderParams, _T("CADLeaderParams"), 0, &gLeaderDesc,
    P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, 1, 0,
    IDD_CAD_LEADER, IDS_PARAMS, 0, 0, nullptr,

    leader_start, _T("start"), TYPE_POINT3, 0, IDS_LEADER_START,
        p_default, Point3(0.0f, 0.0f, 0.0f),
        p_range, float(-1.0e30), float(1.0e30),
        p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE,
            IDC_LEAD_STARTX, IDC_LEAD_STARTXSPIN,
            IDC_LEAD_STARTY, IDC_LEAD_STARTYSPIN,
            IDC_LEAD_STARTZ, IDC_LEAD_STARTZSPIN, SPIN_AUTOSCALE,
        p_end,

    leader_elbow, _T("elbow"), TYPE_POINT3, 0, IDS_LEADER_ELBOW,
        p_default, Point3(50.0f, 0.0f, 0.0f),
        p_range, float(-1.0e30), float(1.0e30),
        p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE,
            IDC_LEAD_ELBOWX, IDC_LEAD_ELBOWXSPIN,
            IDC_LEAD_ELBOWY, IDC_LEAD_ELBOWYSPIN,
            IDC_LEAD_ELBOWZ, IDC_LEAD_ELBOWZSPIN, SPIN_AUTOSCALE,
        p_end,

    leader_textpos, _T("textPos"), TYPE_POINT3, 0, IDS_LEADER_TEXTPOS,
        p_default, Point3(100.0f, 20.0f, 0.0f),
        p_range, float(-1.0e30), float(1.0e30),
        p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE,
            IDC_LEAD_TEXTPOSX, IDC_LEAD_TEXTPOSXSPIN,
            IDC_LEAD_TEXTPOSY, IDC_LEAD_TEXTPOSYSPIN,
            IDC_LEAD_TEXTPOSZ, IDC_LEAD_TEXTPOSZSPIN, SPIN_AUTOSCALE,
        p_end,

    leader_note, _T("note"), TYPE_STRING, 0, IDS_LEADER_NOTE,
        p_default, _T("注释"),
        p_ui, TYPE_EDITBOX, IDC_LEAD_NOTE,
        p_end,

    leader_arrow, _T("arrowSize"), TYPE_FLOAT, 0, IDS_LEADER_ARROW,
        p_default, 8.0f,
        p_range, 0.1f, 1000.0f,
        p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_LEAD_ARROW, IDC_LEAD_ARROWSPIN, 0.1f,
        p_end,

    leader_texth, _T("textHeight"), TYPE_FLOAT, 0, IDS_LEADER_TEXTH,
        p_default, 12.0f,
        p_range, 1.0f, 1000.0f,
        p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_LEAD_TEXTH, IDC_LEAD_TEXTHSPIN, 0.1f,
        p_end,

    leader_linecol, _T("lineColor"), TYPE_RGBA, 0, IDS_LEADER_LINECOL,
        p_default, Color(0.1f, 0.8f, 0.1f),
        p_ui, TYPE_COLORSWATCH, IDC_LEAD_LINECOL,
        p_end,

    leader_textcol, _T("textColor"), TYPE_RGBA, 0, IDS_LEADER_TEXTCOL,
        p_default, Color(1.0f, 1.0f, 1.0f),
        p_ui, TYPE_COLORSWATCH, IDC_LEAD_TEXTCOL,
        p_end,

    leader_font_name, _T("fontName"), TYPE_STRING, 0, IDS_LEAD_FONT,
        p_default, _T("Microsoft YaHei"),
        p_end,

    p_end);

CADLeader::CADLeader()
{
    GetCADLeaderDesc()->MakeAutoParamBlocks(this);
}

void CADLeader::SetReference(int index, RefTargetHandle target)
{
    if (index == 0)
    {
        CADSetPBlockRef(mPBlock, target);
    }
}

RefResult CADLeader::NotifyRefChanged(const Interval&, RefTargetHandle hTarget,
                                      PartID& partID, RefMessage message,
                                      BOOL propagate)
{
    if (message == REFMSG_CHANGE)
    {
        mFontCache.ClearGlyphs();
    }
    return CADNotifyPBlockChange(this, mPBlock, hTarget, partID, message,
                                 propagate, GetCOREInterface(), nullptr);
}

IParamBlock2* CADLeader::GetParamBlockByID(BlockID id)
{
    return (mPBlock && id == kLeaderParams) ? mPBlock : nullptr;
}

bool CADLeader::GetParams(TimeValue t, Point3& start, Point3& elbow,
                          Point3& textPos, MSTR& note, float& arrowSize,
                          float& textHeight, Color& lineColor,
                          Color& textColor, MSTR& fontName)
{
    if (!mPBlock)
    {
        return false;
    }

    Interval validity;
    mPBlock->GetValue(leader_start, t, start, validity);
    mPBlock->GetValue(leader_elbow, t, elbow, validity);
    mPBlock->GetValue(leader_textpos, t, textPos, validity);
    mPBlock->GetValue(leader_arrow, t, arrowSize, validity);
    mPBlock->GetValue(leader_texth, t, textHeight, validity);
    const MCHAR* noteValue = mPBlock->GetStr(leader_note, t, validity);
    const MCHAR* fontValue = mPBlock->GetStr(leader_font_name, t, validity);
    note = noteValue ? noteValue : _T("");
    fontName = fontValue ? fontValue : _T("");
    lineColor = mPBlock->GetColor(leader_linecol, t, validity);
    textColor = mPBlock->GetColor(leader_textcol, t, validity);
    return CADIsFinite(start) && CADIsFinite(elbow) && CADIsFinite(textPos) &&
           CADIsFinite(arrowSize) && CADIsFinite(textHeight);
}

void CADLeader::SetCreationPoint(int index, const Point3& localPoint)
{
    if (!mPBlock || !CADIsFinite(localPoint))
    {
        return;
    }

    if (index <= 0)
    {
        mPBlock->SetValue(leader_start, 0, localPoint);
        mPBlock->SetValue(leader_elbow, 0, localPoint);
        mPBlock->SetValue(leader_textpos, 0, localPoint);
    }
    else if (index == 1)
    {
        mPBlock->SetValue(leader_elbow, 0, localPoint);
        mPBlock->SetValue(leader_textpos, 0, localPoint);
    }
    else
    {
        mPBlock->SetValue(leader_textpos, 0, localPoint);
    }

    NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
    Interface* ip = GetCOREInterface();
    if (ip)
    {
        ip->RedrawViews(ip->GetTime(), REDRAW_INTERACTIVE);
    }
}

class CADLeaderCreateCallBack : public CreateMouseCallBack
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

    void SetObj(CADLeader* obj) { mObj = obj; }

private:
    CADLeader* mObj = nullptr;
};

static CADLeaderCreateCallBack gLeaderCreateCB;

CreateMouseCallBack* CADLeader::GetCreateMouseCallBack()
{
    gLeaderCreateCB.SetObj(this);
    return &gLeaderCreateCB;
}

namespace
{
constexpr UINT_PTR kLeaderDialogSubclassId = 0x4341444C;

LRESULT CALLBACK LeaderDialogSubclass(HWND hWnd, UINT msg, WPARAM wParam,
                                      LPARAM lParam, UINT_PTR, DWORD_PTR refData)
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
             LOWORD(wParam) == IDC_LEAD_FONT)
    {
        CADLeader* leader = reinterpret_cast<CADLeader*>(refData);
        CADApplyFontCombo(hWnd, IDC_LEAD_FONT,
                          leader ? leader->GetParamBlock(0) : nullptr,
                          leader_font_name, leader, IDS_LEAD_FONT);
        return 0;
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}
}

void CADLeader::BeginEditParams(IObjParam* ip, ULONG flags, Animatable* prev)
{
    CADBeginAutoUI(GetCADLeaderDesc(), ip, this, flags, prev);
    IParamMap2* map = mPBlock ? mPBlock->GetMap(0) : nullptr;
    HWND hDlg = map ? map->GetHWnd() : nullptr;
    if (hDlg)
    {
        RemoveWindowSubclass(hDlg, LeaderDialogSubclass,
                             kLeaderDialogSubclassId);
        SetWindowSubclass(hDlg, LeaderDialogSubclass,
                          kLeaderDialogSubclassId,
                          reinterpret_cast<DWORD_PTR>(this));
        CADPopulateFontCombo(hDlg, IDC_LEAD_FONT, mPBlock, leader_font_name);
    }
}

void CADLeader::EndEditParams(IObjParam* ip, ULONG flags, Animatable* next)
{
    IParamMap2* map = mPBlock ? mPBlock->GetMap(0) : nullptr;
    HWND hDlg = map ? map->GetHWnd() : nullptr;
    if (hDlg)
    {
        RemoveWindowSubclass(hDlg, LeaderDialogSubclass,
                             kLeaderDialogSubclassId);
    }
    CADEndAutoUI(GetCADLeaderDesc(), ip, this, flags, next);
}

class CADLeaderPostLoad : public PostLoadCallback
{
public:
    CADLeaderPostLoad(CADLeader* obj, const Point3 points[3]) : mObj(obj)
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
            IParamBlock2* pblock = mObj->GetParamBlockByID(kLeaderParams);
            if (pblock)
            {
                pblock->SetValue(leader_start, 0, mPoints[0]);
                pblock->SetValue(leader_elbow, 0, mPoints[1]);
                pblock->SetValue(leader_textpos, 0, mPoints[2]);
            }
            mObj->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
        }
        delete this;
    }

private:
    CADLeader* mObj = nullptr;
    Point3 mPoints[3];
};

IOResult CADLeader::Save(ISave* isave)
{
    if (!isave || !mPBlock)
    {
        return IO_ERROR;
    }

    Interval validity;
    Point3 points[3] = {};
    if (!mPBlock->GetValue(leader_start, 0, points[0], validity) ||
        !mPBlock->GetValue(leader_elbow, 0, points[1], validity) ||
        !mPBlock->GetValue(leader_textpos, 0, points[2], validity) ||
        !CADIsFinite(points[0]) || !CADIsFinite(points[1]) ||
        !CADIsFinite(points[2]))
    {
        return IO_ERROR;
    }

    ULONG written = 0;
    isave->BeginChunk(kLeaderChunkVersion);
    const DWORD version = kLeaderChunkVersionValue;
    IOResult result = isave->Write(&version, sizeof(version), &written);
    isave->EndChunk();
    if (result != IO_OK)
    {
        return result;
    }

    isave->BeginChunk(kLeaderChunkPoints);
    const DWORD count = 3;
    result = isave->Write(&count, sizeof(count), &written);
    if (result == IO_OK)
    {
        result = isave->Write(points, static_cast<ULONG>(sizeof(points)), &written);
    }
    isave->EndChunk();
    return result;
}

IOResult CADLeader::Load(ILoad* iload)
{
    if (!iload || !mPBlock)
    {
        return IO_ERROR;
    }

    IOResult result = IO_OK;
    Point3 points[3] = {};
    bool havePoints = false;

    while (IO_OK == (result = iload->OpenChunk()))
    {
        if (iload->CurChunkID() == kLeaderChunkVersion)
        {
            DWORD version = 0;
            ULONG read = 0;
            result = iload->Read(&version, sizeof(version), &read);
            if (result == IO_OK && version > kLeaderChunkVersionValue)
            {
                result = IO_ERROR;
            }
        }
        else if (iload->CurChunkID() == kLeaderChunkPoints)
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
                result = iload->Read(points, static_cast<ULONG>(sizeof(points)), &read);
            }
            if (result == IO_OK &&
                (!CADIsFinite(points[0]) || !CADIsFinite(points[1]) ||
                 !CADIsFinite(points[2])))
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
        // Deferred until every reference is loaded so the chunk overrides the
        // pblock mirror regardless of the system load order (B-12/B7).
        iload->RegisterPostLoadCallback(new CADLeaderPostLoad(this, points));
    }
    return IO_OK;
}

RefTargetHandle CADLeader::Clone(RemapDir& remap)
{
    CADLeader* clone = new CADLeader();
    clone->ReplaceReference(0, CADClonePBlockRef(mPBlock, remap));
    BaseClone(this, clone, remap);
    return clone;
}
