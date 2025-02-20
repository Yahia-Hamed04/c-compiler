#pragma once
#include <vector>
#include "../lexer/tokens.h"
#include "instruction.h"

namespace TACKY {
 struct Function {
  Token name;
  std::vector<Token> params;
  std::vector<Instruction> body;
 };
 
 struct Program {
  std::vector<Function> funcs;
 };
}