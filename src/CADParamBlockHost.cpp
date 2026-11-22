#include "CADParamBlockHost.h"

#include "CADAnnotationCommon.h"

#include <GetCOREInterface.h>
#include <bezfont.h>
#include <commctrl.h>
#include <custcont.h>
#include <hold.h>
#include <inode.h>
#include <tchar.h>

#include <cmath>
#include <vector>

namespace
{
class CADFontEnumProc : public BezFontEnumProc
{
public:
    explicit CADFontEnumProc(std::vector<MSTR>& names) : mNames(names) {}

    BOOL CALLBACK Entry(BezFontInfo& info, LPARAM) override
    {
        mNames.push_back(info.name);
        return TRUE;
    }

private:
    std::vector<MSTR>& mNames;
};

INode* FindNodeInTree(INode* node, ReferenceMaker* owner)
{
    if (!node)
    {
        return nullptr;
    }
    if (node->GetObjectRef() == owner)
    {
        return node;
    }

    const int childCount = node->NumberOfChildren();
    for (int i = 0; i < childCount; ++i)
    {
        INode* found = FindNodeInTree(node->GetChildNode(i), owner);
        if (found)
        {
            return found;
        }
    }
    return nullptr;
}

INode* FindNodeForObject(Interface* ip, ReferenceMaker* owner)
{
    if (!ip || !owner)
    {
        return nullptr;
    }

    const int selectedCount = ip->GetSelNodeCount();
    for (int i = 0; i < selectedCount; ++i)
    {
        INode* node = ip->GetSelNode(i);
        if (node && node->GetObjectRef() == owner)
        {
            return node;
        }
    }
    return FindNodeInTree(ip->GetRootNode(), owner);
}

float ScreenScaleForOwner(Interface* ip, ReferenceMaker* owner,
                          const Point3& objectAnchor)
{
    if (!ip)
    {
        return 1.0f;
    }

    INode* node = FindNodeForObject(ip, owner);
    Matrix3 objectTM;
    if (node)
    {
        objectTM = node->GetObjectTM(ip->GetTime());
    }
    Matrix3 referenceTM;
    referenceTM.SetTrans(objectAnchor * objectTM);

    ViewExp& vpt = ip->GetActiveViewExp();
    return CADScreenScale(&vpt, referenceTM);
}

BOOL CALLBACK CenterCustEditProc(HWND hwnd, LPARAM)
{
    TCHAR className[64] = {};
    if (GetClassName(hwnd, className, _countof(className)) == 0 ||
        _tcsicmp(className, _T("CustEdit")) != 0)
    {
        return TRUE;
    }
    if (GetProp(hwnd, _T("CADCenteredEdit")) != nullptr)
    {
        return TRUE;
    }

    ICustEdit* edit = GetICustEdit(hwnd);
    if (!edit)
    {
        return TRUE;
    }

    RECT rc = {};
    if (GetClientRect(hwnd, &rc))
    {
        HFONT font =
            reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
        if (!font)
        {
            font = reinterpret_cast<HFONT>(
                SendMessage(GetParent(hwnd), WM_GETFONT, 0, 0));
        }

        HDC dc = GetDC(hwnd);
        if (dc)
        {
            const HGDIOBJ oldFont = font ? SelectObject(dc, font) : nullptr;
            TEXTMETRIC tm = {};
            if (GetTextMetrics(dc, &tm))
            {
                const int glyphHeight = tm.tmAscent + tm.tmDescent;
                const int clientHeight = rc.bottom - rc.top;
                const int lead = (clientHeight - glyphHeight) / 2;
                if (lead > 0)
                {
                    edit->SetLeading(lead);
                }
            }
            if (oldFont)
            {
                SelectObject(dc, oldFont);
            }
            ReleaseDC(hwnd, dc);
        }
    }

    ReleaseICustEdit(edit);
    SetProp(hwnd, _T("CADCenteredEdit"), reinterpret_cast<HANDLE>(1));
    return TRUE;
}
}

