#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;


void Sheet::SetCell(Position pos, std::string text)
{
    PositionCorrect(pos);
    auto *existing_cell = dynamic_cast<Cell *>(GetCell(pos));
    if (existing_cell != nullptr && existing_cell->GetText() == text)
    {
        return;
    }

    if (existing_cell != nullptr)
    {
        std::string old_text = existing_cell->GetText();
        InvalidateCellsByPos(pos);
        existing_cell->DeleteDependencedCells();
        existing_cell->Set(std::move(text));
        if (existing_cell->hasCircularDependency(existing_cell, pos))
        {
            existing_cell->Set(std::move(old_text));
            throw CircularDependencyException("Circular Exception!");
        }

        for (const auto ref_pos : existing_cell->GetReferencedCells())
        {
            existing_cell->AddDependencedCell(ref_pos);
        }
    }
    else
    {
        auto tmp_cell = std::make_unique<Cell>(*this, text);
        if (tmp_cell.get()->hasCircularDependency(tmp_cell.get(), pos))
        {
            throw CircularDependencyException("Circular Exception!");
        }

        for (const auto ref_pos : tmp_cell.get()->GetReferencedCells())
        {
            tmp_cell->AddDependencedCell(ref_pos);
        }

        sheet_[pos] = std::move(tmp_cell);
    }
}

const CellInterface* Sheet::GetCell(Position pos) const
{
    return const_cast<Sheet*>(this)->GetCell(pos);
}

CellInterface* Sheet::GetCell(Position pos)
{
    PositionCorrect(pos);
    if (sheet_.find(pos) != sheet_.end())
    {
        return sheet_.at(pos).get();
    }
    return nullptr;
}

void Sheet::ClearCell(Position pos)
{
    PositionCorrect(pos);
    if (pos.IsValid() && sheet_.find(pos) != sheet_.end())
    {
        sheet_.erase(pos);
    }
}

Size Sheet::GetPrintableSize() const
{
    Size s;
    for (const auto& [pos, cell] : sheet_)
    {
        s.cols = std::max(s.cols, pos.col + 1);
        s.rows = std::max(s.rows, pos.row + 1);
    }
    return s;
}

void Sheet::PrintValues(std::ostream& output) const
{
    auto size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row)
    {
        bool flag = false;
        for (int col = 0; col < size.cols; ++col)
        {
            if (flag)
            {
                output << '\t';
            }
            flag = true;
            if (const auto *cell = GetCell({ row, col }); cell)
            {
                auto val = cell->GetValue();
                if (std::holds_alternative<std::string>(val))
                {
                    output << std::get<std::string>(val);
                }
                if (std::holds_alternative<double>(val))
                {
                    output << std::get<double>(val);
                }
                if (std::holds_alternative<FormulaError>(val))
                {
                    output << std::get<FormulaError>(val);
                }
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const
{
    auto size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row)
    {
        bool flag = false;
        for (int col = 0; col < size.cols; ++col)
        {
            if (flag)
            {
                output << '\t';
            }
            flag = true;
            if (const auto *cell = GetCell({ row, col }); cell)
            {
                output << cell->GetText();
            }
        }
        output << '\n';
    }
}

void Sheet::InvalidateCellsByPos(Position pos)
{
    for (const auto cell_pos : GetDependencedCellByPos(pos))
    {
        auto *cell = dynamic_cast<Cell*>(GetCell(cell_pos));
        cell->InvalidateCache();
        InvalidateCellsByPos(cell_pos);
    }
}

std::vector<Position> Sheet::GetDependencedCellByPos(Position pos) const
{
    try
    {
        const auto *cell = dynamic_cast<const Cell *>(GetCell(pos));
        return cell == nullptr ? std::vector<Position>() : cell->GetReferencedCells();
    }
    catch (...)
    {
        return {};
    }
}

void Sheet::PositionCorrect(Position pos)
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("The position is incorrect");
    }
}

std::unique_ptr<SheetInterface> CreateSheet()
{
    return std::make_unique<Sheet>();
}
