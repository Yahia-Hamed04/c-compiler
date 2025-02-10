#pragma once
#include <cstddef>
#include <string>
#include <vector>
#include "tokens.h"

using std::size_t;
using std::string;

class Lexer {
 private:
  string src;
  size_t start, current;
  int line, column;

  bool at_end();
  void advance();
  char peek(int advance = 0);
  char consume();
  bool did_consume(char c);

  void add_token(TokenType type, size_t length);
  void tokenise();

  void scan_number();
  void scan_keyword();
  void scan_symbol();

  void print_token(Token &token);

 public:
  std::vector<Token> tokens;
  
  Lexer() = delete;
  Lexer(string src);

  void print_tokens(int start = 0);
};