void CADPopulateFontCombo(HWND hDlg, int controlId, IParamBlock2* pblock,
                          int paramId)
{
    if (!hDlg || !pblock)
    {
        return;
    }

    Interval valid;
    const MCHAR* current = pblock->GetStr(paramId, 0, valid);
    const MSTR currentName = current ? current : _T("");

    std::vector<MSTR> names;
    CADFontEnumProc proc(names);
    theBezFontManager.EnumerateFonts(proc, 0);

    SendDlgItemMessage(hDlg, controlId, CB_RESETCONTENT, 0, 0);
    for (size_t i = 0; i < names.size(); ++i)
    {
        SendDlgItemMessage(hDlg, controlId, CB_ADDSTRING, 0,
                           reinterpret_cast<LPARAM>(names[i].data()));
    }

    LRESULT index = CB_ERR;
    if (currentName.Length() > 0)
    {
        index = SendDlgItemMessage(
            hDlg, controlId, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1),
            reinterpret_cast<LPARAM>(currentName.data()));
        if (index == CB_ERR)
        {
            index = SendDlgItemMessage(hDlg, controlId, CB_ADDSTRING, 0,
                                       reinterpret_cast<LPARAM>(currentName.data()));
        }
    }
    if (index != CB_ERR)
    {
        SendDlgItemMessage(hDlg, controlId, CB_SETCURSEL,
                           static_cast<WPARAM>(index), 0);
    }
}

void CADApplyFontCombo(HWND hDlg, int controlId, IParamBlock2* pblock,
                       int paramId, ReferenceMaker* owner, int stringId)
{
    if (!hDlg || !pblock)
    {
        return;
    }

    TCHAR buffer[256] = {};
    GetDlgItemText(hDlg, controlId, buffer, _countof(buffer));

    Interface* ip = GetCOREInterface();
    const TimeValue t = ip ? ip->GetTime() : 0;
    theHold.Begin();
    pblock->SetValue(paramId, t, buffer);
    theHold.Accept(GetString(stringId));

    if (owner)
    {
        owner->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
    }
    if (ip)
    {
        ip->RedrawViews(t, REDRAW_INTERACTIVE);
    }
}

void CADSyncFontComboSelection(HWND hDlg, int controlId, IParamBlock2* pblock,
                               int paramId)
{
    if (!hDlg || !pblock)
    {
        return;
    }

    Interval valid;
    const MCHAR* current = pblock->GetStr(paramId, 0, valid);
    const MSTR currentName = current ? current : _T("");
    if (currentName.Length() == 0)
    {
        return;
    }

    LRESULT index = SendDlgItemMessage(
        hDlg, controlId, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1),
        reinterpret_cast<LPARAM>(currentName.data()));
    if (index == CB_ERR)
    {
        index = SendDlgItemMessage(hDlg, controlId, CB_ADDSTRING, 0,
                                   reinterpret_cast<LPARAM>(currentName.data()));
    }
    if (index != CB_ERR)
    {
        SendDlgItemMessage(hDlg, controlId, CB_SETCURSEL,
                           static_cast<WPARAM>(index), 0);
    }
}

void CADPopulateSizeModeCombo(HWND hDlg, int controlId, int sizeMode)
{
    if (!hDlg)
    {
        return;
    }

    SendDlgItemMessage(hDlg, controlId, CB_RESETCONTENT, 0, 0);
    SendDlgItemMessage(hDlg, controlId, CB_ADDSTRING, 0,
                       reinterpret_cast<LPARAM>(GetString(IDS_SIZEMODE_SCREEN)));
    SendDlgItemMessage(hDlg, controlId, CB_ADDSTRING, 0,
                       reinterpret_cast<LPARAM>(GetString(IDS_SIZEMODE_MODEL)));
    SendDlgItemMessage(hDlg, controlId, CB_SETCURSEL,
                       sizeMode == 1 ? 1 : 0, 0);
}

