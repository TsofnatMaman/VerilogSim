// Comprehensive parser-driven tests for mvs::CircuitSimulator
// These tests feed Verilog-like module text through Lexer -> Parser -> CircuitSimulator.
// Uses Catch2 single-header. Place this file in your test folder and compile with project sources.

#include "catch.hpp"

#include "mvs/lexer.hpp"
#include "mvs/parser.hpp"
#include "mvs/simulator.hpp"

using namespace mvs;

// Helper: run whole pipeline from source string -> tokenize -> parse -> simulate
static std::optional<Simulator> build_sim_from_source(const std::string &src, std::string &error_out)
{
    Lexer lx(src);
    auto tokens = lx.Tokenize();

    Parser p(tokens);
    auto opt_mod = p.parseModule();
    if (!opt_mod.has_value())
    {
        if (p.getError().has_value())
        {
            error_out = p.getError()->toString();
        }
        else
        {
            error_out = "unknown parse error";
        }
        return std::nullopt;
    }

    Module m = std::move(opt_mod.value());
    Simulator sim(std::move(m));
    return std::make_optional<Simulator>(std::move(sim));
}

static int sym_value(const Simulator &sim, const std::string &name)
{
    return sim.get_symbols().get_value(name);
}

// ---------------------- TESTS using textual modules ----------------------

TEST_CASE("Simple module: single assign constant", "[parser][sim]")
{
    const std::string src = R"(
module simple(output a);
    assign a = 42;
endmodule
)";

    std::string err;
    auto opt_sim = build_sim_from_source(src, err);
    REQUIRE(opt_sim.has_value());

    auto sim = std::move(opt_sim.value());
    sim.simulate();

    REQUIRE(sym_value(sim, "a") == 42);
}

TEST_CASE("Chained assigns parsed from text", "[parser][chain]")
{
    const std::string src = R"(
module chain(output a);
    wire b, c;
    assign a = b + 1;
    assign b = c + 2;
    assign c = 3;
endmodule
)";
    std::string err;
    auto opt_sim = build_sim_from_source(src, err);
    REQUIRE(opt_sim.has_value());
    auto sim = std::move(opt_sim.value());
    sim.simulate();

    REQUIRE(sym_value(sim, "c") == 3);
    REQUIRE(sym_value(sim, "b") == 5);
    REQUIRE(sym_value(sim, "a") == 6);
}

TEST_CASE("Slice assigns from text", "[parser][slice]")
{
    const std::string src = R"(
module slice_test(output [15:0] w);
    assign w[7:0] = 8'hFF;
endmodule
)";

    std::string err;
    auto opt_sim = build_sim_from_source(src, err);
    REQUIRE(opt_sim.has_value());
    auto sim = std::move(opt_sim.value());
    sim.simulate();

    REQUIRE((sym_value(sim, "w") & 0xFFFF) == 0x00FF);
}

TEST_CASE("Width truncation and bitwise ops from text", "[parser][ops]")
{
    const std::string src = R"(
module ops(output [7:0] a);
    wire b;
    assign b = 8'b00001010;
    assign a = ~b;
endmodule
)";

    std::string err;
    auto opt_sim = build_sim_from_source(src, err);
    REQUIRE(opt_sim.has_value());
    auto sim = std::move(opt_sim.value());
    sim.simulate();

    REQUIRE((sym_value(sim, "a") & 0xFF) == 0xF5);
}

TEST_CASE("Combined textual scenario: slices + arithmetic", "[parser][combined]")
{
    const std::string src = R"(
module combined(output [15:0] out);
    wire [3:0] hi, lo;
    wire [7:0] in1, in2, in3;

    assign hi = (in1 + in2) & 4'hF;
    assign lo = in3 & 4'hF;
    assign out[15:12] = hi;
    assign out[3:0] = lo;

    assign in1 = 8'h9;
    assign in2 = 8'h6;
    assign in3 = 8'hB;
endmodule
)";
    std::string err;
    auto opt_sim = build_sim_from_source(src, err);
    REQUIRE(opt_sim.has_value());
    auto sim = std::move(opt_sim.value());
    sim.simulate();

    int outv = sym_value(sim, "out");
    REQUIRE(((outv >> 12) & 0xF) == 0xF);
    REQUIRE((outv & 0xF) == 0xB);
}

// Negative test: ensure parser reports error for malformed module
TEST_CASE("Parser rejects malformed module", "[parser][error]")
{
    const std::string src = R"(
module bad(output a)
    assign a = ; // missing RHS
endmodule
)";
    std::string err;
    auto opt_sim = build_sim_from_source(src, err);
    REQUIRE(!opt_sim.has_value());
    REQUIRE(!err.empty());
}

// Notes:
// - These tests depend on Lexer/Parser accepting the Verilog-like syntax used here (e.g., "output [15:0] w;",
//   numeric literals like 8'hFF, 8'b00001010, and expressions using +, &, ~). If your lexer/parser uses a
//   slightly different numeric or slice syntax, adjust the module strings accordingly.
// - If numeric literal parsing isn't implemented (e.g., 8'hFF), replace with decimal constants like 255.
// - The tests intentionally exercise the full flow you requested: textual module -> lexer -> parser -> simulator.
// - If Parser::parseModule() returns Module but your Parser API uses a different method name or error reporting,
//   adapt the helper function build_sim_from_source accordingly.
