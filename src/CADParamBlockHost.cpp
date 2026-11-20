#include "CADParamBlockHost.h"

#include "CADAnnotationCommon.h"

#include <GetCOREInterface.h>
#include <bezfont.h>
#include <commctrl.h>
#include <hold.h>
#include <tchar.h>

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
                           int paramId, ReferenceMaker* owner)
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
    theHold.Begin();
    pblock->SetValue(paramId, t, selection == 1 ? 1 : 0);
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

void CADMakeAutoParamBlocks(ReferenceMaker* owner, ClassDesc2* desc)
{
    if (owner && desc)
    {
        desc->MakeAutoParamBlocks(owner);
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
