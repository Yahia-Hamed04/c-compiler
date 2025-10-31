#pragma once
#include "types.h"
#include "../lexer/tokens.h"
#include "../parser/parser.h"
using namespace TACKY;

class TACKYifier {
 public:
  Parser::CParser *parser;
  TACKY::Program program;
  TACKY::Function *current_function;
  int temp_var_count;
  int label_count;

  Var make_temporary(bool increment_var_count = true);
  Var make_tacky_var(Parser::Type type = Parser::Type::Int, bool increment_var_count = true);
  Var make_label(string prefix = "");
  Var make_label(string prefix, string suffix);

  void tackyify();
  void tackyify(Parser::FuncDecl function);
  void tackyify(Parser::Block block);
  void tackyify(Parser::Block_Item item);
  void tackyify(Parser::Declaration decl);
  void tackyify(Parser::ForInit &init);
  void tackyify(Parser::Statement *stmt);
  Value tackyify(Parser::Expression *expr);

  Parser::SymbolTable *symbols;
  TACKYifier() = delete;
  TACKYifier(Parser::CParser &parser);

  TACKY::Program get_program();
};