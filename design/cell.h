#pragma once

#include <unordered_set>

#include "common.h"
#include "formula.h"

class Cell : public CellInterface
{
public:
    Cell(SheetInterface& sheet);
    ~Cell() override;

    void Set(std::string text);
    void Clear();

    bool IsEmpty() const;

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    SheetInterface &sheet_;
    std::unique_ptr<Impl> impl_;
    std::vector<std::shared_ptr<CellInterface>> dependent_cells_;
    std::vector<Position> referenced_cells_;
};
