#include "mvs/parser.hpp"
#include <memory>
#include <iterator>

namespace mvs
{
    Parser::Parser(const std::vector<Token> &tokens) : tokens_(tokens) {}

    bool Parser::_at_end() const
    {
        return idx_ >= tokens_.size();
    }

    const Token &Parser::_current() const
    {
        static Token eoft{TokenKind::END, "end of file", 0, 0};
        if (_at_end())
            return eoft;
        return tokens_[idx_];
    }

    void Parser::_advance()
    {
        if (!_at_end())
            idx_++;
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
        return _expect_generic([&]() { return _accept_keyword(kw); },
                               "Expected keyword: " + std::to_string(static_cast<int>(kw)));
    }

    bool Parser::_expect_symbol(const std::string &sym)
    {
        return _expect_generic([&]() { return _accept_symbol(sym); }, "Expected symbol: " + sym);
    }

    bool Parser::_expect_identifier(std::string &out)
    {
        return _expect_generic([&]() { return _accept_identifier(out); }, "Expected identifier");
    }

    bool Parser::_expect_number(int &out)
    {
        return _expect_generic([&]() { return _accept_number(out); }, "Expected number");
    }

    // ----------------------------------------
    // Ports parsing
    // ----------------------------------------
    std::optional<std::vector<Port>> Parser::_parse_port_list()
    {
        std::vector<Port> ports;

        if (!_expect_symbol("("))
            return std::nullopt;

        if (_accept_symbol(")"))
        {
            _accept_symbol(";"); // optional semicolon
            return ports;
        }

        while (!_at_end())
        {
            Port p;

            // Direction
            if (_accept_keyword(Keyword::INPUT))
                p.dir = PortDir::INPUT;
            else if (_accept_keyword(Keyword::OUTPUT))
                p.dir = PortDir::OUTPUT;
            else if (_accept_keyword(Keyword::INOUT))
                p.dir = PortDir::INOUT;

            // Optional bus width
            if (auto bus_opt = _parse_bit_or_bus_selection(); bus_opt.has_value())
                p.width = bus_opt.value().msb.value() - bus_opt.value().lsb.value() + 1;

            if (!_expect_identifier(p.name))
                return std::nullopt;

            ports.push_back(std::move(p));

            if (_accept_symbol(")"))
            {
                _accept_symbol(";");
                return ports;
            }

            if (!_expect_symbol(","))
                return std::nullopt;
        }

        return _expect_symbol(")") ? std::make_optional(ports) : std::nullopt;
    }

    bool Parser::_is_port_list_valid()
    {
        size_t backup_idx = idx_;
        auto backup_error = error_info_;

        auto ports = _parse_port_list();

        idx_ = backup_idx;
        error_info_ = backup_error;

        return ports.has_value();
    }

    // ----------------------------------------
    // Wire parsing
    // ----------------------------------------
    std::optional<std::vector<Wire>> Parser::_parse_wire_declaration()
    {
        std::vector<Wire> res;
        int width = 32;

        if (auto bus_opt = _parse_bit_or_bus_selection(); bus_opt.has_value())
            width = bus_opt.value().msb.value() - bus_opt.value().lsb.value() + 1;

        do
        {
            std::string wire_name;
            if (!_expect_identifier(wire_name))
                return std::nullopt;

            res.push_back({wire_name, width});
        } while (_accept_symbol(","));

        if (!_expect_symbol(";"))
            return std::nullopt;

        return res;
    }

    // ----------------------------------------
    // Expressions
    // ----------------------------------------
    std::optional<ExprPtr> Parser::_parse_expression()
    {
        return _parse_binary(0);
    }

    std::optional<ExprPtr> Parser::_parse_unary()
    {
        if (_accept_symbol("~"))
        {
            auto rhs = _parse_unary();
            if (!rhs.has_value())
                return std::nullopt;

            auto unary = std::make_shared<ExprUnary>();
            unary->op = '~';
            unary->rhs = std::move(rhs.value());
            return unary;
        }

        std::string id_name;
        if (_accept_identifier(id_name))
        {
            auto ident = std::make_shared<ExprIdent>();
            ident->name = id_name;

            if (auto bus_opt = _parse_bit_or_bus_selection(); bus_opt.has_value())
                ident->tb = bus_opt.value();

            return ident;
        }

        int num;
        if (_accept_number(num))
        {
            auto c = std::make_shared<ConstExpr>();
            c->value = num;
            return c;
        }

        if (_accept_symbol("("))
        {
            auto expr = _parse_expression();
            if (!expr.has_value())
                return std::nullopt;

            if (!_expect_symbol(")"))
                return std::nullopt;

            return expr;
        }

        _set_error("Expected identifier or unary operator, got: " + _current().text);
        return std::nullopt;
    }

