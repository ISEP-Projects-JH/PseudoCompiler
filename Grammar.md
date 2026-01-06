# Grammar Specification (Pseudo Language)

This section describes the **formal grammar and parsing model** of the Pseudo language as implemented by the compiler.
The grammar is intentionally small and unambiguous, designed to demonstrate a complete compilation pipeline rather than language richness.

The parser is implemented using **GNU Bison**, with tokens produced by **Flex**.
All semantic constructs are built as **closed ADTs** using `std::variant`.

---

## 1. Lexical Structure

### 1.1 Tokens

The lexer produces the following categories of tokens:

#### Keywords

* `if`
* `else`
* `while`
* `int`
* `string`
* `print`
* `prints`

#### Literals

* **Integer literal**

  ```
  [0-9]+
  ```
* **String literal**

  ```
  "…"
  ```

  Strings are raw byte sequences:

    * No escape sequences
    * No interpolation
    * No encoding rules beyond byte equality

#### Identifiers

```
[A-Za-z_][A-Za-z0-9_]*
```

Identifiers that are not keywords are treated as variables.

#### Operators

* Arithmetic: `+`, `-`, `*`, `/`
* Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`
* Assignment: `=`

#### Delimiters

* `(` `)`
* `{` `}`
* `;`
* `,`

#### Comments

```
# … (until end of line)
```

Comments are ignored by the parser.

---

## 2. Program Structure

A program consists of **zero or more statements**, optionally terminated by end-of-file.

```
program
  ::= statements
```

Statements are parsed sequentially and combined into a linear AST structure.

---

## 3. Statements

### 3.1 Statement Kinds

The following statements are supported:

* Conditional (`if / else`)
* Loop (`while`)
* Variable declaration
* Assignment
* Printing
* Block statement (`{ … }`)

```
statement
  ::= if_statement
   | while_statement
   | declarations
   | assignment
   | printing
   | "{" statements "}"
```

Blocks introduce **grouping only**.
There is no block-local scope: all variables are global.

---

## 4. Expressions

Expressions support integer computation only.

### 4.1 Expression Grammar

Operator precedence is encoded declaratively:

1. Comparison
2. Addition / subtraction
3. Multiplication / division

```
expr
  ::= term
   | expr "+" term
   | expr "-" term

term
  ::= factor
   | term "*" factor
   | term "/" factor

factor
  ::= INT_LITERAL
   | IDENTIFIER
   | STRING_LITERAL
   | "(" expr ")"
```

Notes:

* String literals are valid only in limited contexts (printing and string assignment).
* No unary operators are supported.
* No implicit conversions exist.

---

## 5. Conditions

Conditions are binary comparisons between expressions:

```
condition
  ::= expr COMPARISON expr
```

Where `COMPARISON` is one of:

```
==  !=  <  <=  >  >=
```

Conditions are used exclusively in `if` and `while`.

---

## 6. Control Flow Statements

### 6.1 If Statement

```
if_statement
  ::= "if" "(" condition ")" "{" statements "}"
   | "if" "(" condition ")" "{" statements "}"
     "else" "{" statements "}"
```

* `else` is optional
* There is no `else if`
* Conditions must be explicit comparisons

---

### 6.2 While Statement

```
while_statement
  ::= "while" "(" condition ")" "{" statements "}"
```

* No `break`
* No `continue`
* Loop condition is evaluated before each iteration

---

## 7. Variable Declarations

### 7.1 Integer Declarations

```
int a;
int a, b, c;
int a = expr;
```

Grammar forms:

```
declarations
  ::= "int" identifier_list ";"
   | "int" IDENTIFIER "=" expr ";"
```

Rules:

* Initialization is allowed only for **single-variable declarations**
* Multiple declarations cannot be initialized

---

### 7.2 String Declarations

```
string s;
string s = "hello";
```

Grammar forms:

```
declarations
  ::= "string" identifier_list ";"
   | "string" IDENTIFIER "=" STRING_LITERAL ";"
```

Rules:

* Strings can only be initialized with string literals
* No string expressions exist

---

## 8. Assignment

```
assignment
  ::= IDENTIFIER "=" expr ";"
```

Rules:

* Left-hand side must be a variable
* Assignment expressions are not expressions themselves
* Type checking is enforced during semantic analysis

---

## 9. Printing

The language provides two printing forms:

```
print(expr);
prints(string_literal);
prints(string_variable);
```

Grammar:

```
printing
  ::= "print" "(" expr ")" ";"
   | "prints" "(" STRING_LITERAL ")" ";"
   | "prints" "(" IDENTIFIER ")" ";"
```

### Semantic Model

Although syntactically distinct, printing is modeled internally as a **two-state operation**:

* Integer print
* String print

Only one payload exists per print statement, represented using a `std::variant`.
This avoids redundant fields and eliminates string-based dispatch during later compilation stages.

---

## 10. Design Constraints (Intentional)

The grammar deliberately omits:

* User-defined types
* Arrays
* Functions
* Local scopes
* Boolean literals
* Logical operators (`&&`, `||`, `!`)
* Escape sequences in strings

These omissions are **by design**, keeping the grammar mechanically transparent and suitable for teaching compiler structure rather than language expressiveness.

---

## 11. Error Handling

* Lexical errors immediately raise runtime errors
* Syntax errors are reported with line numbers
* No recovery rules are defined

The grammar favors **early failure** over error recovery to simplify reasoning.

---

## Summary

This grammar defines a **minimal but complete imperative language**:

* Small enough to understand fully
* Rich enough to demonstrate a real compiler pipeline
* Designed around closed ADTs and value semantics

The structure is intentionally strict, explicit, and limited—mirroring the goals of the PseudoCompiler project.
