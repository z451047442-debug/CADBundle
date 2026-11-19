#pragma once

#include "CADBundle.h"

#include <vector>

class CADTable;

class CADTableEditor
{
public:
    static bool Edit(HWND parent, CADTable* table);

private:
    explicit CADTableEditor(CADTable* table);
    ~CADTableEditor() = default;

    INT_PTR Proc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam);
    static INT_PTR CALLBACK ProcStatic(HWND hdlg, UINT msg, WPARAM wparam,
                                       LPARAM lparam);

    void Initialize(HWND hdlg);
    void RefreshList();
    void SelectCell(int index);
    void ApplyCurrentCell();
    bool ApplyDimensions();
    bool Commit();
    void SetStatus(const TCHAR* text);

    CADTable* mTable = nullptr;
    HWND mHwnd = nullptr;
    HWND mList = nullptr;
    int mRows = 0;
    int mCols = 0;
    int mSelected = -1;
    std::vector<MSTR> mCells;
};
