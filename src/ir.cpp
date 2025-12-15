#include "ir.hpp"
#include <string_view>

static std::shared_ptr<AssignmentCode> make_assign(std::string_view v, std::string_view l, std::string_view op, std::string_view r)
{
    auto a = std::make_shared<AssignmentCode>();
    a->var = v;
    a->left = l;
    a->op = op;
    a->right = r;
    return a;
}
static std::shared_ptr<JumpCode> make_jump(std::string_view d)
{
    auto j = std::make_shared<JumpCode>();
    j->dist = d;
    return j;
}
static std::shared_ptr<LabelCode> make_label(std::string_view l)
{
    auto x = std::make_shared<LabelCode>();
    x->label = l;
    return x;
}
static std::shared_ptr<CompareCodeIR> make_compare(std::string_view l, std::string_view op, std::string_view r, std::string_view j)
{
    auto c = std::make_shared<CompareCodeIR>();
    c->left = l;
    c->operation = op;
    c->right = r;
    c->jump = j;
    return c;
}
static std::shared_ptr<PrintCodeIR> make_print(std::string_view t, std::string_view v)
{
    auto p = std::make_shared<PrintCodeIR>();
    p->type = t;
    p->value = v;
    return p;
}

IntermediateCodeGen::IntermediateCodeGen(const std::shared_ptr<Node> &root) : root(root)
{
    exec_statement(root);
}

GeneratedIR IntermediateCodeGen::get() { return GeneratedIR{arr, identifiers, constants}; }

std::string IntermediateCodeGen::nextTemp() { return "T" + std::to_string(tCounter++); }
std::string IntermediateCodeGen::nextLabel() { return "L" + std::to_string(lCounter++); }

[[maybe_unused]] std::string IntermediateCodeGen::currentLabel() const { return "L" + std::to_string(lCounter); }
std::string IntermediateCodeGen::nextStringSym() { return "S" + std::to_string(sCounter++); }

std::string IntermediateCodeGen::exec_expr(const std::shared_ptr<Node> &n)
{
    if (auto id = std::dynamic_pointer_cast<IdentifierNode>(n))
        return id->getValue();
    if (auto num = std::dynamic_pointer_cast<NumberNode>(n))
        return num->getValue();
    if (auto str = std::dynamic_pointer_cast<StringLiteralNode>(n))
    {
        auto sym = nextStringSym();
        constants[sym] = str->getValue();
        return sym;
    }


    // Extend this with more operators (optional)
    auto bin = std::dynamic_pointer_cast<BinOpNode>(n);
    auto left = exec_expr(bin->left);
    auto right = exec_expr(bin->right);
    auto t = nextTemp();
    identifiers[t] = "int";
    arr.append(make_assign(t, left, bin->op_tok.value, right));
    return t;
}

void IntermediateCodeGen::exec_assignment(const std::shared_ptr<Assignment> &a)
{
    if (!identifiers.count(a->identifier.value)) {
        identifiers[a->identifier.value] = "string";
    }
    auto right = exec_expr(a->expression);
    arr.append(make_assign(a->identifier.value, right, "", ""));
}

std::string IntermediateCodeGen::exec_condition(const std::shared_ptr<Condition> &c)
{
    auto left = exec_expr(c->left_expression);
    auto right = exec_expr(c->right_expression);

    // TRUE label should be allocated here
    auto trueLabel = nextLabel();

    arr.append(make_compare(left, c->comparison.value, right, trueLabel));

    return trueLabel;
}

void IntermediateCodeGen::exec_if(const std::shared_ptr<IfStatement> &i)
{
    auto thenLabel = exec_condition(i->if_condition);
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

void IntermediateCodeGen::exec_while(const std::shared_ptr<WhileStatement> &w)
{
    auto startLabel = nextLabel();
    auto bodyLabel  = nextLabel();
    auto endLabel   = nextLabel();

    arr.append(make_label(startLabel));

    // generate condition — true jumps to bodyLabel
    auto trueLabel = exec_condition(w->condition);

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


void IntermediateCodeGen::exec_print(const std::shared_ptr<PrintStatement> &p)
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

void IntermediateCodeGen::exec_declaration(const std::shared_ptr<Declaration> &d)
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


void IntermediateCodeGen::exec_statement(const std::shared_ptr<Node> &n)
{
    if (!n)
        return;
    if (auto st = std::dynamic_pointer_cast<Statement>(n))
    {
        exec_statement(st->left);
        exec_statement(st->right);
        return;
    }
    if (auto is = std::dynamic_pointer_cast<IfStatement>(n))
    {
        exec_if(is);
        return;
    }
    if (auto wh = std::dynamic_pointer_cast<WhileStatement>(n))
    {
        exec_while(wh);
        return;
    }
    if (auto pr = std::dynamic_pointer_cast<PrintStatement>(n))
    {
        exec_print(pr);
        return;
    }
    if (auto de = std::dynamic_pointer_cast<Declaration>(n))
    {
        exec_declaration(de);
        return;
    }
    if (auto asg = std::dynamic_pointer_cast<Assignment>(n))
    {
        exec_assignment(asg);
        return;
    }
}
