#pragma once

#include "mvs/module.hpp" // Contains ExprVisitor, ExprIdent, etc.
#include <vector>
#include <string>
#include <unordered_set>

namespace mvs
{
    /**
     * @brief A visitor to traverse an expression AST and collect all unique identifier names (ExprIdent).
     */
    struct IdentifierFinder : ExprVisitor
    {
        std::unordered_set<std::string> identifiers;

        // Visitor functions implementation:
        int visit(const ExprIdent &e) override
        {
            identifiers.insert(e.name);
            return 0;
        }

        int visit(const ConstExpr &) override
        {
            // Constants have no dependencies
            return 0;
        }

        int visit(const ExprUnary &e) override
        {
            // Recursively visit the RHS
            if (e.rhs)
                e.rhs->accept(*this);
            return 0;
        }

        int visit(const ExprBinary &e) override
        {
            // Recursively visit both LHS and RHS
            if (e.lhs)
                e.lhs->accept(*this);
            if (e.rhs)
                e.rhs->accept(*this);
            return 0;
        }
        
        // Add support for other expression types (e.g., Ternary) if they exist in your AST
        // For example:
        /*
        int visit(const ExprTernary &e) override
        {
            if (e.cond) e.cond->accept(*this);
            if (e.true_expr) e.true_expr->accept(*this);
            if (e.false_expr) e.false_expr->accept(*this);
            return 0;
        }
        */

        /**
         * @brief Utility function to run the finder on an expression.
         * @param expr The root of the expression tree.
         * @return A list of unique identifier names used in the expression.
         */
        static std::vector<std::string> find(const ExprPtr& expr)
        {
            IdentifierFinder finder;
            if (expr)
                expr->accept(finder);
            
            // Convert set to vector for return consistency
            return std::vector<std::string>(finder.identifiers.begin(), finder.identifiers.end());
        }
    };
} // namespace mvs