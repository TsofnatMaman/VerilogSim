#pragma once

#include "mvs/module.hpp"
#include <iostream>
#include <string>

namespace mvs
{
    /**
     * @brief מבקר להדפסת ה-AST בצורת עץ באמצעות הזחה.
     */
    struct TreePrintVisitor : mvs::ExprVisitor
    {
        // רמת העומק הנוכחית של ההדפסה
        int current_depth = 0;
        
        // --- פונקציות עזר ---
        
        /** יוצר מחרוזת הזחה לפי רמת העומק הנוכחית. */
        std::string get_indent() const
        {
            // משתמש ב-4 רווחים לכל רמת הזחה
            return std::string(current_depth * 4, ' ');
        }
        
        /** מדפיס את הכותרת של הצומת הנוכחי עם הזחה. */
        void print_node_header(const std::string& type, const std::string& value = "")
        {
            std::cout << get_indent() << "|-- " << type;
            if (!value.empty()) {
                std::cout << " (" << value << ")";
            }
            std::cout << "\n";
        }

        // --- מתודות Visit ---

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
            
            // צד ימין (RHS)
            current_depth++;
            e.rhs->accept(*this);
            current_depth--;
            return 0;
        }

        int visit(const ExprBinary &e) override
        {
            print_node_header("BINARY", std::string(1, e.op));

            // צד שמאל (LHS)
            current_depth++;
            if (e.lhs) {
                e.lhs->accept(*this);
            } else {
                // מקרה לא סביר בביטוי בינארי תקין
                std::cout << get_indent() << "|-- <Empty LHS>\n";
            }
            current_depth--;

            // צד ימין (RHS)
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
     * @brief פונקציית עטיפה להדפסת ה-AST כעץ.
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