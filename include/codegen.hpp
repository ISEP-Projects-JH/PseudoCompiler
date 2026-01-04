#pragma once

#include "ir.hpp"
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>

namespace pseu::codegen {

    class CodeGenerator final {
    public:
        CodeGenerator(const ir::InterCodeArray &arr,
                      const std::unordered_map<std::string, std::string> &identifiers,
                      const std::unordered_map<std::string, std::string> &constants,
                      const std::unordered_map<std::string, std::string> &tempmap = {});

        void writeAsm(const std::string &path);

    private:
        void pr(std::string_view s);

        void gen_variables();

        void gen_start();

        void gen_end();

        void gen_code();

        void gen_assignment(const ir::AssignmentCode &a);

        void gen_jump(const ir::JumpCode &j);

        void gen_label(const ir::LabelCode &l);

        void gen_compare(const ir::CompareCodeIR &c);

        void gen_print(const ir::PrintCodeIR &p);

        void gen_print_num_function();

        void gen_print_string_function();

        static std::string handleVar(const std::string &a, const std::unordered_map<std::string, std::string> &tempmap);

    private:
        const ir::InterCodeArray &arr;
        std::unordered_map<std::string, std::string> ids;
        std::unordered_map<std::string, std::string> consts;
        std::unordered_map<std::string, std::string> tempmap;
        std::vector<char> out;
        bool need_print_num = false;
        bool need_print_string = false;
    };

    
} // namespace pseu::codegen
