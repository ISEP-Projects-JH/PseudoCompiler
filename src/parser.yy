/* 1. PREAMBLE */
%{
#include <iostream>
#include <memory>
#include <string>

#include "ast.hpp"
#include "tokens.hpp"

// Forward declare the scanner function (defined in Flex)
extern int yylex();
extern int yylineno;

// Forward declare the error function
void yyerror(const char *s);

// This will hold the final, complete AST
std::shared_ptr<ASTNode> g_ast_root;

static int node_line(const std::shared_ptr<ASTNode>& node)
{
    if (!node)
        return yylineno;

    if (auto* num = std::get_if<NumberNode>(node.get()))
        return num->tok.line;

    if (auto* id = std::get_if<IdentifierNode>(node.get()))
        return id->tok.line;

    if (auto* bin = std::get_if<BinOpNode>(node.get()))
        return bin->op_tok.line;

    if (auto* assign = std::get_if<Assignment>(node.get()))
        return assign->identifier.line;

    return yylineno;
}

%}

/* 2. BISON DECLARATIONS */

// The %union defines all possible data types that can be
// associated with a token (terminal) or a grammar rule (non-terminal).
%code requires {
    #include <memory>
    #include <vector>
    #include "ast.hpp"
    #include "tokens.hpp"

    struct SemanticValue {
        std::shared_ptr<ASTNode> node;
        std::vector<Token> token_list;
        Token token;
    };
}

%define api.value.type {SemanticValue}

// Tokens that carry additional data use yylval.token from the scanner.
%token T_INTLIT T_VAR T_COMPARISON T_STRING

// Define simple tokens (keywords, punctuation) that don't carry data
%token T_IF T_ELSE T_WHILE T_INT T_STRINGKW T_PRINT T_PRINTS
%token T_ASSIGN
%token T_LPAREN T_RPAREN T_LBRACE T_RBRACE T_SEMICOLON T_END

// Define operator precedence (lowest to highest) and associativity.
%left T_COMPARISON
%left '+' '-'
%left '*' '/'

// The top-level rule to start parsing
%start program

%%
/* 3. GRAMMAR RULES */

// The 'program' rule waits for all statements and an END token or EOF.
// $1 refers to the value of 'statements'. $$ is the value of 'program'.
program
    : statements T_END
    {
        g_ast_root = $1.node; // Save the completed AST
    }
    | statements
    {
        g_ast_root = $1.node; // Save the completed AST (EOF case)
    }
    ;

// This rule translates your `statements()` logic [cite: 2]
statements
    : /* empty */
    {
        $$.node = nullptr; // Base case: no statements
    }
    | statements statement
    {
        auto st = std::make_shared<ASTNode>(Statement{});
        auto& stmt = std::get<Statement>(*st);
        stmt.left  = $1.node; // The previous statements
        stmt.right = $2.node; // The new statement
        $$.node = st;        // Pass the combined list up
    }
    ;

statement
    : if_statement    { $$.node = $1.node; }
    | while_statement { $$.node = $1.node; }
    | declarations    { $$.node = $1.node; }
    | assignment      { $$.node = $1.node; }
    | printing        { $$.node = $1.node; }
    | T_LBRACE statements T_RBRACE // For nested blocks
    {
        $$.node = $2.node;
    }
    ;

// --- Expression Parsing (Replaces expr, term, factor) ---
// Note how the precedence rules make this simple.

expr
    : term
    {
        $$.node = $1.node;
    }
    | expr '+' term  // $1 is 'expr', $2 is '+', $3 is 'term'
    {
        auto bin = std::make_shared<ASTNode>(BinOpNode{});

        auto& b = std::get<BinOpNode>(*bin);
        b.left  = $1.node;
        b.op_tok = Token{TokenType::Arth, "+", node_line($1.node)};
        b.right = $3.node;

        $$.node = bin;
    }
    | expr '-' term
    {
        auto bin = std::make_shared<ASTNode>(BinOpNode{});
        auto& b = std::get<BinOpNode>(*bin);
        b.left = $1.node;
        b.op_tok = Token{TokenType::Arth, "-", node_line($1.node)};
        b.right = $3.node;
        $$.node = bin;
    }
    ;

term
    : factor
    {
        $$.node = $1.node;
    }
    | term '*' factor
    {
        auto bin = std::make_shared<ASTNode>(BinOpNode{});
        auto& b = std::get<BinOpNode>(*bin);
        b.left = $1.node;
        b.op_tok = Token{TokenType::Arth, "*", node_line($1.node)};
        b.right = $3.node;
        $$.node = bin;
    }

    | term '/' factor
    {
        auto bin = std::make_shared<ASTNode>(BinOpNode{});
        auto& b = std::get<BinOpNode>(*bin);
        b.left = $1.node;
        b.op_tok = Token{TokenType::Arth, "/", node_line($1.node)};
        b.right = $3.node;
        $$.node = bin;
    }

    ;

factor
    : T_INTLIT
    {
        // $1 is the Token from the scanner (via %union.token)
        $$.node = std::make_shared<ASTNode>(NumberNode{$1.token});
    }
    | T_VAR
    {
        $$.node = std::make_shared<ASTNode>(IdentifierNode{$1.token});
    }
    | T_LPAREN expr T_RPAREN
    {
        $$.node = $2.node; // Pass the inner expression's node up
    }
    | T_STRING
    {
        $$.node = std::make_shared<ASTNode>(StringLiteralNode{$1.token});
    }
    ;

