#include "ir.hpp"
#include <string_view>

static std::shared_ptr<IRInstr>
make_assign(std::string_view v,
            std::string_view l,
            std::string_view op,
            std::string_view r)
{
    return std::make_shared<IRInstr>(
            AssignmentCode{
                    std::string(v),
                    std::string(l),
                    std::string(op),
                    std::string(r)
            }
    );
}
static std::shared_ptr<IRInstr>
make_jump(std::string_view d)
{
    return std::make_shared<IRInstr>(
            JumpCode{ std::string(d) }
    );
}
static std::shared_ptr<IRInstr>
make_label(std::string_view l)
{
    return std::make_shared<IRInstr>(
            LabelCode{ std::string(l) }
    );
}
static std::shared_ptr<IRInstr>
make_compare(std::string_view l,
             std::string_view op,
             std::string_view r,
             std::string_view j)
{
    return std::make_shared<IRInstr>(
            CompareCodeIR{
                    std::string(l),
                    std::string(op),
                    std::string(r),
                    std::string(j)
            }
    );
}
static std::shared_ptr<IRInstr>
make_print(std::string_view t,
           std::string_view v)
{
    return std::make_shared<IRInstr>(
            PrintCodeIR{
                    std::string(t),
                    std::string(v)
            }
    );
}

IntermediateCodeGen::IntermediateCodeGen(const std::shared_ptr<ASTNode> &root) : root(root)
{
    exec_statement(root);
}

GeneratedIR IntermediateCodeGen::get() { return GeneratedIR{arr, identifiers, constants}; }

std::string IntermediateCodeGen::nextTemp() { return "T" + std::to_string(tCounter++); }
std::string IntermediateCodeGen::nextLabel() { return "L" + std::to_string(lCounter++); }

[[maybe_unused]] std::string IntermediateCodeGen::currentLabel() const { return "L" + std::to_string(lCounter); }
std::string IntermediateCodeGen::nextStringSym() { return "S" + std::to_string(sCounter++); }

std::string IntermediateCodeGen::exec_expr(const std::shared_ptr<ASTNode> &n)
{
    if (auto* id = std::get_if<IdentifierNode>(n.get()))
        return id->getValue();

    if (auto* num = std::get_if<NumberNode>(n.get()))
        return num->getValue();

    if (auto* str = std::get_if<StringLiteralNode>(n.get())) {
        auto sym = nextStringSym();
        constants[sym] = str->getValue();
        return sym;
    }

    // Extend this with more operators (optional)
    auto* bin = std::get_if<BinOpNode>(n.get());
    auto left = exec_expr(bin->left);
    auto right = exec_expr(bin->right);
    auto t = nextTemp();
    identifiers[t] = "int";
    arr.append(make_assign(t, left, bin->op_tok.value, right));
    return t;
}

void IntermediateCodeGen::exec_assignment(const Assignment *a)
{
    if (!identifiers.count(a->identifier.value)) {
        identifiers[a->identifier.value] = "string";
    }
    auto right = exec_expr(a->expression);
    arr.append(make_assign(a->identifier.value, right, "", ""));
}

std::string IntermediateCodeGen::exec_condition(const Condition *c)
{
    auto left = exec_expr(c->left_expression);
    auto right = exec_expr(c->right_expression);

    // TRUE label should be allocated here
    auto trueLabel = nextLabel();

    arr.append(make_compare(left, c->comparison.value, right, trueLabel));

    return trueLabel;
}

void IntermediateCodeGen::exec_if(const IfStatement *i)
{
    auto *if_condition = std::get_if<Condition>((i->if_condition).get());
    auto thenLabel = exec_condition(if_condition);
    auto elseLabel = nextLabel();
    auto endLabel  = nextLabel();

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

void IntermediateCodeGen::exec_while(const WhileStatement *w)
{
    auto startLabel = nextLabel();
    auto bodyLabel  = nextLabel();
    auto endLabel   = nextLabel();

    arr.append(make_label(startLabel));

    auto *w_condition = std::get_if<Condition>((w->condition).get());
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


void IntermediateCodeGen::exec_print(const PrintStatement *p)
{
    if (p->type == "string")
    {
        if (!p->strValue.empty())
        {
            // literal
            auto sym = nextStringSym();
            constants[sym] = p->strValue;
            arr.code.push_back(make_print("string", sym));
        }
        else
        {
            // variable
            auto name = exec_expr(p->intExpr);
            arr.code.push_back(make_print("string", name));
        }
    }
    else
    {
        // int print
        auto name = exec_expr(p->intExpr);
        arr.append(make_print("int", name));
    }
}

void IntermediateCodeGen::exec_declaration(const Declaration *d)
{
    for (const auto &i : d->identifiers)

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


void IntermediateCodeGen::exec_statement(const std::shared_ptr<ASTNode> &n)
{
    if (!n)
        return;
    if (auto* st = std::get_if<Statement>(n.get())) {
        exec_statement(st->left);
        exec_statement(st->right);
        return;
    }
    if (auto* is = std::get_if<IfStatement>(n.get())) {
        exec_if(is);
        return;
    }
    if (auto* wh = std::get_if<WhileStatement>(n.get())) {
        exec_while(wh);
        return;
    }
    if (auto* pr = std::get_if<PrintStatement>(n.get())) {
        exec_print(pr);
        return;
    }
    if (auto* de = std::get_if<Declaration>(n.get())) {
        exec_declaration(de);
        return;
    }
    if (auto* asg = std::get_if<Assignment>(n.get())) {
        exec_assignment(asg);
        return;
    }
}
