#include "CADAnnotatePanel.h"
#include "CADDimension.h"
#include "CADText.h"
#include "CADLeader.h"
#include "CADTable.h"

#include <GetCOREInterface.h>
#include <maxapi.h>
#include <winutil.h>
#include <tchar.h>

namespace
{
constexpr UINT_PTR kPanelTimerId = 1;
constexpr UINT kPanelTimerMs = 250;
}

class CADAnnotatePanel : public UtilityObj
{
public:
    void BeginEditParams(Interface* ip, IUtil* iu) override;
    void EndEditParams(Interface* ip, IUtil* iu) override;
    void DeleteThis() override { delete this; }

private:
    static INT_PTR CALLBACK PanelProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

    void StartCreation(ClassDesc* desc);
    void ClosePanel();
    void UpdateStatus();

    HWND mHwnd = nullptr;
    Interface* mIp = nullptr;
    IUtil* mIUtil = nullptr;
    bool mClosing = false;
    bool mWasCreating = false;
};

void CADAnnotatePanel::BeginEditParams(Interface* ip, IUtil* iu)
{
    mIp = ip;
    mIUtil = iu;

    if (mHwnd && IsWindow(mHwnd))
    {
        SetActiveWindow(mHwnd);
        SetForegroundWindow(mHwnd);
        UpdateStatus();
        return;
    }

    mHwnd = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_CAD_PANEL),
                              ip ? ip->GetMAXHWnd() : nullptr,
                              CADAnnotatePanel::PanelProc, reinterpret_cast<LPARAM>(this));
    if (mHwnd)
    {
        ShowWindow(mHwnd, SW_SHOW);
        UpdateWindow(mHwnd);
    }
}

void CADAnnotatePanel::EndEditParams(Interface* ip, IUtil* iu)
{
    (void)ip;
    (void)iu;
    Interface7* ip7 = GetCOREInterface7();
    if (ip7 && ip7->IsCreatingObject())
    {
        if (mIp)
        {
            mIp->StopCreating();
        }
    }

    if (mHwnd && IsWindow(mHwnd))
    {
        KillTimer(mHwnd, kPanelTimerId);
        DestroyWindow(mHwnd);
        mHwnd = nullptr;
    }
    mIp = nullptr;
    mIUtil = nullptr;
    mClosing = false;
    mWasCreating = false;
}

void CADAnnotatePanel::StartCreation(ClassDesc* desc)
{
    if (!desc)
    {
        return;
    }

    Interface7* ip7 = GetCOREInterface7();
    if (!ip7)
    {
        return;
    }

    if (ip7->IsCreatingObject(desc->ClassID()))
    {
        UpdateStatus();
        return;
    }

    if (ip7->IsCreatingObject())
    {
        GetCOREInterface()->StopCreating();
    }

    mWasCreating = true;
    ip7->StartCreatingObject(desc);

    if (!ip7->IsCreatingObject(desc->ClassID()))
    {
        mWasCreating = false;
        MessageBox(mHwnd, GetString(IDS_ERR_CREATE), GetString(IDS_PANEL_TITLE),
                   MB_OK | MB_ICONERROR);
    }
    UpdateStatus();
}

void CADAnnotatePanel::ClosePanel()
{
    if (mClosing)
    {
        return;
    }
    mClosing = true;

    Interface7* ip7 = GetCOREInterface7();
    if (ip7 && ip7->IsCreatingObject())
    {
        if (mIp)
        {
            mIp->StopCreating();
        }
        else
        {
            GetCOREInterface()->StopCreating();
        }
    }

    if (mHwnd && IsWindow(mHwnd))
    {
        KillTimer(mHwnd, kPanelTimerId);
        DestroyWindow(mHwnd);
        mHwnd = nullptr;
    }

    IUtil* iu = mIUtil;
    mIUtil = nullptr;
    if (iu)
    {
        iu->CloseUtility();
    }

    mClosing = false;
    mWasCreating = false;
}

