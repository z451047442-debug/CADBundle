#include "CADTable.h"
#include "CADAnnotationCommon.h"

#include <algorithm>
#include <utility>
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

constexpr USHORT kTableChunkVersion = 0x0F07;
constexpr USHORT kTableChunkData = 0x0F08;
constexpr DWORD kTableChunkVersionValue = 1;
constexpr int kMinRows = 1;
constexpr int kMaxRows = 50;
constexpr int kMaxCellCharacters = 256;
}

class CADTablePostLoad : public PostLoadCallback
{
public:
    CADTablePostLoad(CADTable* table, bool haveData, int rows, int cols,
                     const Point3& insertPos, std::vector<MSTR> cells)
        : mTable(table), mHaveData(haveData), mRows(rows), mCols(cols),
          mInsertPos(insertPos), mCells(std::move(cells))
    {
    }

    void proc(ILoad*) override
    {
        if (mTable)
        {
            mTable->ApplyLoadedData(mHaveData, mRows, mCols, mInsertPos, mCells);
        }
        delete this;
    }

private:
    CADTable* mTable = nullptr;
    bool mHaveData = false;
    int mRows = 0;
    int mCols = 0;
    Point3 mInsertPos;
    std::vector<MSTR> mCells;
};

IOResult CADTable::Save(ISave* isave)
{
    if (!isave || !mPBlock)
    {
        return IO_ERROR;
    }

    EnsureDimensions(0);
    Interval validity;
    Point3 insertPos;
    int rows = mRows;
    int cols = mCols;
    mPBlock->GetValue(table_insert_pos, 0, insertPos, validity);
    if (!CADIsFinite(insertPos) || rows < kMinRows || rows > kMaxRows ||
        cols < kMinRows || cols > kMaxRows ||
        mCells.size() != static_cast<size_t>(rows) * cols)
    {
        return IO_ERROR;
    }

    ULONG written = 0;
    isave->BeginChunk(kTableChunkVersion);
    const DWORD version = kTableChunkVersionValue;
    IOResult result = isave->Write(&version, sizeof(version), &written);
    isave->EndChunk();
    if (result != IO_OK)
    {
        return result;
    }

    isave->BeginChunk(kTableChunkData);
    const DWORD rowValue = rows;
    const DWORD colValue = cols;
    const DWORD cellCount = static_cast<DWORD>(mCells.size());
    result = isave->Write(&rowValue, sizeof(rowValue), &written);
    if (result == IO_OK)
    {
        result = isave->Write(&colValue, sizeof(colValue), &written);
    }
    if (result == IO_OK)
    {
        result = isave->Write(&cellCount, sizeof(cellCount), &written);
    }
    if (result == IO_OK)
    {
        result = isave->Write(&insertPos, sizeof(insertPos), &written);
    }
    for (int i = 0; result == IO_OK && i < static_cast<int>(mCells.size()); ++i)
    {
        const int chars = std::min(kMaxCellCharacters, mCells[i].Length());
        const DWORD charCount = static_cast<DWORD>(chars);
        result = isave->Write(&charCount, sizeof(charCount), &written);
        if (result == IO_OK && chars > 0)
        {
            result = isave->Write(
                mCells[i].data(),
                static_cast<ULONG>(sizeof(MCHAR) * chars), &written);
        }
    }
    isave->EndChunk();
    return result;
}

IOResult CADTable::Load(ILoad* iload)
{
    if (!iload || !mPBlock)
    {
        return IO_ERROR;
    }

    IOResult result = IO_OK;
    DWORD rows = 0;
    DWORD cols = 0;
    Point3 insertPos(0.0f, 0.0f, 0.0f);
    std::vector<MSTR> cells;
    bool haveData = false;

    while (IO_OK == (result = iload->OpenChunk()))
    {
        if (iload->CurChunkID() == kTableChunkVersion)
        {
            DWORD version = 0;
            ULONG read = 0;
            result = iload->Read(&version, sizeof(version), &read);
            if (result == IO_OK && version > kTableChunkVersionValue)
            {
                result = IO_ERROR;
            }
        }
        else if (iload->CurChunkID() == kTableChunkData)
        {
            DWORD cellCount = 0;
            ULONG read = 0;
            result = iload->Read(&rows, sizeof(rows), &read);
            if (result == IO_OK)
            {
                result = iload->Read(&cols, sizeof(cols), &read);
            }
            if (result == IO_OK)
            {
                result = iload->Read(&cellCount, sizeof(cellCount), &read);
            }
            if (result == IO_OK)
            {
                result = iload->Read(&insertPos, sizeof(insertPos), &read);
            }
            if (result == IO_OK &&
                (rows < static_cast<DWORD>(kMinRows) ||
                 rows > static_cast<DWORD>(kMaxRows) ||
                 cols < static_cast<DWORD>(kMinRows) ||
                 cols > static_cast<DWORD>(kMaxRows) ||
                 cellCount != rows * cols ||
                 !CADIsFinite(insertPos)))
            {
                result = IO_ERROR;
            }
            if (result == IO_OK)
            {
                cells.resize(cellCount);
                for (DWORD i = 0; result == IO_OK && i < cellCount; ++i)
                {
                    DWORD chars = 0;
                    result = iload->Read(&chars, sizeof(chars), &read);
                    if (result == IO_OK &&
                        chars > static_cast<DWORD>(kMaxCellCharacters))
                    {
                        result = IO_ERROR;
                    }
                    if (result == IO_OK)
                    {
                        std::vector<MCHAR> text(chars + 1, 0);
                        if (chars > 0)
                        {
                            result = iload->Read(
                                text.data(),
                                static_cast<ULONG>(sizeof(MCHAR) * chars),
                                &read);
                        }
                        if (result == IO_OK)
                        {
                            text[chars] = 0;
                            cells[i] = MSTR(text.data());
                        }
                    }
                }
            }
            if (result == IO_OK)
            {
                haveData = true;
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

    // Defer applying the chunk until every reference is in place so the chunk
    // overrides the pblock mirror regardless of the system load order
    // (B-12/B7).
    iload->RegisterPostLoadCallback(new CADTablePostLoad(
        this, haveData, static_cast<int>(rows), static_cast<int>(cols),
        insertPos, std::move(cells)));
    return IO_OK;
}

void CADTable::ApplyLoadedData(bool haveData, int rows, int cols,
                               const Point3& insertPos,
                               const std::vector<MSTR>& cells)
{
    if (!mPBlock)
    {
        return;
    }

    if (!haveData)
    {
        EnsureDimensions(0);
        return;
    }

    rows = std::max(kMinRows, std::min(kMaxRows, rows));
    cols = std::max(kMinRows, std::min(kMaxRows, cols));
    const size_t needed = static_cast<size_t>(rows) * cols;
    std::vector<MSTR> loaded(needed);
    for (size_t i = 0; i < needed && i < cells.size(); ++i)
    {
        loaded[i] = cells[i];
    }

    const bool previous = mSyncing;
    mSyncing = true;
    mRows = rows;
    mCols = cols;
    mCells.swap(loaded);
    mPBlock->SetValue(table_insert_pos, 0, insertPos);
    mPBlock->SetValue(table_rows, 0, mRows);
    mPBlock->SetValue(table_cols, 0, mCols);
    mPBlock->SetCount(table_cell_texts, static_cast<int>(mCells.size()));
    for (int i = 0; i < static_cast<int>(mCells.size()); ++i)
    {
        mPBlock->SetValue(table_cell_texts, 0, mCells[i].data(), i);
    }
    mSyncing = previous;

    NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}
