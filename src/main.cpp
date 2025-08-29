#include <iostream>
#include <fstream>
#include <sstream>

#include "mvs/Version.h"
#include "mvs/Lexer.h"
#include "mvs/Parser.h"
#include "mvs/AST.h"

int main(int argc, char** argv) {

    // start
    std::cout << "MiniVerilogSim starting..." << MVS_VERSION << std::endl;

    // check args and open file
    if(argc < 2) {
        std::cerr << "Usage: mvsim <file.v>\n";
        return 0;
    }

    std::ifstream in(argv[1]);
    if(!in) {
        std::cerr << "Failed to open: " << argv[1] << "\n";
        return 1;
    }

    std::ostringstream ss; ss << in.rdbuf();
    mvs::Lexer lx(ss.str());
    auto toks = lx.Tokenize();

    mvs::Parser p(toks);
    if(p.parseModuleStub()) {
        std::cout << "Parsed (stub) OK\n";
    }


    // check AST building
    auto a = std::make_shared<mvs::ExprIdent>();
    a->name = "a";
    auto b = std::make_shared<mvs::ExprIdent>();
    b->name = "b";
    auto c = std::make_shared<mvs::ExprIdent>();
    c->name = "a";

    auto ab = std::make_shared<mvs::ExprBinary>();
    ab->op = '&';
    ab->lhs = a;
    ab->rhs = b;

    auto nc = std::make_shared<mvs::ExprUnary>();
    nc->op = '~';
    nc->rhs = c;

    auto expr = std::make_shared<mvs::ExprBinary>();
    expr->op = '|';
    expr->lhs = ab;
    expr->rhs = nc;

    std::cout << "AST node count: " << node_count(*expr) << "\n";

    return 0;
}
