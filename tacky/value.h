#pragma once
#include "../lexer/tokens.h"
#include <variant>
#include <string>
namespace TACKY {
 /* struct Constant {
  int _const;
 
  Constant(int _const): _const(_const) {}
 }; */
 
 struct Var {
  Token name;
 };
 
 typedef std::variant<int /* Constant */, Var> Value;
}