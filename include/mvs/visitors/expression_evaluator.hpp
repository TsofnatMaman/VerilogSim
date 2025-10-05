// include/mvs/ExpressionEvaluator.hpp
#pragma once

#include "mvs/module.hpp"
#include "mvs/symbol_table.hpp"

namespace mvs
{
    /**
     * @brief Implements the ExprVisitor interface to calculate the value of an AST expression.
     */
    class ExpressionEvaluator : public ExprVisitor
    {
    private:
        // reference for symbols table for Lookup Identifiers values
        const SymbolTable& symbols_;

    public:
        ExpressionEvaluator(const SymbolTable& symbols) : symbols_(symbols) {}
        
        int visit(const ExprIdent& expr) override;
        int visit(const ConstExpr& expr) override;
        int visit(const ExprUnary& expr) override;
        int visit(const ExprBinary& expr) override;
    };
} // namespace mvs