// --- Other Statements ---

assignment
    : T_VAR T_ASSIGN expr T_SEMICOLON
    {
        auto a = std::make_shared<ASTNode>(Assignment{});
        auto& as = std::get<Assignment>(*a);
        as.identifier = $1.token;
        as.expression = $3.node;
        $$.node = a;
    }

    ;

if_statement
    : T_IF T_LPAREN condition T_RPAREN T_LBRACE statements T_RBRACE
    {
        auto ifs = std::make_shared<ASTNode>(IfStatement{});
        auto& if_stmt = std::get<IfStatement>(*ifs);
        if_stmt.if_condition = $3.node;
        if_stmt.if_body = $6.node;
        if_stmt.else_body = nullptr;
        $$.node = ifs;
    }
    | T_IF T_LPAREN condition T_RPAREN T_LBRACE statements T_RBRACE
      T_ELSE T_LBRACE statements T_RBRACE
    {
        auto ifs = std::make_shared<ASTNode>(IfStatement{});
        auto& if_stmt = std::get<IfStatement>(*ifs);
        if_stmt.if_condition = $3.node;
        if_stmt.if_body = $6.node;
        if_stmt.else_body = $10.node;
        $$.node = ifs;
    }

    ;

condition
    : expr T_COMPARISON expr
    {
        $$.node = std::make_shared<ASTNode>(Condition{
            $1.node,
            $2.token,
            $3.node
            }
        );
    }

    ;

while_statement
    : T_WHILE T_LPAREN condition T_RPAREN T_LBRACE statements T_RBRACE
    {
        auto w = std::make_shared<ASTNode>(WhileStatement{});
        auto& w_stmt = std::get<WhileStatement>(*w);
        w_stmt.condition = $3.node;
        w_stmt.body = $6.node;
        $$.node = w;
    }
    ;

printing
    : T_PRINT T_LPAREN expr T_RPAREN T_SEMICOLON
    {
        auto p = std::make_shared<ASTNode>(PrintStatement{});
        auto& p_stmt = std::get<PrintStatement>(*p);
        p_stmt.type = "int";
        p_stmt.intExpr = $3.node;
        $$.node = p;
    }
    | T_PRINTS T_LPAREN T_STRING T_RPAREN T_SEMICOLON
    {
        auto p = std::make_shared<ASTNode>(PrintStatement{});
        auto& p_stmt = std::get<PrintStatement>(*p);
        p_stmt.type = "string";
        p_stmt.strValue = $3.token.value;
        $$.node = p;
    }
    | T_PRINTS T_LPAREN T_VAR T_RPAREN T_SEMICOLON
    {
        auto p = std::make_shared<ASTNode>(PrintStatement{});
        auto& p_stmt = std::get<PrintStatement>(*p);
        p_stmt.type = "string";

        p_stmt.intExpr = std::make_shared<ASTNode>(IdentifierNode{$3.token});

        $$.node = p;
    }
    ;

declarations
    : T_INT identifier_list T_SEMICOLON
    {
        auto decl = std::make_shared<ASTNode>(Declaration{});
        auto& d = std::get<Declaration>(*decl);
        d.declaration_type = Token{TokenType::Int, "int", yylineno};
        d.identifiers = $2.token_list;
        $$.node = decl;
    }
    | T_INT T_VAR T_ASSIGN expr T_SEMICOLON
    {
        auto decl = std::make_shared<ASTNode>(Declaration{});
        auto& d = std::get<Declaration>(*decl);
        d.declaration_type = Token{TokenType::Int, "int", yylineno};
        d.identifiers = { $2.token };
        d.init_expr = $4.node;
        $$.node = decl;
    }
    | T_STRINGKW identifier_list T_SEMICOLON
    {
        auto decl = std::make_shared<ASTNode>(Declaration{});
        auto& d = std::get<Declaration>(*decl);
        d.declaration_type = Token{TokenType::StringKw, "string", yylineno};
        d.identifiers = $2.token_list;
        $$.node = decl;
    }
    | T_STRINGKW T_VAR T_ASSIGN T_STRING T_SEMICOLON
    {
        // Build string literal node
        auto lit = std::make_shared<ASTNode>(StringLiteralNode{$4.token});

        // Assignment node
        auto assign = std::make_shared<ASTNode>(Assignment{});
        auto& ass = std::get<Assignment>(*assign);
        ass.identifier = $2.token;
        ass.expression = lit;

        $$.node = assign;
    }
    ;

identifier_list
    : T_VAR
    {
        $$.token_list = std::vector<Token>{ $1.token };
    }
    | identifier_list ',' T_VAR
    {
        auto list = $1.token_list;
        list.push_back($3.token);
        $$.token_list = list;
    }
    ;

%%
/* 4. EPILOGUE */

// Bison calls this function on a syntax error.
void yyerror(const char *s) {
    std::string error_msg = std::string(s) + " at line " + std::to_string(yylineno);
    throw std::runtime_error(error_msg);
}
