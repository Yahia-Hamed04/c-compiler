# A work-in-progress optimising [C99](https://en.wikipedia.org/wiki/C99) compiler

Made in C++ and using what i learned from reading [Crafting Interpreters](https://craftinginterpreters.com/) and [Writing a C Compiler](https://nostarch.com/writing-c-compiler)

## Usage
Requires [Clang](https://github.com/llvm/llvm-project) and [Just](https://github.com/casey/just).
Must be run on a UNIX-based OS. (MacOS, Linux, etc.)

```sh
 just driver compiler
 just compile main.c
```

## Feature Roadmap
 - [x] Arithmetic expressions
 - [x] Variables
 - [x] Boolean expressions
 - [x] Control-Flow statements (`if`, `else`, `while`, `for`, etc.)
 - [x] Functions
 - [x] Primitive types
 - [ ] Pointers
 - [ ] Arrays
 - [ ] Strings
 - [ ] Dynamic memory allocation
 - [ ] Structures
