#pragma once
#include "mvs/lexer.hpp"
#include "mvs/ast.hpp"
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

    private:
        // mutable because parseModuleStub is const in main; keep state per-instance but allow const method.
        std::vector<Token> tokens_;
        size_t idx_ = 0;

        // helpers
        bool error_ = false;
        std::string err_msg_;

        const Token &_current() const;
        void _advance();
        bool _at_end() const;

        bool _accept_keyword(const Keyword kw);
        bool _accept_symbol(const std::string &sym);
        bool _accept_identifier(std::string &out);

        template <typename AcceptFunc>
        bool _expect_generic(AcceptFunc accept, const std::string &msg)
        {
            if (accept())
            {
                return true;
            }
            error_ = true;
            err_msg_ = msg;
            return false;
        }

        bool _expect_keyword(const Keyword kw);
        bool _expect_symbol(const std::string &sym);
        bool _expect_identifier(std::string &out);

        // parsing helpers
        void _skip_end_tokens();

        std::optional<std::vector<Port>> Parser::_parse_port_list();
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
