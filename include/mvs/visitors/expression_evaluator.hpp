#include <map>
#include <string>
#include <stdexcept>
#include "mvs/module.hpp"

namespace mvs
{
    /**
     * @brief Hypothetical function to retrieve the current value of a signal (identifier).
     * * In a full simulator, this function would query a Symbol Table or signal map.
     * For this basic evaluation, we assume a default value (e.g., 1) or throw 
     * an error if the signal's value is required but unknown.
     */
    int getIdentifierValue(const std::string& name) {
        // NOTE: In a real system, you'd look this up in a symbol table.
        // For a simple calculator, we'll return 1 for unknown signals, or you can throw.
        // throw std::runtime_error("Cannot evaluate identifier '" + name + "' without a symbol table.");
        return 1; 
    }
}

/**
 * @brief ExpressionEvaluator traverses the AST to calculate the resulting integer value of an expression.
 */
struct ExpressionEvaluator : mvs::ExprVisitor
{
    // The evaluation logic is recursive, relying on the accept method to delegate 
    // the evaluation to the correct visit method for each node type.

    int visit(const mvs::ConstExpr &e) override
    {
        // Base case: The value of a constant expression is its stored value.
        return e.value;
    }

    int visit(const mvs::ExprIdent &e) override
    {
        // Base case: The value of an identifier must be looked up.
        return mvs::getIdentifierValue(e.name); 
    }
    
    int visit(const mvs::ExprUnary &e) override
    {
        // Recursive step: Evaluate the right-hand side first.
        int rhs_val = e.rhs->accept(*this);
        
        // Handle unary operators
        if (e.op == '~')
        {
            // Bitwise NOT (Invert all bits)
            return ~rhs_val; 
        }

        // Handle other unary operators if added (e.g., reduction OR: |)
        
        // Default error handling for unknown operators
        throw std::runtime_error("Unsupported unary operator: " + std::string(1, e.op));
    }

    int visit(const mvs::ExprBinary &e) override
    {
        // Recursive step: Evaluate both sides of the binary expression.
        int lhs_val = e.lhs->accept(*this);
        int rhs_val = e.rhs->accept(*this);

        // Handle binary logical/arithmetic operators
        if (e.op == '&')
            return lhs_val & rhs_val; // Bitwise AND
        if (e.op == '|')
            return lhs_val | rhs_val; // Bitwise OR
        if (e.op == '^')
            return lhs_val ^ rhs_val; // Bitwise XOR

        if (e.op == '+')
            return lhs_val + rhs_val; // Addition
        if (e.op == '-')
            return lhs_val - rhs_val; // Subtraction
        if (e.op == '*')
            return lhs_val * rhs_val; // Multiplication
        if (e.op == '/')
        {
            if (rhs_val == 0) throw std::runtime_error("Division by zero in expression evaluation.");
            return lhs_val / rhs_val; // Division
        }
        
        // Default error handling for unknown operators
        throw std::runtime_error("Unsupported binary operator: " + std::string(1, e.op));
    }
};