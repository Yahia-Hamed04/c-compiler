#pragma once
#include <optional>
#include "../lexer/tokens.h"
#include "statements.h"

namespace Parser {
 struct Block;

 enum class StorageClass {
  None,
  Static,
  Extern
 };

 struct VarDecl {
  Token name;
  StorageClass storage_class;
  std::optional<Expression> init = std::nullopt;
 };

 struct FuncDecl {
  Token name;
  StorageClass storage_class;
  std::unordered_map<string, Token> labels;
  std::vector<Token> params;
  Block *body;
 };

 using Declaration = std::variant<VarDecl, FuncDecl>;
 using Block_Item  = std::variant<Declaration, Statement>;

 struct Block {
  std::vector<Block_Item> items;
 };
 
 struct Program {
  std::vector<Declaration> functions;
 };
}