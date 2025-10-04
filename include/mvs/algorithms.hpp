#pragma once

#include "mvs/visitors/node_count.hpp"
#include "mvs/visitors/expression_evaluator.hpp"

namespace mvs{
    
    struct Expr;
    
    int node_count(const Expr &expr);
    int expression_evaluator();
}