#pragma once
#include "../lexer/tokens.h"
#include "../tacky/tacky.h"
#include "types.h"
#include <unordered_map>

class Generator {
 private:
  TACKYifier *tackyifier;
  Gen::Program program;

  void generate();
  void add_inst(std::unordered_map<string, size_t> &vars, size_t &stack_alloc_amount, Gen::Instruction &&instruction);

 public:
  Generator() = delete;
  Generator(TACKYifier &tackyifier);

  Gen::Program get_program();
};