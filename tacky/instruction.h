#pragma once
#include <variant>
#include "value.h"
#include "../parser/expression.h"

namespace TACKY {
 struct Unary {
  Parser::UnaryOp op;
  Value src, dst;

  Unary(Value src = 0, Value dst = 0):
   src(src), dst(dst) {}
 };
 
 struct Binary {
  Parser::BinaryOp op;
  Value src1, src2, dst;

  Binary(Value src1 = 0, Value src2 = 0, Value dst = 0):
   src1(src1), src2(src2), dst(dst) {}
 };
 
 struct Return {
  Value val;

  Return(Value val = 0) : val(val) {}
 };
 
 struct Copy {
  Value src, dst;
 
  Copy(Value src, Value dst) : src(src), dst(dst) {}
  Copy(Value src, Token dst) : src(src), dst(TACKY::Var(dst)) {}
  Copy(Value src, Parser::VarDecl dst) : src(src) {
   Var dst_var(dst.name);
   dst_var.type = dst.tasc.type;

   this->dst = dst_var;
  }
 };
 
 struct SignExtend {
  Value src, dst;
 
  SignExtend(Value src, Value dst) : src(src), dst(dst) {}
  SignExtend(Value src, Token dst) : src(src), dst(TACKY::Var(dst)) {}
 };
 
 struct ZeroExtend {
  Value src, dst;
 
  ZeroExtend(Value src, Value dst) : src(src), dst(dst) {}
  ZeroExtend(Value src, Token dst) : src(src), dst(TACKY::Var(dst)) {}
 };
 
 struct Truncate {
  Value src, dst;
 
  Truncate(Value src, Value dst) : src(src), dst(dst) {}
  Truncate(Value src, Token dst) : src(src), dst(TACKY::Var(dst)) {}
 };
 
 struct Jump {
  Var target;
 
  Jump(Var target) : target(target) {}
 };
 
 struct JumpIfZero {
  Value val;
  Var target;

  JumpIfZero(Value val, Var target) : val(val), target(target) {}
 };
 
 struct JumpIfNotZero {
  Value val;
  Var target;

  JumpIfNotZero(Value val, Var target) : val(val), target(target) {}
 };
 
 struct Label {
  Var name;

  Label(Var name) : name(name) {}
 };

 struct FunCall {
  Token name;
  std::vector<Value> args;
  Value dst;

  FunCall(Token name) : name(name) {}
 };
 
 using Instruction = std::variant<
  Unary,
  Return,
  Binary,
  Copy,
  SignExtend,
  ZeroExtend,
  Truncate,
  Jump,
  JumpIfZero,
  JumpIfNotZero,
  Label,
  FunCall
 >;
}
