#pragma once
#include <string>
#include <cctype>
#include <unordered_map>
#include <stdexcept>

namespace mvs
{
    enum class Keyword
    {
        MODULE,
        ENDMODULE,
        INPUT,
        OUTPUT,
        INOUT,
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
        case '+':
        case '*':
        case '[':
        case ']':
        case ':':
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
            {"inout", Keyword::INOUT},
            {"wire", Keyword::WIRE},
            {"assign", Keyword::ASSIGN}};

        auto it = keywords.find(str);
        return it != keywords.end() ? it->second : Keyword::NONE;
    }

    inline int parse_number(const std::string &str)
    {
        size_t i = 0;

        // 1. optional width
        if (str[i] == ']')
        {
            while (i < str.size() && std::isdigit(str[i]))
                i++;
            if (str[i] != ']')
                throw std::runtime_error("invalid brackets " + str);
        }

        // 2. default base = 10
        int base = 10;

        if (i < str.size() && str[i] == '\'')
        {
            i++; // skip '
            if (i >= str.size())
                throw std::runtime_error("Incomplete number literal: missing base character after ' in \"" + str + "\"");
            char base_char = std::tolower(str[i++]);
            switch (base_char)
            {
            case 'b':
                base = 2;
                break;
            case 'h':
                base = 16;
                break;
            case 'd':
                base = 10;
                break;
            default:
                throw std::runtime_error("Invalid base character '" + std::string(1, base_char) + "' in number literal \"" + str + "\"");
            }
        }

        // 3. collect digits
        std::string digits;
        while (i < str.size())
        {
            char c = str[i++];
            if (c == '_')
                continue; // ignore underscores
            digits.push_back(c);
        }

        if (digits.empty())
            throw std::runtime_error("Missing value digits in number literal \"" + str + "\"");

        return std::stoi(digits, nullptr, base);
    }

} // namespace mvs
