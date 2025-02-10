#pragma once
#include "instruction.h"

namespace Gen {
 struct Function {
  Token name;
  std::vector<Instruction> instructions;
 };
 
 struct Program {
  Function func;
 };
}