#pragma once
#include "operand.h"
#include "../lexer/tokens.h"
#include <variant>
#include <cctype>

namespace Gen {
 struct Mov {
  AssemblyType type;
  Operand src, dst;
 };

 struct Movsx {
  Operand src, dst;
 };

 struct Movzx {
  Operand src, dst;
 };
 
 enum UnaryOp {
  Neg, Not,
  Inc, Dec,
 };
 
 struct Unary {
  AssemblyType type;
  UnaryOp op;
  Operand operand;
 };
 
 enum BinaryOp {
  And, Or, Xor, Sal, Sar, Shl, Shr,
  Add, Sub, Mult,
 };
 
 struct Binary {
  AssemblyType type;
  BinaryOp op;
  Operand src, dst;
 };
 
 struct Div {
  AssemblyType type;
  Operand operand;
 };

 struct Cmp {
  AssemblyType type;
  Operand op1, op2;
 };

 enum class Condition {
  Equal,
  Not_Equal,
  Greater,
  Greater_Equal,
  Less,
  Less_Equal,
  Above,
  Above_Equal,
  Below,
  Below_Equal
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
 struct Cdq {
  AssemblyType type;
 };
 
 using Instruction = std::variant<
  Mov, Movsx, Movzx,
  Unary, Binary, Div,
  Cmp, Jmp, Conditional_Jmp, Set_Condition, Label,  
  Ret, Cdq,
  Push, Call
 >;
}