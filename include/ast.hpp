#pragma once
#include <memory>
#include <string>
#include <vector>
#include <variant>
#include "tokens.hpp"


/* ======================
 * Literal Nodes
 * ====================== */
struct NumberNode;
struct StringLiteralNode;
struct IdentifierNode;
struct BinOpNode;
struct Statement;
struct Condition;
struct IfStatement;
struct WhileStatement;
struct PrintStatement;
struct Assignment;
struct Declaration;

using ASTNode = std::variant<
        NumberNode,
        StringLiteralNode,
        IdentifierNode,
        BinOpNode,
        Statement,
        Condition,
        IfStatement,
        WhileStatement,
        PrintStatement,
        Assignment,
        Declaration
>;

struct NumberNode final
{
    Token tok;
    explicit NumberNode(Token t) : tok(std::move(t)) {}
    [[nodiscard]] std::string getValue() const { return tok.value; }
};

struct IdentifierNode final
{
    Token tok;
    explicit IdentifierNode(Token t) : tok(std::move(t)) {}
    [[nodiscard]] std::string getValue() const { return tok.value; }
};


struct StringLiteralNode final
{
    Token tok;
    explicit StringLiteralNode(Token t) : tok(std::move(t)) {}
    [[nodiscard]] std::string getValue() const { return tok.value; }
};


/* ======================
 * Binary Operation
 * ====================== */

struct BinOpNode final
{
    std::shared_ptr<ASTNode> left;
    Token op_tok;
    std::shared_ptr<ASTNode> right;
};


/* ======================
 * Statements List (linked tree)
 * ====================== */

struct Statement final
{
    std::shared_ptr<ASTNode> left;
    std::shared_ptr<ASTNode> right; // may be null
};


/* ======================
 * Condition
 * must match parser:
 * std::make_shared<Condition>(expr, token, expr)
 * ====================== */

struct Condition final
{
    std::shared_ptr<ASTNode> left_expression;
    Token comparison;
    std::shared_ptr<ASTNode> right_expression;

    Condition(std::shared_ptr<ASTNode> left,
              Token op,
              std::shared_ptr<ASTNode> right)
            : left_expression(std::move(left)),
              comparison(std::move(op)),
              right_expression(std::move(right)) {}
};


/* ======================
 * IfStatement
 * parser.yy uses:
 *   if_body = Node*
 *   else_body = Node* or null
 * ====================== */

struct IfStatement final
{
    std::shared_ptr<ASTNode> if_condition;
    std::shared_ptr<ASTNode> if_body;
    std::shared_ptr<ASTNode> else_body;   // may be null
};


/* ======================
 * While
 * ====================== */

struct WhileStatement final
{
    std::shared_ptr<ASTNode> condition;
    std::shared_ptr<ASTNode> body;
};


/* ======================
 * Print
 * parser.yy uses:
 *  - type = "int" => intExpr
 *  - type = "string" => strValue
 * ====================== */

struct PrintStatement final
{
    std::string type;
    std::shared_ptr<ASTNode> intExpr;    // if type == "int"
    std::string strValue;                // if type == "string"
};


/* ======================
 * Assignment
 * ====================== */

struct Assignment final
{
    Token identifier;                    // e.g., T_VAR
    std::shared_ptr<ASTNode> expression; // e.g., BinOpNode / NumberNode
};


/* ======================
 * Declaration
 * parser.yy supports two forms:
 *   int a, b, c;
 *   int x = expr;
 *
 * So we need BOTH:
 *   - identifiers list
 *   - init_expr (optional)
 * ====================== */

struct Declaration final
{
    Token declaration_type;              // "int" or "string"
    std::vector<Token> identifiers;      // list of var IDs
    std::shared_ptr<ASTNode> init_expr;  // only for int x = expr;
    Declaration() : init_expr(nullptr) {}
};
