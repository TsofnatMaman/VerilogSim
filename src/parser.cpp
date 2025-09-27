#include <stdexcept>

#include "mvs/parser.h"
#include "mvs/lexer.h"

namespace mvs
{
    bool Parser::_at_end() const
    {
        return idx_ >= tokens_.size();
    }

    const Token &Parser::_current() const
    {
        if (_at_end())
        {
            static Token eoft{TokenKind::END, "end of file", 0, 0};
            return eoft;
            // throw std::out_of_range("no more token idx: " + std::to_string(idx_));
        }
        return tokens_[idx_];
    }

    void Parser::_advance() const
    {
        if (!_at_end())
        {
            idx_++;
        }
    }

    void Parser::_skip_end_tokens() const
    {
        while (!_at_end() && _current().type == TokenKind::END)
            _advance();
    }

    bool Parser::_accept_keyword(const Keyword kw) const
    {
        if (!_at_end() && _current().type == TokenKind::KEYWORD && _current().kw == kw)
        {
            _advance();
            return true;
        }
        return false;
    }

    bool Parser::_accept_symbol(const std::string &sym) const
    {
        if (!_at_end() && _current().type == TokenKind::SYMBOL && _current().text == sym)
        {
            _advance();
            return true;
        }
        return false;
    }

    bool Parser::_accept_identifier(std::string &out) const
    {
        if (!_at_end() && _current().type == TokenKind::IDENTIFIER)
        {
            out = _current().text;
            _advance();
            return true;
        }
        return false;
    }

    bool Parser::_expect_keyword(const Keyword kw) const
    {
        return _expect_generic([&]()
                               { return _accept_keyword(kw); }, "Expected keyword: " + std::to_string(static_cast<int>(kw)));
    }

    bool Parser::_expect_symbol(const std::string &sym) const
    {
        return _expect_generic([&]()
                               { return _accept_symbol(sym); }, "Expected symbol: " + sym);
    }

    bool Parser::_expect_identifier(std::string &out) const
    {
        return _expect_generic([&]()
                               { return _accept_identifier(out); }, "Expected identifier");
    }

    // parse comma-separated identifiers inside parentheses:
    // ( a , b , c )
    bool Parser::_parse_port_list() const
    {
        if (!_expect_symbol("("))
        {
            return false;
        }

        // Accept optional empty list: allow immediate ')'
        if (_expect_symbol(")"))
        {
            // empty list
            return true;
        }

        while (!_at_end())
        {
            std::string name;

            // optional: port dir
            _accept_keyword(Keyword::INPUT) || _accept_keyword(Keyword::OUTPUT);

            if (!_expect_identifier(name))
            {
                return false;
            }

            // after identifier: either , or )
            if (_accept_symbol(")"))
            {
                return true;
            }
            if (!_expect_symbol(","))
                return false;
        }

        return _expect_symbol(")");
    }

    bool Parser::parseModuleStub() const
    {
        // reset state
        idx_ = 0;
        error_ = false;
        err_msg_.clear();

        // look for 'module'
        if (!_expect_keyword(Keyword::MODULE))
        {
            return false;
        }

        // module name
        std::string modname;
        if (!_expect_identifier(modname))
        {
            return false;
        }

        // '(' portlist ')'
        if (!_parse_port_list())
        {
            return false;
        }

        _accept_symbol(";");

        // For a stub we don't require full body — just find matching 'endmodule'
        // We'll scan tokens until we see 'endmodule' keyword
        while (!_at_end())
        {
            if (_accept_keyword(Keyword::ENDMODULE))
            {
                return true;
            }

            _advance();
        }
        // if we exhausted tokens without endmodule, fail
        error_ = true;
        err_msg_ = "Reached end of input without 'endmodule'";
        return false;
    }

} // namespace mvs