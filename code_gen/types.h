#pragma once
#include "instruction.h"
#include "../lexer/tokens.h"
#include "../parser/parser.h"
#include <vector>

namespace Gen {
 using Instructions = std::vector<Instruction>;

 struct Function {
  Token name;
  bool global;
  Instructions instructions;
 };

 struct StaticVariable {
  Token name;
  bool global;
  Parser::InitValType init_val_type;
  size_t init;
  int alignment;
 };
 
 struct Program {
  std::vector<Function> funcs;
  std::vector<StaticVariable> statics;
 };
}