#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>
#include "ast.hpp"

struct AssignmentCode;
struct JumpCode;
struct LabelCode;
struct CompareCodeIR;
struct PrintCodeIR;

using IRInstr = std::variant<
        AssignmentCode,
        JumpCode,
        LabelCode,
        CompareCodeIR,
        PrintCodeIR
>;

struct AssignmentCode final
{
    std::string var;
    std::string left;
    std::string op;    // empty if none
    std::string right; // may be empty
};

struct JumpCode final
{
    std::string dist;
};

struct LabelCode final
{
    std::string label;
};

struct CompareCodeIR final
{
    std::string left;
    std::string operation;
    std::string right;
    std::string jump;
};

struct PrintCodeIR final
{
    std::string type; // "string" or "int"
    std::string value;
};

struct InterCodeArray final
{
    std::vector<std::shared_ptr<IRInstr>> code;
    void append(const std::shared_ptr<IRInstr> &n) { code.push_back(n); }
};

struct GeneratedIR final
{
    InterCodeArray code;
    std::unordered_map<std::string, std::string> identifiers;
    std::unordered_map<std::string, std::string> constants;
};

class IntermediateCodeGen final
{
public:
    explicit IntermediateCodeGen(const std::shared_ptr<ASTNode> &root);
    GeneratedIR get();

private:
    std::string exec_expr(const std::shared_ptr<ASTNode> &n);
    void exec_assignment(const Assignment *a);
    void exec_if(const IfStatement *i);
    void exec_while(const WhileStatement *w);
    std::string exec_condition(const Condition *c);
    void exec_print(const PrintStatement *p);
    void exec_declaration(const Declaration *d);
    void exec_statement(const std::shared_ptr<ASTNode> &n);
    std::string nextTemp();
    std::string nextLabel();

    [[maybe_unused]] [[nodiscard]] std::string currentLabel() const;
    std::string nextStringSym();

    template <typename T>
    std::string exec_expr_node(const T&);

    [[maybe_unused]] std::string exec_expr_node(const IdentifierNode& id);
    [[maybe_unused]] std::string exec_expr_node(const NumberNode& num);
    [[maybe_unused]] std::string exec_expr_node(const StringLiteralNode& str);
    [[maybe_unused]] std::string exec_expr_node(const BinOpNode& bin);

    template <typename T>
    void exec_statement_node(const T&);

    [[maybe_unused]] void exec_statement_node(const Statement& st);
    [[maybe_unused]] void exec_statement_node(const IfStatement& is);
    [[maybe_unused]] void exec_statement_node(const WhileStatement& wh);
    [[maybe_unused]] void exec_statement_node(const PrintStatement& pr);
    [[maybe_unused]] void exec_statement_node(const Declaration& de);
    [[maybe_unused]] void exec_statement_node(const Assignment& asg);

private:
    std::shared_ptr<ASTNode> root;
    InterCodeArray arr;
    std::unordered_map<std::string, std::string> identifiers;
    std::unordered_map<std::string, std::string> constants;
    int tCounter{1};
    int lCounter{1};
    int sCounter{1};
};
