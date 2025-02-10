#pragma once
#include "code_gen/code_gen.h"
#include "lexer/tokens.h"

class Emitter {
 private:
  Generator *gen;
  std::string code;

  void emit();
  void emit_instruction(Gen::Instruction &inst);
  void emit_operand(Gen::Operand &operand, bool is_dst = false, bool is_set = false);
 public:
  Emitter() = delete;
  Emitter(Generator &gen);

  std::string get_code();
};