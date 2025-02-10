#pragma once
#include <optional>
#include "../lexer/tokens.h"
#include "statements.h"

namespace Parser {
 struct Block;

 struct VarDecl {
  Token name;
  std::optional<Expression> init = std::nullopt;
 };

 struct FuncDecl {
  Token name;
  std::vector<Token> params;
  Block *body;
 };

 using Declaration = std::variant<VarDecl, FuncDecl>;
 using Block_Item  = std::variant<Declaration, Statement>;

 struct Block {
  std::vector<Block_Item> items;
 };
 
 struct Program {
  std::vector<FuncDecl> functions;
 };
}