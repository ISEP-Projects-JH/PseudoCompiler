#include "ir.hpp"
#include <string_view>

namespace pseu {

    inline ir::ir_pool_t pool{};

    static ir::ir_pool_t::ptr
    make_assign(std::string_view v,
                std::string_view l,
                std::string_view op,
                std::string_view r) {
        return pool.acquire(ir::IRInstr{
                ir::AssignmentCode{
                        std::string(v),
                        std::string(l),
                        std::string(op),
                        std::string(r)
                }}
        );
    }

    static ir::ir_pool_t::ptr
    make_jump(std::string_view d) {
        return pool.acquire(ir::IRInstr{
                ir::JumpCode{std::string(d)}}
        );
    }

    static ir::ir_pool_t::ptr
    make_label(std::string_view l) {
        return pool.acquire(ir::IRInstr{
                ir::LabelCode{std::string(l)}}
        );
    }

    static ir::ir_pool_t::ptr
    make_compare(std::string_view l,
                 std::string_view op,
                 std::string_view r,
                 std::string_view j) {
        return pool.acquire(ir::IRInstr{
                ir::CompareCodeIR{
                        std::string(l),
                        std::string(op),
                        std::string(r),
                        std::string(j)
                }}
        );
    }

    static ir::ir_pool_t::ptr
    make_print(std::string_view t,
               std::string_view v) {
        return pool.acquire(ir::IRInstr{
                ir::PrintCodeIR{
                        std::string(t),
                        std::string(v)
                }}
        );
    }

    ir::IntermediateCodeGen::IntermediateCodeGen(const std::shared_ptr<ast::ASTNode> &root) : root(root) {
        exec_statement(root);
    }

    ir::GeneratedIR ir::IntermediateCodeGen::get() { return GeneratedIR{arr, identifiers, constants}; }

    std::string ir::IntermediateCodeGen::nextTemp() { return "T" + std::to_string(tCounter++); }

    std::string ir::IntermediateCodeGen::nextLabel() { return "L" + std::to_string(lCounter++); }

    [[maybe_unused]] std::string ir::IntermediateCodeGen::currentLabel() const { return "L" + std::to_string(lCounter); }

    std::string ir::IntermediateCodeGen::nextStringSym() { return "S" + std::to_string(sCounter++); }

    template<typename T>
    std::string ir::IntermediateCodeGen::exec_expr_node(const T &) {
        return {};
    }

    std::string ir::IntermediateCodeGen::exec_expr_node(const ast::IdentifierNode &id) {
        return id.getValue();
    }

    std::string ir::IntermediateCodeGen::exec_expr_node(const ast::NumberNode &num) {
        return num.getValue();
    }

    std::string ir::IntermediateCodeGen::exec_expr_node(const ast::StringLiteralNode &str) {
        auto sym = nextStringSym();
        constants[sym] = str.getValue();
        return sym;
    }

    std::string ir::IntermediateCodeGen::exec_expr_node(const ast::BinOpNode &bin) {
        auto left = exec_expr(bin.left);
        auto right = exec_expr(bin.right);

        auto t = nextTemp();
        identifiers[t] = "int";
        arr.append(make_assign(t, left, bin.op_tok.value, right));
        return t;
    }

    std::string ir::IntermediateCodeGen::exec_expr(const std::shared_ptr<ast::ASTNode> &n) {
        return std::visit(
                [&](const auto &node) -> std::string {
                    return exec_expr_node(node);
                },
                *n
        );
    }

    void ir::IntermediateCodeGen::exec_assignment(const ast::Assignment *a) {
        if (!identifiers.count(a->identifier.value)) {
            identifiers[a->identifier.value] = "string";
        }
        auto right = exec_expr(a->expression);
        arr.append(make_assign(a->identifier.value, right, "", ""));
    }

