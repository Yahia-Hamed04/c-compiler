#pragma once
#include "../lexer/tokens.h"
#include "../parser/parser.h"
#include <variant>
#include <string>
namespace TACKY {
 struct Constant {
  Parser::Type type = Parser::Type::Int;
  size_t _const;
 
  Constant(): _const(0) {}
  Constant(size_t _const): _const(_const) {}
  Constant(Parser::Constant *_const): type(_const->type), _const(std::stoull(_const->_const)) {}
 };
 
 struct Var {
  Parser::Type type;
  Token name;

  Var() {}
  Var(Token name): name(name) {}
 };
 
 typedef std::variant<Constant, Var> Value;
}