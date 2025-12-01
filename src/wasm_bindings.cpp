#include <emscripten/bind.h>
#include <string>
#include <stdexcept>

// 💡 נניח שהנתיך הזה עובד (אם הורדת את json.hpp)
#include "json.hpp" 

#include "mvs/lexer.hpp"
#include "mvs/parser.hpp"
#include "mvs/netlist_extractor.hpp"
#include "mvs/netlist_types.hpp"
#include "mvs/netlist_to_dot.hpp" // נדרש לשימוש ב-gateTypeToString

using namespace emscripten;
using json = nlohmann::json;

// פונקציית עזר להמרת NetlistComponent ל-JSON
json to_json(const mvs::NetlistComponent& comp)
{
    return json{
        {"output", comp.output_wire},
        {"type", mvs::NetlistToDotConverter::gateTypeToString(comp.type)},
        {"inputs", comp.input_wires}
        // אפשר להוסיף גם את constant_value
    };
}

// 💡 הפונקציה המרכזית ש-JavaScript יקרא
std::string generate_netlist_json(const std::string& verilog_source)
{
    try 
    {
        if (verilog_source.empty()) {
            return json{{"error", "Empty Verilog source"}}.dump();
        }
        
        // 1. Tokenization (Lexing)
        mvs::Lexer lexer(verilog_source);
        auto tokens = lexer.Tokenize();
        
        if (tokens.empty()) {
            return json{{"error", "Tokenization resulted in no tokens - possibly empty or whitespace only"}}.dump();
        }
        
        // 2. Parsing
        mvs::Parser parser(tokens);
        std::optional<mvs::Module> module_opt = parser.parseModule();

        if (!module_opt.has_value())
        {
            std::string error_msg = "Parsing failed";
            if (parser.hasError()) {
                error_msg = parser.getErrorMessage();
            } else {
                error_msg = "Unknown parsing error - parseModule() returned empty optional";
            }
            return json{{"error", error_msg}}.dump();
        }
        
        mvs::Module module = std::move(module_opt.value());

        // 3. Extract netlist
        mvs::Netlist netlist = mvs::NetlistExtractor::extract(module);

        // 4. Convert to JSON
        json netlist_json = json::array();
        for (const auto& comp : netlist) {
            netlist_json.push_back(to_json(comp));
        }

        return json{{"success", true}, {"netlist", netlist_json}}.dump();
    }
    catch (const std::exception& e)
    {
        // Return JSON error with full exception details
        std::string error_msg = std::string(e.what());
        return json{{"error", error_msg}}.dump();
    }
    catch (...)
    {
        // Catch all other exceptions
        return json{{"error", "Unknown C++ exception occurred"}}.dump();
    }
}

// סימולציה עם ערכי קלט
std::string simulate_circuit(const std::string& verilog_source, const std::string& inputs_json)
{
    try
    {
        if (verilog_source.empty()) {
            return json{{"error", "Empty Verilog source"}}.dump();
        }

        // 1. Tokenization (Lexing)
        mvs::Lexer lexer(verilog_source);
        auto tokens = lexer.Tokenize();

        if (tokens.empty()) {
            return json{{"error", "No tokens generated"}}.dump();
        }

        // 2. Parsing
        mvs::Parser parser(tokens);
        std::optional<mvs::Module> module_opt = parser.parseModule();

        if (!module_opt.has_value())
        {
            std::string error_msg = "Parsing failed";
            if (parser.hasError()) {
                error_msg = parser.getErrorMessage();
            }
            return json{{"error", error_msg}}.dump();
        }

        mvs::Module module = std::move(module_opt.value());

        // 3. Extract netlist
        mvs::Netlist netlist = mvs::NetlistExtractor::extract(module);

        // 4. Parse input values
        json inputs_data = json::parse(inputs_json);
        std::map<std::string, bool> wire_values;

        // Initialize input wires from user input
        for (auto& [key, value] : inputs_data.items()) {
            if (value.is_number()) {
                wire_values[key] = (value.get<int>() != 0);
            }
        }

        // 5. Simulate - evaluate each gate
        for (const auto& comp : netlist) {
            bool result = false;

            if (comp.type == mvs::GateType::AND) {
                result = true;
                for (const auto& input : comp.input_wires) {
                    if (wire_values.find(input) != wire_values.end()) {
                        result = result && wire_values[input];
                    }
                }
            }
            else if (comp.type == mvs::GateType::OR) {
                result = false;
                for (const auto& input : comp.input_wires) {
                    if (wire_values.find(input) != wire_values.end()) {
                        result = result || wire_values[input];
                    }
                }
            }
            else if (comp.type == mvs::GateType::XOR) {
                result = false;
                for (const auto& input : comp.input_wires) {
                    if (wire_values.find(input) != wire_values.end()) {
                        result = result ^ wire_values[input];
                    }
                }
            }
            else if (comp.type == mvs::GateType::NOT) {
                if (!comp.input_wires.empty() && wire_values.find(comp.input_wires[0]) != wire_values.end()) {
                    result = !wire_values[comp.input_wires[0]];
                }
            }
            else if (comp.type == mvs::GateType::IDENTITY) {
                if (!comp.input_wires.empty() && wire_values.find(comp.input_wires[0]) != wire_values.end()) {
                    result = wire_values[comp.input_wires[0]];
                }
            }
            else if (comp.type == mvs::GateType::CONSTANT) {
                result = comp.constant_value.has_value() ? comp.constant_value.value() != 0 : false;
            }

            wire_values[comp.output_wire] = result;
        }

        // 6. Return results
        json result_json;
        for (const auto& [wire, value] : wire_values) {
            result_json[wire] = value ? 1 : 0;
        }

        return json{{"success", true}, {"values", result_json}}.dump();
    }
    catch (const std::exception& e)
    {
        return json{{"error", std::string(e.what())}}.dump();
    }
    catch (...)
    {
        return json{{"error", "Unknown error during simulation"}}.dump();
    }
}

// חשיפת הפונקציות ל-JavaScript
EMSCRIPTEN_BINDINGS(mvs_bindings) {
    function("generateNetlistJson", &generate_netlist_json);
    function("simulateCircuit", &simulate_circuit);
}