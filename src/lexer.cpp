#include "mvs/Lexer.h"
#include "mvs/utils.h"

using namespace mvs;

char Lexer::get()
{
    if (eof())
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

void Lexer::skip_space_and_comments()
{
    while (!eof())
    {
        char c = peek();
        if (std::isspace(static_cast<unsigned char>(c)))
        {
            get();
        }
        else if (c == '/' && i_ + 1 < src_.size() && src_[i_ + 1] == '/')
        {
            while (!eof() && get() != '\n');
        }
        else if (c == '/' && i_ + 1 < src_.size() && src_[i_ + 1] == '*')
        {
            get(); get();
            while (!eof()) {
                if (peek() == '*' && i_ + 1 < src_.size() && src_[i_ + 1] == '/') {
                    get(); get(); 
                    break;
                }
                get();
            }
        }
        else break;
    }
}

Token Lexer::lex_identifier_or_keyword()
{
    int start_line = line_;
    int start_col  = col_;
    std::string ident;

    while (!eof() && is_identifier_char(peek())) {
        ident += get();
    }

    Keyword k = to_keyword(ident);
    if (k != Keyword::NONE) {
        return Token{TokenKind::KEYWORD, ident, start_line, start_col, k};
    } else {
        return Token{TokenKind::IDENTIFIER, ident, start_line, start_col, Keyword::NONE};
    }
}

Token Lexer::lex_number() {
    int start_line = line_;
    int start_col  = col_;
    std::string raw;

    while (!eof() && (std::isdigit(peek()) || peek() == '\'' || peek() == '.' || std::isxdigit(peek()))) {
        raw += get();
    }

    int value = parse_number(raw);  // המרה לערך אחיד

    Token tok;
    tok.type = TokenKind::NUMBER;
    tok.text = raw;
    tok.line = start_line;
    tok.col  = start_col;
    tok.number_value = value;

    return tok;
}

Token Lexer::lex_symbol() {
    int start_line = line_;
    int start_col  = col_;
    char c = get();

    return Token{TokenKind::SYMBOL, std::string(1, c), start_line, start_col};
}

std::vector<Token> Lexer::Tokenize() {
    std::vector<Token> tokens;
    while (!eof()) {
        skip_space_and_comments();
        if (eof()) break;

        char c = peek();

        if (is_identifier_start(c)) {
            tokens.push_back(lex_identifier_or_keyword());
        }
        else if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(lex_number());
        }
        else if (is_symbol_char(c)) {
            tokens.push_back(lex_symbol());
        }
        else {
            get();
        }
    }

    tokens.push_back(Token{TokenKind::END, "", line_, col_});
    return tokens;
}

