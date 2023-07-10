#pragma once

#include "common.h"
#include "formula.h"

#include <cmath>
#include <optional>
#include <functional>
#include <set>
#include <unordered_set>

class Sheet;

enum ImplType
{
	EMPTY,
	TEXT,
	FORMULA
};

class Cell : public CellInterface
{
public:

	using CellPtr = std::unique_ptr<Cell>;

    Cell(SheetInterface &sheet, const std::string &text);

	~Cell();

	void Set(std::string text);
	void Clear();

    [[nodiscard]] Value GetValue() const override;
    [[nodiscard]] std::string GetText() const override;

    [[nodiscard]] std::vector<Position> GetReferencedCells() const override;

    [[nodiscard]] bool hasCircularDependency(const Cell *main_cell, Position pos) const;
	void InvalidateCache();

    [[nodiscard]] ImplType GetType() const;

    void AddDependencedCell(Position pos);

    void DeleteDependencedCells();

private:

    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    SheetInterface &sheet_;
    std::unique_ptr<Impl> cell_value_;
    std::set<Position> dependenced_cells_;
};
