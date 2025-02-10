#pragma once
#include "parser.h"
#include "../helpers.h"
using namespace Parser;

void CParser::parse() {
 while (token_index < lexer->tokens.size()) {
  expect(TokenType::Int, "expected function return type");
  Token name = expect(TokenType::Identifier, "Expected identifier after type.");
  FuncDecl function = parse_function();
  function.name = name;

  program.functions.push_back(function);
 }
}

FuncDecl CParser::parse_function() {
 FuncDecl function;

 int line = expect(TokenType::Left_Paren, "Expected '(' before parameter list.").line;

 if (did_consume(TokenType::Right_Paren)) {
  error_at_line(line, "Expected parameter list; did you mean to use \"void\" instead?");
 } else if (!did_consume(TokenType::Void)) do {
  expect(TokenType::Int, "Expected parameter type.");
  Token name = expect(TokenType::Identifier, "Expected parameter name.");

  function.params.push_back(name);
 } while (did_consume(TokenType::Comma));

 expect(TokenType::Right_Paren, "Expected ')' after parameter list.");

 function.body = nullptr;
 if (did_consume(TokenType::Left_Curly)) {
  function.body = new Block(parse_block());
 } else {
  expect(TokenType::Semicolon, "Expected semicolon after function declaration.");
 }

 return function;
}

Block CParser::parse_block() {
 Block block;
 while (token_index < lexer->tokens.size() && peek().type != TokenType::Right_Curly) {
  Block_Item new_item = parse_block_item();
  
  block.items.push_back(new_item);
 } 

 expect(TokenType::Right_Curly, "Expected '}' after block.");

 return block;
}

Block_Item CParser::parse_block_item() {
 Block_Item block_item;

 if (did_consume(TokenType::Int)) {
  block_item = parse_declaration();
 } else {
  block_item = parse_statement();
 }

 return block_item;
}

Declaration CParser::parse_declaration() {
 Declaration decl;
 Token name = expect(TokenType::Identifier, "Expected identifier after type.");

 if (peek().type == TokenType::Left_Paren) {
  FuncDecl func_decl = parse_function();
  func_decl.name = name;

  decl = func_decl;
 } else {
  VarDecl var_decl = {.name = name};
  if (did_consume(TokenType::Equal)) {
   var_decl.init = parse_expression();
  }

  decl = var_decl;
  expect(TokenType::Semicolon, "Expected semicolon after variable declaration.");
 }
 
 return decl;
}

