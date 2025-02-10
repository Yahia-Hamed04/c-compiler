#pragma once
#include <variant>

namespace Gen {
 enum Register {
  AX,
  CX,
  DX,
  R10,
  R11
 };
 
 struct Immediate {
  int val;
 };
 
 struct Pseudo {
  Token name;
 };
 
 struct StackOffset {
  size_t offset;
 };
 
 typedef std::variant<Immediate, Register, Pseudo, StackOffset> Operand;
}