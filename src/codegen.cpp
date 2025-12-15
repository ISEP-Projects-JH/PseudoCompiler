#include "codegen.hpp"
#include <fstream>
#include <array>
#include <cstdint>
#include <cctype>

using namespace std::literals;

struct StrMap {
    std::string_view key;
    std::string_view val;
};

static std::string op_to_asm(std::string_view op) {
    static constexpr std::array<StrMap, 4> table{{
                                                         { "*"sv, "imul"sv },
                                                         { "+"sv, "add"sv  },
                                                         { "-"sv, "sub"sv  },
                                                         { "/"sv, "idiv"sv },
                                                 }};

    auto it = std::lower_bound(
            table.begin(), table.end(), op,
            [](const StrMap &e, std::string_view k) {
                return e.key < k;
            });

    if (it != table.end() && it->key == op)
        return std::string(it->val);

    return {};
}


static std::string cmp_to_jmp(std::string_view c) {
    static constexpr std::array<StrMap, 6> table{{
                                                         { "!="sv, "jne"sv },
                                                         { "<"sv,  "jl"sv  },
                                                         { "<="sv, "jle"sv },
                                                         { "=="sv, "je"sv  },
                                                         { ">"sv,  "jg"sv  },
                                                         { ">="sv, "jge"sv },
                                                 }};

    auto it = std::lower_bound(
            table.begin(), table.end(), c,
            [](const StrMap &e, std::string_view k) {
                return e.key < k;
            });

    if (it != table.end() && it->key == c)
        return std::string(it->val);

    return {};
}


CodeGenerator::CodeGenerator(const InterCodeArray &arr,
                             const std::unordered_map<std::string, std::string> &identifiers,
                             const std::unordered_map<std::string, std::string> &constants,
                             const std::unordered_map<std::string, std::string> &tempmap)
        : arr(arr), ids(identifiers), consts(constants), tempmap(tempmap), need_print_num(false),
          need_print_string(false) {}

void CodeGenerator::pr(std::string_view s) {
    out.insert(out.end(), s.begin(), s.end());
    out.push_back('\n');
}

std::string CodeGenerator::handleVar(
        const std::string &a,
        [[maybe_unused]] const std::unordered_map<std::string, std::string> &tempmap) {
    // immediate number
    if (std::isdigit(a[0]) || (a[0] == '-' && std::isdigit(a[1])))
        return a;

    // x86 / x86-64 memory operand
    return "[" + a + "]";
}

void CodeGenerator::gen_variables() {
    pr("section .bss");

    if (need_print_num) {
        const auto S = R"(	digitSpace resb 100
	digitSpacePos resb 8

)"s;
        pr(S);
    }

    for (auto &kv : ids) {
        pr("\t" + kv.first + " resb 8");
    }
}

void CodeGenerator::gen_start() {
    pr("section .data");
    for (auto &kv: consts) {
        pr("\t" + kv.first + " db \"" + kv.second + "\",10,0");
    }
    const auto S = R"(section .text
	global _start

_start:
)"s;

    pr(S);
}

void CodeGenerator::gen_end() {
    const auto S = R"(	mov rax, 60      ; __NR_exit
	mov rdi, 0       ; status
	syscall

)"s;

    pr(S);
}

void CodeGenerator::gen_assignment(const AssignmentCode &a) {
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
        pr("\t" + op_to_asm(a.op) + " rax, rbx");
    }

    pr("\tmov " + handleVar(a.var, tempmap) + ", rax");
}

void CodeGenerator::gen_jump(const JumpCode &j) {
    pr("\tjmp " + j.dist);
}

void CodeGenerator::gen_label(const LabelCode &l) {
    pr(l.label + ":");
}

void CodeGenerator::gen_compare(const CompareCodeIR &c) {
    pr("\tmov rax, " + handleVar(c.left, tempmap));
    pr("\tcmp rax, " + handleVar(c.right, tempmap));
    pr("\t" + cmp_to_jmp(c.operation) + " " + c.jump);
}

void CodeGenerator::gen_print(const PrintCodeIR &p) {
    if (p.type == "int"sv) {
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

void CodeGenerator::gen_code() {
    for (auto &ins: arr.code) {
        switch (ins->kind()) {
            case IRKind::Assignment:
                gen_assignment(*std::static_pointer_cast<AssignmentCode>(ins));
                break;
            case IRKind::Jump:
                gen_jump(*std::static_pointer_cast<JumpCode>(ins));
                break;
            case IRKind::Label:
                gen_label(*std::static_pointer_cast<LabelCode>(ins));
                break;
            case IRKind::Compare:
                gen_compare(*std::static_pointer_cast<CompareCodeIR>(ins));
                break;
            case IRKind::Print:
                gen_print(*std::static_pointer_cast<PrintCodeIR>(ins));
                break;
        }
    }
}

void CodeGenerator::writeAsm(const std::string& path) {
    out.clear();

    // MUST reset every time
    need_print_num = false;
    need_print_string = false;

    // Pre-scan IR to determine which helpers are needed
    for (auto &ins: arr.code) {
        if (ins->kind() == IRKind::Print) {
            auto print_ins = std::static_pointer_cast<PrintCodeIR>(ins);
            if (print_ins->type == "string"sv)
                need_print_string = true;
            else
                need_print_num = true;
        }
    }

    gen_variables();
    gen_start();
    gen_code();
    gen_end();

    if (need_print_num)
        gen_print_num_function();
    if (need_print_string)
        gen_print_string_function();

    std::ofstream f(path, std::ios::binary);
    f.write(out.data(), static_cast<std::int64_t>(out.size()));
    f.close();
}

void CodeGenerator::gen_print_num_function() {
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

void CodeGenerator::gen_print_string_function() {
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
