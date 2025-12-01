#pragma once
#include <memory>
#include <string>
#include <vector>
#include "tokens.hpp"

struct Node
{
    virtual ~Node() = default;
};

/* ======================
 * Literal Nodes
 * ====================== */

struct NumberNode : Node
{
    Token tok;
    explicit NumberNode(Token t) : tok(std::move(t)) {}
    [[nodiscard]] std::string getValue() const { return tok.value; }
};

struct IdentifierNode : Node
{
    Token tok;
    explicit IdentifierNode(Token t) : tok(std::move(t)) {}
    [[nodiscard]] std::string getValue() const { return tok.value; }
};


struct StringLiteralNode : Node
{
    Token tok;
    explicit StringLiteralNode(Token t) : tok(std::move(t)) {}
    [[nodiscard]] std::string getValue() const { return tok.value; }
};


/* ======================
 * Binary Operation
 * ====================== */

struct BinOpNode : Node
{
    std::shared_ptr<Node> left;
    Token op_tok;
    std::shared_ptr<Node> right;
};


/* ======================
 * Statements List (linked tree)
 * ====================== */

struct Statement : Node
{
    std::shared_ptr<Node> left;
    std::shared_ptr<Node> right; // may be null
};


/* ======================
 * Condition
 * must match parser:
 * std::make_shared<Condition>(expr, token, expr)
 * ====================== */

struct Condition : Node
{
    std::shared_ptr<Node> left_expression;
    Token comparison;
    std::shared_ptr<Node> right_expression;

    Condition(std::shared_ptr<Node> left,
              Token op,
              std::shared_ptr<Node> right)
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

struct IfStatement : Node
{
    std::shared_ptr<Condition> if_condition;
    std::shared_ptr<Node> if_body;
    std::shared_ptr<Node> else_body; // may be null
};


/* ======================
 * While
 * ====================== */

struct WhileStatement : Node
{
    std::shared_ptr<Condition> condition;
    std::shared_ptr<Node> body;
};


/* ======================
 * Print
 * parser.yy uses:
 *  - type = "int" => intExpr
 *  - type = "string" => strValue
 * ====================== */

struct PrintStatement : Node
{
    std::string type;
    std::shared_ptr<Node> intExpr;    // if type == "int"
    std::string strValue;             // if type == "string"
};


/* ======================
 * Assignment
 * ====================== */

struct Assignment : Node
{
    Token identifier;                 // e.g., T_VAR
    std::shared_ptr<Node> expression; // e.g., BinOpNode / NumberNode
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

struct Declaration : Node
{
    Token declaration_type;           // "int" or "string"
    std::vector<Token> identifiers;   // list of var IDs
    std::shared_ptr<Node> init_expr;  // only for int x = expr;

    Declaration() : init_expr(nullptr) {}
};
