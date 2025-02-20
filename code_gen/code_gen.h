#pragma once
#include "../lexer/tokens.h"
#include "../tacky/tacky.h"
#include "../parser/parser.h"
#include "types.h"
#include <unordered_map>

class Generator {
 private:
  TACKYifier *tackyifier;
  Gen::Program program;

  void generate();
  Gen::Instructions generate(TACKY::Function function);
  void add_inst(
   std::unordered_map<string, size_t> &vars,
   Gen::StackAlloc &stack_alloc,
   Gen::Instructions &insts,
   Gen::Instruction &&instruction
  );

 public:
  Parser::SymbolTable *symbols;
  Generator() = delete;
  Generator(TACKYifier &tackyifier);

  Gen::Program get_program();
};