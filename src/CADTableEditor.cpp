#include "CADTableEditor.h"
#include "CADTable.h"
#include "CADAnnotationCommon.h"

#include <commctrl.h>
#include <icolorman.h>
#include <tchar.h>
#include <winutil.h>

#include <algorithm>

CADTableEditor::CADTableEditor(CADTable* table)
    : mTable(table)
{
}

bool CADTableEditor::Edit(HWND parent, CADTable* table)
{
    if (!table)
    {
        return false;
    }

    CADTableEditor editor(table);
    table->GetEditorSnapshot(editor.mRows, editor.mCols, editor.mCells);
    const INT_PTR result = DialogBoxParam(
        hInstance, MAKEINTRESOURCE(IDD_CAD_TABLE_EDITOR), parent,
        CADTableEditor::ProcStatic, reinterpret_cast<LPARAM>(&editor));
    return result == IDOK;
}

INT_PTR CALLBACK CADTableEditor::ProcStatic(HWND hdlg, UINT msg,
                                            WPARAM wparam, LPARAM lparam)
{
    CADTableEditor* self = reinterpret_cast<CADTableEditor*>(
        GetWindowLongPtr(hdlg, DWLP_USER));
    if (msg == WM_INITDIALOG)
    {
        self = reinterpret_cast<CADTableEditor*>(lparam);
        SetWindowLongPtr(hdlg, DWLP_USER, reinterpret_cast<LONG_PTR>(self));
    }
    if (!self)
    {
        return FALSE;
    }

    const INT_PTR result = self->Proc(hdlg, msg, wparam, lparam);
    if (msg == WM_NCDESTROY)
    {
        SetWindowLongPtr(hdlg, DWLP_USER, 0);
    }
    return result;
}

INT_PTR CADTableEditor::Proc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
        return CADApplyThemeColors(msg, wparam);

    case WM_INITDIALOG:
        Initialize(hdlg);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDC_TABLE_EDIT_APPLY:
            ApplyCurrentCell();
            ApplyDimensions();
            return TRUE;
        case IDOK:
            if (Commit())
            {
                EndDialog(hdlg, IDOK);
            }
            return TRUE;
        case IDC_TABLE_EDIT_CANCEL:
        case IDCANCEL:
            EndDialog(hdlg, IDCANCEL);
            return TRUE;
        default:
            return FALSE;
        }

    case WM_NOTIFY:
    {
        const NMHDR* header = reinterpret_cast<NMHDR*>(lparam);
        if (header && header->idFrom == IDC_TABLE_EDIT_LIST &&
            header->code == LVN_ITEMCHANGED)
        {
            const NMLISTVIEW* change = reinterpret_cast<NMLISTVIEW*>(lparam);
            if (change->iItem >= 0 &&
                (change->uNewState & LVIS_SELECTED) != 0)
            {
                LVITEM item = {};
                item.mask = LVIF_PARAM;
                item.iItem = change->iItem;
                if (ListView_GetItem(mList, &item))
                {
                    SelectCell(static_cast<int>(item.lParam));
                }
            }
            return TRUE;
        }
        return FALSE;
    }

    case WM_CLOSE:
        EndDialog(hdlg, IDCANCEL);
        return TRUE;

    default:
        return FALSE;
    }
}

void CADTableEditor::Initialize(HWND hdlg)
{
    mHwnd = hdlg;
    CenterWindow(hdlg, GetParent(hdlg));

    INITCOMMONCONTROLSEX commonControls = {};
    commonControls.dwSize = sizeof(commonControls);
    commonControls.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&commonControls);

    mList = GetDlgItem(hdlg, IDC_TABLE_EDIT_LIST);
    IColorManager* colorManager = GetColorManager();
    if (mList)
    {
        if (colorManager)
        {
            ListView_SetBkColor(mList,
                                colorManager->CustSysColor(COLOR_WINDOW));
            ListView_SetTextBkColor(mList,
                                    colorManager->CustSysColor(COLOR_WINDOW));
            ListView_SetTextColor(mList,
                                  colorManager->CustSysColor(COLOR_WINDOWTEXT));
        }
        ListView_SetExtendedListViewStyle(
            mList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        LVCOLUMN column = {};
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        column.cx = 50;
        column.pszText = const_cast<MCHAR*>(_T("行"));
        ListView_InsertColumn(mList, 0, &column);
        column.cx = 50;
        column.pszText = const_cast<MCHAR*>(_T("列"));
        ListView_InsertColumn(mList, 1, &column);
        column.cx = 230;
        column.pszText = const_cast<MCHAR*>(_T("文本"));
        ListView_InsertColumn(mList, 2, &column);
    }

    SetDlgItemInt(hdlg, IDC_TABLE_EDIT_ROWS, mRows, FALSE);
    SetDlgItemInt(hdlg, IDC_TABLE_EDIT_COLS, mCols, FALSE);
    SendDlgItemMessage(hdlg, IDC_TABLE_EDIT_ROWSSPIN, UDM_SETRANGE, 0,
                       MAKELPARAM(50, 1));
    SendDlgItemMessage(hdlg, IDC_TABLE_EDIT_COLSSPIN, UDM_SETRANGE, 0,
                       MAKELPARAM(50, 1));

    RefreshList();
    SetStatus(_T("选择单元格后可编辑；应用后行列按 (行,列) 交集保留"));
}

