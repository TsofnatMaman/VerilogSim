#pragma once
#include <string>
#include <cctype>
#include <unordered_map>

namespace mvs
{
    enum class Keyword
    {
        MODULE,
        ENDMODULE,
        INPUT,
        OUTPUT,
        WIRE,
        ASSIGN,
        NONE
    };

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
    inline Keyword to_keyword(const std::string &str)
    {
        static const std::unordered_map<std::string, Keyword> keywords = {
            {"module", Keyword::MODULE},
            {"endmodule", Keyword::ENDMODULE},
            {"input", Keyword::INPUT},
            {"output", Keyword::OUTPUT},
            {"wire", Keyword::WIRE},
            {"assign", Keyword::ASSIGN}};

        auto it = keywords.find(str);
        return it != keywords.end() ? it->second : Keyword::NONE;
    }

    inline int parse_number(const std::string &str)
    {
        size_t i = 0;
        int width = 0;

        // optional width
        while (i < str.size() && std::isdigit(str[i]))
        {
            width = width * 10 + (str[i] - '0');
            i++;
        }

        if (i < str.size() && str[i] == '\'')
            i++; // skip of ` char

        if (i >= str.size())
            return 0;

        char base_char = std::tolower(str[i++]);
        std::string digits = str.substr(i);

        int base_int = 10;
        switch (base_char)
        {
        case 'b':
            base_int = 2;
            break;
        case 'h':
            base_int = 16;
            break;
        default:
            base_int = 10;
            break;
        }
        return std::stoi(digits, nullptr, base_int);
        ;
    }

} // namespace mvs