void CADApplySizeModeCombo(HWND hDlg, int controlId, IParamBlock2* pblock,
                           int paramId, ReferenceMaker* owner,
                           const Point3& objectAnchor, const int* sizeParamIds,
                           int sizeParamCount)
{
    if (!hDlg || !pblock)
    {
        return;
    }

    const LRESULT selection =
        SendDlgItemMessage(hDlg, controlId, CB_GETCURSEL, 0, 0);
    if (selection == CB_ERR)
    {
        return;
    }

    Interface* ip = GetCOREInterface();
    const TimeValue t = ip ? ip->GetTime() : 0;
    Interval valid;
    const int newMode = selection == 1 ? 1 : 0;
    const int oldMode = pblock->GetInt(paramId, t, valid) == 1 ? 1 : 0;

    float factor = 1.0f;
    if (newMode != oldMode && sizeParamIds && sizeParamCount > 0)
    {
        const float scale = ScreenScaleForOwner(ip, owner, objectAnchor);
        if (std::isfinite(scale) && scale > 0.0f)
        {
            factor = newMode == 1 ? scale : 1.0f / scale;
        }
    }

    theHold.Begin();
    if (factor != 1.0f)
    {
        for (int i = 0; i < sizeParamCount; ++i)
        {
            Interval sizeValid;
            const float value = pblock->GetFloat(sizeParamIds[i], t, sizeValid);
            if (value > 0.0f)
            {
                pblock->SetValue(sizeParamIds[i], t, value * factor);
            }
        }
    }
    pblock->SetValue(paramId, t, newMode);
    theHold.Accept(GetString(IDS_SIZEMODE));

    if (owner)
    {
        owner->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
    }
    if (ip)
    {
        ip->RedrawViews(t, REDRAW_INTERACTIVE);
    }
}

void CADSyncSizeModeCombo(HWND hDlg, int controlId, int sizeMode)
{
    if (!hDlg)
    {
        return;
    }
    SendDlgItemMessage(hDlg, controlId, CB_SETCURSEL,
                       sizeMode == 1 ? 1 : 0, 0);
}

void CADCenterCustEditText(HWND hDlg)
{
    if (hDlg)
    {
        EnumChildWindows(hDlg, CenterCustEditProc, 0);
    }
}

void CADMakeAutoParamBlocks(ReferenceMaker* owner, ClassDesc2* desc,
                            int defaultSizeModeParamId)
{
    if (!owner || !desc)
    {
        return;
    }

    desc->MakeAutoParamBlocks(owner);
    if (defaultSizeModeParamId < 0)
    {
        return;
    }

    IParamBlock2* pblock = nullptr;
    CADSetPBlockRef(pblock, owner->GetReference(0));
    if (pblock)
    {
        pblock->SetValue(defaultSizeModeParamId, 0, 1);
    }
}

ReferenceTarget* CADClonePBlockRef(IParamBlock2* pblock, RemapDir& remap)
{
    return pblock ? remap.CloneRef(pblock) : nullptr;
}

void CADSetPBlockRef(IParamBlock2*& pblock, RefTargetHandle target)
{
    pblock = target ? static_cast<IParamBlock2*>(target) : nullptr;
}

RefResult CADNotifyPBlockChange(ReferenceMaker* owner, IParamBlock2* pblock,
                                RefTargetHandle target, PartID& partID,
                                RefMessage message, BOOL propagate,
                                Interface* ip, bool* invalidateCache)
{
    (void)partID;
    (void)propagate;

    if (pblock && target == pblock && message == REFMSG_CHANGE)
    {
        if (invalidateCache)
        {
            *invalidateCache = true;
        }
        if (owner)
        {
            owner->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
        }
        if (ip)
        {
            ip->RedrawViews(ip->GetTime());
        }
    }

    return REF_SUCCEED;
}

void CADBeginAutoUI(ClassDesc2* desc, IObjParam* ip, ReferenceMaker* owner,
                    ULONG flags, Animatable* prev)
{
    if (desc)
    {
        desc->BeginEditParams(ip, owner, flags, prev);
    }
}

void CADEndAutoUI(ClassDesc2* desc, IObjParam* ip, ReferenceMaker* owner,
                  ULONG flags, Animatable* prev)
{
    if (desc)
    {
        desc->EndEditParams(ip, owner, flags, prev);
    }
}
