#include <fstream>
#include <iostream>
#include <ranges>
#include <sstream>
#include <vector>
#include <filesystem>

#include "ast.hpp"
#include "ir.hpp"
#include "codegen.hpp"

// Flex/Bison
struct yy_buffer_state;
using YYBufferState = yy_buffer_state *;

YYBufferState yy_scan_string(const char *str);

void yy_delete_buffer(YYBufferState b);

int yyparse();

extern std::shared_ptr<pseu::ast::ASTNode> g_ast_root;

namespace detail {

    class FlexBuffer {
    public:
        explicit FlexBuffer(const std::string &input)
                : buf_(yy_scan_string(input.c_str())) {}

        FlexBuffer(const FlexBuffer &) = delete;

        FlexBuffer &operator=(const FlexBuffer &) = delete;

        FlexBuffer(FlexBuffer &&other) noexcept: buf_(other.buf_) {
            other.buf_ = nullptr;
        }

        ~FlexBuffer() {
            if (buf_)
                yy_delete_buffer(buf_);
        }

    private:
        YYBufferState buf_;
    };

    struct AstStackItem {
        std::shared_ptr<pseu::ast::ASTNode> node;
        std::string prefix;
        bool isLast;
    };

    template<typename T>
    static void print_ast_node(const T &, std::string_view, bool) {
        std::cout << "Unknown ASTNode\n";
    }

    static void print_ast_node(const pseu::ast::NumberNode &n, std::string_view, bool) {
        std::cout << "Number: " << n.tok.value << "\n";
    }

    static void print_ast_node(const pseu::ast::IdentifierNode &n, std::string_view, bool) {
        std::cout << "Identifier: " << n.tok.value << "\n";
    }

    static void print_ast_node(const pseu::ast::StringLiteralNode &n, std::string_view, bool) {
        std::cout << "StringLiteral: \"" << n.tok.value << "\"\n";
    }

    static void print_ast_node(const pseu::ast::BinOpNode &n, std::string_view, bool) {
        std::cout << "BinOp (" << n.op_tok.value << ")\n";
    }

    static void print_ast_node(const pseu::ast::Condition &n, std::string_view, bool) {
        std::cout << "Condition (" << n.comparison.value << ")\n";
    }

    static void print_ast_node(const pseu::ast::IfStatement &, std::string_view, bool) {
        std::cout << "If\n";
    }

    static void print_ast_node(const pseu::ast::WhileStatement &, std::string_view, bool) {
        std::cout << "While\n";
    }

    static void print_ast_node(const pseu::ast::PrintStatement &p, std::string_view, bool) {
        std::cout << "Print(" << p.type << ")\n";
    }

    static void print_ast_node(const pseu::ast::Declaration &d, std::string_view, bool) {
        std::cout << "Declaration (" << d.declaration_type.value << ")\n";
    }

    static void print_ast_node(const pseu::ast::Assignment &, std::string_view, bool) {
        std::cout << "Assignment\n";
    }

    static void print_ast_node(const pseu::ast::Statement &, std::string_view, bool) {
        std::cout << "Statement\n";
    }

    template<typename T>
    static void collect_children(const T &, std::vector<std::shared_ptr<pseu::ast::ASTNode>> &) {
    }

    static void collect_children(const pseu::ast::BinOpNode &n,
                                 std::vector<std::shared_ptr<pseu::ast::ASTNode>> &out) {
        out.emplace_back(n.left);
        out.emplace_back(n.right);
    }

    static void collect_children(const pseu::ast::Condition &n,
                                 std::vector<std::shared_ptr<pseu::ast::ASTNode>> &out) {
        out.emplace_back(n.left_expression);
        out.emplace_back(n.right_expression);
    }

    static void collect_children(const pseu::ast::IfStatement &n,
                                 std::vector<std::shared_ptr<pseu::ast::ASTNode>> &out) {
        out.emplace_back(n.if_condition);
        out.emplace_back(n.if_body);
        if (n.else_body)
            out.emplace_back(n.else_body);
    }

    static void collect_children(const pseu::ast::WhileStatement &n,
                                 std::vector<std::shared_ptr<pseu::ast::ASTNode>> &out) {
        out.emplace_back(n.condition);
        out.emplace_back(n.body);
    }

    static void collect_children(const pseu::ast::PrintStatement &n,
                                 std::vector<std::shared_ptr<pseu::ast::ASTNode>> &out) {
        if (n.strValue.empty())
            out.emplace_back(n.intExpr);
    }

    static void collect_children(const pseu::ast::Declaration &n,
                                 std::vector<std::shared_ptr<pseu::ast::ASTNode>> &out) {
        if (n.init_expr)
            out.emplace_back(n.init_expr);
    }

    static void collect_children(const pseu::ast::Assignment &n,
                                 std::vector<std::shared_ptr<pseu::ast::ASTNode>> &out) {
        out.emplace_back(n.expression);
    }

