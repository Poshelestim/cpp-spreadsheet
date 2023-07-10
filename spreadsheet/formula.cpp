#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <iostream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe)
{
    switch (fe.GetCategory())
    {
    case FormulaError::Category::Ref: return output << "#REF!";
    case FormulaError::Category::Value: return output << "#VALUE!";
    case FormulaError::Category::Div0: return output << "#DIV/0!";
    }
    return output << "";
}

namespace
{

double visit(const std::string &arg)
{
    try
    {
        return arg.empty() ? 0.0 : std::stod(arg);
    }
    catch (...)
    {
        throw FormulaError(FormulaError::Category::Value);
    }
}

double visit(double arg)
{
    return arg;
}

double visit(FormulaError arg)
{
    throw arg;
}

struct ValueVisitor
{
    double operator()(const std::string &arg)
    {
        return visit(arg);
    }

    double operator()(double arg)
    {
        return visit(arg);
    }

    double operator()(FormulaError arg)
    {
        return visit(arg);
    }
};

// Формула, позволяющая вычислять и обновлять арифметическое выражение.
// Поддерживаемые возможности:
// * Простые бинарные операции и числа, скобки: 1+2*3, 2.5*(2+3.5/7)
// * Значения ячеек в качестве переменных: A1+B2*C3
// Ячейки, указанные в формуле, могут быть как формулами, так и текстом. Если это
// текст, но он представляет число, тогда его нужно трактовать как число. Пустая
// ячейка или ячейка с пустым текстом трактуется как число ноль.
class Formula : public FormulaInterface
{
public:
    explicit Formula(const std::string &expression):
        ast_(ParseFormulaAST(expression)),
        referenced_cells_(ast_.GetCells().begin(), ast_.GetCells().end())
    {

    }

    [[nodiscard]] Value Evaluate(const SheetInterface& sheet) const override
    {
        try
        {
            auto cell_func = [&sheet](Position pos)
            {
                if (sheet.GetCell(pos) == nullptr)
                {
                    return 0.0;
                }
                auto val = sheet.GetCell(pos)->GetValue();

                if (std::holds_alternative<double>(val))
                {
                    return std::get<double>(val);
                }

                if (std::holds_alternative<std::string>(val))
                {
                    try
                    {
                        return std::stod(std::get<std::string>(val));
                    }
                    catch (...)
                    {
                        throw FormulaError(FormulaError::Category::Value);
                    }
                }
                else
                {
                    throw std::get<FormulaError>(val);
                }

                return 0.0;
            };

            return ast_.Execute(cell_func);
        }
        catch (FormulaError& fe)
        {
            return fe;
        }
    }

    [[nodiscard]] std::string GetExpression() const override
    {
        std::stringstream ss;
        ast_.PrintFormula(ss);
        return ss.str();
    }

    [[nodiscard]] std::vector<Position> GetReferencedCells() const override
    {
        const std::set<Position> poses(referenced_cells_.begin(),
                                       referenced_cells_.end());
        std::vector<Position> res(poses.size());
        std::copy(poses.cbegin(), poses.cend(), res.begin());
        return res;
    }

private:

    FormulaAST ast_;
    std::vector<Position> referenced_cells_;
};

}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression)
{
    try
    {
        return std::make_unique<Formula>(std::move(expression));
    }
    catch (const std::exception&)
    {
        throw FormulaException("ParseError");
    }
}
