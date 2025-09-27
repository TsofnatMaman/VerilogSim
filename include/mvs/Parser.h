#pragma once
#include "mvs/lexer.h"
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
        bool parseModuleStub() const;

    private:
        // mutable because parseModuleStub is const in main; keep state per-instance but allow const method.
        std::vector<Token> tokens_;
        mutable size_t idx_ = 0;

        // helpers
        mutable bool error_ = false;
        mutable std::string err_msg_;

        const Token &_current() const;
        void _advance() const;
        bool _at_end() const;

        bool _accept_keyword(const Keyword kw) const;
        bool _accept_symbol(const std::string &sym) const;
        bool _accept_identifier(std::string &out) const;

        template <typename AcceptFunc>
        bool _expect_generic(AcceptFunc accept, const std::string &msg) const
        {
            if (accept())
            {
                return true;
            }
            error_ = true;
            err_msg_ = msg;
            return false;
        }

        bool _expect_keyword(const Keyword kw) const;
        bool _expect_symbol(const std::string &sym) const;
        bool _expect_identifier(std::string &out) const;

        // parsing helpers
        void _skip_end_tokens() const;

        bool _parse_port_list() const;
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
