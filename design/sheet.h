#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    void AddCell(const Position &pos, const Cell *new_cell);

    void DeleteCell(const Position &pos);

private:

    enum TypePrint
    {
        VALUE,
        TEXT
    };

    void UpdatePrintAreaSize();

    void Print(std::ostream& output, TypePrint type_print) const;

    Size print_area_size_;

    std::unordered_map<Position, std::unique_ptr<CellInterface>> sheet_;
};
