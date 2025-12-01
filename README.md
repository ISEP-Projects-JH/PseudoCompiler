# PseudoCompiler

[![CI](https://github.com/ISEP-Projects-JH/PseudoCompiler/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/ISEP-Projects-JH/PseudoCompiler/actions/workflows/ci.yml)

A lightweight pseudo-compiler derived from an educational project framework.
This project demonstrates a basic pipeline consisting of **tokenization**, **parsing**, **AST construction**, and **IR generation**.

> ⚠️ **This repository is for teaching and demonstration only.
> It is *not* intended for distribution or production use.**

---

## Overview

This pseudo-compiler is built on top of a student project framework and contains substantial modifications.
It provides:

* **AST (Abstract Syntax Tree)** generation
* **IR (Intermediate Representation)** generation
* A minimal end-to-end compilation pipeline

The entire project targets **C++17 or newer**.

---

## About C++ Compatibility

Although the project is compiled using **C++17+**, the internal architectural style still follows an older **C++11/14 OOP dispatching paradigm** inherited from the distributed framework.

### Why MSVC Is Not Supported?

MSVC’s implementation of C++17+ diverges from ISO C++ in several corners and lacks full conformance for certain features needed by this project.
For this reason, **MSVC is not supported**.

Recommended compilers:

* **Clang ≥ 15**
* **GCC ≥ 13**

---

## Architecture Notes

The framework uses classic open-hierarchy OOP with virtual dispatch:

```cpp
struct Node {
    virtual ~Node() = default;
};

struct IRInstr {
    virtual ~IRInstr() = default;
    [[nodiscard]] virtual IRKind kind() const = 0;
};
```

This design is *intentionally preserved for educational continuity*, but is **not encouraged** as a learning model for modern C++.

### Limitations of Open Polymorphism in This Context

* `vtable`-based dispatch risks ABI issues and dynamic-link intrusion
* Heap fragmentation caused by scattered allocations
* `typeinfo`-driven RTTI is hack-like and not desirable in compiler pipelines
* Grammar constructs in a compiler are *finite* — open hierarchies contradict this property
* Modern C++17/20 practice prefers **closed algebraic data types (ADTs)** via
  `std::variant` + SFINAE checking, pattern-matching, and value-semantics

However, since the project builds on a provided educational skeleton, replacing the OOP dispatch model is out of scope.

---

## Global Namespace Disclaimer

The original distribution framework exposes all components in the **global namespace**.
This is retained for compatibility but should not be considered good practice.

---

## Build Instructions

```bash
mkdir -p build
cd build
cmake -G Ninja ..
ninja
```

run with:

```bash
./build/compiler
```

The compiler reads from `read.txt`, with the example `read.txt`, the expected output in `expected.txt`.
After each output, the compiler waits for `std::cin` input. Entering "q;" will exit; 
any other input will reread `read.txt` and recompile.

---

## Directory Structure

```
PseudoCompiler/
├── .github/
│   └── workflows/
│       └── ci.yml
├── include/
│   ├── ast.hpp
│   ├── codegen.hpp
│   ├── ir.hpp
│   └── tokens.hpp
├── src/
│   ├── ir.cpp
│   ├── main.cpp
│   ├── parser.yy
│   └── scanner.l
├── .gitignore
├── CMakeLists.txt
├── expected.txt
├── LICENSE
├── read.txt
└── README.md
```

---

## License

This project follows the **[MIT License](LICENSE)**.

---

## Educational Purpose

This repository exists **only as a teaching demonstration**:

* Shows the structure of a minimal compiler
* Preserves the legacy OOP framework for comparison with modern C++ design
* Not intended for reuse, redistribution, or real-world compilation tasks
