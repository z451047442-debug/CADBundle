#include "CADText.h"
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

#include <vector>

namespace
{
enum TextParam
{
    text_position,
    text_string,
    text_font_size,
    text_font_name,
    text_color,
    text_size_mode,
};

constexpr BlockID kTextParams = 0x1002;
constexpr USHORT kTextChunkVersion = 0x0F05;
constexpr USHORT kTextChunkPosition = 0x0F06;
constexpr DWORD kTextChunkVersionValue = 1;
constexpr float kMinTextHeight = 1.0f;
}

class CADTextClassDesc : public ClassDesc2
{
public:
    int IsPublic() override { return TRUE; }
    void* Create(BOOL) override { return new CADText(); }
    const TCHAR* ClassName() override { return GetString(IDS_TEXT_CLASS); }
    const TCHAR* NonLocalizedClassName() override { return _T("CADText"); }
    SClass_ID SuperClassID() override { return HELPER_CLASS_ID; }
    Class_ID ClassID() override { return kCADTextClassID; }
    const TCHAR* Category() override { return GetString(IDS_CATEGORY); }
    const TCHAR* InternalName() override { return _T("CADText"); }
    HINSTANCE HInstance() override { return hInstance; }
};

static CADTextClassDesc gTextDesc;

ClassDesc2* GetCADTextDesc()
{
    return &gTextDesc;
}

static ParamBlockDesc2 gTextParamBlock(
    kTextParams, _T("CADTextParams"), 0, &gTextDesc,
    P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, 1, 0,
    IDD_CAD_TEXT, IDS_PARAMS, 0, 0, nullptr,

    text_position, _T("textPosition"), TYPE_POINT3, 0, IDS_TEXT_POS,
        p_default, Point3(0.0f, 0.0f, 0.0f),
        p_range, float(-1.0e30), float(1.0e30),
        p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE,
            IDC_TEXT_POSX, IDC_TEXT_POSXSPIN,
            IDC_TEXT_POSY, IDC_TEXT_POSYSPIN,
            IDC_TEXT_POSZ, IDC_TEXT_POSZSPIN, SPIN_AUTOSCALE,
        p_end,

    text_string, _T("textString"), TYPE_STRING, 0, IDS_TEXT_STRING,
        p_default, _T("文字"),
        p_ui, TYPE_EDITBOX, IDC_TEXT_STRING,
        p_end,

    text_font_size, _T("fontSize"), TYPE_FLOAT, 0, IDS_TEXT_SIZE,
        p_default, 12.0f,
        p_range, kMinTextHeight, 1000.0f,
        p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TEXT_SIZE, IDC_TEXT_SIZESPIN, 0.1f,
        p_end,

    text_font_name, _T("fontName"), TYPE_STRING, 0, IDS_TEXT_FONT,
        p_default, _T("宋体"),
        p_end,

    text_color, _T("textColor"), TYPE_RGBA, 0, IDS_TEXT_COLOR,
        p_default, Color(1.0f, 1.0f, 1.0f),
        p_ui, TYPE_COLORSWATCH, IDC_TEXT_COLOR,
        p_end,

    text_size_mode, _T("sizeMode"), TYPE_INT, 0, IDS_SIZEMODE,
        p_default, 0,
        p_range, 0, 1,
        p_end,

    p_end);

CADText::CADText()
{
    CADMakeAutoParamBlocks(this, GetCADTextDesc(), text_size_mode);
}

void CADText::SetReference(int index, RefTargetHandle target)
{
    if (index == 0)
    {
        CADSetPBlockRef(mPBlock, target);
    }
}

RefResult CADText::NotifyRefChanged(const Interval&, RefTargetHandle hTarget,
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
            CADSyncFontComboSelection(hDlg, IDC_TEXT_FONT, mPBlock,
                                      text_font_name);
            Interval valid;
            const int mode =
                mPBlock ? mPBlock->GetInt(text_size_mode, 0, valid) : 0;
            CADSyncSizeModeCombo(hDlg, IDC_TEXT_SIZEMODE, mode);
        }
    }
    return CADNotifyPBlockChange(this, mPBlock, hTarget, partID, message,
                                 propagate, GetCOREInterface(), nullptr);
}

IParamBlock2* CADText::GetParamBlockByID(BlockID id)
{
    return (mPBlock && id == kTextParams) ? mPBlock : nullptr;
}

