#include "catch.hpp"
#include "mvs/lexer.hpp"
#include "mvs/parser.hpp"
#include "mvs/simulator.hpp"
#include <string>
#include <sstream>

using namespace mvs;

// 🔑 התיקון הקריטי מתמקד בהצגת הודעת השגיאה של ה-Parser
TEST_CASE("Full Pipeline Integration Test: Masking and Stabilization", "[integration][simulator]")
{
    const std::string VERILOG_CODE = R"(
module full_pipeline_test(input [31:0] IN, output [7:0] Y);
    wire [31:0] W;
    assign W = IN + 256; 
    assign Y = W; 
endmodule
)";

    Lexer lexer(VERILOG_CODE);
    Parser parser(lexer.Tokenize());

    // 1. קבלת התוצאה כאופציונלית
    std::optional<Module> optional_module = parser.parseModule();

    // 2. בדוק שה-Parser הצליח. אם לא, הדפס את הודעת השגיאה של ה-Parser!
    if (!optional_module.has_value())
    {
        FAIL("Parser failed to parse module: " << parser.getErrorMessage());
    }

    // אם הגענו לכאן, ה-Module קיים וניתן להשתמש בו בבטחה
    Module module = optional_module.value();

    // 3. Setup and Run Simulator (המשך הטסט)
    Simulator sim(std::move(module));
    sim.symbols_.set_value("IN", 257);
    sim.simulate();

    // 4. בדיקת התוצאה
    REQUIRE(sim.get_symbols().get_value("Y") == 1);
}

TEST_CASE("Parser Error Reporting - Line Number", "[parser][error]")
{
    // קוד Verilog שמכיל מילת מפתח לא מוכרת בכוונה בשורה 4
    const std::string BROKEN_VERILOG_CODE = R"(module error_test(input A, output Y);
    wire W;
    assign W = A;
    not_a_keyword Y = W; // <--- Error occurs here, now Line 4
endmodule
)";

    Lexer lexer(BROKEN_VERILOG_CODE);
    Parser parser(lexer.Tokenize());

    // 1. נסה לנתח את המודול
    std::optional<Module> optional_module = parser.parseModule();

    // 2. בדוק שהניתוח נכשל כצפוי
    REQUIRE_FALSE(optional_module.has_value());

    // 3. בדוק שמידע השגיאה קיים
    REQUIRE(parser.hasError());

    // 4. ודא שמספר השורה המדווח הוא 4
    // ההודעה הצפויה מ-parseModule() היא "Unexpected token in module body: not_a_keyword"

    // הערה: עלינו להשתמש ב-value() רק אחרי שבדקנו ש-hasError() הוא true
    const Error &error_info = parser.getError().value();

    // 5. דרישה ראשית: מספר השורה
    REQUIRE(error_info.line == 4);

    INFO("Actual Error Message: " << error_info.message);

    // 6. דרישה משנית: בדיקת תוכן ההודעה כדי לוודא שהגענו לנקודת הכישלון הנכונה
    REQUIRE(error_info.message.find("Unexpected token: not_a_keyword") != std::string::npos);
}