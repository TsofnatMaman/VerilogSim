#include "catch.hpp"
#include "mvs/lexer.hpp"
#include "mvs/parser.hpp"
#include <string>

using namespace mvs;

// --- VALID STUB TESTS ---

TEST_CASE("Parser parses stub with diverse ports", "[parser][valid]") {
    // Covers INPUT, OUTPUT, and INOUT
    std::string code = "module FullPort(input clk, output reset, inout data); endmodule";

    Lexer lexer(code);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);

    REQUIRE(parser.isModuleStubValid() == true);
}

TEST_CASE("Parser parses stub without port directions (default assumed)", "[parser][valid]") {
    // Ports without 'input'/'output' direction keywords
    std::string code = "module DefaultPort(a, b, c); endmodule";

    Lexer lexer(code);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);

    REQUIRE(parser.isModuleStubValid() == true);
}

TEST_CASE("Parser parses stub with body content (skip test)", "[parser][valid]") {
    // Tests that the parser correctly skips unparsed body items until 'endmodule'
    std::string code = "module WithBody(a); wire tmp; assign tmp = a; endmodule";

    Lexer lexer(code);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);

    REQUIRE(parser.isModuleStubValid() == true);
}

TEST_CASE("Parser parses stub without trailing semicolon", "[parser][valid]") {
    // Tests optional semicolon after the port list
    std::string code = "module NoSemi(a) endmodule";

    Lexer lexer(code);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);

    REQUIRE(parser.isModuleStubValid() == true);
}

// --- INVALID STUB TESTS ---

TEST_CASE("Parser fails on missing 'module' keyword", "[parser][invalid]") {
    // Missing the required first keyword
    std::string code = "MyModule(input a); endmodule";

    Lexer lexer(code);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);

    REQUIRE(parser.isModuleStubValid() == false);
}

TEST_CASE("Parser fails on missing module name", "[parser][invalid]") {
    // Missing identifier after 'module'
    std::string code = "module (input a); endmodule";

    Lexer lexer(code);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);

    REQUIRE(parser.isModuleStubValid() == false);
}

TEST_CASE("Parser fails on missing opening parenthesis '('", "[parser][invalid]") {
    // Module header syntax error
    std::string code = "module MyModule input a); endmodule";

    Lexer lexer(code);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);

    REQUIRE(parser.isModuleStubValid() == false);
}

TEST_CASE("Parser fails on missing closing parenthesis ')'", "[parser][invalid]") {
    // Module header syntax error
    std::string code = "module MyModule(input a; endmodule";

    Lexer lexer(code);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);

    REQUIRE(parser.isModuleStubValid() == false);
}

TEST_CASE("Parser fails on trailing comma in port list", "[parser][invalid]") {
    // Syntax error: trailing comma is not allowed (a, b,)
    std::string code = "module BadList(input a, output b,); endmodule";

    Lexer lexer(code);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);

    REQUIRE(parser.isModuleStubValid() == false);
}

TEST_CASE("Parser fails on missing identifier in port list", "[parser][invalid]") {
    // Syntax error: missing port name after comma
    std::string code = "module MissingId(input a, output); endmodule";

    Lexer lexer(code);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);

    REQUIRE(parser.isModuleStubValid() == false);
}