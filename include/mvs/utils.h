#pragma once
#include <string>
#include <cctype>

namespace mvs
{
    inline bool is_identifier_start(char c)
    {
        return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
    }

    inline bool is_identifier_char(char c)
    {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
    }

    inline bool is_symbol_char(char c)
    {
        switch (c)
        {
        case '(':
        case ')':
        case ',':
        case ';':
        case '=':
        case '&':
        case '|':
        case '^':
        case '~':
            return true;
        default:
            return false;
        }
    }

    inline bool is_keyword(const std::string& str)
    {
        return str == "module" || str == "endmodule" || str == "input" || str == "output" || str == "wire" || str == "assign";
    }

} // namespace mvs
