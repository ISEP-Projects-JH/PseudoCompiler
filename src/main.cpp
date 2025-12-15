#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <filesystem>

#include "ast.hpp"
#include "ir.hpp"
#include "codegen.hpp"

// Flex/Bison
struct yy_buffer_state;
using YYBufferState = yy_buffer_state*;

YYBufferState yy_scan_string(const char* str);
void yy_delete_buffer(YYBufferState b);
int yyparse();

extern std::shared_ptr<Node> g_ast_root;

class FlexBuffer {
public:
    explicit FlexBuffer(const std::string& input)
            : buf_(yy_scan_string(input.c_str())) {}

    FlexBuffer(const FlexBuffer&) = delete;
    FlexBuffer& operator=(const FlexBuffer&) = delete;

    FlexBuffer(FlexBuffer&& other) noexcept : buf_(other.buf_) {
        other.buf_ = nullptr;
    }

    ~FlexBuffer() {
        if (buf_)
            yy_delete_buffer(buf_);
    }

private:
    YYBufferState buf_;
};


static void print_ast(const std::shared_ptr<Node> &node,
                      std::string_view prefix = "",
                      bool isLast = true)
{
    if (!node)
        return;

    std::cout << prefix << (isLast ? "└── " : "├── ");

    // ---------------- Number ----------------
    if (auto num = std::dynamic_pointer_cast<NumberNode>(node)) {
        std::cout << "Number: " << num->tok.value << "\n";
        return;
    }

    // ---------------- Identifier ----------------
    if (auto id = std::dynamic_pointer_cast<IdentifierNode>(node)) {
        std::cout << "Identifier: " << id->tok.value << "\n";
        return;
    }

    // ---------------- String Literal ----------------
    if (auto str = std::dynamic_pointer_cast<StringLiteralNode>(node)) {
        std::cout << "StringLiteral: \"" << str->tok.value << "\"\n";
        return;
    }

    // ---------------- BinOp ----------------
    if (auto bin = std::dynamic_pointer_cast<BinOpNode>(node)) {
        std::cout << "BinOp (" << bin->op_tok.value << ")\n";
        auto np = std::string(prefix) + (isLast ? "    " : "│   ");
        print_ast(bin->left,  np, false);
        print_ast(bin->right, np, true);
        return;
    }

    // ---------------- Condition ----------------
    if (auto cond = std::dynamic_pointer_cast<Condition>(node)) {
        std::cout << "Condition (" << cond->comparison.value << ")\n";
        auto np = std::string(prefix) + (isLast ? "    " : "│   ");
        print_ast(cond->left_expression,  np, false);
        print_ast(cond->right_expression, np, true);
        return;
    }

    // ---------------- IfStatement ----------------
    if (auto iff = std::dynamic_pointer_cast<IfStatement>(node)) {
        std::cout << "If\n";
        auto base = std::string(prefix) + (isLast ? "    " : "│   ");

        // Condition
        print_ast(iff->if_condition, base, false);

        // THEN
        std::cout << base << "├── Then\n";
        print_ast(iff->if_body, base + "│   ", true);

        // ELSE
        std::cout << base << "└── Else\n";
        print_ast(iff->else_body, base + "    ", true);
        return;
    }

    // ---------------- WhileStatement ----------------
    if (auto wh = std::dynamic_pointer_cast<WhileStatement>(node)) {
        std::cout << "While\n";
        auto base = std::string(prefix) + (isLast ? "    " : "│   ");

        print_ast(wh->condition, base, false);
        print_ast(wh->body,      base, true);
        return;
    }

    // ---------------- Print ----------------
    if (auto p = std::dynamic_pointer_cast<PrintStatement>(node)) {
        std::cout << "Print(" << p->type << ")\n";
        auto np = std::string(prefix) + (isLast ? "    " : "│   ");

        if (!p->strValue.empty()) {
            std::cout << np << "└── \"" << p->strValue << "\"\n";
        } else {
            print_ast(p->intExpr, np, true);
        }
        return;
    }

    // ---------------- Declaration ----------------
    if (auto decl = std::dynamic_pointer_cast<Declaration>(node)) {
        std::cout << "Declaration (" << decl->declaration_type.value << ")\n";
        auto np = std::string(prefix) + (isLast ? "    " : "│   ");

        // identifiers (list of tokens)
        for (size_t i = 0; i < decl->identifiers.size(); ++i) {
            bool last = (i == decl->identifiers.size() - 1 && !decl->init_expr);
            std::cout << np << (last ? "└── " : "├── ");
            std::cout << "Identifier: " << decl->identifiers[i].value << "\n";
        }

        // initializer
        if (decl->init_expr) {
            std::cout << np << "└── init\n";
            print_ast(decl->init_expr, np + "    ", true);
        }
        return;
    }

    // ---------------- Assignment ----------------
    if (auto a = std::dynamic_pointer_cast<Assignment>(node)) {
        std::cout << "Assignment\n";
        auto np = std::string(prefix) + (isLast ? "    " : "│   ");

        std::cout << np << "├── Identifier: " << a->identifier.value << "\n";
        print_ast(a->expression, np, true);
        return;
    }

    // ---------------- Statement (binary pair) ----------------
    if (auto st = std::dynamic_pointer_cast<Statement>(node)) {
        std::cout << "Statement\n";
        auto np = std::string(prefix) + (isLast ? "    " : "│   ");

        if (st->left)
            print_ast(st->left, np, false);
        if (st->right)
            print_ast(st->right, np, true);
        return;
    }

    std::cout << "Unknown Node\n";
}

