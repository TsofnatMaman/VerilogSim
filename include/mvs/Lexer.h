#pragma once

#include <string>
#include <vector>
#include "mvs/utils.h"

namespace mvs
{
    enum class TokenKind
    {
        IDENTIFIER,
        NUMBER,
        KEYWORD,
        SYMBOL,
        END
    };

    struct Token
    {
        TokenKind type;
        std::string text;
        int line;
        int col;
        Keyword kw = Keyword::NONE;
        int number_value;
    };
    class Lexer
    {

    public:
        explicit Lexer(std::string src) : src_(std::move(src)) {}
        std::vector<Token> Tokenize();

    private:
        std::string src_;
        size_t i_ = 0;
        int line_ = 1;
        int col_ = 1;

        bool eof() const { return i_ >= src_.size(); }
        char peek() const { return eof() ? '\0' : src_[i_]; }
        char get();

        void skip_space_and_comments();

        Token lex_identifier_or_keyword();
        Token lex_number();
        Token lex_symbol();
    };
}