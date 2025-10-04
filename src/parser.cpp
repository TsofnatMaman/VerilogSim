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

    bool Parser::_accept_number(int &out){
        if(!_at_end() && _current().type == TokenKind::NUMBER){
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

    std::optional<std::vector<Wire>> Parser::_parse_wire_declaration()
    {
        // TODO: optional: support [] to width
        std::vector<Wire> res;

        std::string wire_name;
        if (!_expect_identifier(wire_name))
        {
            return std::nullopt;
        }
        res.push_back({wire_name});

        while (_accept_symbol(","))
        {
            if (!_expect_identifier(wire_name))
            {
                return std::nullopt;
            }

            res.push_back({wire_name});
        }

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
        // 1. handle one element operator (ex ~)
        if (_current().type == TokenKind::SYMBOL)
        {
            if (_current().text == "~")
            {
                char op = _current().text[0];
                _advance(); // consume the ~

                // calc the follow expression
                auto rhs = _parse_unary();
                if (!rhs.has_value())
                    return std::nullopt; // error after the operator

                // build the ExprUnary node
                auto unary = std::make_shared<ExprUnary>();
                unary->op = op;
                unary->rhs = std::move(rhs.value());
                return unary;
            }
        }

        // 2. handle identifier variables
        std::string identifier_name;
        if (_accept_identifier(identifier_name))
        {
            // this is identifier (ExprIdent)
            auto ident = std::make_shared<ExprIdent>();
            ident->name = identifier_name;
            return ident;
        }
        
        int num;
        if(_accept_number(num)){
            auto expr = std::make_shared<ConstExpr>();
            expr->value = num;
            return expr;
        }

        // 3. handle parenthesis ( <expression> )
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
        error_ = true;
        err_msg_ = "Expected identifier or unary operator in expression, got: " + _current().text;
        return std::nullopt;
    }

    
    int Parser::_get_precedence(const std::string &op) const
    {
        if (op == "*" || op == "/")
            return 5;
        if (op == "+" || op == "-")
            return 4;
        if (op == "&")
            return 3;
        if (op == "^")
            return 2;
        if (op == "|")
            return 1;
        return 0;
    }

    std::optional<ExprPtr> Parser::_parse_binary(int precedence)
    {
        // התחל עם צד שמאל, שהוא בהכרח ביטוי יסודי/חד-איברי
        auto lhs = _parse_unary();
        if (!lhs.has_value())
            return std::nullopt;

        while (!_at_end())
        {
            // 1. בדוק את האופרטור הנוכחי
            std::string op_text = _current().text;
            int current_prec = _get_precedence(op_text);

            // 2. קדימות: אם הקדימות נמוכה מהקדימות הנוכחית, הפסק את הניתוח הדו-איברי
            if (current_prec < precedence)
                break;
            if (current_prec == 0)
                break; // לא אופרטור

            // 3. צרך את האופרטור
            _advance(); // צרך את האופרטור (&, |, + וכו')

            // 4. נתח את צד ימין כביטוי חד-איברי או עדיפות גבוהה יותר
            auto rhs = _parse_unary();
            if (!rhs.has_value())
                return std::nullopt;

            // 5. בדוק את האופרטור הבא
            while (!_at_end())
            {
                std::string next_op_text = _current().text;
                int next_prec = _get_precedence(next_op_text);

                // אם האופרטור הבא חזק יותר, או שווה (אסוציאטיביות משמאל), נתח אותו קודם
                if (next_prec > current_prec)
                {
                    // נתח את שאר הביטוי (צד ימין החדש) באופן רקורסיבי
                    rhs = _parse_binary(next_prec);
                    if (!rhs.has_value())
                        return std::nullopt;
                    continue; // חזור לבדוק את האופרטור שאחריו
                }
                break;
            }

            // 6. בנה צומת ExprBinary חדש
            auto binary = std::make_shared<ExprBinary>();
            binary->op = op_text[0]; // שמור את התו הראשון של האופרטור
            binary->lhs = std::move(lhs.value());
            binary->rhs = std::move(rhs.value());

            // 7. הפוך את הביטוי החדש לצד שמאל (lhs) לסיבוב הבא של הלולאה
            lhs = binary;
        }

        return lhs;
    }

    std::optional<Assign> Parser::_parse_assign_statement()
    {
        Assign assign_stmt;

        // 1. Expect the LHS identifier (the target of the assignment)
        // NOTE: In full Verilog, this could be a concatenation, but for simplicity, we expect an identifier
        if (!_expect_identifier(assign_stmt.lhs))
        {
            return std::nullopt;
        }

        // 2. Expect the assignment symbol '='
        if (!_expect_symbol("="))
        {
            return std::nullopt;
        }

        // 3. Parse the full expression on the RHS
        // This calls the expression parsing hierarchy to build the AST for the RHS.
        auto rhs_expr = _parse_expression();
        if (!rhs_expr.has_value())
        {
            // The error message is set by _parse_expression or its helpers
            return std::nullopt;
        }
        assign_stmt.rhs = std::move(rhs_expr.value());

        // 4. Expect the semicolon ';' to terminate the statement
        if (!_expect_symbol(";"))
        {
            return std::nullopt;
        }

        // Return the successfully constructed Assign object
        return assign_stmt;
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
            else
            {
                error_ = true;
                err_msg_ = "Unexpected token in module body: " + _current().text;
                return std::nullopt;
            }
        }
        error_ = true;
        err_msg_ = "Reached end of file before 'endmodule'";
        return std::nullopt;
    }
} // namespace mvs