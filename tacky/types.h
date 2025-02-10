#pragma once
#include <vector>
#include "../lexer/tokens.h"
#include "instruction.h"

namespace TACKY {
 struct Function {
  Token name;
  std::vector<Instruction> body;
 };
 
 struct Program {
  Function func;
 };
}