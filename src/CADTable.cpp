#include "CADTable.h"
#include "CADTableEditor.h"
#include "CADAnnotationCommon.h"
#include "CADParamBlockHost.h"

#include <GetCOREInterface.h>
#include <iparamm2.h>
#include <maxapi.h>
#include <mouseman.h>
#include <object.h>
#include <tchar.h>

#include <algorithm>
#include <vector>

namespace
{
enum TableParam
{
    table_insert_pos,
    table_rows,
    table_cols,
    table_cell_width,
    table_cell_height,
    table_cell_texts,
    table_line_color,
    table_text_color,
};

constexpr BlockID kTableParams = 0x1004;
constexpr int kMinRows = 1;
constexpr int kMaxRows = 50;
}

class CADTableClassDesc : public ClassDesc2
{
public:
    int IsPublic() override { return TRUE; }
    void* Create(BOOL) override { return new CADTable(); }
    const TCHAR* ClassName() override { return GetString(IDS_TABLE_CLASS); }
    const TCHAR* NonLocalizedClassName() override { return _T("CADTable"); }
    SClass_ID SuperClassID() override { return HELPER_CLASS_ID; }
    Class_ID ClassID() override { return kCADTableClassID; }
    const TCHAR* Category() override { return GetString(IDS_CATEGORY); }
    const TCHAR* InternalName() override { return _T("CADTable"); }
    HINSTANCE HInstance() override { return hInstance; }
};

static CADTableClassDesc gTableDesc;

ClassDesc2* GetCADTableDesc()
{
    return &gTableDesc;
}

static ParamBlockDesc2 gTableParamBlock(
    kTableParams, _T("CADTableParams"), 0, &gTableDesc,
    P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, 1, 0,
    IDD_CAD_TABLE, IDS_PARAMS, 0, 0, nullptr,

    table_insert_pos, _T("insertPos"), TYPE_POINT3, 0, IDS_TABLE_POS,
        p_default, Point3(0.0f, 0.0f, 0.0f),
        p_range, float(-1.0e30), float(1.0e30),
        p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE,
            IDC_TABLE_POSX, IDC_TABLE_POSXSPIN,
            IDC_TABLE_POSY, IDC_TABLE_POSYSPIN,
            IDC_TABLE_POSZ, IDC_TABLE_POSZSPIN, SPIN_AUTOSCALE,
        p_end,

    table_rows, _T("rows"), TYPE_INT, 0, IDS_TABLE_ROWS,
        p_default, 2,
        p_range, kMinRows, kMaxRows,
        p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_TABLE_ROWS, IDC_TABLE_ROWSSPIN, 0.1f,
        p_end,

    table_cols, _T("cols"), TYPE_INT, 0, IDS_TABLE_COLS,
        p_default, 2,
        p_range, kMinRows, kMaxRows,
        p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_TABLE_COLS, IDC_TABLE_COLSSPIN, 0.1f,
        p_end,

    table_cell_width, _T("cellWidth"), TYPE_FLOAT, 0, IDS_TABLE_CW,
        p_default, 60.0f,
        p_range, 1.0f, 1000.0f,
        p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TABLE_CW, IDC_TABLE_CWSPIN, 0.1f,
        p_end,

    table_cell_height, _T("cellHeight"), TYPE_FLOAT, 0, IDS_TABLE_CH,
        p_default, 20.0f,
        p_range, 1.0f, 1000.0f,
        p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TABLE_CH, IDC_TABLE_CHSPIN, 0.1f,
        p_end,

    table_cell_texts, _T("cellTexts"), TYPE_STRING_TAB, 0, P_VARIABLE_SIZE,
        IDS_TABLE_CELLS,
        p_end,

    table_line_color, _T("lineColor"), TYPE_RGBA, 0, IDS_TABLE_LINECOL,
        p_default, Color(0.1f, 0.8f, 0.1f),
        p_ui, TYPE_COLORSWATCH, IDC_TABLE_LINECOL,
        p_end,

    table_text_color, _T("textColor"), TYPE_RGBA, 0, IDS_TABLE_TEXTCOL,
        p_default, Color(1.0f, 1.0f, 1.0f),
        p_ui, TYPE_COLORSWATCH, IDC_TABLE_TEXTCOL,
        p_end,

    p_end);

CADTable::CADTable()
{
    GetCADTableDesc()->MakeAutoParamBlocks(this);
}

