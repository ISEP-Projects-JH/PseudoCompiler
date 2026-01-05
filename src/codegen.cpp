#include "codegen.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <jh/meta>


namespace pseu {
    using namespace std::literals;
    using namespace jh::pod::literals;

    namespace detail {
        constexpr auto op_table =
                jh::meta::make_lookup_map(
                        std::array{
                                std::pair{"*"_psv, "imul"_psv},
                                std::pair{"+"_psv, "add"_psv},
                                std::pair{"-"_psv, "sub"_psv},
                                std::pair{"/"_psv, "idiv"_psv},
                        },
                        ""_psv   // default value
                );
    }

    static jh::pod::string_view op_to_asm(std::string_view op) {
        return detail::op_table[op];
    }

    namespace detail {
        constexpr auto cmp_table =
                jh::meta::make_lookup_map(
                        std::array{
                                std::pair{"=="_psv, "je"_psv},
                                std::pair{"!="_psv, "jne"_psv},
                                std::pair{"<"_psv, "jl"_psv},
                                std::pair{"<="_psv, "jle"_psv},
                                std::pair{">"_psv, "jg"_psv},
                                std::pair{">="_psv, "jge"_psv},
                        },
                        ""_psv
                );
    }

    static jh::pod::string_view cmp_to_jmp(std::string_view c) {
        return detail::cmp_table[c];
    }

    codegen::CodeGenerator::CodeGenerator(const ir::InterCodeArray &arr,
                                          const std::unordered_map<std::string, std::string> &identifiers,
                                          const std::unordered_map<std::string, std::string> &constants,
                                          const std::unordered_map<std::string, std::string> &tempmap)
            : arr(arr), ids(identifiers), consts(constants), tempmap(tempmap), need_print_num(false),
              need_print_string(false) {}

    void codegen::CodeGenerator::pr(std::string_view s) {
        out.insert(out.end(), s.begin(), s.end());
        out.emplace_back('\n');
    }

    void codegen::CodeGenerator::emit_sv(jh::pod::string_view sv) {
        out.insert(out.end(), sv.begin(), sv.end());
    }

    std::string codegen::CodeGenerator::handleVar(
            const std::string &a,
            [[maybe_unused]] const std::unordered_map<std::string, std::string> &tempmap) {
        // immediate number
        if (std::isdigit(a[0]) || (a[0] == '-' && std::isdigit(a[1])))
            return a;

        // x86 / x86-64 memory operand
        return "[" + a + "]";
    }

    void codegen::CodeGenerator::gen_variables() {
        pr("section .bss");

        if (need_print_num) {
            const auto S = R"(	digitSpace resb 100
	digitSpacePos resb 8

)"s;
            pr(S);
        }

