#pragma once
#include "../lexer/tokens.h"
#include <variant>
#include <string>

namespace Parser {
 struct Constant;
 struct Var;
 struct Unary;
 struct Binary;
 struct Assignment;
 struct Conditional;
 struct FunctionCall;
 
 using Expression = std::variant<
  Constant,
  Var,
  Unary,
  Binary,
  Assignment,
  Conditional,
  FunctionCall
 >;
 
 struct Constant {
  std::string _const;
 };

 struct Var {
  Token name;
 };
 
 enum UnaryOp {
  Complement,
  Decrement,
  Increment,
  Negate,
  Not
 };
 
 struct Unary {
  bool postfix;
  UnaryOp op;
  Expression *expr;
 };
 
 enum BinaryOp {
  Addition,
  Subtract,
  Multiply,
  Divide,
  Remainder,
  And,
  Or,
  Equal,
  Not_Equal,
  Less_Than,
  Less_Or_Equal,
  Greater_Than,
  Greater_Or_Equal,
  Bitwise_And,
  Bitwise_Or,
  Exclusive_Or,
  Shift_Left,
  Shift_Right,
  Error
 };
 
 struct Binary {
  BinaryOp op;
  Expression *left, *right;
 };

 struct Assignment {
  BinaryOp op;
  Expression *left, *right;
 };

 struct Conditional {
  Expression *condition;
  Expression *left, *right;
 };

 struct FunctionCall {
  Token name;
  std::vector<Expression *> args;
 };
}