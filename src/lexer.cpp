#include "mvs/lexer.hpp"
#include "mvs/utils.hpp"
#include <iostream>

using namespace mvs;

char Lexer::_get()
{
    if (_eof())
        return '\0';
    char c = src_[i_++];
    if (c == '\n')
    {
        line_++;
        col_ = 1;
    }
    else
    {
        col_++;
    }
    return c;
}

void Lexer::_skip_space_and_comments()
{
    while (!_eof())
    {
        char c = _current();
        if (std::isspace(static_cast<unsigned char>(c)))
        {
            _get();
        }
        else if (c == '/' && i_ + 1 < src_.size() && src_[i_ + 1] == '/')
        {
            while (!_eof() && _get() != '\n')
                ;
        }
        else if (c == '/' && i_ + 1 < src_.size() && src_[i_ + 1] == '*')
        {
            _get();
            _get();
            while (!_eof())
            {
                if (_current() == '*' && i_ + 1 < src_.size() && src_[i_ + 1] == '/')
                {
                    _get();
                    _get();
                    break;
                }
                _get();
            }
        }
        else
            break;
    }
}

Token Lexer::_lex_identifier_or_keyword()
{
    int start_line = line_;
    int start_col = col_;
    std::string ident;

    while (!_eof() && is_identifier_char(_current()))
    {
        ident += _get();
    }

    Keyword k = to_keyword(ident);
    if (k != Keyword::NONE)
    {
        return Token{TokenKind::KEYWORD, ident, start_line, start_col, k};
    }
    else
    {
        return Token{TokenKind::IDENTIFIER, ident, start_line, start_col, Keyword::NONE};
    }
}

Token Lexer::_lex_number()
{
    int start_line = line_;
    int start_col = col_;
    std::string raw;

    // Read optional width (digits)
    while (!_eof() && std::isdigit(_current()))
    {
        raw += _get();
    }

    // Check for Verilog base marker '
    if (!_eof() && _current() == '\'')
    {
        raw += _get(); // consume '
        if (!_eof())
            raw += _get(); // consume base char (b/h/d)
    }

    // Now collect remaining digits (hex/bin/decimal)
    while (!_eof() && (std::isalnum(_current()) || _current() == '_'))
    {
        raw += _get();
    }

    int value = parse_number(raw);

    Token tok;
    tok.type = TokenKind::NUMBER;
    tok.text = raw;
    tok.line = start_line;
    tok.col = start_col;
    tok.number_value = value;

    return tok;
}

Token Lexer::_lex_symbol()
{
    int start_line = line_;
    int start_col = col_;
    char c = _get();

    return Token{TokenKind::SYMBOL, std::string(1, c), start_line, start_col};
}

std::vector<Token> Lexer::Tokenize()
{
    std::vector<Token> tokens;
    while (!_eof())
    {
        _skip_space_and_comments();
        if (_eof())
            break;

        char c = _current();

        if (is_identifier_start(c))
        {
            tokens.push_back(_lex_identifier_or_keyword());
        }
        else if (std::isdigit(static_cast<unsigned char>(c)))
        {
            tokens.push_back(_lex_number());
        }
        else if (is_symbol_char(c))
        {
            tokens.push_back(_lex_symbol());
        }
        else
        {
            std::cerr << "Lexer Warning: Skipping unrecognized character '" << c << "' at " << line_ << ":" << col_
                      << std::endl;
            _get();
        }
    }

    tokens.push_back(Token{TokenKind::END, "", line_, col_});
    return tokens;
}