void CADTable::SetReference(int index, RefTargetHandle target)
{
    if (index == 0)
    {
        CADSetPBlockRef(mPBlock, target);
    }
}

RefResult CADTable::NotifyRefChanged(const Interval&, RefTargetHandle hTarget,
                                     PartID& partID, RefMessage message,
                                     BOOL propagate)
{
    if (mSyncing)
    {
        return REF_SUCCEED;
    }
    if (hTarget == mPBlock && message == REFMSG_CHANGE)
    {
        EnsureDimensions(0);
        SyncCellsFromMirror(0);
    }
    return CADNotifyPBlockChange(this, mPBlock, hTarget, partID, message,
                                 propagate, GetCOREInterface(), nullptr);
}

IParamBlock2* CADTable::GetParamBlockByID(BlockID id)
{
    return (mPBlock && id == kTableParams) ? mPBlock : nullptr;
}

void CADTable::WriteCellMirror()
{
    if (!mPBlock)
    {
        return;
    }

    const bool previous = mSyncing;
    mSyncing = true;
    mPBlock->SetCount(table_cell_texts, static_cast<int>(mCells.size()));
    for (int i = 0; i < static_cast<int>(mCells.size()); ++i)
    {
        mPBlock->SetValue(table_cell_texts, 0, mCells[i].data(), i);
    }
    mSyncing = previous;
}

void CADTable::EnsureDimensions(TimeValue t)
{
    if (!mPBlock)
    {
        return;
    }

    Interval validity;
    int rows = 2;
    int cols = 2;
    mPBlock->GetValue(table_rows, t, rows, validity);
    mPBlock->GetValue(table_cols, t, cols, validity);
    rows = std::max(kMinRows, std::min(kMaxRows, rows));
    cols = std::max(kMinRows, std::min(kMaxRows, cols));

    const size_t needed = static_cast<size_t>(rows) * cols;
    if (rows == mRows && cols == mCols && mCells.size() == needed)
    {
        return;
    }

    std::vector<MSTR> cells(needed);
    if (mRows > 0 && mCols > 0 &&
        mCells.size() == static_cast<size_t>(mRows) * mCols)
    {
        const int copyRows = std::min(rows, mRows);
        const int copyCols = std::min(cols, mCols);
        for (int row = 0; row < copyRows; ++row)
        {
            for (int col = 0; col < copyCols; ++col)
            {
                cells[static_cast<size_t>(row) * cols + col] =
                    mCells[static_cast<size_t>(row) * mCols + col];
            }
        }
    }
    else
    {
        for (int i = 0; i < static_cast<int>(needed); ++i)
        {
            const MCHAR* value = mPBlock->GetStr(table_cell_texts, t, i);
            if (value)
            {
                cells[i] = value;
            }
        }
    }

    mRows = rows;
    mCols = cols;
    mCells.swap(cells);
    WriteCellMirror();
}

void CADTable::SyncCellsFromMirror(TimeValue t)
{
    if (!mPBlock || mSyncing)
    {
        return;
    }

    const size_t needed = static_cast<size_t>(mRows) * mCols;
    if (needed == 0)
    {
        return;
    }

    std::vector<MSTR> cells(needed);
    for (size_t i = 0; i < needed; ++i)
    {
        const MCHAR* value =
            mPBlock->GetStr(table_cell_texts, t, static_cast<int>(i));
        if (value)
        {
            cells[i] = value;
        }
    }
    mCells.swap(cells);
}

bool CADTable::GetParams(TimeValue t, Point3& insertPos, int& rows, int& cols,
                         float& cellWidth, float& cellHeight, Color& lineColor,
                         Color& textColor)
{
    if (!mPBlock)
    {
        return false;
    }

    EnsureDimensions(t);
    Interval validity;
    mPBlock->GetValue(table_insert_pos, t, insertPos, validity);
    mPBlock->GetValue(table_rows, t, rows, validity);
    mPBlock->GetValue(table_cols, t, cols, validity);
    mPBlock->GetValue(table_cell_width, t, cellWidth, validity);
    mPBlock->GetValue(table_cell_height, t, cellHeight, validity);
    lineColor = mPBlock->GetColor(table_line_color, t, validity);
    textColor = mPBlock->GetColor(table_text_color, t, validity);
    rows = std::max(kMinRows, std::min(kMaxRows, rows));
    cols = std::max(kMinRows, std::min(kMaxRows, cols));
    return CADIsFinite(insertPos) && CADIsFinite(cellWidth) &&
           CADIsFinite(cellHeight);
}

