#pragma once

#include <memory>
#include <string>
#include <vector>

namespace mvs
{
    enum class PortDir
    {
        INPUT,
        OUTPUT,
        INOUT
    };

    struct ExprIdent;
    struct ConstExpr;
    struct ExprUnary;
    struct ExprBinary;

    // --- Visitor interface ---
    struct ExprVisitor
    {
        virtual ~ExprVisitor() = default;
        virtual int visit(const ExprIdent &) = 0;
        virtual int visit(const ConstExpr &) = 0;
        virtual int visit(const ExprUnary &) = 0;
        virtual int visit(const ExprBinary &) = 0;
    };
    
    struct Expr
    {
        virtual ~Expr() = default;
        virtual int accept(ExprVisitor &v) const = 0;
    };

    // --- Smart pointer type ---
    using ExprPtr = std::shared_ptr<Expr>;

    // --- AST node types ---
    struct ExprIdent : Expr
    {
        std::string name;
        int accept(ExprVisitor &v) const override { return v.visit(*this); }
    };

    struct ConstExpr : Expr
    {
        int value = 0;
        int accept(ExprVisitor &v) const override { return v.visit(*this); }
    };

    struct ExprUnary : Expr
    {
        char op = '~';
        ExprPtr rhs;
        int accept(ExprVisitor &v) const override { return v.visit(*this); }
    };

    struct ExprBinary : Expr
    {
        char op = '&';
        ExprPtr lhs;
        ExprPtr rhs;
        int accept(ExprVisitor &v) const override { return v.visit(*this); }
    };

    struct Assign
    {
        std::string lhs;
        ExprPtr rhs;
    };

    struct Port
    {
        PortDir dir = PortDir::INPUT;
        std::string name;
    };

    struct Wire
    {
        std::string name;
    };

    struct Module
    {
        std::string name;
        std::vector<Port> ports;
        std::vector<Wire> wires;
        std::vector<Assign> assigns;
    };

} // namespace mvs