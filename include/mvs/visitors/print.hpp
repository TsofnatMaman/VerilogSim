#pragma once

#include "mvs/module.hpp"
#include <iostream>
#include <string>

namespace mvs
{
    /**
     * @brief A visitor to print the AST in an indented tree format.
     */
    struct TreePrintVisitor : mvs::ExprVisitor
    {
        // Current depth for indentation.
        int current_depth = 0;
        
        // --- Helper functions ---
        
        /** Creates an indentation string based on the current depth. */
        std::string get_indent() const
        {
            // Use 4 spaces per indentation level.
            return std::string(current_depth * 4, ' ');
        }
        
        /** Prints the current node's header with indentation. */
        void print_node_header(const std::string& type, const std::string& value = "")
        {
            std::cout << get_indent() << "|-- " << type;
            if (!value.empty()) {
                std::cout << " (" << value << ")";
            }
            std::cout << "\n";
        }

        // --- Visit Methods ---

        int visit(const ExprIdent &e) override
        {
            print_node_header("IDENTIFIER", e.name);
            return 0;
        }

        int visit(const ConstExpr &e) override
        {
            print_node_header("CONSTANT", std::to_string(e.value));
            return 0;
        }

        int visit(const ExprUnary &e) override
        {
            print_node_header("UNARY", std::string(1, e.op));
            
            // Right-hand side (RHS)
            current_depth++;
            e.rhs->accept(*this);
            current_depth--;
            return 0;
        }

        int visit(const ExprBinary &e) override
        {
            print_node_header("BINARY", std::string(1, e.op));

            // Left-hand side (LHS)
            current_depth++;
            if (e.lhs) {
                e.lhs->accept(*this);
            } else {
                // Unlikely case in a valid binary expression
                std::cout << get_indent() << "|-- <Empty LHS>\n";
            }
            current_depth--;

            // Right-hand side (RHS)
            current_depth++;
            if (e.rhs) {
                e.rhs->accept(*this);
            } else {
                std::cout << get_indent() << "|-- <Empty RHS>\n";
            }
            current_depth--;
            return 0;
        }
    };

    /**
     * @brief Wrapper function to print the AST as a tree.
     */
    inline int to_string(const Expr &e)
    {
        TreePrintVisitor visitor;
        std::cout << "--- AST Tree ---\n";
        e.accept(visitor);
        std::cout << "----------------\n";
        return 1;
    }
}