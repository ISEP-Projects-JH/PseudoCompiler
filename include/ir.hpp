#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>
#include "ast.hpp"

enum class IRKind
{
    Assignment,
    Jump,
    Label,
    Compare,
    Print
};

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

struct AssignmentCode
{
    std::string var;
    std::string left;
    std::string op;    // empty if none
    std::string right; // may be empty
    [[nodiscard]] static IRKind kind() { return IRKind::Assignment; }
};

struct JumpCode
{
    std::string dist;
    [[nodiscard]] static IRKind kind() { return IRKind::Jump; }
};

struct LabelCode
{
    std::string label;
    [[nodiscard]] static IRKind kind() { return IRKind::Label; }
};

struct CompareCodeIR
{
    std::string left;
    std::string operation;
    std::string right;
    std::string jump;
    [[nodiscard]] static IRKind kind() { return IRKind::Compare; }
};

struct PrintCodeIR
{
    std::string type; // "string" or "int"
    std::string value;
    [[nodiscard]] static IRKind kind() { return IRKind::Print; }
};

struct InterCodeArray
{
    std::vector<std::shared_ptr<IRInstr>> code;
    void append(const std::shared_ptr<IRInstr> &n) { code.push_back(n); }
};

struct GeneratedIR
{
    InterCodeArray code;
    std::unordered_map<std::string, std::string> identifiers;
    std::unordered_map<std::string, std::string> constants;
};

class IntermediateCodeGen
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

private:
    std::shared_ptr<ASTNode> root;
    InterCodeArray arr;
    std::unordered_map<std::string, std::string> identifiers;
    std::unordered_map<std::string, std::string> constants;
    int tCounter{1};
    int lCounter{1};
    int sCounter{1};
};
