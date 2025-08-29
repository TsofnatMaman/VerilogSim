#pragma once

#include <memory>
#include <string>
#include <vector>

namespace mvs
{
    enum class PortDir{INPUT,OUTPUT};

    struct Expr{
        virtual ~Expr() = default;
    };

    using ExprPtr = std::shared_ptr<Expr>;

    struct ExprIdent : Expr{
        std::string name;
    };

    struct ConstExpr : Expr{
        int value=0;
    };

    struct ExprUnary : Expr{
        char op='~';
        ExprPtr rhs;
    };

    struct ExprBinary : Expr{
        char op='&';
        ExprPtr lhs;
        ExprPtr rhs;
    };

    struct Assign{
        std::string lhs;
        ExprPtr rhs;
    };

    struct Port{
        PortDir dir=PortDir::INPUT;
        std::string name;
    };

    struct Wire{
        std::string name;
    };

    struct Module{
        std::string name;
        std::vector<Port> ports;
        std::vector<Wire> wires;
        std::vector<Assign> assigns;
    };

    inline int node_count(const Expr&e){
        if(auto id = dynamic_cast<const ExprIdent*>(&e)){
            (void)id;
            return 1;
        }

        if(auto c = dynamic_cast<const ConstExpr*>(&e)){
            (void)c;
            return 1;
        }

        if(auto u = dynamic_cast<const ExprUnary*>(&e)){
            return 1 + (u->rhs? node_count(*u->rhs):0);
        }

        if(auto b = dynamic_cast<const ExprBinary*>(&e)){
            return 1 + (b->lhs? node_count(*b->lhs):0) + (b->rhs? node_count(*b->rhs):0);
        }

        return 0;
    }
}