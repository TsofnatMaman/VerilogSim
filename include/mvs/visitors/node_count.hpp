#pragma once

#include "mvs/module.hpp"

namespace mvs
{
    struct NodeCountVisitor : mvs::ExprVisitor
    {
        int visit(const ExprIdent &e) override { return 1; }
        int visit(const ConstExpr &e) override { return 1; }
        int visit(const ExprUnary &e) override { return 1 + (e.rhs ? e.rhs->accept(*this) : 0); }
        int visit(const ExprBinary &e) override { return 1 + (e.lhs ? e.lhs->accept(*this) : 0) + (e.rhs ? e.rhs->accept(*this) : 0); }
    };

    inline int node_count(const Expr& e){
        NodeCountVisitor visitor;
        return e.accept(visitor);
    }
}