#include <stdexcept>
#include <vector>
#include "mvs/parser.hpp"
#include "mvs/lexer.hpp"

namespace mvs
{
    Parser::Parser(const std::vector<Token> &tokens) : tokens_(tokens) {}

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

    void Parser::_advance()
    {
        if (!_at_end())
        {
            idx_++;
        }
    }

    void Parser::_skip_end_tokens()
    {
        while (!_at_end() && _current().type == TokenKind::END)
            _advance();
    }

    bool Parser::_accept_keyword(const Keyword kw)
    {
        if (!_at_end() && _current().type == TokenKind::KEYWORD && _current().kw == kw)
        {
            _advance();
            return true;
        }
        return false;
    }

    bool Parser::_accept_symbol(const std::string &sym)
    {
        if (!_at_end() && _current().type == TokenKind::SYMBOL && _current().text == sym)
        {
            _advance();
            return true;
        }
        return false;
    }

    bool Parser::_accept_identifier(std::string &out)
    {
        if (!_at_end() && _current().type == TokenKind::IDENTIFIER)
        {
            out = _current().text;
            _advance();
            return true;
        }
        return false;
    }

    bool Parser::_expect_keyword(const Keyword kw)
    {
        return _expect_generic([&]()
                               { return _accept_keyword(kw); }, "Expected keyword: " + std::to_string(static_cast<int>(kw)));
    }

    bool Parser::_expect_symbol(const std::string &sym)
    {
        return _expect_generic([&]()
                               { return _accept_symbol(sym); }, "Expected symbol: " + sym);
    }

    bool Parser::_expect_identifier(std::string &out)
    {
        return _expect_generic([&]()
                               { return _accept_identifier(out); }, "Expected identifier");
    }

    // parse comma-separated identifiers inside parentheses:
    // ( a , b , c )
    bool Parser::_is_port_list_valid()
    {
        // We want to check validity without consuming tokens permanently.
        auto saved_idx = idx_;
        auto saved_error = error_;
        auto saved_err_msg = err_msg_;

        auto ports = _parse_port_list();

        // restore parser state (this function is only a validator, not a consumer)
        idx_ = saved_idx;
        error_ = saved_error;
        err_msg_ = saved_err_msg;

        return ports.has_value();
    }

    bool Parser::isModuleStubValid()
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
        if (!_is_port_list_valid())
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

    std::optional<std::vector<Port>> Parser::_parse_port_list()
    {
        std::vector<Port> ports;
        if (!_expect_symbol("("))
        {
            return std::nullopt;
        }

        // Accept optional empty list: allow immediate ')'
        if (_accept_symbol(")"))
        {
            return ports; // Empty list
        }

        while (!_at_end())
        {
            Port p; // Create a new Port struct

            // Parse Port Direction (optional)
            if (_accept_keyword(Keyword::INPUT))
            {
                p.dir = PortDir::INPUT;
            }
            else if (_accept_keyword(Keyword::OUTPUT))
            {
                p.dir = PortDir::OUTPUT;
            }
            else if (_accept_keyword(Keyword::INOUT))
            {
                p.dir = PortDir::INOUT;
            }
            // TODO: if you support ranges like [7:0], parse here:

            // Expect Port Identifier (and save its name)
            if (!_expect_identifier(p.name))
            {
                return std::nullopt;
            }

            ports.push_back(std::move(p)); // Save the constructed port

            // After identifier: either , or )
            if (_accept_symbol(")"))
            {
                return ports; // End of list
            }
            if (!_expect_symbol(","))
            {
                return std::nullopt; // Error: expected comma or ')'
            }
        }

        // If we reach the end of the tokens without a closing ')'
        return _expect_symbol(")") ? std::make_optional(ports) : std::nullopt; // Will likely fail and set error_
    }

    std::optional<Module> Parser::parseModule(){
        return std::nullopt;
    }
} // namespace mvs