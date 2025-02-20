#pragma once
#include "code_gen/code_gen.h"
#include "lexer/tokens.h"
#include "parser/parser.h"

class Emitter {
 private:
  Generator *gen;
  std::string code;
  Parser::SymbolTable *symbols;

  void emit();
  void emit_instruction(Gen::Instruction &inst);
  void emit_operand(Gen::Operand &operand, bool is_dst = false, int num_bytes = 4);
 public:
  Emitter() = delete;
  Emitter(Generator &gen);

  std::string get_code();
};