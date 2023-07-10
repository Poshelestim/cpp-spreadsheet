#include "cell.h"

#include <cassert>
#include <iostream>
#include <optional>
#include <string>

class Cell::Impl
{
public:

    [[nodiscard]] virtual CellInterface::Value ImplGetValue() const = 0;
    [[nodiscard]] virtual std::string ImplGetText() const = 0;
    [[nodiscard]] virtual std::vector<Position> GetReferencedCells() const = 0;
    [[nodiscard]] virtual ImplType GetType() const = 0;
    [[nodiscard]] virtual bool isCacheValid() const;
    virtual void ResetCache();

    virtual ~Impl() = default;

};

class Cell::EmptyImpl : public Cell::Impl
{
public:

    [[nodiscard]] CellInterface::Value ImplGetValue() const override;
    [[nodiscard]] std::string ImplGetText() const override;
    [[nodiscard]] std::vector<Position> GetReferencedCells() const override;
    [[nodiscard]] ImplType GetType() const override;

private:

    std::string empty_;
};

class Cell::TextImpl : public Cell::Impl
{
public:
    TextImpl(std::string& text);
    [[nodiscard]] CellInterface::Value ImplGetValue() const override;
    [[nodiscard]] std::string ImplGetText() const override;
    [[nodiscard]] std::vector<Position> GetReferencedCells() const override;
    [[nodiscard]] ImplType GetType() const override;

private:

    std::string text_value_;
};

class Cell::FormulaImpl : public Cell::Impl
{
public:
    FormulaImpl(std::string &expr, SheetInterface &sheet);
    [[nodiscard]] CellInterface::Value ImplGetValue() const override;
    [[nodiscard]] std::string ImplGetText() const override;
    [[nodiscard]] std::vector<Position> GetReferencedCells() const override;
    [[nodiscard]] bool isCacheValid() const override;
    void ResetCache() override;
    [[nodiscard]] ImplType GetType() const override;

private:

    std::unique_ptr<FormulaInterface> formula_;
    SheetInterface &sheet_;
    mutable std::optional<CellInterface::Value> cache_value_;
};


bool Cell::Impl::isCacheValid() const
{
    return true;
}

void Cell::Impl::ResetCache()
{

}

CellInterface::Value Cell::EmptyImpl::ImplGetValue() const
{
    return 0.0;
}

std::string Cell::EmptyImpl::ImplGetText() const
{
    return empty_;
}

std::vector<Position> Cell::EmptyImpl::GetReferencedCells() const
{
    return {};
}

ImplType Cell::EmptyImpl::GetType() const
{
    return ImplType::EMPTY;
}

Cell::TextImpl::TextImpl(std::string &text)
    :text_value_(std::move(text))
{

}

CellInterface::Value Cell::TextImpl::ImplGetValue() const
{
    if (text_value_[0] == ESCAPE_SIGN)
    {
        std::string sub_str = text_value_;
        sub_str.erase(0, 1);
        return sub_str;
    }

    return text_value_;
}

std::string Cell::TextImpl::ImplGetText() const
{
    return text_value_;
}

std::vector<Position> Cell::TextImpl::GetReferencedCells() const
{
    return {};
}

ImplType Cell::TextImpl::GetType() const
{
    return ImplType::TEXT;
}

Cell::FormulaImpl::FormulaImpl(std::string &expr, SheetInterface &sheet):
    formula_(ParseFormula(expr)),
    sheet_(sheet)
{

}

CellInterface::Value Cell::FormulaImpl::ImplGetValue() const
{
    if (!cache_value_.has_value())
    {
        for (const auto &pos : formula_->GetReferencedCells())
        {
            if (dynamic_cast<Cell *>(sheet_.GetCell(pos))->GetType() == ImplType::TEXT)
            {
                cache_value_ = FormulaError(FormulaError::Category::Value);
                return cache_value_.value();
            }
        }

        const auto val = formula_->Evaluate(sheet_);

        if (std::holds_alternative<double>(val))
        {
            double res = std::get<double>(val);
            if (std::isinf(res))
            {
                cache_value_ = FormulaError(FormulaError::Category::Div0);
            }
            else
            {
                cache_value_ = res;
            }
        }
        else if (std::holds_alternative<FormulaError>(val))
        {
            cache_value_ = std::get<FormulaError>(val);
        }

    }

    return cache_value_.has_value() ? cache_value_.value() : "";
}

std::string Cell::FormulaImpl::ImplGetText() const
{
    return FORMULA_SIGN + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const
{
    return formula_->GetReferencedCells();
}

bool Cell::FormulaImpl::isCacheValid() const
{
    return cache_value_.has_value();
}

void Cell::FormulaImpl::ResetCache()
{
    cache_value_.reset();
}

ImplType Cell::FormulaImpl::GetType() const
{
    return ImplType::FORMULA;
}

Cell::Cell(SheetInterface &sheet, const std::string &text) :
    sheet_(sheet),
    cell_value_(std::make_unique<EmptyImpl>())
{
    Set(text);
}

Cell::~Cell()
{
    cell_value_.reset();
}

void Cell::Set(std::string text)
{
    if (text.empty())
    {
        cell_value_ = std::make_unique<EmptyImpl>();
    }
    else if ( text[0] != FORMULA_SIGN ||
              (text[0] == FORMULA_SIGN && text.size() == 1) )
    {
        cell_value_ = std::make_unique<TextImpl>(text);
    }
    else
    {
        std::string sub_str = text.erase(0, 1);

        try
        {
            cell_value_ = std::make_unique<FormulaImpl>(sub_str, sheet_);
        }
        catch (...)
        {
            throw FormulaException(FormulaError(FormulaError::Category::Ref).ToString().data());
        }
    }
}

void Cell::Clear()
{
    cell_value_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const
{
    return cell_value_->ImplGetValue();
}

std::string Cell::GetText() const
{
    return cell_value_->ImplGetText();
}

std::vector<Position> Cell::GetReferencedCells() const
{
    return cell_value_->GetReferencedCells();
}

bool Cell::hasCircularDependency(const Cell *main_cell, Position pos) const
{
    for (const auto &cell_pos : GetReferencedCells())
    {
        const auto *ref_cell = dynamic_cast<const Cell *>(sheet_.GetCell(cell_pos));

        if (pos == cell_pos)
        {
            return true;
        }

        if (ref_cell == nullptr)
        {
            sheet_.SetCell(cell_pos, "");
            ref_cell = dynamic_cast<const Cell *>(sheet_.GetCell(cell_pos));
        }

        if (main_cell == ref_cell)
        {
            return true;
        }

        if (ref_cell->hasCircularDependency(main_cell, pos))
        {
            return true;
        }
    }
    return false;
}

void Cell::InvalidateCache()
{
    cell_value_->ResetCache();
}

ImplType Cell::GetType() const
{
    return cell_value_->GetType();
}

void Cell::AddDependencedCell(Position pos)
{
    this->dependenced_cells_.insert(pos);
}

void Cell::DeleteDependencedCells()
{
    this->dependenced_cells_.clear();
}