namespace fs = std::filesystem;

struct Config {
    std::string src_path   = "read.txt";
    std::string target_path = "out.asm";
    bool print_ast = false;
    bool print_ir  = false;
};

Config parse_args(int argc, char** argv)
{
    Config cfg;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-src") {
            if (i + 1 >= argc)
                throw std::runtime_error("Missing value for -src");
            cfg.src_path = argv[++i];
        }
        else if (arg == "-target") {
            if (i + 1 >= argc)
                throw std::runtime_error("Missing value for -target");
            cfg.target_path = argv[++i];
        }
        else if (arg == "--ast") {
            cfg.print_ast = true;
        }
        else if (arg == "--ir") {
            cfg.print_ir = true;
        }
        else {
            throw std::runtime_error("Unknown argument: " + arg);
        }
    }

    cfg.src_path    = fs::absolute(fs::path(cfg.src_path)).lexically_normal().string();
    cfg.target_path = fs::absolute(fs::path(cfg.target_path)).lexically_normal().string();

    return cfg;
}

int main(int argc, char** argv)
{
    Config cfg;
    try {
        cfg = parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Argument error: " << e.what() << "\n";
        return 1;
    }
    while (true)
    {
        std::ifstream fin(cfg.src_path);
        if (!fin) {
            std::cerr << "Cannot open " << cfg.src_path << "\n";
            return 1;
        }

        std::stringstream buffer;
        buffer << fin.rdbuf();
        std::string input = buffer.str();

        try
        {
            FlexBuffer f_buffer(input);
            int parse_result = yyparse();

            if (parse_result == 0)
            {
                if (cfg.print_ast) {
                    std::cout << "===== AST =====\n";
                    print_ast(g_ast_root);
                }

                IntermediateCodeGen irgen(g_ast_root);
                auto gen = irgen.get();

                if (cfg.print_ir) {
                    std::cout << "\n===== IR =====\n";
                    for (auto &instr : gen.code.code) {
                        switch (instr->kind()) {
                            case IRKind::Assignment: {
                                auto *a = dynamic_cast<AssignmentCode*>(instr.get());
                                std::cout << a->var << " = " << a->left;
                                if (!a->op.empty())
                                    std::cout << " " << a->op << " " << a->right;
                                std::cout << "\n";
                                break;
                            }
                            case IRKind::Jump: {
                                auto *j = dynamic_cast<JumpCode*>(instr.get());
                                std::cout << "jump " << j->dist << "\n";
                                break;
                            }
                            case IRKind::Label: {
                                auto *l = dynamic_cast<LabelCode*>(instr.get());
                                std::cout << l->label << ":\n";
                                break;
                            }
                            case IRKind::Compare: {
                                auto *c = dynamic_cast<CompareCodeIR*>(instr.get());
                                std::cout << "if " << c->left << " " << c->operation
                                          << " " << c->right << " goto " << c->jump << "\n";
                                break;
                            }
                            case IRKind::Print: {
                                auto *p = dynamic_cast<PrintCodeIR*>(instr.get());
                                std::cout << "print(" << p->type << ", " << p->value << ")\n";
                                break;
                            }
                        }
                    }
                }

                CodeGenerator codegen(
                        gen.code,
                        gen.identifiers,
                        gen.constants
                );

                codegen.writeAsm(cfg.target_path);
            }
            else
            {
                std::cerr << "Parsing failed.\n";
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << "\n";
        }

        std::cout << "------------------------------\n";
        std::string dummy;
        std::getline(std::cin, dummy);

        auto trim = [](std::string s) {
            s.erase(0, s.find_first_not_of(" \t\r\n"));
            s.erase(s.find_last_not_of(" \t\r\n") + 1);
            return s;
        };

        if (trim(dummy) == "q;")
            break;
    }
}