    int Parser::_get_precedence(const char &op) const
    {
        if (op == '^') return 5;
        if (op == '*' || op == '/') return 4;
        if (op == '+' || op == '-') return 3;
        if (op == '&') return 2;
        if (op == '|') return 1;
        return 0;
    }

    std::optional<ExprPtr> Parser::_parse_binary(int precedence)
    {
        auto lhs = _parse_unary();
        if (!lhs.has_value())
            return std::nullopt;

        while (!_at_end())
        {
            char op = _current().text[0];
            int current_prec = _get_precedence(op);

            if (current_prec <= precedence || current_prec == 0)
                break;

            _advance();

            auto rhs = _parse_binary(current_prec);
            if (!rhs.has_value())
                return std::nullopt;

            auto bin = std::make_shared<ExprBinary>();
            bin->op = op;
            bin->lhs = std::move(lhs.value());
            bin->rhs = std::move(rhs.value());

            lhs = bin;
        }

        return lhs;
    }

    // ----------------------------------------
    // Assignment
    // ----------------------------------------
    std::optional<Assign> Parser::_parse_assign_statement()
    {
        Assign assign_stmt;

        if (!_expect_identifier(assign_stmt.name))
            return std::nullopt;

        if (auto bus_opt = _parse_bit_or_bus_selection(); bus_opt.has_value())
            assign_stmt.tb = bus_opt.value();

        if (!_expect_symbol("="))
            return std::nullopt;

        auto rhs_expr = _parse_expression();
        if (!rhs_expr.has_value())
            return std::nullopt;

        assign_stmt.rhs = std::move(rhs_expr.value());

        if (!_expect_symbol(";"))
            return std::nullopt;

        return assign_stmt;
    }

    // ----------------------------------------
    // Bit/bus parsing
    // ----------------------------------------
    std::optional<TargetBits> Parser::_parse_bit_or_bus_selection()
    {
        if (!_accept_symbol("["))
            return std::nullopt;

        int msb = 0;
        if (!_expect_number(msb))
            return std::nullopt;

        if (_accept_symbol(":"))
        {
            int lsb = 0;
            if (!_expect_number(lsb))
                return std::nullopt;

            if (!_expect_symbol("]"))
                return std::nullopt;

            return TargetBits{msb, lsb};
        }
        else
        {
            if (!_expect_symbol("]"))
                return std::nullopt;

            return TargetBits{msb, msb};
        }
    }

    // ----------------------------------------
    // Module parsing
    // ----------------------------------------
    std::optional<Module> Parser::parseModule()
    {
        if (!_expect_keyword(Keyword::MODULE))
            return std::nullopt;

        std::string modname;
        if (!_expect_identifier(modname))
            return std::nullopt;

        Module mod;
        mod.name = modname;

        auto ports = _parse_port_list();
        if (!ports.has_value())
            return std::nullopt;
        mod.ports = std::move(ports.value());

        while (!_at_end())
        {
            if (_accept_keyword(Keyword::WIRE))
            {
                auto wires = _parse_wire_declaration();
                if (!wires.has_value())
                    return std::nullopt;

                mod.wires.insert(mod.wires.end(),
                                 std::make_move_iterator(wires->begin()),
                                 std::make_move_iterator(wires->end()));
            }
            else if (_accept_keyword(Keyword::ASSIGN))
            {
                auto assign = _parse_assign_statement();
                if (!assign.has_value())
                    return std::nullopt;

                mod.assigns.push_back(assign.value());
            }
            else if (_accept_keyword(Keyword::ENDMODULE))
            {
                return mod;
            }
            else
            {
                _set_error("Unexpected token: " + _current().text);
                return std::nullopt;
            }
        }

        _set_error("Reached end of file before 'endmodule'");
        return std::nullopt;
    }

    bool Parser::isModuleStubValid()
    {
        idx_ = 0;
        error_info_ = std::nullopt;

        if (!_expect_keyword(Keyword::MODULE))
            return false;

        std::string modname;
        if (!_expect_identifier(modname))
            return false;

        if (!_is_port_list_valid())
            return false;

        _accept_symbol(";");

        while (!_at_end())
        {
            if (_accept_keyword(Keyword::ENDMODULE))
                return true;

            _advance();
        }

        _set_error("Reached end of input without 'endmodule'");
        return false;
    }

} // namespace mvs