    std::string ir::IntermediateCodeGen::exec_condition(const ast::Condition *c) {
        auto left = exec_expr(c->left_expression);
        auto right = exec_expr(c->right_expression);

        // TRUE label should be allocated here
        auto trueLabel = nextLabel();

        arr.append(make_compare(left, c->comparison.value, right, trueLabel));

        return trueLabel;
    }

    void ir::IntermediateCodeGen::exec_if(const ast::IfStatement *i) {
        auto *if_condition = std::get_if<ast::Condition>((i->if_condition).get());
        auto thenLabel = exec_condition(if_condition);
        auto elseLabel = nextLabel();
        auto endLabel = nextLabel();

        // FALSE branch
        arr.append(make_jump(elseLabel));

        // TRUE branch
        arr.append(make_label(thenLabel));
        exec_statement(i->if_body);
        arr.append(make_jump(endLabel));

        // ELSE branch
        arr.append(make_label(elseLabel));
        if (i->else_body)
            exec_statement(i->else_body);

        // END
        arr.append(make_label(endLabel));
    }

    void ir::IntermediateCodeGen::exec_while(const ast::WhileStatement *w) {
        auto startLabel = nextLabel();
        auto bodyLabel = nextLabel();
        auto endLabel = nextLabel();

        arr.append(make_label(startLabel));

        auto *w_condition = std::get_if<ast::Condition>((w->condition).get());
        // generate condition — true jumps to bodyLabel
        auto trueLabel = exec_condition(w_condition);

        // false → end
        arr.append(make_jump(endLabel));

        // true branch = body
        arr.append(make_label(trueLabel));
        exec_statement(w->body);

        // loop back
        arr.append(make_jump(startLabel));

        // end of while
        arr.append(make_label(endLabel));
    }


    void ir::IntermediateCodeGen::exec_print(const ast::PrintStatement *p) {
        if (p->type == "string") {
            if (!p->strValue.empty()) {
                // literal
                auto sym = nextStringSym();
                constants[sym] = p->strValue;
                arr.code.push_back(make_print("string", sym));
            } else {
                // variable
                auto name = exec_expr(p->intExpr);
                arr.code.push_back(make_print("string", name));
            }
        } else {
            // int print
            auto name = exec_expr(p->intExpr);
            arr.append(make_print("int", name));
        }
    }

    void ir::IntermediateCodeGen::exec_declaration(const ast::Declaration *d) {
        for (const auto &i: d->identifiers)

            identifiers[i.value] = d->declaration_type.value;


        // Handle initialization for single-variable declaration
        if (d->init_expr) {
            if (d->identifiers.size() != 1)
                throw std::runtime_error("Init only allowed for single variable declaration");

            auto varname = d->identifiers[0].value;
            auto right = exec_expr(d->init_expr);
            arr.append(make_assign(varname, right, "", ""));

        }
    }

    template<typename T>
    void ir::IntermediateCodeGen::exec_statement_node(const T &) {
        // no-op
    }

    void ir::IntermediateCodeGen::exec_statement_node(const ast::Statement &st) {
        exec_statement(st.left);
        exec_statement(st.right);
    }

    void ir::IntermediateCodeGen::exec_statement_node(const ast::IfStatement &is) {
        exec_if(&is);
    }

    void ir::IntermediateCodeGen::exec_statement_node(const ast::WhileStatement &wh) {
        exec_while(&wh);
    }

    void ir::IntermediateCodeGen::exec_statement_node(const ast::PrintStatement &pr) {
        exec_print(&pr);
    }

    void ir::IntermediateCodeGen::exec_statement_node(const ast::Declaration &de) {
        exec_declaration(&de);
    }

    void ir::IntermediateCodeGen::exec_statement_node(const ast::Assignment &asg) {
        exec_assignment(&asg);
    }

    void ir::IntermediateCodeGen::exec_statement(const std::shared_ptr<ast::ASTNode> &n) {
        if (!n) {
            return;
        }

        std::visit(
                [&](const auto &node) {
                    exec_statement_node(node);
                },
                *n
        );
    }

} // namespace pseu
