# PseudoCompiler

[![CI](https://github.com/ISEP-Projects-JH/PseudoCompiler/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/ISEP-Projects-JH/PseudoCompiler/actions/workflows/ci.yml)
[![zread](https://img.shields.io/badge/Ask_Zread-_.svg?style=flat&color=00b0aa&labelColor=000000&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB3aWR0aD0iMTYiIGhlaWdodD0iMTYiIHZpZXdCb3g9IjAgMCAxNiAxNiIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj4KPHBhdGggZD0iTTQuOTYxNTYgMS42MDAxSDIuMjQxNTZDMS44ODgxIDEuNjAwMSAxLjYwMTU2IDEuODg2NjQgMS42MDE1NiAyLjI0MDFWNC45NjAxQzEuNjAxNTYgNS4zMTM1NiAxLjg4ODEgNS42MDAxIDIuMjQxNTYgNS42MDAxSDQuOTYxNTZDNS4zMTUwMiA1LjYwMDEgNS42MDE1NiA1LjMxMzU2IDUuNjAxNTYgNC45NjAxVjIuMjQwMUM1LjYwMTU2IDEuODg2NjQgNS4zMTUwMiAxLjYwMDEgNC45NjE1NiAxLjYwMDFaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik00Ljk2MTU2IDEwLjM5OTlIMi4yNDE1NkMxLjg4ODEgMTAuMzk5OSAxLjYwMTU2IDEwLjY4NjQgMS42MDE1NiAxMS4wMzk5VjEzLjc1OTlDMS42MDE1NiAxNC4xMTM0IDEuODg4MSAxNC4zOTk5IDIuMjQxNTYgMTQuMzk5OUg0Ljk2MTU2QzUuMzE1MDIgMTQuMzk5OSA1LjYwMTU2IDE0LjExMzQgNS42MDE1NiAxMy43NTk5VjExLjAzOTlDNS42MDE1NiAxMC42ODY0IDUuMzE1MDIgMTAuMzk5OSA0Ljk2MTU2IDEwLjM5OTlaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik0xMy43NTg0IDEuNjAwMUgxMS4wMzg0QzEwLjY4NSAxLjYwMDEgMTAuMzk4NCAxLjg4NjY0IDEwLjM5ODQgMi4yNDAxVjQuOTYwMUMxMC4zOTg0IDUuMzEzNTYgMTAuNjg1IDUuNjAwMSAxMS4wMzg0IDUuNjAwMUgxMy43NTg0QzE0LjExMTkgNS42MDAxIDE0LjM5ODQgNS4zMTM1NiAxNC4zOTg0IDQuOTYwMVYyLjI0MDFDMTQuMzk4NCAxLjg4NjY0IDE0LjExMTkgMS42MDAxIDEzLjc1ODQgMS42MDAxWiIgZmlsbD0iI2ZmZiIvPgo8cGF0aCBkPSJNNCAxMkwxMiA0TDQgMTJaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik00IDEyTDEyIDQiIHN0cm9rZT0iI2ZmZiIgc3Ryb2tlLXdpZHRoPSIxLjUiIHN0cm9rZS1saW5lY2FwPSJyb3VuZCIvPgo8L3N2Zz4K&logoColor=ffffff)](https://zread.ai/ISEP-Projects-JH/PseudoCompiler)

A **minimal pseudo-language compiler** intended purely for **teaching and demonstration**.

> Note: You can consult zread for a quick start guide (available in English or Simplified Chinese), but please avoid
> over-reliance on zread. AI-generated summaries may occasionally be affected by hallucinations and deviate from
> actual content. Refer to the documentation and doxygen comments for the most accurate information.

This project is derived from a distributed educational compiler framework, but has since been **fully refactored** to
demonstrate a modern C++20-style compiler pipeline with **value semantics**, **closed ADTs**, and **explicit memory
management**.

> ⚠️ **Educational use only**
>
> This repository is *not* intended for production, distribution, or real-world language implementation.

---

## Overview

> If you need to use AI to analyze your repository, emphasize the importance of Doxygen and documentation in your prompt
> to avoid AI overinterpreting implementation code. This is a complete antipattern in modern C++ (mcpp) and is
> misleading. C++ emphasizes that the API defines semantic boundaries, and documentation describes behavior. You should
> not overemphasize implementation details, as they offer **exactly no practical benefit** to your usage (understanding
> implementation details is relevant for rewriters, and even maintainers should not obsess over them).

PseudoCompiler demonstrates a **complete but minimal compilation pipeline**:

1. Lexical analysis (Flex)
2. Parsing (Bison)
3. AST construction
4. IR (Intermediate Representation) generation
5. Assembly code generation (x86-64 NASM-style)

The language and implementation are intentionally constrained to keep the design **fully understandable** and
**mechanically transparent**.

---

## Language Description (Pseudo Language)

This compiler implements a **very small pseudo language** with deliberately limited features.

See [Grammar](Grammar.md) for details

### Types

Only **two built-in types** exist:

* `int`
* `string`

There are **no user-defined types**, **no arrays**, **no structs**, and **no pointers**.

---

### Variables and Declarations

```c
int a;
int b = 3;
string s = "hello";
```

* Declarations may list multiple identifiers
* Initialization is allowed **only for single-variable declarations**
* All variables are global in scope

---

### Expressions

Supported expressions include:

* Integer literals
* Identifiers
* Binary arithmetic:

    * `+`, `-`, `*`, `/`
* Simple comparisons:

    * `==`, `!=`, `<`, `<=`, `>`, `>=`

---

### Control Flow

```c
if (a < b) {
    print(a);
} else {
    print(b);
}

while (a < 10) {
    a = a + 1;
}
```

Supported constructs:

* `if / else`
* `while`

No `for`, no `break`, no `continue`.

---

### Printing

```c
print(123);
prints("hello");
```

* `print(int)`
* `prints(string_variable)` or `prints(string_literal)`

Internally, printing is modeled as a **closed two-state operation** rather than
a string-dispatched command. Since only two print modes exist (integer vs string),
the compiler represents the print kind using a small enum instead of string tags,
avoiding unnecessary string comparisons during IR generation and code emission.

---

### Strings (Important Limitations)

String literals are **extremely restricted**:

* Only raw characters are supported
* **No escape sequences**

    * ❌ `\n`
    * ❌ `\t`
    * ❌ `\xNN`
    * ❌ `\u{...}`

Strings are treated as **verbatim byte sequences** terminated by `'\0'`.

This is intentional: the goal is to demonstrate **compiler structure**, not string parsing complexity.

---

## Architectural Design

### From Teaching Framework to Refactored Compiler

The original teaching framework relied on:

* Open inheritance hierarchies
* Virtual dispatch
* RTTI (`dynamic_cast`, `typeid`)
* Heap-heavy object graphs

This repository **no longer follows that model**.

---

### Closed ADTs with `std::variant`

All major language constructs are represented using **closed algebraic data types**:

* `ASTNode` → `std::variant<...>`
* `IRInstr` → `std::variant<...>`

Dispatch is performed using:

* `std::visit`
* `if constexpr`
* Compile-time type resolution

As a result:

* **RTTI is completely removed**
* The compiler is built with `-fno-rtti`
* No virtual functions or vtables exist in AST or IR

---

### Value-Semantic IR

The IR layer is designed as **pure data**:

* IR instructions are immutable value objects
* Equality and hashing are explicitly defined
* No hidden ownership or polymorphic behavior

This makes the IR:

* Deduplicable
* Cache-friendly
* Deterministic

---

### Memory Management with JH-Toolkit

To address allocation pressure and fragmentation, the compiler integrates **JH-Toolkit**:

* IR instructions are stored in `jh::conc::flat_pool`
* This provides:

    * Arena-like contiguous storage
    * Key-based deduplication
    * GC-like reuse semantics
* No explicit pool shrinking is performed:

    * Compiler processes typically run once and exit
    * Capacity growth reflects legitimate workload demand

This eliminates IR-level heap fragmentation while keeping semantics simple.

---

## Toolchain Requirements

### Language Standard

* **GNU C++20** (`-std=gnu++20`)

---

### Supported Compilers

* **Apple Clang ≥ 15**
* **LLVM Clang 20**
* **GCC ≥ 13** *(GCC 14.3+ recommended)*
* **MinGW-based** versions of the above *(UCRT recommended for Windows)*

> Note: The output assembly of the pseudo compiler (executable build target of **this project**)  
> is designed for **x86-64 NASM-style syntax** and may not be compatible with
> non-x86-64 targets or assemblers with different syntax expectations.  
> Please use a x86-64 linux in docker or VM if your host environment is incompatible.

---

### Unsupported Compilers

* ❌ **MSVC**

MSVC is not supported due to incomplete or divergent behavior in modern C++20 features and ABI expectations required by
this project.

---

## Dependencies

### Build-Time

* Flex
* Bison
* CMake ≥ 3.16
* Ninja (recommended)

---

### External Libraries

* **JH-Toolkit**

    * License: **Apache License 2.0**
    * Repository:
      [https://github.com/JeongHan-Bae/JH-Toolkit](https://github.com/JeongHan-Bae/JH-Toolkit)

JH-Toolkit is used **only as a low-level utility library** and does not impose runtime dependencies.

> Note: up to now, the used modules are still in dev branch (1.4.0-dev) of JH-Toolkit.  
> The branch is almost stable but may have minor changes before the official release.  
> Once released, the dependency will be updated accordingly.

See [NOTICE](NOTICE.md) for details.

---

## Build Instructions

```bash
mkdir -p build
cd build
cmake -G Ninja ..
ninja
```

## Running

```bash
./compiler [options]
```

### Options

* `-src <path>`
  Source file path.
  If specified, **a path argument is required**.

* `-target <path>`
  Output assembly file path.
  If specified, **a path argument is required**.

* `--ast`
  Print AST.

* `--ir`
  Print IR.

### Defaults

If not specified:

```c
-src     read.txt
-target  out.asm
```

Paths are resolved **relative to the execution directory**.

The compiler:

* Reads input from source file
* Writes assembly output to target file
* Prints AST and/or IR if requested
* Waits for user input after each run

    * Enter `q;` to exit
    * Any other input recompiles `read.txt`

---

## Project Structure

```
PseudoCompiler/
├── .github/
│   └── workflows/
│       └── ci.yml     # GitHub Actions CI configuration
├── include/
│   ├── ast.hpp        # AST definitions (variant-based)
│   ├── codegen.hpp    # Assembly code generator
│   ├── ir.hpp         # IR definitions + flat_pool integration
│   └── tokens.hpp     # Lexer token definitions
├── src/
│   ├── codegen.cpp
│   ├── ir.cpp
│   ├── main.cpp
│   ├── parser.yy
│   └── scanner.l
├── CMakeLists.txt
├── read.txt
├── expected.txt
├── LICENSE
├── NOTICE.md
└── README.md
```

---

## License

This repository is licensed under the **[MIT License](LICENSE)**.

Third-party dependency **[JH-Toolkit](https://github.com/JeongHan-Bae/JH-Toolkit)** is licensed
under **[Apache License 2.0](https://github.com/JeongHan-Bae/JH-Toolkit/blob/main/LICENSE)**.  
See the [NOTICE file](NOTICE.md) for details.

---

## Educational Purpose

This project exists to demonstrate:

* A **minimal but complete compiler pipeline**
* Modern C++20 design using **value semantics**
* Why **closed ADTs** outperform open OOP hierarchies for compilers
* Practical memory management without GC or RTTI

It is intentionally small, strict, and limited by design.