bool CADText::GetParams(TimeValue t, Point3& position, MSTR& text,
                        float& fontSize, MSTR& fontName, Color& color,
                        int& sizeMode)
{
    if (!mPBlock)
    {
        return false;
    }

    Interval validity;
    mPBlock->GetValue(text_position, t, position, validity);
    mPBlock->GetValue(text_font_size, t, fontSize, validity);
    sizeMode = mPBlock->GetInt(text_size_mode, t, validity);
    const MCHAR* textValue = mPBlock->GetStr(text_string, t, validity);
    const MCHAR* fontValue = mPBlock->GetStr(text_font_name, t, validity);
    text = textValue ? textValue : _T("");
    fontName = fontValue ? fontValue : _T("");
    color = mPBlock->GetColor(text_color, t, validity);

    if (!CADIsFinite(position) || !CADIsFinite(fontSize))
    {
        return false;
    }
    if (fontSize < kMinTextHeight)
    {
        fontSize = kMinTextHeight;
    }
    sizeMode = sizeMode == 1 ? 1 : 0;
    return true;
}

void CADText::SetCreationPoint(const Point3& localPoint)
{
    if (!mPBlock || !CADIsFinite(localPoint))
    {
        return;
    }

    mPBlock->SetValue(text_position, 0, localPoint);
    NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
    Interface* ip = GetCOREInterface();
    if (ip)
    {
        ip->RedrawViews(ip->GetTime(), REDRAW_INTERACTIVE);
    }
}

class CADTextCreateCallBack : public CreateMouseCallBack
{
public:
    int proc(ViewExp* vpt, int msg, int point, int flags, IPoint2 m,
             Matrix3& mat) override
    {
        (void)point;
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
        mObj->SetCreationPoint(local);
        return msg == MOUSE_POINT ? CREATE_STOP : CREATE_CONTINUE;
    }

    int override(int mode) override
    {
        (void)mode;
        return CLICK_MOVE_CLICK;
    }

    void SetObj(CADText* obj) { mObj = obj; }

private:
    CADText* mObj = nullptr;
};

static CADTextCreateCallBack gTextCreateCB;

CreateMouseCallBack* CADText::GetCreateMouseCallBack()
{
    gTextCreateCB.SetObj(this);
    return &gTextCreateCB;
}

class CADTextFontEnumProc : public BezFontEnumProc
{
public:
    explicit CADTextFontEnumProc(std::vector<MSTR>& names) : mNames(names) {}

    BOOL CALLBACK Entry(BezFontInfo& info, LPARAM) override
    {
        mNames.push_back(info.name);
        return TRUE;
    }

private:
    std::vector<MSTR>& mNames;
};
namespace
{
constexpr UINT_PTR kTextDialogSubclassId = 0x43414454;

void PopulateFontCombo(HWND hDlg, CADText* obj)
{
    IParamBlock2* pblock = obj ? obj->GetParamBlock(0) : nullptr;
    if (!hDlg || !pblock)
    {
        return;
    }
    Interval valid;
    const MCHAR* current = pblock->GetStr(text_font_name, 0, valid);
    const MSTR currentName = current ? current : _T("");

    std::vector<MSTR> names;
    CADTextFontEnumProc proc(names);
    theBezFontManager.EnumerateFonts(proc, 0);

    SendDlgItemMessage(hDlg, IDC_TEXT_FONT, CB_RESETCONTENT, 0, 0);
    for (size_t i = 0; i < names.size(); ++i)
    {
        SendDlgItemMessage(hDlg, IDC_TEXT_FONT, CB_ADDSTRING, 0,
                           reinterpret_cast<LPARAM>(names[i].data()));
    }
    LRESULT index = CB_ERR;
    if (currentName.Length() > 0)
    {
        index = SendDlgItemMessage(
            hDlg, IDC_TEXT_FONT, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1),
            reinterpret_cast<LPARAM>(currentName.data()));
        if (index == CB_ERR)
        {
            index = SendDlgItemMessage(hDlg, IDC_TEXT_FONT, CB_ADDSTRING, 0,
                                       reinterpret_cast<LPARAM>(currentName.data()));
        }
    }
    if (index != CB_ERR)
    {
        SendDlgItemMessage(hDlg, IDC_TEXT_FONT, CB_SETCURSEL,
                           static_cast<WPARAM>(index), 0);
    }
}

void ApplySelectedFont(HWND hDlg, CADText* obj)
{
    IParamBlock2* pblock = obj ? obj->GetParamBlock(0) : nullptr;
    if (!hDlg || !pblock)
    {
        return;
    }
    TCHAR buffer[256] = {};
    GetDlgItemText(hDlg, IDC_TEXT_FONT, buffer, _countof(buffer));
    Interface* ip = GetCOREInterface();
    const TimeValue t = ip ? ip->GetTime() : 0;
    theHold.Begin();
    pblock->SetValue(text_font_name, t, buffer);
    theHold.Accept(GetString(IDS_TEXT_FONT));
    obj->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
    if (ip) ip->RedrawViews(t, REDRAW_INTERACTIVE);
}

