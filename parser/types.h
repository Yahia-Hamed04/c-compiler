#pragma once
#include <optional>
#include "../lexer/tokens.h"
#include "statements.h"

namespace Parser {
 struct Block;

 struct TypeAndStorageClass {
  Type type;
  StorageClass storage_class;
 };

 struct VarDecl {
  Token name;
  TypeAndStorageClass tasc;
  Expression *init = nullptr;
 };

 struct FuncDecl {
  Token name;
  TypeAndStorageClass ret;
  std::unordered_map<string, Token> labels;
  std::vector<Type> param_types;
  std::vector<Token> params;
  Block *body;
 };

 using Declaration = std::variant<VarDecl, FuncDecl>;
 using Block_Item  = std::variant<Declaration, Statement>;

 struct Block {
  std::vector<Block_Item> items;
 };
 
 struct Program {
  std::vector<Declaration> decls;
 };
}