Statement CParser::parse_statement() {
 Statement stmt = EmptyStatement{};
 Token cur = peek();
 bool expect_semicolon = true;

 if (cur.type == TokenType::Identifier
  && peek(1).type == TokenType::Colon) {
  expect_semicolon = false;
  Label label = {.name = consume()};
  consume();
  label.stmt = new Statement(parse_statement());

  stmt = label;
 } else if (did_consume(TokenType::Return)) {
  stmt = Return{.expr = parse_expression()};
 } else if (did_consume(TokenType::Continue)){
  stmt = Continue{};
 } else if (did_consume(TokenType::Break)) {
  stmt = Break{}; 
 } else if (did_consume(TokenType::Case) || did_consume(TokenType::Default)) {
  expect_semicolon = false;
  Case _case;
  _case.is_default = cur.type == TokenType::Default;
  if (!_case.is_default) {
   _case.expr = parse_expression();
  } else _case.expr = Constant{._const = ""};

  expect(TokenType::Colon, "Expected ':' after case statement.");
  _case.stmt = new Statement(parse_statement());
  stmt = _case;
 } else if (did_consume(TokenType::Goto)) {
  Goto goto_stmt;
  goto_stmt.target.name = expect(TokenType::Identifier, "Expected label name after goto.");

  stmt = goto_stmt;
 } else if (did_consume(TokenType::If)) {
  If if_stmt {
   .condition = parse_condition(),
   .then  = new Statement(parse_statement()),
   ._else = nullptr
  };

  if (did_consume(TokenType::Else)) {
   if_stmt._else = new Statement(parse_statement());
  }

  stmt = if_stmt;
  expect_semicolon = false;
 } else if (did_consume(TokenType::While)) {
  While while_stmt {
   .condition = parse_condition(),
   .body = new Statement(parse_statement())
  };

  stmt = while_stmt;
  expect_semicolon = false;
 } else if (did_consume(TokenType::Do)) {
  DoWhile do_while_stmt;
  do_while_stmt.body = new Statement(parse_statement());

  expect(TokenType::While, "Expected \"while\" before condition.");
  do_while_stmt.condition = parse_condition();

  stmt = do_while_stmt;
 } else if (did_consume(TokenType::For)) {
  For for_stmt;
  expect(TokenType::Left_Paren, "Expected opening parenthesis after\"for\".");

  Token next = peek();
  if (did_consume(TokenType::Int)) {
   VarDecl *decl = std::get_if<VarDecl>(new Declaration(parse_declaration()));
   if (decl == nullptr) {
    error_at_line(next.line, "Can only declare a variable in init expression.");
   }

   for_stmt.init = decl;
  } else if (next.type != TokenType::Semicolon) {
   for_stmt.init = parse_expression();
   expect(TokenType::Semicolon, "Expected semicolon after init expression.");
  } else {
   expect(TokenType::Semicolon, "Expected semicolon after init expression.");
  }
  
  next = peek();
  if (next.type != TokenType::Semicolon) {
   for_stmt.condition = parse_expression();
  }
  expect(TokenType::Semicolon, "Expected semicolon after condition.");

  next = peek();
  if (next.type != TokenType::Right_Paren) {
   for_stmt.post = parse_expression();
  }
  expect(TokenType::Right_Paren, "Expected closing parenthesis after post expression.");

  for_stmt.body = new Statement(parse_statement());
  stmt = for_stmt;
  expect_semicolon = false;
 } else if (did_consume(TokenType::Switch)) {
  expect_semicolon = false;
  Switch swtch;
  swtch.expr = parse_condition();
  swtch.body = new Statement(parse_statement());
  
  stmt = swtch;
 } else if (did_consume(TokenType::Left_Curly)) {
  expect_semicolon = false;
  stmt = CompoundStatement{.block = new Block(parse_block())};
 } else if (cur.type != TokenType::Semicolon) {
  stmt = parse_expression();
 }

 if (expect_semicolon) {
  expect(TokenType::Semicolon, "Expected semicolon after statement.");
 }

 return stmt;
}

Expression CParser::parse_condition() {
 Expression expr;
  
 expect(TokenType::Left_Paren, "Expected opening parenthesis before condition.");
 expr = parse_expression();
 expect(TokenType::Right_Paren, "Expected closing parenthesis after condition.");

 return expr;
}

UnaryOp parse_unop(Token token) {
 switch (token.type) {
  case TokenType::Tilde:       return UnaryOp::Complement; break;
  case TokenType::Minus:       return UnaryOp::Negate;
  case TokenType::Plus_Plus:   return UnaryOp::Increment;
  case TokenType::Minus_Minus: return UnaryOp::Decrement;
  case TokenType::Exclamation: return UnaryOp::Not;
  default: {
   error_at_line(token.line, "'" + token.to_string() + "' cannot be used as a unary operator.");
  }
 }

 return UnaryOp::Complement;
}

