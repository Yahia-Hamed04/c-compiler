#include <iostream>
#include <cassert>
#include "lexer.h"
#include "../helpers.h"

Lexer::Lexer(string src) {
 start = current = 0;
 column = 1;
 line = 1;
 this->src = src;

 tokenise();
}

bool Lexer::at_end() {return current >= src.length();}
void Lexer::advance() {start = current;}
char Lexer::peek(int advance) {return src[current + advance];}
char Lexer::consume() {return src[current++];}
bool Lexer::did_consume(char c) {
 bool equal = peek() == c;
 if (equal) consume();

 return equal;
}

void Lexer::add_token(TokenType type, size_t length = 0) {
 Token token;
 token.type   = type;
 token.start  = src.data() + start;
 token.length = length > 0 ? length : current - start;
 token.line = this->line;

 tokens.push_back(token);
 advance();
}

void Lexer::tokenise() {
 while (!at_end()) {
  char c = peek();
  
  if (isdigit(c)) {
   scan_number();

   c = peek();
   if (isalpha(c) || c == '_') {
    error("Identifiers cannot begin with non-alphabetic characters.");
   }
  } else if (isalpha(c) || c == '_') {
   scan_keyword();
  } else if (!isspace(c)) {
   scan_symbol();
  } else {
   while(isspace(c = peek())) {
    consume();
 
    line += c == '\n';  
   } advance();
  }
 }
}

void Lexer::scan_number() {
 while (isdigit(peek()) && !at_end()) {
  consume();
 }

 assert(src[current] != '.' && "Floats aren't supported yet.");

 add_token(TokenType::Number);
}

void Lexer::scan_keyword() {
 while ((isalnum(peek()) || peek() == '_') && !at_end()) {
  consume();
 }

 TokenType type = TokenType::Identifier;
 string token_string = src.substr(start, current - start);

 switch (token_string[0]) {
  case 'b': {
   if (token_string == "break") type = TokenType::Break;
  } break;
  case 'c': {
        if (token_string == "case")     type = TokenType::Case;
   else if (token_string == "continue") type = TokenType::Continue;
  } break;
  case 'd': {
        if (token_string == "do")      type = TokenType::Do;
   else if (token_string == "default") type = TokenType::Default;
  } break;
  case 'e': {
        if (token_string == "else")   type = TokenType::Else;
   else if (token_string == "extern") type = TokenType::Extern;
  } break;
  case 'f': {
   if (token_string == "for") type = TokenType::For;
  } break;
  case 'g': {
   if (token_string == "goto") type = TokenType::Goto;
  } break;
  case 'i': {
        if (token_string == "if")  type = TokenType::If;
   else if (token_string == "int") type = TokenType::Int;
  } break;
  case 'r': {
   if (token_string == "return") type = TokenType::Return;
  } break;
  case 's': {
        if (token_string == "switch") type = TokenType::Switch;
   else if (token_string == "static") type = TokenType::Static;
  } break;
  case 'v': {
   if (token_string == "void") type = TokenType::Void;
  } break;
  case 'w': {
   if (token_string == "while") type = TokenType::While;
  } break;
 }

 add_token(type);
}

void Lexer::scan_symbol() {
 char sym = consume();
 TokenType type;

 switch (sym) {
  case '+': {
   type = did_consume('+') ? TokenType::Plus_Plus
        : did_consume('=') ? TokenType::Plus_Equal
                           : TokenType::Plus;
  } break;
  case '-': {
   type = did_consume('-') ? TokenType::Minus_Minus
        : did_consume('=') ? TokenType::Minus_Equal
                           : TokenType::Minus;
  } break;
  case '*': {
   type = did_consume('=') ? TokenType::Asterisk_Equal
                           : TokenType::Asterisk;
  } break;
  case '/': {
   type = did_consume('=') ? TokenType::Slash_Equal
                           : TokenType::Slash;
  } break;
  case '<': {
   type = TokenType::Less;

   if (did_consume('<')) {
    type = did_consume('=') ? TokenType::Less_Less_Equal
                            : TokenType::Less_Less;
   } else if (did_consume('=')) {
    type = TokenType::Less_Equal;
   }
  } break;
  case '>': {
   type = TokenType::Greater;

   if (did_consume('>')) {
    type = did_consume('=') ? TokenType::Greater_Greater_Equal
                            : TokenType::Greater_Greater;
   } else if (did_consume('=')) {
    type = TokenType::Greater_Equal;
   }
  } break;
  case '%': {
   type = did_consume('=') ? TokenType::Percent_Equal 
                           : TokenType::Percent;
  } break;
  case '&': {
   type = did_consume('&') ? TokenType::Ampersand_Ampersand
        : did_consume('=') ? TokenType::Ampersand_Equal
                           : TokenType::Ampersand;
  } break;
  case '^': {
   type = did_consume('=') ? TokenType::Caret_Equal
                           : TokenType::Caret;
  } break;
  case '|': {
   type = did_consume('|') ? TokenType::Pipe_Pipe
        : did_consume('=') ? TokenType::Pipe_Equal
                           : TokenType::Pipe;
  } break;
  case '!': {
   type = did_consume('=') ? TokenType::Exclamation_Equal
                           : TokenType::Exclamation;
  } break;
  case '=': {
   type = did_consume('=') ? TokenType::Equal_Equal
                           : TokenType::Equal;
  } break;
  case '{': type = TokenType::Left_Curly; break;
  case '}': type = TokenType::Right_Curly; break;
  case '(': type = TokenType::Left_Paren; break;
  case ')': type = TokenType::Right_Paren; break;
  case '?': type = TokenType::Question; break;
  case ':': type = TokenType::Colon; break;
  case ';': type = TokenType::Semicolon; break;
  case ',': type = TokenType::Comma; break;
  case '~': type = TokenType::Tilde; break;
  default: {
   std::string err_msg = "Unknown symbol:  ";
   err_msg[err_msg.size() - 1] = sym;
   error(err_msg);
  }
 }

 add_token(type);
}

void Lexer::print_token(Token &token) {
 if (token.type == TokenType::TokenType_Count) {
  error("Got TokenType_Count, which is not a real token!");
 }
 
 string str = "Type: ";
 str += strings[token.type];

 if (token.type == TokenType::Identifier || token.type == TokenType::Number) {
  str += ", String: " + token.to_string();
 }

 std::cout << str << '\n';
}

void Lexer::print_tokens(int start) {
 for (int i = start; i < tokens.size(); i++) {
  Token token = tokens[i];
  print_token(token);
 }
}