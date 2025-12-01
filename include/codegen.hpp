#pragma once
#include "ir.hpp"
#include <string>
#include <string_view>
#include <unordered_map>

class CodeGenerator
{
public:
    CodeGenerator(const InterCodeArray &arr,
                  const std::unordered_map<std::string, std::string> &identifiers,
                  const std::unordered_map<std::string, std::string> &constants,
                  const std::unordered_map<std::string, std::string> &tempmap);
    void writeAsm(std::string_view path);
    int assembleAndRun(std::string_view asmPath, std::string_view objPath, std::string_view exePath);

private:
    void pr(std::string_view s);
    void gen_variables();
    void gen_start();
    void gen_end();
    void gen_code();
    void gen_assignment(const AssignmentCode &a);
    void gen_jump(const JumpCode &j);
    void gen_label(const LabelCode &l);
    void gen_compare(const CompareCodeIR &c);
    void gen_print(const PrintCodeIR &p);
    void gen_support_functions();
    static std::string handleVar(std::string_view a, const std::unordered_map<std::string, std::string> &tempmap);

private:
    const InterCodeArray &arr;
    std::unordered_map<std::string, std::string> ids;
    std::unordered_map<std::string, std::string> consts;
    std::unordered_map<std::string, std::string> tempmap;
    std::string out;
};
