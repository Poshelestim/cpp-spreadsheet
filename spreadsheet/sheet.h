#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <map>
#include <set>

class Sheet : public SheetInterface
{
public:

    ~Sheet() override = default;

    void SetCell(Position pos, std::string text) override;

    [[nodiscard]] const CellInterface *GetCell(Position pos) const override;
    [[nodiscard]] CellInterface *GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    [[nodiscard]] Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:

    std::map<Position, std::unique_ptr<Cell>> sheet_;

    [[nodiscard]] std::vector<Position> GetDependencedCellByPos(Position pos) const;

    void InvalidateCellsByPos(Position pos);

    static void PositionCorrect(Position pos);
};
