#define CATCH_CONFIG_MAIN 
#include "catch.hpp"
#include "mvs/lexer.hpp"
#include "mvs/parser.hpp"
#include <string>
#include <sstream>

using namespace mvs;

// Helper function to extract tokens and create parser for a given code snippet
// Note: This relies on the Parser having a way to expose _parse_port_list
// The tests assume _parse_port_list can be accessed (e.g., via a test fixture or protected access)

// --- VALID PORT LIST TESTS ---

TEST_CASE("Port List parses mixed directions and content checks", "[parser][ports][valid]") {
    std::string code = "(input clk, output reset, inout data, signal_z)";

    Lexer lexer(code);
    auto tokens = lexer.Tokenize();
    
    // Create a parser starting at the beginning of tokens
    Parser parser(tokens); 

    auto ports_opt = parser._parse_port_list();

    REQUIRE(ports_opt.has_value());
    
    auto ports = ports_opt.value();

    REQUIRE(ports.size() == 4);
    
    // 1. input clk
    REQUIRE(ports[0].name == "clk");
    REQUIRE(ports[0].dir == PortDir::INPUT);

    // 2. output reset
    REQUIRE(ports[1].name == "reset");
    REQUIRE(ports[1].dir == PortDir::OUTPUT);

    // 3. inout data
    REQUIRE(ports[2].name == "data");
    REQUIRE(ports[2].dir == PortDir::INOUT);
    
    // 4. signal_z (defaults to INPUT)
    REQUIRE(ports[3].name == "signal_z");
    REQUIRE(ports[3].dir == PortDir::INPUT);
}

TEST_CASE("Port List parses single port without direction", "[parser][ports][valid]") {
    std::string code = "(a)";

    Lexer lexer(code);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens); 

    auto ports_opt = parser._parse_port_list();

    REQUIRE(ports_opt.has_value());
    auto ports = ports_opt.value();
    
    REQUIRE(ports.size() == 1);
    REQUIRE(ports[0].name == "a");
    REQUIRE(ports[0].dir == PortDir::INPUT); // Check default direction
}

TEST_CASE("Port List parses empty list", "[parser][ports][valid]") {
    std::string code = "()";

    Lexer lexer(code);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens); 

    auto ports_opt = parser._parse_port_list();

    REQUIRE(ports_opt.has_value());
    REQUIRE(ports_opt.value().empty());
}

// --- INVALID PORT LIST TESTS ---

TEST_CASE("Port List fails on missing opening '('", "[parser][ports][invalid]") {
    std::string code = "input clk, output reset)";

    Lexer lexer(code);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens); 

    auto ports_opt = parser._parse_port_list();

    REQUIRE_FALSE(ports_opt.has_value());
}

TEST_CASE("Port List fails on missing closing ')'", "[parser][ports][invalid]") {
    std::string code = "(input clk, output reset";

    Lexer lexer(code);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens); 

    auto ports_opt = parser._parse_port_list();

    REQUIRE_FALSE(ports_opt.has_value());
}

TEST_CASE("Port List fails on missing identifier after comma", "[parser][ports][invalid]") {
    std::string code = "(input a, output)";

    Lexer lexer(code);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens); 

    auto ports_opt = parser._parse_port_list();

    REQUIRE_FALSE(ports_opt.has_value());
}

TEST_CASE("Port List fails on unexpected token after identifier", "[parser][ports][invalid]") {
    // Expects ',' or ')', but finds a random symbol
    std::string code = "(input a / output b)";

    Lexer lexer(code);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens); 

    auto ports_opt = parser._parse_port_list();

    REQUIRE_FALSE(ports_opt.has_value());
}