void CADTable::SetCreationPoint(const Point3& localPoint)
{
    if (!mPBlock || !CADIsFinite(localPoint))
    {
        return;
    }

    mPBlock->SetValue(table_insert_pos, 0, localPoint);
    NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
    Interface* ip = GetCOREInterface();
    if (ip)
    {
        ip->RedrawViews(ip->GetTime(), REDRAW_INTERACTIVE);
    }
}

void CADTable::GetEditorSnapshot(int& rows, int& cols,
                                 std::vector<MSTR>& cells)
{
    EnsureDimensions(0);
    rows = mRows;
    cols = mCols;
    cells = mCells;
}

void CADTable::ApplyEditorSnapshot(int rows, int cols,
                                   const std::vector<MSTR>& cells)
{
    if (!mPBlock)
    {
        return;
    }

    rows = std::max(kMinRows, std::min(kMaxRows, rows));
    cols = std::max(kMinRows, std::min(kMaxRows, cols));
    const size_t needed = static_cast<size_t>(rows) * cols;
    std::vector<MSTR> newCells(needed);
    for (size_t i = 0; i < needed && i < cells.size(); ++i)
    {
        newCells[i] = cells[i];
    }

    const bool previous = mSyncing;
    mSyncing = true;
    mRows = rows;
    mCols = cols;
    mCells.swap(newCells);
    mPBlock->SetValue(table_rows, 0, mRows);
    mPBlock->SetValue(table_cols, 0, mCols);
    mPBlock->SetCount(table_cell_texts, static_cast<int>(mCells.size()));
    for (int i = 0; i < static_cast<int>(mCells.size()); ++i)
    {
        mPBlock->SetValue(table_cell_texts, 0, mCells[i].data(), i);
    }
    mSyncing = previous;

    NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
    Interface* ip = GetCOREInterface();
    if (ip)
    {
        ip->RedrawViews(ip->GetTime(), REDRAW_INTERACTIVE);
    }
}

class CADTableDlgProc : public ParamMap2UserDlgProc
{
public:
    explicit CADTableDlgProc(CADTable* table) : mTable(table) {}

    INT_PTR DlgProc(TimeValue, IParamMap2*, HWND hWnd, UINT msg,
                    WPARAM wParam, LPARAM) override
    {
        if (msg == WM_COMMAND && LOWORD(wParam) == IDC_TABLE_EDIT)
        {
            if (mTable)
            {
                CADTableEditor::Edit(hWnd, mTable);
            }
            return TRUE;
        }
        return FALSE;
    }

    void DeleteThis() override { delete this; }

private:
    CADTable* mTable = nullptr;
};

class CADTableCreateCallBack : public CreateMouseCallBack
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
        mObj->SetCreationPoint(world * Inverse(mat));
        return msg == MOUSE_POINT ? CREATE_STOP : CREATE_CONTINUE;
    }

    int override(int mode) override
    {
        (void)mode;
        return CLICK_MOVE_CLICK;
    }

    void SetObj(CADTable* obj) { mObj = obj; }

private:
    CADTable* mObj = nullptr;
};

static CADTableCreateCallBack gTableCreateCB;

CreateMouseCallBack* CADTable::GetCreateMouseCallBack()
{
    gTableCreateCB.SetObj(this);
    return &gTableCreateCB;
}

void CADTable::BeginEditParams(IObjParam* ip, ULONG flags, Animatable* prev)
{
    CADBeginAutoUI(GetCADTableDesc(), ip, this, flags, prev);
    gTableParamBlock.SetUserDlgProc(new CADTableDlgProc(this));
}

void CADTable::EndEditParams(IObjParam* ip, ULONG flags, Animatable* next)
{
    gTableParamBlock.SetUserDlgProc(nullptr);
    CADEndAutoUI(GetCADTableDesc(), ip, this, flags, next);
}

RefTargetHandle CADTable::Clone(RemapDir& remap)
{
    CADTable* clone = new CADTable();
    clone->ReplaceReference(0, CADClonePBlockRef(mPBlock, remap));
    clone->mRows = mRows;
    clone->mCols = mCols;
    clone->mCells = mCells;
    BaseClone(this, clone, remap);
    return clone;
}
