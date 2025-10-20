#pragma once

#include "mvs/module.hpp"
#include "mvs/symbol_table.hpp"
#include "mvs/visitors/expression_evaluator.hpp"
#include "mvs/visitors/identifier_finder.hpp"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <unordered_set>

namespace mvs
{
    /**
     * @brief Manages the simulation of a Verilog module to find a stable state.
     */
    class CircuitSimulator
    {
    private:
        // Maps a wire name (source) to a list of indices of 'assign' statements 
        // that depend on it (sink). Built once before simulation.
        std::unordered_map<std::string, std::vector<size_t>> dependency_graph_;

        // Cache for wire/port widths (O(1) lookup)
        std::unordered_map<std::string, int> wire_widths_;

        void _initialize_widths()
        {
            wire_widths_.clear();
            for (const auto &port : module_.ports)
            {
                wire_widths_[port.name] = port.width;
            }
            for (const auto &wire : module_.wires)
            {
                wire_widths_[wire.name] = wire.width;
            }
        }

        void _build_dependency_graph()
        {
            // 1. Clear any previous graph
            dependency_graph_.clear();

            // 2. Iterate over all assign statements
            for (size_t i = 0; i < module_.assigns.size(); ++i)
            {
                const auto &assign_stmt = module_.assigns[i];

                // 3. Find all identifiers in the RHS expression
                std::vector<std::string> rhs_identifiers = IdentifierFinder::find(assign_stmt.rhs);

                // 4. Map each dependency (source) to the current assign statement index (sink)
                for (const std::string &dep_name : rhs_identifiers)
                {
                    dependency_graph_[dep_name].push_back(i);
                }
            }
        }

    public:
        Module module_;
        SymbolTable symbols_;

    public:
        CircuitSimulator(Module module)
            : module_(std::move(module)) 
        {
            _initialize_widths(); // Initialize width cache upon creation
        }

        const SymbolTable &get_symbols() const
        {
            return symbols_;
        }

        // Optimized O(1) width lookup
        int get_width(const std::string &name)
        {
            if (wire_widths_.count(name))
            {
                return wire_widths_.at(name);
            }
            // Default width for undeclared identifiers (e.g., input ports might not be added to map if not handled in _initialize_widths)
            return 32; 
        }

        /**
         * @brief Runs the continuous simulation loop (Event-Driven Fixed-Point Iteration)
         * until all logic values stabilize.
         */
        void simulate()
        {
            // Initialization: fill the map for identifier variables with default values (0)

            // Initialize all outputs and internal wires to 0
            for (const auto &port : module_.ports)
            {
                if (port.dir != PortDir::INPUT)
                    symbols_.set_value(port.name, 0);
            }
            for (const auto &wire : module_.wires)
            {
                symbols_.set_value(wire.name, 0);
            }

            // 1. Build the dependency graph once
            _build_dependency_graph();

            // 2. Initialize the Active Queue and Set for Event-Driven Simulation
            // The set prevents duplicate assignments from being recalculated unnecessarily.
            std::vector<size_t> active_queue;
            std::unordered_set<size_t> active_set;
            
            // All assignments must be evaluated at least once at the start.
            for (size_t i = 0; i < module_.assigns.size(); ++i)
            {
                active_queue.push_back(i);
                active_set.insert(i);
            }

            // Use a single Evaluator object throughout the simulation
            ExpressionEvaluator evaluator(symbols_); 

            // Simulation loop (Event-Driven Fixed-Point Iteration)
            // Loop continues as long as there are events (assignments that need re-evaluation)
            while (!active_queue.empty())
            {
                // Get the next assign statement index to evaluate
                size_t assign_index = active_queue.back();
                active_queue.pop_back();
                active_set.erase(assign_index); // Mark as currently being processed/evaluated

                const auto &assign_stmt = module_.assigns[assign_index];

                // 1. Evaluate the Right-Hand Side (RHS)
                int new_raw_value = assign_stmt.rhs->accept(evaluator);

                // 2. Get the full current value of the LHS identifier
                int current_full_value = symbols_.get_value(assign_stmt.name);
                int next_full_value = current_full_value;

                // 3. Bit-Slice / Full Assignment Logic
                if (assign_stmt.tb.msb.has_value())
                {
                    // Assignment to a slice (W[MSB:LSB] = RHS)
                    const int msb = assign_stmt.tb.msb.value();
                    const int lsb = assign_stmt.tb.lsb.value();
                    const int width = msb - lsb + 1;

                    // Mask and Truncate RHS to fit slice width
                    const int rhs_mask = (1 << width) - 1;
                    int truncated_rhs = new_raw_value & rhs_mask;

                    // Clear target bits in current value
                    int clearing_mask = ~(rhs_mask << lsb);
                    next_full_value &= clearing_mask;

                    // Insert the new truncated RHS value
                    next_full_value |= (truncated_rhs << lsb);
                }
                else
                {
                    // Assignment to the full wire/port (W = RHS)
                    const int target_width = get_width(assign_stmt.name);

                    // Apply truncation if width is less than 32 bits
                    if (target_width < 32)
                    {
                        const int mask = (1 << target_width) - 1;
                        next_full_value = new_raw_value & mask;
                    }
                    else
                    {
                        next_full_value = new_raw_value;
                    }
                }
                
                // 4. Check if the value changed
                if (next_full_value != current_full_value)
                {
                    // Update the Symbol Table with the new full value
                    symbols_.set_value(assign_stmt.name, next_full_value);

                    // 5. EVENT GENERATION: Add dependent assignments to the queue
                    if (dependency_graph_.count(assign_stmt.name))
                    {
                        for (size_t next_assign_idx : dependency_graph_.at(assign_stmt.name))
                        {
                            // Only push the index if it's NOT already in the active set/queue
                            if (active_set.find(next_assign_idx) == active_set.end())
                            {
                                active_set.insert(next_assign_idx);
                                active_queue.push_back(next_assign_idx);
                            }
                        }
                    }
                }
            } // end while (!active_queue.empty())

            // End: The circuit has reached a stable state
        }
    };
} // namespace mvs