#pragma once
#include "../lexer/tokens.h"
#include "../tacky/tacky.h"
#include "../parser/parser.h"
#include "types.h"
#include <unordered_map>

struct AsmEntry {
 Gen::AssemblyType type;
 bool is_static, defined;
};

using AsmSymbolTable = std::unordered_map<string, AsmEntry>;

class Generator {
 private:
  TACKYifier *tackyifier;
  Gen::Program program;

  void generate();
  Gen::Instructions generate(TACKY::Function function);
  Gen::Operand generate_operand(TACKY::Value &value, bool print = false);
  bool add_var(
   std::unordered_map<std::string, size_t> &vars,
   size_t &stack_alloc_amount,
   Gen::AssemblyType type,
   Gen::Operand &operand
  );
  void add_inst(
   std::unordered_map<string, size_t> &vars,
   size_t &stack_alloc_amount,
   Gen::Instructions &insts,
   Gen::Instruction &&instruction
  );

 public:
  AsmSymbolTable asm_table;
  Generator() = delete;
  Generator(TACKYifier &tackyifier);

  Gen::Program get_program();
};