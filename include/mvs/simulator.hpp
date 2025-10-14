// include/mvs/Simulator.hpp

#pragma once

#include "mvs/module.hpp"
#include "mvs/symbol_table.hpp"
#include "mvs/visitors/expression_evaluator.hpp"
#include <iostream>
#include <vector>
#include <unordered_map>
#include "mvs/visitors/identifier_finder.hpp" // ✅ נדרש עבור _build_dependency_graph()

namespace mvs
{
    /**
     * @brief Manages the simulation of a Verilog module to find a stable state.
     */
    class CircuitSimulator
    {
        // ... (הכרזות על module_, symbols_ ו-dependency_graph_ נשארות כפי שהן)
    public:
        Module module_;
        SymbolTable symbols_;
        std::unordered_map<std::string, std::vector<size_t>> dependency_graph_;

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
        // ... (קונסטרוקטור, get_symbols, get_width נשארים כפי שהם)

        CircuitSimulator(Module module)
            : module_(std::move(module)) {}

        const SymbolTable &get_symbols() const
        {
            return symbols_;
        }

        int get_width(const std::string &name)
        {
            for (const auto &port : module_.ports)
            {
                if (port.name == name)
                    return port.width;
            }
            for (const auto &wire : module_.wires)
            {
                if (wire.name == name)
                    return wire.width;
            }
            return 32;
        }


        /**
         * @brief Runs the continuous simulation loop (Event-Driven Fixed-Point Iteration)
         * until all logic values stabilize.
         */
        void simulate()
        {
            // Initialization: fill the map for identifier variables with default values (0)

            // מאתחלים את כל הפורטים והחוטים ל-0 (כולל הפלטים והחוטים הפנימיים)
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

            // 2. Initialize the Active Queue with ALL assign indices
            // All assignments must be evaluated at least once at the start.
            std::vector<size_t> active_queue;
            for (size_t i = 0; i < module_.assigns.size(); ++i)
            {
                active_queue.push_back(i);
            }

            // Use a single Evaluator object throughout the simulation
            // (ההערכה תשתמש במצב הנוכחי של symbols_)
            ExpressionEvaluator evaluator(symbols_); 

            // Simulation loop (Event-Driven Fixed-Point Iteration)
            // Loop until the active queue is empty (i.e., no recent changes caused further updates)
            while (!active_queue.empty())
            {
                // Get the next assign statement index to evaluate
                size_t assign_index = active_queue.back();
                active_queue.pop_back();

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
                        // Add all dependent assign indices to the active queue
                        // (יפעיל מחדש את החישובים שמושפעים מהשינוי בחוט הנוכחי)
                        for (size_t next_assign_idx : dependency_graph_.at(assign_stmt.name))
                        {
                            active_queue.push_back(next_assign_idx);
                        }
                    }
                }
            } // end while (!active_queue.empty())

            // End: The circuit has reached a stable state
        }
    };
} // namespace mvs