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
            line_++;
            col_ = 1;
        }
        else if (c == '/' && i_ + 1 < src_.size() && src_[i_ + 1] == '*')
        {
            while (!eof())
            {
                if (peek() != '*' || src_[i_ + 1] != '/')
                {
                    get();
                }
            }
        }
        else
        {
            break;
        }
    }
}

Token Lexer::lex_identifier_or_keyword()
{
    int start_line = line_;
    int start_col  = col_;

    
}