    static void collect_children(const pseu::ast::Statement &n,
                                 std::vector<std::shared_ptr<pseu::ast::ASTNode>> &out) {
        if (n.left) out.emplace_back(n.left);
        if (n.right) out.emplace_back(n.right);
    }

    static void print_ast(const std::shared_ptr<pseu::ast::ASTNode> &root, std::string_view prefix = "") {
        if (!root)
            return;

        std::vector<AstStackItem> stack;
        stack.emplace_back(root, std::string(prefix), 1);

        while (!stack.empty()) {
            auto cur = stack.back();
            stack.pop_back();

            std::cout << cur.prefix
                      << (cur.isLast ? "└── " : "├── ");

            std::visit(
                    [&](const auto &n) {
                        print_ast_node(n, cur.prefix, cur.isLast);
                    },
                    *cur.node
            );

            std::string child_prefix =
                    cur.prefix + (cur.isLast ? "    " : "│   ");

            std::vector<std::shared_ptr<pseu::ast::ASTNode>> children;
            std::visit(
                    [&](const auto &n) {
                        collect_children(n, children);
                    },
                    *cur.node
            );

            bool first = true;

            for (auto &child: children | std::views::reverse) {
                stack.emplace_back(
                        child,
                        child_prefix,
                        first
                );
                first = false;
            }
        }
    }

    namespace fs = std::filesystem;

    struct Config {
        std::string src_path = "read.txt";
        std::string target_path = "out.asm";
        bool print_ast = false;
        bool print_ir = false;
    };

    Config parse_args(int argc, char **argv) {
        Config cfg;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "-src") {
                if (i + 1 >= argc)
                    throw std::runtime_error("Missing value for -src");
                cfg.src_path = argv[++i];
            } else if (arg == "-target") {
                if (i + 1 >= argc)
                    throw std::runtime_error("Missing value for -target");
                cfg.target_path = argv[++i];
            } else if (arg == "--ast") {
                cfg.print_ast = true;
            } else if (arg == "--ir") {
                cfg.print_ir = true;
            } else {
                throw std::runtime_error("Unknown argument: " + arg);
            }
        }

        cfg.src_path = fs::absolute(fs::path(cfg.src_path)).lexically_normal().string();
        cfg.target_path = fs::absolute(fs::path(cfg.target_path)).lexically_normal().string();

        return cfg;
    }

} // namespace detail

int main(int argc, char **argv) {

    detail::Config cfg;
    try {
        cfg = detail::parse_args(argc, argv);
    } catch (const std::exception &e) {
        std::cerr << "Argument error: " << e.what() << "\n";
        return 1;
    }
    while (true) {
        std::ifstream fin(cfg.src_path);
        if (!fin) {
            std::cerr << "Cannot open " << cfg.src_path << "\n";
            return 1;
        }

        std::stringstream buffer;
        buffer << fin.rdbuf();
        std::string input = buffer.str();

        try {
            detail::FlexBuffer f_buffer(input);
            int parse_result = yyparse();

            if (parse_result == 0) {
                if (cfg.print_ast) {
                    std::cout << "===== AST =====\n";
                    detail::print_ast(g_ast_root);
                }

                pseu::ir::IntermediateCodeGen irgen(g_ast_root);
                auto gen = irgen.get();

                if (cfg.print_ir) {
                    std::cout << "\n===== IR =====\n";

                    for (auto instr: gen.code.code) {
                        [[maybe_unused]] auto g = instr.guard();
                        std::visit([&](auto &ir) {
                            using T = std::decay_t<decltype(ir)>;

                            if constexpr (std::is_same_v<T, pseu::ir::AssignmentCode>) {
                                std::cout << ir.var << " = " << ir.left;
                                if (!ir.op.empty())
                                    std::cout << " " << ir.op << " " << ir.right;
                                std::cout << "\n";
                            } else if constexpr (std::is_same_v<T, pseu::ir::JumpCode>) {
                                std::cout << "jump " << ir.dist << "\n";
                            } else if constexpr (std::is_same_v<T, pseu::ir::LabelCode>) {
                                std::cout << ir.label << ":\n";
                            } else if constexpr (std::is_same_v<T, pseu::ir::CompareCodeIR>) {
                                std::cout << "if " << ir.left << " "
                                          << ir.operation << " "
                                          << ir.right << " goto "
                                          << ir.jump << "\n";
                            } else if constexpr (std::is_same_v<T, pseu::ir::PrintCodeIR>) {
                                std::cout << "print(" << ir.type << ", "
                                          << ir.value << ")\n";
                            }
                        }, *instr);
                    }
                }

                pseu::codegen::CodeGenerator codegen(
                        gen.code,
                        gen.identifiers,
                        gen.constants
                );

                codegen.writeAsm(cfg.target_path);
            } else {
                std::cerr << "Parsing failed.\n";
            }
        }
        catch (const std::exception &e) {
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
