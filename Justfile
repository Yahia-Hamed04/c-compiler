set shell := ["bash", "-cu"]
set quiet

compiler debug="0":
 clang++ {{ if debug == "1" { "-g -O0" } else { "" } }} -std=c++23 -Wno-c99-designator -Wno-switch \
 main.cpp \
 helpers.cpp \
 lexer/lexer.cpp \
 parser/parser.cpp \
 tacky/tacky.cpp \
 code_gen/code_gen.cpp \
 emitter.cpp \
 -lstdc++_libbacktrace -o build/compiler

driver:
 clang++ -std=c++17 compiler_driver.cpp -o build/compiler_driver

compile args="":
 build/compiler_driver {{args}}

test-compiler args="":
 build/compiler_driver {{args}} build/file.c

test chapter stage="all" extra_credit="true":
 writing-a-c-compiler-tests/test_compiler build/compiler_driver \
 --chapter {{chapter}} \
 {{ if stage != "all" { "--stage " + stage } else { "" } }} \
 {{ if extra_credit == "true" { "--extra-credit" } else { "" } }}