LRESULT CALLBACK TextDialogSubclass(HWND hWnd, UINT msg, WPARAM wParam,
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
             LOWORD(wParam) == IDC_TEXT_FONT)
    {
        ApplySelectedFont(hWnd, reinterpret_cast<CADText*>(refData));
        return 0;
    }
    else if (msg == WM_COMMAND && HIWORD(wParam) == CBN_SELCHANGE &&
             LOWORD(wParam) == IDC_TEXT_SIZEMODE)
    {
        CADText* obj = reinterpret_cast<CADText*>(refData);
        IParamBlock2* pblock = obj ? obj->GetParamBlock(0) : nullptr;
        const int kSizeParams[] = { text_font_size };
        CADApplySizeModeCombo(hWnd, IDC_TEXT_SIZEMODE, pblock, text_size_mode,
                              obj, Point3(0.0f, 0.0f, 0.0f), kSizeParams, 1);
        return 0;
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}
}

void CADText::BeginEditParams(IObjParam* ip, ULONG flags, Animatable* prev)
{
    CADBeginAutoUI(GetCADTextDesc(), ip, this, flags, prev);
    IParamMap2* map = mPBlock ? mPBlock->GetMap(0) : nullptr;
    HWND hDlg = map ? map->GetHWnd() : nullptr;
    if (hDlg)
    {
        RemoveWindowSubclass(hDlg, TextDialogSubclass, kTextDialogSubclassId);
        SetWindowSubclass(hDlg, TextDialogSubclass, kTextDialogSubclassId,
                          reinterpret_cast<DWORD_PTR>(this));
        PopulateFontCombo(hDlg, this);
        Interval valid;
        const int mode = mPBlock ? mPBlock->GetInt(text_size_mode, 0, valid) : 0;
        CADPopulateSizeModeCombo(hDlg, IDC_TEXT_SIZEMODE, mode);
        CADCenterCustEditText(hDlg);
    }
}

void CADText::EndEditParams(IObjParam* ip, ULONG flags, Animatable* next)
{
    IParamMap2* map = mPBlock ? mPBlock->GetMap(0) : nullptr;
    HWND hDlg = map ? map->GetHWnd() : nullptr;
    if (hDlg)
    {
        RemoveWindowSubclass(hDlg, TextDialogSubclass, kTextDialogSubclassId);
    }
    CADEndAutoUI(GetCADTextDesc(), ip, this, flags, next);
}

class CADTextPostLoad : public PostLoadCallback
{
public:
    CADTextPostLoad(CADText* obj, const Point3& position)
        : mObj(obj), mPosition(position)
    {
    }

    void proc(ILoad*) override
    {
        if (mObj)
        {
            IParamBlock2* pblock = mObj->GetParamBlockByID(kTextParams);
            if (pblock)
            {
                pblock->SetValue(text_position, 0, mPosition);
            }
            mObj->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
        }
        delete this;
    }

private:
    CADText* mObj = nullptr;
    Point3 mPosition;
};

IOResult CADText::Save(ISave* isave)
{
    if (!isave || !mPBlock)
    {
        return IO_ERROR;
    }

    Interval validity;
    Point3 position;
    if (!mPBlock->GetValue(text_position, 0, position, validity) ||
        !CADIsFinite(position))
    {
        return IO_ERROR;
    }

    ULONG written = 0;
    isave->BeginChunk(kTextChunkVersion);
    const DWORD version = kTextChunkVersionValue;
    IOResult result = isave->Write(&version, sizeof(version), &written);
    isave->EndChunk();
    if (result != IO_OK)
    {
        return result;
    }

    isave->BeginChunk(kTextChunkPosition);
    result = isave->Write(&position, sizeof(position), &written);
    isave->EndChunk();
    return result;
}

IOResult CADText::Load(ILoad* iload)
{
    if (!iload || !mPBlock)
    {
        return IO_ERROR;
    }

    IOResult result = IO_OK;
    Point3 position;
    bool havePosition = false;

    while (IO_OK == (result = iload->OpenChunk()))
    {
        if (iload->CurChunkID() == kTextChunkVersion)
        {
            DWORD version = 0;
            ULONG read = 0;
            result = iload->Read(&version, sizeof(version), &read);
            if (result == IO_OK && version > kTextChunkVersionValue)
            {
                result = IO_ERROR;
            }
        }
        else if (iload->CurChunkID() == kTextChunkPosition)
        {
            ULONG read = 0;
            result = iload->Read(&position, sizeof(position), &read);
            if (result == IO_OK && !CADIsFinite(position))
            {
                result = IO_ERROR;
            }
            if (result == IO_OK)
            {
                havePosition = true;
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

    if (havePosition)
    {
        // Deferred until every reference is loaded so the chunk overrides the
        // pblock mirror regardless of the system load order (B-12/B7).
        iload->RegisterPostLoadCallback(new CADTextPostLoad(this, position));
    }
    return IO_OK;
}

RefTargetHandle CADText::Clone(RemapDir& remap)
{
    CADText* clone = new CADText();
    clone->ReplaceReference(0, CADClonePBlockRef(mPBlock, remap));
    BaseClone(this, clone, remap);
    return clone;
}
