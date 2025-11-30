#pragma once
#include "mvs/module.hpp"
#include "mvs/symbol_table.hpp"
#include "mvs/visitors/expression_evaluator.hpp"
#include "mvs/visitors/identifier_finder.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

namespace mvs {

class Simulator
{
private:
    std::unordered_map<std::string, std::vector<size_t>> dependency_graph_;
    std::unordered_map<std::string, int> wire_widths_;

    void _initialize_widths();
    void _build_dependency_graph();

public:
    Module module_;
    SymbolTable symbols_;

    Simulator(Module module);

    const SymbolTable &get_symbols() const;
    int get_width(const std::string &name);
    void simulate();
};

} // namespace mvs
