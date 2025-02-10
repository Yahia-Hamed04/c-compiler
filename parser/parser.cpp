#include <iostream>
#include "parser.h"
#include "../helpers.h"
#include "parse.cpp"
#include "resolve.cpp"
using namespace Parser;

CParser::CParser(Lexer &lexer, bool resolve) {
 this->lexer = &lexer;
 token_index = 0;
 
 parse();
 if (resolve) {
  resolve_labels();
  resolve_idents();
  label_statement();
 }
}

MapEntry CParser::make_var(string prefix, bool is_function) {
 string fmt_str = prefix + ".%d";
 
 Token tmp = {.type = TokenType::Identifier, .start = new char[fmt_str.size() + 10], .line = 0};
 tmp.length = (size_t)sprintf(tmp.start, fmt_str.c_str(), var_count++);

 return {.name = tmp, .from_current_block = true, .is_function = is_function};
}

Token CParser::make_label(string prefix) {
 string fmt_str = prefix + "%d";
 
 Token tmp = {.type = TokenType::Identifier, .start = new char[fmt_str.size() + 10], .line = 0};
 tmp.length = (size_t)sprintf(tmp.start, fmt_str.c_str(), label_count++);

 return tmp;
}

Program CParser::get_program() {
 return program;
}

int CParser::get_var_count() {
 return var_count;
}

Token CParser::peek(int n) {
 return lexer->tokens[token_index + n];
}

Token CParser::consume() {
 return lexer->tokens[token_index++];
}

bool CParser::did_consume(TokenType type) {
 bool equal = peek().type == type;
 if (equal) consume();

 return equal;
}

Token CParser::expect(TokenType type, string err_msg) {
 Token cur = peek();
 
 if (token_index == lexer->tokens.size() || cur.type != type) {
  error_at_line(cur, err_msg);
 }

 return consume();
}