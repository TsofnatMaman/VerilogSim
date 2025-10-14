#pragma once

#include "mvs/module.hpp"
#include <vector>
#include <string>

namespace mvs
{
    /**
     * @brief A visitor to traverse an expression AST and collect all identifier names (ExprIdent).
     */
    struct IdentifierFinder : ExprVisitor
    {
        std::vector<std::string> identifiers;

        // Visitor functions implementation:
        int visit(const ExprIdent &e) override
        {
            identifiers.push_back(e.name);
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
            return finder.identifiers;
        }
    };
} // namespace mvs