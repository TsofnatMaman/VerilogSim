// src/ExpressionEvaluator.cpp
#include "mvs/visitors/expression_evaluator.hpp"
#include <stdexcept>

namespace mvs
{
    int ExpressionEvaluator::visit(const ConstExpr &expr)
    {
        return expr.value;
    }

    int ExpressionEvaluator::visit(const ExprIdent &expr)
    {
        return symbols_.get_value(expr.name);
    }

    int ExpressionEvaluator::visit(const ExprUnary &expr)
    {
        if (expr.op == '~') // NOT
        {
            int rhs_val = expr.rhs->accept(*this);
            return ~rhs_val;
        }

        throw std::runtime_error("Unsupported unary operator: " + std::string(1, expr.op));
    }

    int ExpressionEvaluator::visit(const ExprBinary &expr)
    {
        int lhs_val = expr.lhs->accept(*this);
        int rhs_val = expr.rhs->accept(*this);

        switch (expr.op)
        {
        case '&': // AND
            return lhs_val & rhs_val;
        case '|': // OR
            return lhs_val | rhs_val;
        case '^': // XOR
            return lhs_val ^ rhs_val;
        case '+':
            return lhs_val + rhs_val;
        case '*':
            return lhs_val * rhs_val;
        default:
            throw std::runtime_error("Unsupported binary operator: " + std::string(1, expr.op));
        }
    }
} // namespace mvs