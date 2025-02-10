#pragma once
#include "types.h"
#include "../lexer/tokens.h"
#include "../parser/parser.h"
using namespace TACKY;

class TACKYifier {
 private:
  Parser::CParser *parser;
  Program program;
  int temp_var_count;
  int label_count;

  Var make_temporary(bool increment_var_count = true);
  Var make_label(string prefix = "");
  Var make_label(string prefix, string suffix);

  void tackyify();
  void tackyify(Parser::FuncDecl function);
  void tackyify(Parser::Block block);
  void tackyify(Parser::Block_Item item);
  void tackyify(Parser::Declaration decl);
  void tackyify(Parser::ForInit init);
  void tackyify(Parser::Statement stmt);
  Value tackyify(std::optional<Parser::Expression> expr);
  Value tackyify(Parser::Expression *expr);

 public:
  TACKYifier() = delete;
  TACKYifier(Parser::CParser &parser);

  Program get_program();
};