// include/mvs/SymbolTable.hpp
#pragma once

#include <string>
#include <unordered_map>
#include <stdexcept>

namespace mvs
{
    /**
     * @brief Manages the current logic values (0 or 1) of all identifiers (ports/wires) in a module.
     */
    class SymbolTable
    {
    private:
        // Key: Identifier Name (string) -> Value: Logic Value (0 or 1)
        std::unordered_map<std::string, int> symbols_;

    public:
        /**
         * @brief Returns the current value of an identifier.
         * @throws std::runtime_error if the symbol is not defined.
         */
        int get_value(const std::string& name) const
        {
            if (symbols_.count(name) == 0)
            {
                throw std::runtime_error("Symbol '" + name + "' not defined in SymbolTable.");
            }
            return symbols_.at(name);
        }

        /**
         * @brief Sets or updates the value of an identifier. Normalizes the value to 0 or 1.
         */
        void set_value(const std::string& name, int value)
        {
            // Normalize value to 0 or 1 for 1-bit logic
            // int normalized_value = (value != 0) ? 1 : 0;
            symbols_[name] = value;
        }

        /**
         * @brief Checks if an identifier is defined.
         */
        bool is_defined(const std::string& name) const
        {
            return symbols_.count(name) > 0;
        }
    };
} // namespace mvs