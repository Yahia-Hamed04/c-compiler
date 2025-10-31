#pragma once
#include <vector>
#include "../lexer/tokens.h"
#include "../parser/parser.h"
#include "../parser/expression.h"
#include "instruction.h"

namespace TACKY {
 struct Function {
  Token name;
  std::vector<Var> params;
  std::vector<Instruction> body;
  bool global;
 };

 struct StaticVariable {
  Token name;
  Parser::Type type;
  Parser::InitValType init_val_type;
  size_t init;
  bool global;
 };
 
 struct Program {
  std::vector<StaticVariable> statics;
  std::vector<Function> funcs;
 };
}