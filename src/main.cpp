#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "ast.hpp"
#include "ir.hpp"
#include "codegen.hpp"

// Flex/Bison
typedef struct yy_buffer_state *YY_BUFFER_STATE;
YY_BUFFER_STATE yy_scan_string(const char* str);
void yy_delete_buffer(YY_BUFFER_STATE b);
extern int yyparse();
extern std::shared_ptr<Node> g_ast_root;

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




int main()
{
    while (true)
    {
        std::ifstream fin("../read.txt");
        if (!fin) { std::cerr << "Cannot open read.txt\n"; return 1; }

        std::stringstream buffer;
        buffer << fin.rdbuf();
        std::string input = buffer.str();

        try
        {
            YY_BUFFER_STATE buf = yy_scan_string(input.c_str());
            int parse_result = yyparse();
            yy_delete_buffer(buf);

            if (parse_result == 0)
            {
                std::cout << "Parsing successful!\n\n===== AST =====\n";
                print_ast(g_ast_root);

                std::cout << "\n===== IR =====\n";
                IntermediateCodeGen irgen(g_ast_root);
                auto gen = irgen.get();

                for (auto &instr : gen.code.code)
                {
                    switch (instr->kind())
                    {
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
                std::cout << "\n===== NASM =====\n";

                CodeGenerator codegen(
                        gen.code,
                        gen.identifiers,
                        gen.constants
                );

                const std::string asm_path = "out.asm";
                codegen.writeAsm(asm_path);
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
