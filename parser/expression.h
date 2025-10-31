#pragma once
#include "../lexer/tokens.h"
#include <variant>
#include <string>

namespace Parser {
 enum class Type {
  Function,
  UInt, 
  ULong,
  Int, 
  Long,
 };

 enum class StorageClass {
  None,
  Static,
  Extern
 };

 class ExpressionVisitor;
 struct Constant;
 struct Var;
 struct Unary;
 struct Binary;
 struct Assignment;
 struct Conditional;
 struct FunctionCall;
 struct Cast;
 
 struct Expression {
  Type type;
  
  virtual bool constexpr is_var() {return false;}
  virtual bool constexpr is_const() {return false;}
  virtual void accept(ExpressionVisitor *v) = 0;
 };

 class ExpressionVisitor {
  public:
   virtual void visit(Constant *expr) = 0;
   virtual void visit(Var *expr) = 0;
   virtual void visit(Unary *expr) = 0;
   virtual void visit(Binary *expr) = 0;
   virtual void visit(Assignment *expr) = 0;
   virtual void visit(Conditional *expr) = 0;
   virtual void visit(FunctionCall *expr) = 0;
   virtual void visit(Cast *expr) = 0;
 };
 
 struct Constant : Expression {
  std::string _const;

  Constant(std::string _const = "") : _const(_const) {}

  bool constexpr is_const() {return true;}
  void accept(ExpressionVisitor *v) {
   v->visit(this);
  }
 };

 struct Var : Expression {
  Token name;

  Var(Token name = {}) : name(name) {}

  bool constexpr is_var() {return true;}
  void accept(ExpressionVisitor *v) {
   v->visit(this);
  }
 };
 
 enum UnaryOp {
  Complement,
  Decrement,
  Increment,
  Negate,
  Not
 };
 
 struct Unary : Expression {
  bool postfix;
  UnaryOp op;
  Expression *expr;

  void accept(ExpressionVisitor *v) {
   v->visit(this);
  }
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
 
 struct Binary : Expression {
  BinaryOp op;
  Expression *left, *right;

  Binary(BinaryOp op, Expression *left, Expression *right):
   op(op),
   left(left),
   right(right) {}

  void accept(ExpressionVisitor *v) {
   v->visit(this);
  }
 };

 struct Assignment : Expression {
  BinaryOp op;
  Expression *left, *right;

  Assignment(BinaryOp op, Expression *left, Expression *right):
   op(op),
   left(left),
   right(right) {}

  void accept(ExpressionVisitor *v) {
   v->visit(this);
  }
 };

 struct Conditional : Expression {
  Expression *condition;
  Expression *left, *right;

  Conditional(Expression *condition, Expression *left, Expression *right):
   condition(condition),
   left(left),
   right(right) {}

  void accept(ExpressionVisitor *v) {
   v->visit(this);
  }
 };

 struct FunctionCall : Expression {
  Token name;
  std::vector<Expression *> args;

  FunctionCall(Token name = {}) : name(name) {}

  void accept(ExpressionVisitor *v) {
   v->visit(this);
  }
 };

 struct Cast : Expression {
  Expression* expr;

  void accept(ExpressionVisitor *v) {
   v->visit(this);
  }
 };
}