BinaryOp parse_binop(Token token) {
 switch (token.type) {
  case TokenType::Question:            return BinaryOp::Addition;
  case TokenType::Ampersand_Equal:
  case TokenType::Ampersand:           return BinaryOp::Bitwise_And;
  case TokenType::Ampersand_Ampersand: return BinaryOp::And;
  case TokenType::Pipe_Equal:
  case TokenType::Pipe:                return BinaryOp::Bitwise_Or;
  case TokenType::Pipe_Pipe:           return BinaryOp::Or;
  case TokenType::Caret_Equal:
  case TokenType::Caret:               return BinaryOp::Exclusive_Or;
  case TokenType::Less:                return BinaryOp::Less_Than;
  case TokenType::Less_Less_Equal:
  case TokenType::Less_Less:           return BinaryOp::Shift_Left;
  case TokenType::Less_Equal:          return BinaryOp::Less_Or_Equal;
  case TokenType::Greater:             return BinaryOp::Greater_Than;
  case TokenType::Greater_Greater_Equal:
  case TokenType::Greater_Greater:     return BinaryOp::Shift_Right;
  case TokenType::Greater_Equal:       return BinaryOp::Greater_Or_Equal;
  case TokenType::Equal_Equal:
  case TokenType::Equal:               return BinaryOp::Equal;
  case TokenType::Exclamation_Equal:   return BinaryOp::Not_Equal;
  case TokenType::Plus_Equal:
  case TokenType::Plus:                return BinaryOp::Addition;
  case TokenType::Minus_Equal:
  case TokenType::Minus:               return BinaryOp::Subtract;
  case TokenType::Asterisk_Equal:
  case TokenType::Asterisk:            return BinaryOp::Multiply;
  case TokenType::Slash_Equal:
  case TokenType::Slash:               return BinaryOp::Divide;
  case TokenType::Percent_Equal:
  case TokenType::Percent:             return BinaryOp::Remainder;
  default: {
   error_at_line(token.line, string(strings[token.type]) + " (" + token.to_string() + ") cannot be used as a binary operator.");
  }
 }

 return BinaryOp::Addition;
}

bool is_binop(TokenType type) {
 return type == TokenType::Question || 
        (TokenType::Plus <= type && type <= TokenType::Greater_Greater_Equal);
}

bool is_unop(TokenType type) {
 return TokenType::Plus_Plus <= type && type <= TokenType::Minus;
}

Expression CParser::parse_expression(int min_prec) {
 Expression *left = new Expression(parse_factor()), *right, *middle;
 Token next_token = peek();
 TokenType type = next_token.type;

 while (is_binop(type) && precedence[type] >= min_prec) {
  consume();
  BinaryOp op = parse_binop(next_token);

  if (type >= TokenType::Equal) {
   right = new Expression(parse_expression(precedence[type]));
   left  = new Expression(Assignment{.op = op, .left = left, .right = right});
  } else if (type == TokenType::Question) {
   middle = new Expression(parse_conditional_middle());
   right  = new Expression(parse_expression(precedence[type]));
   left   = new Expression(Conditional{.condition = left, .left = middle, .right = right});
  } else {
   right = new Expression(parse_expression(precedence[type] + 1));
   left  = new Expression(Binary{.op = op, .left = left, .right = right});
  }

  next_token = peek();
  type = next_token.type;
 }

 return *left;
}

Expression CParser::parse_conditional_middle() {
 Expression expr = parse_expression();

 expect(TokenType::Colon, "Expected a ':' after first half of conditional expression.");
 return expr;
}

Expression CParser::parse_factor() {
 Token cur = consume();
 TokenType type = cur.type;
 Expression expr;
 
 if (type == TokenType::Number) {
  expr = Constant{._const = cur.to_string()};
 } else if (type == TokenType::Identifier) {
  if (did_consume(TokenType::Left_Paren)) {
   FunctionCall call = {.name = cur};
   if (!did_consume(TokenType::Right_Paren)) {
    do {
     Expression *arg = new Expression(parse_expression());
     
     call.args.push_back(arg);
    } while (did_consume(TokenType::Comma));
    
    expect(TokenType::Right_Paren, "Expected ')' after function arguments.");
   }

   expr = call;
  } else {
   expr = Var{.name = cur};
  }
 } else if (is_unop(type)) {
  Unary un;
  un.postfix = false;
  un.op = parse_unop(cur);
  un.expr = new Expression(parse_factor());

  expr = un;
 } else if (type == TokenType::Left_Paren) {
  expr = parse_expression();
  expect(TokenType::Right_Paren, "Expected closing parenthesis.");
 } else {
  error_at_line(cur.line, "Not an expression! " + cur.to_string());
 }
  
 TokenType next_token_type = peek().type;
 while (next_token_type == TokenType::Plus_Plus || next_token_type == TokenType::Minus_Minus) {
  consume();
  Unary unary;
  unary.postfix = true;
  unary.op = next_token_type == TokenType::Plus_Plus ? UnaryOp::Increment : UnaryOp::Decrement;
  unary.expr = new Expression(expr);

  expr = unary;
  next_token_type = peek().type;
 }

 return expr;
}