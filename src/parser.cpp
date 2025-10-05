#include <stdexcept>
#include <vector>
#include "mvs/parser.hpp"
#include "mvs/lexer.hpp"
#include "mvs/utils.hpp"

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

    bool Parser::_accept_number(int &out)
    {
        if (!_at_end() && _current().type == TokenKind::NUMBER)
        {
            out = _current().number_value;
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

    bool Parser::_expect_number(int &out)
    {
        return _expect_generic([&]()
                               { return _accept_number(out); }, "Expected number");
    }

    // parse comma-separated identifiers inside parentheses:
    // ( a , b , c )
    bool Parser::_is_port_list_valid()
    {
        // We want to check validity without consuming tokens permanently.
        auto saved_idx = idx_;
        auto saved_error = error_info_;

        auto ports = _parse_port_list();

        // restore parser state (this function is only a validator, not a consumer)
        idx_ = saved_idx;
        error_info_ = saved_error;

        return ports.has_value();
    }

    bool Parser::isModuleStubValid()
    {
        // reset state
        idx_ = 0;
        error_info_ = std::nullopt;

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
        _set_error("Reached end of input without 'endmodule'");
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
            // support ranges like [7:0], parse here:
            std::optional<int> width = _parse_bus_width();
            if (width.has_value())
            {
                p.width = width.value();
            }

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

    std::optional<std::vector<Wire>> Parser::_parse_wire_declaration()
    {
        std::vector<Wire> res;

        std::string wire_name;

        // optional: support [] to width
        int width = 32;
        std::optional<int> parsed_width = _parse_bus_width();
        if (parsed_width.has_value())
        {
            width = parsed_width.value();
        }

        do
        {
            if (!_expect_identifier(wire_name))
            {
                return std::nullopt;
            }

            res.push_back({wire_name, width});
        } while (_accept_symbol(","));

        if (!_expect_symbol(";"))
        {
            return std::nullopt;
        }

        return res;
    }

    std::optional<ExprPtr> Parser::_parse_expression()
    {
        // call two element func with lowest priority
        return _parse_binary(0);
    }

    std::optional<ExprPtr> Parser::_parse_unary()
    {
        // handle one element operator (ex ~)
        if (_current().type == TokenKind::SYMBOL && _accept_symbol("~"))
        {
            // calc the follow expression
            auto rhs = _parse_unary();
            if (!rhs.has_value())
                return std::nullopt; // error after the operator

            // build the ExprUnary node
            auto unary = std::make_shared<ExprUnary>();
            unary->op = '~';
            unary->rhs = std::move(rhs.value());
            return unary;
        }

        // handle identifier variables
        std::string identifier_name;
        if (_accept_identifier(identifier_name))
        {
            // this is identifier (ExprIdent)
            auto ident = std::make_shared<ExprIdent>();
            ident->name = identifier_name;
            return ident;
        }

        int num;
        if (_accept_number(num))
        {
            auto expr = std::make_shared<ConstExpr>();
            expr->value = num;
            return expr;
        }

        // handle parenthesis ( <expression> )
        if (_accept_symbol("("))
        {
            auto expr = _parse_expression(); // analize full expression inside
            if (!expr.has_value())
                return std::nullopt;

            if (!_expect_symbol(")"))
                return std::nullopt; // must close the parenthesis

            return expr;
        }

        // If there is no identifier and no parentheses, it is a syntax error in the expression
        _set_error("Expected identifier or unary operator in expression, got: " + _current().text);
        return std::nullopt;
    }

    int Parser::_get_precedence(const char &op) const
    {
        if (op == '^')
            return 5;
        if (op == '*' || op == '/')
            return 4;
        if (op == '+' || op == '-')
            return 3;
        if (op == '&')
            return 2;
        if (op == '|')
            return 1;
        return 0;
    }

    std::optional<ExprPtr> Parser::_parse_binary(int precedence)
    {
        // Start with the left-hand side, which is necessarily a fundamental/single-term expression
        auto lhs = _parse_unary();
        if (!lhs.has_value())
            return std::nullopt;

        while (!_at_end())
        {
            // check current operator
            char op = _current().text[0];
            int current_prec = _get_precedence(op);

            // Precedence: If the precedence is lower than the current precedence, stop the bipartite analysis
            if (current_prec <= precedence)
                break;
            if (current_prec == 0)
                break; // not operator

            // consume operator
            _advance();

            // Parse the right-hand side as a single-term expression or higher precedence
            auto rhs = _parse_binary(current_prec);
            if (!rhs.has_value())
                return std::nullopt;

            // build new node ExprBinary
            auto binary = std::make_shared<ExprBinary>();
            binary->op = op;
            binary->lhs = std::move(lhs.value());
            binary->rhs = std::move(rhs.value());

            // Make the new expression the left-hand side (lhs) for the next iteration of the loop
            lhs = binary;
        }

        return lhs;
    }

    std::optional<Assign> Parser::_parse_assign_statement()
    {
        Assign assign_stmt;

        // Expect the LHS identifier (the target of the assignment)
        // NOTE: In full Verilog, this could be a concatenation, but for simplicity, we expect an identifier
        if (!_expect_identifier(assign_stmt.lhs))
        {
            return std::nullopt;
        }

        // Expect the assignment symbol '='
        if (!_expect_symbol("="))
        {
            return std::nullopt;
        }

        // Parse the full expression on the RHS
        // This calls the expression parsing hierarchy to build the AST for the RHS.
        auto rhs_expr = _parse_expression();
        if (!rhs_expr.has_value())
        {
            // The error message is set by _parse_expression or its helpers
            return std::nullopt;
        }
        assign_stmt.rhs = std::move(rhs_expr.value());

        // Expect the semicolon ';' to terminate the statement
        if (!_expect_symbol(";"))
        {
            return std::nullopt;
        }

        // Return the successfully constructed Assign object
        return assign_stmt;
    }

    std::optional<int> Parser::_parse_bus_width()
    {
        if (!_accept_symbol("["))
        {
            return std::nullopt;
        }

        int msb = 0; // Most Significant Bit
        if (!_expect_number(msb))
        {
            return std::nullopt;
        }

        if (!_expect_symbol(":"))
        {
            return std::nullopt;
        }

        int lsb = 0; // Least Significant Bit
        if (!_expect_number(lsb))
        {
            return std::nullopt;
        }

        if (!_expect_symbol("]"))
        {
            return std::nullopt;
        }

        // calc width- width = MSB - LSB + 1.
        // for example: [7:0] => 7 - 0 + 1 = 8
        int width = msb - lsb + 1;
        if (width <= 0)
        {
           _set_error("Bus width cannot be zero or negative: [" + std::to_string(msb) + ":" + std::to_string(lsb) + "]");
            return std::nullopt;
        }

        return width;
    }

    std::optional<Module> Parser::parseModule()
    {
        if (!_expect_keyword(Keyword::MODULE))
        {
            return std::nullopt;
        }

        std::string modname;
        if (!_expect_identifier(modname))
        {
            return std::nullopt;
        }

        Module mod;
        mod.name = modname;

        std::optional<std::vector<Port>> ports = _parse_port_list();
        if (ports.has_value())
        {
            mod.ports = std::move(ports.value());
        }
        else
        {
            return std::nullopt;
        }

        while (!_at_end())
        {
            if (_accept_keyword(Keyword::WIRE))
            {
                std::optional<std::vector<Wire>> wires = _parse_wire_declaration();
                if (wires.has_value())
                {
                    mod.wires.insert(mod.wires.end(),
                                     std::make_move_iterator(wires->begin()),
                                     std::make_move_iterator(wires->end()));
                }
                else
                {
                    return std::nullopt;
                }
            }
            else if (_accept_keyword(Keyword::ASSIGN))
            {
                std::optional<Assign> assign = _parse_assign_statement();
                if (assign.has_value())
                {
                    mod.assigns.push_back(assign.value());
                }
                else
                {
                    return std::nullopt;
                }
            }
            else if (_accept_keyword(Keyword::ENDMODULE))
            {
                return mod;
            }
            else if(!_accept_symbol(";"))
            {
                _set_error("Unexpected token in module body: " + _current().text);
                return std::nullopt;
            }
        }
        _set_error("Reached end of file before 'endmodule'");
        return std::nullopt;
    }
} // namespace mvs