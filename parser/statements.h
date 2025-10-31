#pragma once
#include <variant>
#include "expression.h"

namespace Parser {
 struct Return;
 struct If;
 struct VarDecl;
 struct For;
 struct While;
 struct DoWhile;
 struct Switch;
 struct Case;
 struct Break;
 struct Continue;
 struct Goto;
 struct Label;
 struct Block;
 struct CompoundStatement;
 struct ExpressionStatement;
 struct EmptyStatement;
 
 using Statement = std::variant<
  Return,
  If,
  For,
  While,
  DoWhile,
  Switch,
  Case,
  Break,
  Continue,
  Goto,
  Label,
  ExpressionStatement,
  CompoundStatement,
  EmptyStatement
 >;

 struct Return {
  Type type;
  Expression *expr;
 };

 struct If {
  Expression *condition;
  Statement *then, *_else;
 };

 struct Break {
  Token label;
 };

 struct Continue {
  Token label;
 };

 using ForInit = std::variant<VarDecl *, Expression *>;
 struct For {
  ForInit init = static_cast<Expression *>(nullptr);
  Expression *condition = nullptr;
  Expression *post = nullptr;
  Statement *body;
  Token label;
 };

 struct While {
  Expression *condition;
  Statement *body;
  Token label;
 };

 struct DoWhile {
  Statement *body;
  Expression *condition;
  Token label;
 };

 struct Switch {
  Expression *expr;
  Statement *body;
  std::vector<Case*> cases;
  Token label;
 };

 struct Case {
  Expression *expr = nullptr;
  Statement *stmt;
  Token label;
 };

 struct Goto {
  Var target;
 };

 struct Label {
  Token name;
  Statement *stmt;
 };

 struct ExpressionStatement {
  Expression *expr;
 };

 struct CompoundStatement {
  Block* block;
 };

 struct EmptyStatement {};
}