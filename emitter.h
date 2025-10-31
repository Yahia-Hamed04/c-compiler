#pragma once
#include "code_gen/code_gen.h"
#include "lexer/tokens.h"
#include "parser/parser.h"

class Emitter {
 private:
  Generator *gen;
  std::string code;
  AsmSymbolTable &symbols;

  void emit();
  void emit_var(Gen::StaticVariable &var);
  void emit_type(Gen::AssemblyType &type);
  void emit_instruction(Gen::Instruction &inst);
  void emit_operand(Gen::Operand &operand, Gen::AssemblyType type, bool is_dst = false);
 public:
  Emitter() = delete;
  Emitter(Generator &gen);

  std::string get_code();
};