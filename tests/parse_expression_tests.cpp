#include "catch.hpp"
#include "mvs/lexer.hpp"
#include "mvs/parser.hpp"
#include "mvs/module.hpp"
#include "mvs/algorithms.hpp"
#include <string>
#include <memory>

using namespace mvs;

// -----------------------------------------------------------------------------
// Helpers for inspecting the AST structure
// -----------------------------------------------------------------------------

// Helper to check if an expression is a constant (number)
bool check_const(const ExprPtr& expr, int expected_value) {
    auto const_expr = std::dynamic_pointer_cast<ConstExpr>(expr);
    return const_expr != nullptr && const_expr->value == expected_value;
}

// Helper to check if an expression is an identifier (variable name)
bool check_ident(const ExprPtr& expr, const std::string& expected_name) {
    auto ident_expr = std::dynamic_pointer_cast<ExprIdent>(expr);
    return ident_expr != nullptr && ident_expr->name == expected_name;
}

// -----------------------------------------------------------------------------
// Test Cases for Expression Parsing
// -----------------------------------------------------------------------------

TEST_CASE("Expression Parsing - Precedence and Parentheses") {
    // Expression: 5 + a * 2 ^ (b + 0)
    // Expected AST structure (precedence: () > ^ > * > +):
    // 
    //            (+)
    //           /   \
    //         (5)    (*)
    //               /   \
    //             (a)    (^)
    //                   /   \
    //                 (2)    (+)
    //                       /   \
    //                     (b)   (0)
    
    std::string code = "5 + a * 2 ^ (b + 0)";
    Lexer lexer(code);
    Parser parser(lexer.Tokenize());

    std::optional<ExprPtr> result = parser._parse_expression();

    // 1. Ensure the expression was parsed successfully
    REQUIRE(result.has_value());
    ExprPtr expr = result.value();

    to_string(*expr);
    // to_string(*expr); // השאר את זה כ-Comment או השתמש בו ל-Debug

    // 2. The root operator should be '+' (lowest precedence)
    auto root_add = std::dynamic_pointer_cast<ExprBinary>(expr);
    REQUIRE(root_add != nullptr);
    REQUIRE(root_add->op == '+'); // התיקון: הפעלת הוספת הפלוס

    // 3. Check LHS of '+': Must be '5'
    REQUIRE(check_const(root_add->lhs, 5));

    // 4. Check RHS of '+': Must be the multiplication expression (a * ...)
    auto mul_op = std::dynamic_pointer_cast<ExprBinary>(root_add->rhs);
    REQUIRE(mul_op != nullptr);
    REQUIRE(mul_op->op == '*'); // התיקון: RHS הוא כפל

    // 5. Check LHS of '*': Must be 'a'
    REQUIRE(check_ident(mul_op->lhs, "a"));

    // 6. Check RHS of '*': Must be the power expression (2 ^ ...)
    auto pow_op = std::dynamic_pointer_cast<ExprBinary>(mul_op->rhs);
    REQUIRE(pow_op != nullptr);
    REQUIRE(pow_op->op == '^'); // התיקון: RHS הוא חזקה

    // 7. Check LHS of '^': Must be '2'
    REQUIRE(check_const(pow_op->lhs, 2));

    // 8. Check RHS of '^': Must be the parenthesis expression (b + 0)
    auto inner_add = std::dynamic_pointer_cast<ExprBinary>(pow_op->rhs);
    REQUIRE(inner_add != nullptr);
    REQUIRE(inner_add->op == '+'); // התיקון: ה-RHS הוא חיבור

    // 9. Check LHS of inner '+': Must be 'b'
    REQUIRE(check_ident(inner_add->lhs, "b"));

    // 10. Check RHS of inner '+': Must be '0'
    REQUIRE(check_const(inner_add->rhs, 0));
}

TEST_CASE("Expression Parsing - Left Associativity Check (Precedence Climbing)") {
    // Expression: A - B + C
    // Expected AST structure (Left Associativity for same precedence + and -):
    //         (+)
    //        /   \
    //      (-)    (C)
    //      / \
    //    (A) (B)
    
    std::string code = "A - B + C";
    // Tokens: A, -, B, +, C
    std::vector<Token> tokens = {
        {TokenKind::IDENTIFIER, "A"}, {TokenKind::SYMBOL, "-"}, 
        {TokenKind::IDENTIFIER, "B"}, {TokenKind::SYMBOL, "+"}, 
        {TokenKind::IDENTIFIER, "C"}
    };
    Parser parser(tokens);
    
    std::optional<ExprPtr> result = parser._parse_expression();
    REQUIRE(result.has_value());
    ExprPtr expr = result.value();
    to_string(*expr);

    // 1. Root: The second operator ('+') due to left-associativity handling in _parse_binary
    auto root_add = std::dynamic_pointer_cast<ExprBinary>(expr);
    REQUIRE(root_add != nullptr);
    REQUIRE(root_add->op == '+');
    REQUIRE(check_ident(root_add->rhs, "C")); // RHS is C

    // 2. LHS of '+': Must be the subtraction expression (A - B)
    auto inner_sub = std::dynamic_pointer_cast<ExprBinary>(root_add->lhs);
    REQUIRE(inner_sub != nullptr);
    REQUIRE(inner_sub->op == '-');
    
    // 3. Check subtraction operands
    REQUIRE(check_ident(inner_sub->lhs, "A"));
    REQUIRE(check_ident(inner_sub->rhs, "B"));
}

TEST_CASE("Expression Parsing - Unary Operator") {
    // Expression: ~in_signal
    
    std::string code = "~in_signal";
    Lexer lexer(code);
    Parser parser(lexer.Tokenize());
    
    std::optional<ExprPtr> result = parser._parse_expression();
    REQUIRE(result.has_value());
    ExprPtr expr = result.value();

    // 1. Root must be ExprUnary with '~'
    auto root_unary = std::dynamic_pointer_cast<ExprUnary>(expr);
    REQUIRE(root_unary != nullptr);
    REQUIRE(root_unary->op == '~');

    // 2. RHS must be the identifier 'in_signal'
    REQUIRE(check_ident(root_unary->rhs, "in_signal"));
}
