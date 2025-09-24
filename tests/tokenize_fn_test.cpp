#include <iostream>
#include "mvs/Lexer.h"
#include "mvs/utils.h"

int main() {
    using namespace mvs;

    std::string src = R"(
module my_module;
    input a, b;
    output y;
    wire tmp;
    assign tmp = a & b;
    assign y = tmp | 4'b1010;
endmodule
)";

    Lexer lexer(src);
    auto tokens = lexer.Tokenize();

    for (const auto& tok : tokens) {
        std::cout << "Token: \"" << tok.text << "\"";

        switch (tok.type) {
            case TokenKind::IDENTIFIER:
                std::cout << " (IDENTIFIER)";
                break;
            case TokenKind::KEYWORD:
                std::cout << " (KEYWORD)";
                break;
            case TokenKind::NUMBER:
                std::cout << " (NUMBER, value=" << tok.number_value << ")";
                break;
            case TokenKind::SYMBOL:
                std::cout << " (SYMBOL)";
                break;
            case TokenKind::END:
                std::cout << " (END)";
                break;
        }

        std::cout << " [line=" << tok.line << ", col=" << tok.col << "]" << std::endl;
    }

    return 0;
}
