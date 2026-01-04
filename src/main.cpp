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

extern std::shared_ptr<ASTNode> g_ast_root;

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


static void print_ast(const std::shared_ptr<ASTNode>& node,
                      std::string_view prefix = "",
                      bool isLast = true)
{
    if (!node)
        return;

    std::cout << prefix << (isLast ? "└── " : "├── ");

    // ---------------- Number ----------------
    if (auto* num = std::get_if<NumberNode>(node.get())) {
        std::cout << "Number: " << num->tok.value << "\n";
        return;
    }

    // ---------------- Identifier ----------------
    if (auto* id = std::get_if<IdentifierNode>(node.get())) {
        std::cout << "Identifier: " << id->tok.value << "\n";
        return;
    }

    // ---------------- String Literal ----------------
    if (auto* str = std::get_if<StringLiteralNode>(node.get())) {
        std::cout << "StringLiteral: \"" << str->tok.value << "\"\n";
        return;
    }

    // ---------------- BinOp ----------------
    if (auto* bin = std::get_if<BinOpNode>(node.get())) {
        std::cout << "BinOp (" << bin->op_tok.value << ")\n";
        auto np = std::string(prefix) + (isLast ? "    " : "│   ");
        print_ast(bin->left,  np, false);
        print_ast(bin->right, np, true);
        return;
    }

    // ---------------- Condition ----------------
    if (auto* cond = std::get_if<Condition>(node.get())) {
        std::cout << "Condition (" << cond->comparison.value << ")\n";
        auto np = std::string(prefix) + (isLast ? "    " : "│   ");
        print_ast(cond->left_expression,  np, false);
        print_ast(cond->right_expression, np, true);
        return;
    }

    // ---------------- IfStatement ----------------
    if (auto* iff = std::get_if<IfStatement>(node.get())) {
        std::cout << "If\n";
        auto base = std::string(prefix) + (isLast ? "    " : "│   ");

        print_ast(iff->if_condition, base, false);

        std::cout << base << "├── Then\n";
        print_ast(iff->if_body, base + "│   ", true);

        std::cout << base << "└── Else\n";
        print_ast(iff->else_body, base + "    ", true);
        return;
    }

    // ---------------- WhileStatement ----------------
    if (auto* wh = std::get_if<WhileStatement>(node.get())) {
        std::cout << "While\n";
        auto base = std::string(prefix) + (isLast ? "    " : "│   ");
        print_ast(wh->condition, base, false);
        print_ast(wh->body,      base, true);
        return;
    }

    // ---------------- Print ----------------
    if (auto* p = std::get_if<PrintStatement>(node.get())) {
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
    if (auto* decl = std::get_if<Declaration>(node.get())) {
        std::cout << "Declaration (" << decl->declaration_type.value << ")\n";
        auto np = std::string(prefix) + (isLast ? "    " : "│   ");

        for (size_t i = 0; i < decl->identifiers.size(); ++i) {
            bool last = (i == decl->identifiers.size() - 1 && !decl->init_expr);
            std::cout << np << (last ? "└── " : "├── ");
            std::cout << "Identifier: " << decl->identifiers[i].value << "\n";
        }

        if (decl->init_expr) {
            std::cout << np << "└── init\n";
            print_ast(decl->init_expr, np + "    ", true);
        }
        return;
    }

    // ---------------- Assignment ----------------
    if (auto* a = std::get_if<Assignment>(node.get())) {
        std::cout << "Assignment\n";
        auto np = std::string(prefix) + (isLast ? "    " : "│   ");
        std::cout << np << "├── Identifier: " << a->identifier.value << "\n";
        print_ast(a->expression, np, true);
        return;
    }

    // ---------------- Statement ----------------
    if (auto* st = std::get_if<Statement>(node.get())) {
        std::cout << "Statement\n";
        auto np = std::string(prefix) + (isLast ? "    " : "│   ");
        if (st->left)
            print_ast(st->left, np, false);
        if (st->right)
            print_ast(st->right, np, true);
        return;
    }

    std::cout << "Unknown ASTNode\n";
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

                    for (auto& instr : gen.code.code) {
                        std::visit([&](auto& ir) {
                            using T = std::decay_t<decltype(ir)>;

                            if constexpr (std::is_same_v<T, AssignmentCode>) {
                                std::cout << ir.var << " = " << ir.left;
                                if (!ir.op.empty())
                                    std::cout << " " << ir.op << " " << ir.right;
                                std::cout << "\n";
                            }
                            else if constexpr (std::is_same_v<T, JumpCode>) {
                                std::cout << "jump " << ir.dist << "\n";
                            }
                            else if constexpr (std::is_same_v<T, LabelCode>) {
                                std::cout << ir.label << ":\n";
                            }
                            else if constexpr (std::is_same_v<T, CompareCodeIR>) {
                                std::cout << "if " << ir.left << " "
                                          << ir.operation << " "
                                          << ir.right << " goto "
                                          << ir.jump << "\n";
                            }
                            else if constexpr (std::is_same_v<T, PrintCodeIR>) {
                                std::cout << "print(" << ir.type << ", "
                                          << ir.value << ")\n";
                            }
                        }, *instr);
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
