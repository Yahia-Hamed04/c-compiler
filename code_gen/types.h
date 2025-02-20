#pragma once
#include "instruction.h"

namespace Gen {
 using Instructions = std::vector<Instruction>;

 struct Function {
  Token name;
  Instructions instructions;
 };
 
 struct Program {
  std::vector<Function> funcs;
 };
}