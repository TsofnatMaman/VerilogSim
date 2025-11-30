#include "mvs/simulator.hpp"
#include <iostream>
#include <algorithm>

namespace mvs {

// ---------------- Constructor ----------------
Simulator::Simulator(Module module) : module_(std::move(module))
{
    _initialize_widths(); // Initialize wire/port width cache
}

// ---------------- Accessors ----------------
const SymbolTable &Simulator::get_symbols() const
{
    return symbols_;
}

int Simulator::get_width(const std::string &name)
{
    if (wire_widths_.count(name))
        return wire_widths_.at(name);
    return 32; // Default width if not found
}

// ---------------- Private Helpers ----------------
void Simulator::_initialize_widths()
{
    wire_widths_.clear();
    for (const auto &port : module_.ports)
        wire_widths_[port.name] = port.width;
    for (const auto &wire : module_.wires)
        wire_widths_[wire.name] = wire.width;
}

void Simulator::_build_dependency_graph()
{
    dependency_graph_.clear();
    for (size_t i = 0; i < module_.assigns.size(); ++i)
    {
        const auto &assign_stmt = module_.assigns[i];
        std::vector<std::string> rhs_identifiers = IdentifierFinder::find(assign_stmt.rhs);
        for (const auto &dep_name : rhs_identifiers)
            dependency_graph_[dep_name].push_back(i);
    }
}

// ---------------- Simulation ----------------
void Simulator::simulate()
{
    // Initialize all outputs and internal wires to 0
    for (const auto &port : module_.ports)
    {
        if (port.dir != PortDir::INPUT)
            symbols_.set_value(port.name, 0);
    }
    for (const auto &wire : module_.wires)
        symbols_.set_value(wire.name, 0);

    // Build dependency graph once
    _build_dependency_graph();

    // Active queue for event-driven simulation
    std::vector<size_t> active_queue;
    std::unordered_set<size_t> active_set;

    for (size_t i = 0; i < module_.assigns.size(); ++i)
    {
        active_queue.push_back(i);
        active_set.insert(i);
    }

    ExpressionEvaluator evaluator(symbols_); // Single evaluator object

    while (!active_queue.empty())
    {
        size_t assign_index = active_queue.back();
        active_queue.pop_back();
        active_set.erase(assign_index);

        const auto &assign_stmt = module_.assigns[assign_index];
        int new_raw_value = assign_stmt.rhs->accept(evaluator);
        int current_full_value = symbols_.get_value(assign_stmt.name);
        int next_full_value = current_full_value;

        // Bit-slice / full assignment
        if (assign_stmt.tb.msb.has_value())
        {
            int msb = assign_stmt.tb.msb.value();
            int lsb = assign_stmt.tb.lsb.value();
            int width = msb - lsb + 1;

            int rhs_mask = (1 << width) - 1;
            int truncated_rhs = new_raw_value & rhs_mask;

            int clearing_mask = ~(rhs_mask << lsb);
            next_full_value &= clearing_mask;

            next_full_value |= (truncated_rhs << lsb);
        }
        else
        {
            int target_width = get_width(assign_stmt.name);
            if (target_width < 32)
            {
                int mask = (1 << target_width) - 1;
                next_full_value = new_raw_value & mask;
            }
            else
            {
                next_full_value = new_raw_value;
            }
        }

        // If value changed, update symbol table and propagate
        if (next_full_value != current_full_value)
        {
            symbols_.set_value(assign_stmt.name, next_full_value);

            if (dependency_graph_.count(assign_stmt.name))
            {
                for (size_t next_idx : dependency_graph_.at(assign_stmt.name))
                {
                    if (active_set.find(next_idx) == active_set.end())
                    {
                        active_set.insert(next_idx);
                        active_queue.push_back(next_idx);
                    }
                }
            }
        }
    } // end while

} // simulate()

} // namespace mvs