        for (auto &kv: ids) {
            pr("\t" + kv.first + " resb 8");
        }
    }

    static inline void append_u8(std::vector<char> &buf, unsigned char v) {
        if (v >= 100) {
            buf.emplace_back(static_cast<char>(48 + v / 100));
            v %= 100;
            buf.emplace_back(static_cast<char>(48 + v / 10));
            buf.emplace_back(static_cast<char>(48 + v % 10));
        } else if (v >= 10) {
            buf.emplace_back(static_cast<char>(48 + v / 10));
            buf.emplace_back(static_cast<char>(48 + v % 10));
        } else {
            buf.emplace_back(static_cast<char>(48 + v));
        }
    }

    static constexpr char DB_PREFIX[] = "\tdb ";
    static constexpr char COMMA_SPACE[] = ", ";
    static constexpr char NL_NUL[] = "10, 0";


    void codegen::CodeGenerator::gen_start() {
        pr("section .data");

        std::vector<char> buf;
        buf.reserve(128);

        for (auto &kv: consts) {
            buf.clear();

            // "\t<label> db "
            buf.emplace_back('\t');
            buf.insert(buf.end(), kv.first.begin(), kv.first.end());
            buf.emplace_back(' ');
            buf.insert(buf.end(), std::begin(DB_PREFIX) + 1,
                       std::end(DB_PREFIX) - 1); // skip '\t' duplication

            // string bytes
            for (unsigned char c: kv.second) {
                append_u8(buf, c);
                buf.insert(buf.end(),
                           std::begin(COMMA_SPACE),
                           std::end(COMMA_SPACE) - 1);
            }

            // newline + NUL
            buf.insert(buf.end(),
                       std::begin(NL_NUL),
                       std::end(NL_NUL) - 1);

            pr(std::string_view(buf.data(), buf.size()));
        }

        const auto S = R"(section .text
	global _start

_start:
)"s;

        pr(S);
    }

    void codegen::CodeGenerator::gen_end() {
        const auto S = R"(	mov rax, 60      ; __NR_exit
	mov rdi, 0       ; status
	syscall

)"s;

        pr(S);
    }

    void codegen::CodeGenerator::gen_assignment(const ir::AssignmentCode &a) {
        // x = y
        if (a.op.empty()) {
            if (consts.count(a.left)) {
                // string literal: take address
                pr("\tlea rax, [rel " + a.left + "]");
            } else {
                pr("\tmov rax, " + handleVar(a.left, tempmap));
            }
            pr("\tmov " + handleVar(a.var, tempmap) + ", rax");

            return;
        }

        // x = l op r
        pr("\tmov rax, " + handleVar(a.left, tempmap));

        if (a.op == "/"sv) {
            pr("\tcqo");
            pr("\tmov rbx, " + handleVar(a.right, tempmap));
            pr("\tidiv rbx");
        } else {
            pr("\tmov rbx, " + handleVar(a.right, tempmap));
            emit_sv("\t"_psv);
            emit_sv(op_to_asm(a.op));
            pr(" rax, rbx\n");
        }

        pr("\tmov " + handleVar(a.var, tempmap) + ", rax");
    }

    void codegen::CodeGenerator::gen_jump(const ir::JumpCode &j) {
        pr("\tjmp " + j.dist);
    }

    void codegen::CodeGenerator::gen_label(const ir::LabelCode &l) {
        pr(l.label + ":");
    }

    void codegen::CodeGenerator::gen_compare(const ir::CompareCodeIR &c) {
        pr("\tmov rax, " + handleVar(c.left, tempmap));
        pr("\tcmp rax, " + handleVar(c.right, tempmap));
        emit_sv("\t"_psv);
        emit_sv(cmp_to_jmp(c.operation));
        pr(" " + c.jump);
    }

    void codegen::CodeGenerator::gen_print(const ir::PrintCodeIR &p) {
        if (p.type == ast::PrintType::Int) {
            pr("\tmov rdi, " + handleVar(p.value, tempmap));
            pr("\tcall print_num");
        } else {
            if (consts.count(p.value)) {
                // string literal: pass address directly
                pr("\tmov rdi, " + p.value);
            } else {
                // string variable: load pointer
                pr("\tmov rdi, [" + p.value + "]");
            }
            pr("\tcall print_string");
        }
    }

    void codegen::CodeGenerator::gen_code() {
        for (auto ins: arr.code) {
            [[maybe_unused]] auto g = ins.guard();
            std::visit([&](auto &ir) {
                using T = std::decay_t<decltype(ir)>;

                if constexpr (std::is_same_v<T, ir::AssignmentCode>) {
                    gen_assignment(ir);
                } else if constexpr (std::is_same_v<T, ir::JumpCode>) {
                    gen_jump(ir);
                } else if constexpr (std::is_same_v<T, ir::LabelCode>) {
                    gen_label(ir);
                } else if constexpr (std::is_same_v<T, ir::CompareCodeIR>) {
                    gen_compare(ir);
                } else if constexpr (std::is_same_v<T, ir::PrintCodeIR>) {
                    gen_print(ir);
                }
            }, *ins);
        }
    }


    void codegen::CodeGenerator::writeAsm(const std::string &path) {
        out.clear();

        // MUST reset every time
        need_print_num = false;
        need_print_string = false;

        // Pre-scan IR to determine which helpers are needed
        for (auto ins: arr.code) {
            [[maybe_unused]] auto g = ins.guard();
            std::visit([&](auto &ir) {
                using T = std::decay_t<decltype(ir)>;

                if constexpr (std::is_same_v<T, ir::PrintCodeIR>) {
                    if (ir.type == ast::PrintType::Str)
                        need_print_string = true;
                    else
                        need_print_num = true;
                }
            }, *ins);
        }

        gen_variables();
        gen_start();
        gen_code();
        gen_end();

        if (need_print_num) // NOLINT
            gen_print_num_function();
        if (need_print_string) // NOLINT
            gen_print_string_function();

        std::ofstream f(path, std::ios::binary);
        f.write(out.data(), static_cast<std::int64_t>(out.size()));
        f.close();
    }

    void codegen::CodeGenerator::gen_print_num_function() {
        const auto S = R"(
print_num:
    mov rcx, digitSpace       ; buffer start
    mov rbx, 10
    mov rax, rdi              ; number

    ; handle negative
    cmp rax, 0
    jge .loop
    neg rax
    mov byte [rcx], '-'
    inc rcx

.loop:
    xor rdx, rdx
    div rbx                   ; rax = rax/10, rdx = rax%10
    add dl, '0'
    mov [rcx], dl
    inc rcx
    test rax, rax
    jnz .loop

    ; rcx now points one past last char
    ; reverse-print
    mov rsi, rcx              ; rsi = end
    dec rsi                   ; last digit
    mov rdx, 1                ; write 1 byte at a time

.rev_loop:
    mov rax, 1                ; write
    mov rdi, 1                ; stdout
    syscall
    dec rsi
    cmp rsi, digitSpace
    jl .newline
    jmp .rev_loop

.newline:
    mov byte [digitSpacePos], 10
    mov rax, 1
    mov rdi, 1
    mov rsi, digitSpacePos
    mov rdx, 1
    syscall
    ret)"s;

        pr(S);
    }

    void codegen::CodeGenerator::gen_print_string_function() {
        const auto S = R"(
print_string:
    ; rdi = char*
    mov rsi, rdi
    xor rdx, rdx

.len_loop:
    cmp byte [rsi + rdx], 0
    je .write
    inc rdx
    jmp .len_loop

.write:
    mov rax, 1      ; sys_write
    mov rdi, 1      ; stdout
    syscall
    ret)"s;

        pr(S);
    }

} // namespace pseu