void CADAnnotatePanel::UpdateStatus()
{
    if (!mHwnd || !IsWindow(mHwnd))
    {
        return;
    }

    Interface7* ip7 = GetCOREInterface7();
    const bool creating = ip7 && ip7->IsCreatingObject();
    TCHAR buffer[512] = {};

    if (creating)
    {
        mWasCreating = true;
        const TCHAR* name = _T("");
        if (ip7->IsCreatingObject(kCADDimensionClassID))
        {
            name = GetString(IDS_DIM_CLASS);
        }
        else if (ip7->IsCreatingObject(kCADTextClassID))
        {
            name = GetString(IDS_TEXT_CLASS);
        }
        else if (ip7->IsCreatingObject(kCADLeaderClassID))
        {
            name = GetString(IDS_LEADER_CLASS);
        }
        else if (ip7->IsCreatingObject(kCADTableClassID))
        {
            name = GetString(IDS_TABLE_CLASS);
        }

        const TCHAR* format = GetString(IDS_STATUS_CREATING);
        _stprintf_s(buffer, _countof(buffer), format ? format : _T("正在创建：%s"),
                    name ? name : _T(""));
    }
    else if (mWasCreating)
    {
        mWasCreating = false;
        const TCHAR* done = GetString(IDS_STATUS_DONE);
        _tcscpy_s(buffer, _countof(buffer), done ? done : _T("已结束创建"));
    }
    else
    {
        const TCHAR* ready = GetString(IDS_STATUS_READY);
        _tcscpy_s(buffer, _countof(buffer), ready ? ready : _T("状态：就绪"));
    }

    SetDlgItemText(mHwnd, IDC_PANEL_STATUS, buffer);
}

INT_PTR CALLBACK CADAnnotatePanel::PanelProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CADAnnotatePanel* self = reinterpret_cast<CADAnnotatePanel*>(
        GetWindowLongPtr(hDlg, DWLP_USER));

    switch (msg)
    {
    case WM_INITDIALOG:
        self = reinterpret_cast<CADAnnotatePanel*>(lParam);
        SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(self));
        if (!self)
        {
            return FALSE;
        }
        self->mHwnd = hDlg;
        if (self->mIp)
        {
            CenterWindow(hDlg, self->mIp->GetMAXHWnd());
        }
        SetTimer(hDlg, kPanelTimerId, kPanelTimerMs, nullptr);
        self->UpdateStatus();
        return TRUE;

    case WM_TIMER:
        if (wParam == kPanelTimerId && self)
        {
            self->UpdateStatus();
        }
        return TRUE;

    case WM_ACTIVATE:
        if (self)
        {
            self->UpdateStatus();
        }
        return TRUE;

    case WM_COMMAND:
        if (!self)
        {
            return FALSE;
        }
        switch (LOWORD(wParam))
        {
        case IDC_PANEL_BTN_DIM:
            self->StartCreation(GetCADDimensionDesc());
            return TRUE;
        case IDC_PANEL_BTN_TEXT:
            self->StartCreation(GetCADTextDesc());
            return TRUE;
        case IDC_PANEL_BTN_LEADER:
            self->StartCreation(GetCADLeaderDesc());
            return TRUE;
        case IDC_PANEL_BTN_TABLE:
            self->StartCreation(GetCADTableDesc());
            return TRUE;
        case IDC_PANEL_CLOSE:
            SendMessage(hDlg, WM_CLOSE, 0, 0);
            return TRUE;
        default:
            return FALSE;
        }

    case WM_CLOSE:
        if (self)
        {
            self->ClosePanel();
        }
        return TRUE;

    case WM_DESTROY:
        KillTimer(hDlg, kPanelTimerId);
        if (self && self->mHwnd == hDlg)
        {
            self->mHwnd = nullptr;
        }
        return TRUE;

    default:
        return FALSE;
    }
}

class CADAnnotatePanelClassDesc : public ClassDesc2
{
public:
    int IsPublic() override { return TRUE; }
    void* Create(BOOL) override { return new CADAnnotatePanel(); }
    const TCHAR* ClassName() override { return GetString(IDS_UTILITY_CLASS); }
    const TCHAR* NonLocalizedClassName() override { return _T("CADAnnotatePanel"); }
    SClass_ID SuperClassID() override { return UTILITY_CLASS_ID; }
    Class_ID ClassID() override { return kCADAnnotatePanelClassID; }
    const TCHAR* Category() override { return GetString(IDS_CATEGORY); }
    const TCHAR* InternalName() override { return _T("CADAnnotatePanel"); }
    HINSTANCE HInstance() override { return hInstance; }
};

ClassDesc2* GetCADAnnotatePanelDesc()
{
    static CADAnnotatePanelClassDesc desc;
    return &desc;
}
