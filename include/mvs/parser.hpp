#pragma once
#include "mvs/lexer.hpp"
#include "mvs/module.hpp"
#include "mvs/error.hpp"
#include <optional>
#include <vector>
#include <string>

namespace mvs
{
    class Parser
    {
    public:
        explicit Parser(const std::vector<Token> &tokens);

        // Parses just a minimal module stub: module <ident> ( <ports> ) ... endmodule
        // Returns true on success (consumed a syntactically valid module stub).
        bool isModuleStubValid();

        std::optional<Module> parseModule();

        const std::optional<Error> &getError() const
        {
            return error_info_;
        }

        // private:
        std::vector<Token> tokens_;
        size_t idx_ = 0;

        std::optional<Error> error_info_;

        std::string getErrorMessage() const
        {
            return error_info_.value().toString();
        }

        bool hasError() const
        {
            return error_info_.has_value();
        }

        void _set_error(std::string msg)
        {
            error_info_ = Error{msg, _current().line};
        }

        const Token &_current() const;
        void _advance();
        bool _at_end() const;

        bool _accept_keyword(const Keyword kw);
        bool _accept_symbol(const std::string &sym);
        bool _accept_identifier(std::string &out);
        bool _accept_number(int &out);

        template <typename AcceptFunc>
        bool _expect_generic(AcceptFunc accept, const std::string &msg)
        {
            if (accept())
            {
                return true;
            }
            _set_error(msg);
            return false;
        }

        bool _expect_keyword(const Keyword kw);
        bool _expect_symbol(const std::string &sym);
        bool _expect_identifier(std::string &out);
        bool _expect_number(int &out);

        // parsing helpers
        void _skip_end_tokens();

        std::optional<std::vector<Port>> _parse_port_list();
        std::optional<std::vector<Wire>> _parse_wire_declaration();
        std::optional<Assign> _parse_assign_statement();

        std::optional<ExprPtr> _parse_expression();
        std::optional<ExprPtr> _parse_unary();
        std::optional<ExprPtr> _parse_binary(int precedence);
        int _get_precedence(const char &op) const;

        std::optional<int> _parse_bus_width();

        bool _is_port_list_valid();
    };
}

// Start Parsing Module Stub
//           │
//           ▼
//    idx_ = 0, error_ = false
//           │
//           ▼
//    peek() / current() → get first token
//           │
//           ▼
// expect_keyword("module") ?
//    │                 │
//    ▼                 ▼
//   ✅                  ❌
//  advance()            set error_ = true
//                       store err_msg_
//                       return false
//           │
//           ▼
// accept_identifier(module_name) ?
//    │                 │
//    ▼                 ▼
//   ✅                  ❌
//  advance()            set error_ = true
//                       store err_msg_
//                       return false
//           │
//           ▼
// parse_port_list() ?
//    │                 │
//    ▼                 ▼
//   ✅                  ❌
//  advance()            set error_ = true
//                       store err_msg_
//                       return false
//           │
//           ▼
// skip_end_tokens()
//           │
//           ▼
// expect_keyword("endmodule") ?
//    │                 │
//    ▼                 ▼
//   ✅                  ❌
//  advance()            set error_ = true
//                       store err_msg_
//                       return false
//           │
//           ▼
// All checks passed → return true
