/**
 * @file ast.hpp
 * @author JeongHan-Bae &lt;mastropseudo&#64;gmail.com&gt;
 * @brief Abstract Syntax Tree (AST) node definitions for PseudoCompiler.
 *
 * @license MIT
 *
 * @verbatim
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * @endverbatim
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <variant>
#include "tokens.hpp"

namespace pseu::ast {

    // Forward declarations of all AST node types.
    // These are used to construct the ASTNode variant.

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

    /**
     * @brief Unified AST node type.
     *
     * ASTNode is a closed sum type representing every possible
     * syntactic construct in the language. Dispatch is performed
     * exclusively via std::visit, ensuring static, exhaustive handling.
     */
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

    /**
     * @brief Integer literal node.
     *
     * Represents a numeric literal token.
     */
    struct NumberNode final {
        lexer::Token tok;

        explicit NumberNode(lexer::Token t) : tok(std::move(t)) {}

        /// @brief Retrieve the literal value as string.
        [[nodiscard]] std::string getValue() const { return tok.value; }
    };

    /**
     * @brief Identifier node.
     *
     * Represents a variable or symbol reference.
     */
    struct IdentifierNode final {
        lexer::Token tok;

        explicit IdentifierNode(lexer::Token t) : tok(std::move(t)) {}

        /// @brief Retrieve the identifier name.
        [[nodiscard]] std::string getValue() const { return tok.value; }
    };

    /**
     * @brief String literal node.
     *
     * Represents a quoted string literal.
     */
    struct StringLiteralNode final {
        lexer::Token tok;

        explicit StringLiteralNode(lexer::Token t) : tok(std::move(t)) {}

        /// @brief Retrieve the string literal value.
        [[nodiscard]] std::string getValue() const { return tok.value; }
    };

    /**
     * @brief Binary operation node.
     *
     * Represents expressions such as addition, subtraction,
     * multiplication, or comparison.
     */
    struct BinOpNode final {
        std::shared_ptr<ASTNode> left;
        lexer::Token op_tok;
        std::shared_ptr<ASTNode> right;
    };

    /**
     * @brief Statement sequence node.
     *
     * Represents a linked list of statements, constructed
     * by the parser as a binary tree.
     */
    struct Statement final {
        std::shared_ptr<ASTNode> left;
        std::shared_ptr<ASTNode> right; // may be null
    };

    /**
     * @brief Conditional expression node.
     *
     * Represents a comparison used in control-flow constructs.
     */
    struct Condition final {
        std::shared_ptr<ASTNode> left_expression;
        lexer::Token comparison;
        std::shared_ptr<ASTNode> right_expression;

        Condition(std::shared_ptr<ASTNode> left,
                  lexer::Token op,
                  std::shared_ptr<ASTNode> right)
                : left_expression(std::move(left)),
                  comparison(std::move(op)),
                  right_expression(std::move(right)) {}
    };

    /**
     * @brief If-statement node.
     *
     * Represents conditional branching with optional else clause.
     */
    struct IfStatement final {
        std::shared_ptr<ASTNode> if_condition;
        std::shared_ptr<ASTNode> if_body;
        std::shared_ptr<ASTNode> else_body;   // may be null
    };

    /**
     * @brief While-loop node.
     *
     * Represents a loop with a condition and body.
     */
    struct WhileStatement final {
        std::shared_ptr<ASTNode> condition;
        std::shared_ptr<ASTNode> body;
    };

    /**
     * @brief Print statement node.
     *
     * Represents output of either integer expressions
     * or string literals.
     */
    struct PrintStatement final {
        std::string type;
        std::shared_ptr<ASTNode> intExpr;    // if type == "int"
        std::string strValue;                // if type == "string"
    };

    /**
     * @brief Assignment statement node.
     *
     * Represents variable assignment.
     */
    struct Assignment final {
        lexer::Token identifier;                    // e.g., T_VAR
        std::shared_ptr<ASTNode> expression;        // e.g., BinOpNode / NumberNode
    };

    /**
     * @brief Variable declaration node.
     *
     * Represents typed variable declarations with
     * optional initialization.
     */
    struct Declaration final {
        lexer::Token declaration_type;              // "int" or "string"
        std::vector<lexer::Token> identifiers;      // list of var IDs
        std::shared_ptr<ASTNode> init_expr;         // only for int x = expr;
        Declaration() : init_expr(nullptr) {}
    };

} // namespace pseu::ast