void CADTableEditor::RefreshList()
{
    if (!mList)
    {
        return;
    }

    ListView_DeleteAllItems(mList);
    for (int index = 0; index < static_cast<int>(mCells.size()); ++index)
    {
        const int row = index / (mCols > 0 ? mCols : 1);
        const int col = index % (mCols > 0 ? mCols : 1);
        TCHAR rowText[16] = {};
        TCHAR colText[16] = {};
        _stprintf_s(rowText, _countof(rowText), _T("%d"), row + 1);
        _stprintf_s(colText, _countof(colText), _T("%d"), col + 1);

        LVITEM item = {};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = index;
        item.lParam = index;
        item.pszText = rowText;
        ListView_InsertItem(mList, &item);
        ListView_SetItemText(mList, index, 1, colText);
        ListView_SetItemText(mList, index, 2,
                             const_cast<MCHAR*>(mCells[index].data()));
    }

    if (!mCells.empty())
    {
        SelectCell(0);
    }
    else
    {
        mSelected = -1;
    }
}

void CADTableEditor::SelectCell(int index)
{
    if (index < 0 || index >= static_cast<int>(mCells.size()))
    {
        return;
    }

    mSelected = index;
    SetDlgItemText(mHwnd, IDC_TABLE_EDIT_CELL, mCells[index].data());
    ListView_SetItemState(mList, index,
                          LVIS_SELECTED | LVIS_FOCUSED,
                          LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(mList, index, FALSE);
    SetStatus(_T("已选择单元格；修改文本后点“应用单元格/行列”"));
}

void CADTableEditor::ApplyCurrentCell()
{
    if (mSelected < 0 || mSelected >= static_cast<int>(mCells.size()))
    {
        SetStatus(_T("请先选择一个单元格"));
        return;
    }

    TCHAR buffer[1025] = {};
    GetDlgItemText(mHwnd, IDC_TABLE_EDIT_CELL, buffer, _countof(buffer));
    TCHAR capped[257] = {};
    _tcsncpy_s(capped, _countof(capped), buffer, _TRUNCATE);
    mCells[mSelected] = capped;

    if (mList)
    {
        ListView_SetItemText(mList, mSelected, 2,
                             const_cast<MCHAR*>(mCells[mSelected].data()));
    }
    SetStatus(_T("单元格文本已应用"));
}

bool CADTableEditor::ApplyDimensions()
{
    BOOL validRows = FALSE;
    BOOL validCols = FALSE;
    const UINT rowsValue = GetDlgItemInt(mHwnd, IDC_TABLE_EDIT_ROWS,
                                         &validRows, FALSE);
    const UINT colsValue = GetDlgItemInt(mHwnd, IDC_TABLE_EDIT_COLS,
                                         &validCols, FALSE);
    int newRows = (validRows && rowsValue >= 1)
        ? static_cast<int>(rowsValue) : mRows;
    int newCols = (validCols && colsValue >= 1)
        ? static_cast<int>(colsValue) : mCols;
    newRows = std::max(1, std::min(50, newRows));
    newCols = std::max(1, std::min(50, newCols));

    SetDlgItemInt(mHwnd, IDC_TABLE_EDIT_ROWS, newRows, FALSE);
    SetDlgItemInt(mHwnd, IDC_TABLE_EDIT_COLS, newCols, FALSE);
    if (newRows == mRows && newCols == mCols)
    {
        return true;
    }

    std::vector<MSTR> migrated(static_cast<size_t>(newRows) * newCols);
    const int copyRows = std::min(newRows, mRows);
    const int copyCols = std::min(newCols, mCols);
    for (int row = 0; row < copyRows; ++row)
    {
        for (int col = 0; col < copyCols; ++col)
        {
            migrated[static_cast<size_t>(row) * newCols + col] =
                mCells[static_cast<size_t>(row) * mCols + col];
        }
    }

    mRows = newRows;
    mCols = newCols;
    mCells.swap(migrated);
    mSelected = -1;
    RefreshList();
    SetStatus(_T("行列已调整；新增为空，超出部分已丢弃"));
    return true;
}

bool CADTableEditor::Commit()
{
    if (!mTable)
    {
        return false;
    }

    ApplyCurrentCell();
    ApplyDimensions();
    if (mRows < 1 || mCols < 1 ||
        mCells.size() != static_cast<size_t>(mRows) * mCols)
    {
        SetStatus(_T("表格数据无效，未保存"));
        return false;
    }

    mTable->ApplyEditorSnapshot(mRows, mCols, mCells);
    return true;
}

void CADTableEditor::SetStatus(const TCHAR* text)
{
    if (mHwnd && text)
    {
        SetDlgItemText(mHwnd, IDC_TABLE_EDIT_STATUS, text);
    }
}
