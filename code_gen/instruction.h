#pragma once
#include "operand.h"
#include <variant>
#include <cctype>

namespace Gen {
 struct Mov {
  Operand src, dst;
 };
 
 enum UnaryOp {
  Neg, Not,
  Inc, Dec,
 };
 
 struct Unary {
  UnaryOp op;
  Operand operand;
 };
 
 enum BinaryOp {
  And, Or, Xor, Sal, Sar,
  Add, Sub, Mult,
 };
 
 struct Binary {
  BinaryOp op;
  Operand src, dst;
 };
 
 struct Idiv {
  Operand operand;
 };

 struct Cmp {
  Operand op1, op2;
 };

 enum class Condition {
  Equal,
  Not_Equal,
  Greater,
  Greater_Equal,
  Less,
  Less_Equal
 };

 struct Jmp {
  Token target;
 };

 struct Conditional_Jmp {
  Condition condition;
  Token target;
 };

 struct Set_Condition {
  Condition condition;
  Operand operand;
 };

 struct Label {
  Token name;
 };
 
 struct StackAlloc {
  size_t amount;
 };
 
 struct StackFree {
  size_t amount;
 };
 
 struct Push {
  Operand operand;
 };
 
 struct Call {
  Token name;
 };
 
 struct Ret {};
 struct Cdq {};
 
 using Instruction = std::variant<
  Mov,
  Unary, Binary, Idiv,
  Cmp, Jmp, Conditional_Jmp, Set_Condition, Label,  
  StackAlloc, StackFree, Ret, Cdq,
  Push, Call
 >;
}