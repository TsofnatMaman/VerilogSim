// include/mvs/Simulator.hpp
#pragma once

#include "mvs/module.hpp"
#include "mvs/symbol_table.hpp"
#include "mvs/visitors/expression_evaluator.hpp"
#include <iostream>

namespace mvs
{
    /**
     * @brief Manages the simulation of a Verilog module to find a stable state.
     */
    class CircuitSimulator
    {
    private:
        Module module_;
        SymbolTable symbols_;

    public:
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
         * @brief Runs the continuous simulation loop (Fixed-Point Iteration)
         * until all logic values stabilize.
         */
        void simulate()
        {
            // Initialization: fill the map for identifier variables with default values (0)

            for (const auto &port : module_.ports)
            {
                symbols_.set_value(port.name, 0);
            }

            for (const auto &wire : module_.wires)
            {
                symbols_.set_value(wire.name, 0);
            }

            // NOTE: The helper function get_width() must be defined elsewhere
            // (e.g., as a private method or a lambda in the scope) to retrieve
            // the target_width from module_.ports or module_.wires.

            // Simulation loop (Fixed-Point Iteration)
            bool changed;
            do
            {
                changed = false;

                // Create a new Evaluator for the current run with the current state of symbols_
                ExpressionEvaluator evaluator(symbols_);

                // Iterate over all assign statements
                for (const auto &assign_stmt : module_.assigns)
                {
                    // Evaluate the Right-Hand Side (RHS)
                    int new_raw_value = assign_stmt.rhs->accept(evaluator);

                    // Find the bus width of the Left-Hand Side (LHS)
                    const int target_width = get_width(assign_stmt.lhs);

                    // Masking/Truncation to bus width: Conform value to Verilog standard
                    int truncated_new_value = new_raw_value;

                    if (target_width < 32)
                    {
                        // Create mask: (1 << 8) - 1 = 0xFF
                        const int mask = (1 << target_width) - 1;
                        truncated_new_value = new_raw_value & mask;
                    }

                    // Get the current value of LHS
                    int current_value = symbols_.get_value(assign_stmt.lhs);

                    // Check if the value changed (comparison between truncated values)
                    if (truncated_new_value != current_value)
                    {
                        // If changed: Update the Symbol Table with the truncated value
                        symbols_.set_value(assign_stmt.lhs, truncated_new_value);

                        // Mark that a change occurred
                        changed = true;
                    }
                }
            } while (changed);

            // End: The circuit has reached a stable state
        }
    };
} // namespace mvs