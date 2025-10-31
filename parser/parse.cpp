#pragma once
#include "parser.h"
#include "../helpers.h"
using namespace Parser;

bool is_specifier(TokenType type) {
 return TokenType::Int <= type && type <= TokenType::Extern;
}

bool is_type(TokenType type) {
 return TokenType::Int <= type && type <= TokenType::Unsigned;
}

void CParser::parse() {
 while (token_index < lexer->tokens.size()) {
  Declaration decl = parse_declaration();

  program.decls.push_back(decl);
 }
}

FuncDecl CParser::parse_function() {
 FuncDecl function;

 int line = expect(TokenType::Left_Paren, "Expected '(' before parameter list.").line;

 if (did_consume(TokenType::Right_Paren)) {
  error_at_line(line, "Expected parameter list; did you mean to use \"void\" instead?");
 } else if (!did_consume(TokenType::Void)) do {
  std::vector<Type> types;
  bool has_signed = false, has_unsigned = false;
  for (TokenType type = peek().type; is_type(type); consume(), type = peek().type) {
   switch (type) {
    case TokenType::Int:  types.push_back(Type::Int); break;
    case TokenType::Long: types.push_back(Type::Long); break;
    case TokenType::Signed: {
     if (has_signed || has_unsigned) error_at_line(peek().line, "Invalid Type");
 
     has_signed = true;
    } break;
    case TokenType::Unsigned: {
     if (has_signed || has_unsigned) error_at_line(peek().line, "Invalid Type");
 
     has_unsigned = true;
    } break;
   }
  }
 
  if (types.size() == 0 && (has_signed || has_unsigned)) {
   types.push_back(has_unsigned ? Type::UInt : Type::Int);
  } else if (has_unsigned) {
   for (Type &t : types) {
    if (t == Type::Int)  t = Type::UInt;
    if (t == Type::Long) t = Type::ULong;
   }
  }

  function.param_types.push_back(parse_type(types));
  function.params.push_back(expect(TokenType::Identifier, "Expected parameter name."));
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
 TokenType type = peek().type;

 if (is_specifier(type)) {
  block_item = parse_declaration();
 } else {
  block_item = parse_statement();
 }

 return block_item;
}

Declaration CParser::parse_declaration() {
 Declaration decl;
 TypeAndStorageClass tasc = parse_type_and_storage_class();
 Token name = expect(TokenType::Identifier, "Expected identifier after specifiers.");

 if (peek().type == TokenType::Left_Paren) {
  FuncDecl func_decl = parse_function();
  func_decl.name = name;
  func_decl.ret = tasc;

  decl = func_decl;
 } else {
  VarDecl var_decl = {.name = name, .tasc = tasc};
  if (did_consume(TokenType::Equal)) {
   var_decl.init = parse_expression();
  }

  decl = var_decl;
  expect(TokenType::Semicolon, "Expected semicolon after variable declaration.");
 }
 
 return decl;
}

Type CParser::parse_type(std::vector<Type> types) {
 switch (types.size()) {
  case 1: return types[0]; break;
  case 2: {
   if (types[0] == types[1]) error_at_line(peek().line, "Invalid Type");

   if (types[0] == Type::Long || types[1] == Type::Long) {
    return Type::Long;
   } else if (types[0] == Type::ULong || types[1] == Type::ULong) {
    return Type::ULong;
   }
  } break;
  default: error_at_line(peek().line, std::to_string(types.size()));
 }

 return Type::Int;
}

TypeAndStorageClass CParser::parse_type_and_storage_class() {
 std::vector<Type> types;
 std::vector<StorageClass> storage_classes;
 TypeAndStorageClass tasc = {.storage_class = StorageClass::None};

 bool has_unsigned = false;
 bool has_signed = false;
 for (TokenType type = peek().type; is_specifier(type); consume(), type = peek().type) {
  switch (type) {
   case TokenType::Int:  types.push_back(Type::Int); break;
   case TokenType::Long: types.push_back(Type::Long); break;
   case TokenType::Static: storage_classes.push_back(StorageClass::Static); break;
   case TokenType::Extern: storage_classes.push_back(StorageClass::Extern); break;
   case TokenType::Signed: {
    if (has_signed || has_unsigned) error_at_line(peek().line, "Invalid Type");

    has_signed = true;
   } break;
   case TokenType::Unsigned: {
    if (has_signed || has_unsigned) error_at_line(peek().line, "Invalid Type");

    has_unsigned = true;
   } break;
  }
 }

 if (types.size() == 0 && (has_signed || has_unsigned)) {
  types.push_back(Type::Int);
 }

 for (int i = 0; has_unsigned && i < types.size(); i++) {
  types[i] = types[i] == Type::Long ? Type::ULong : Type::UInt;
 }

 int line = peek().line;
 if (storage_classes.size() > 1) error_at_line(line, "Invalid storage class.");

 tasc.type = parse_type(types);
 if (storage_classes.size() == 1) {
  tasc.storage_class = storage_classes[0];
 }

 return tasc;
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
  if (cur.type != TokenType::Default) {
   _case.expr = parse_expression();
  }

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
  if (is_specifier(next.type)) {
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
  stmt = ExpressionStatement{.expr = parse_expression()};
 }

 if (expect_semicolon) {
  expect(TokenType::Semicolon, "Expected semicolon after statement.");
 }

 return stmt;
}

Expression* CParser::parse_condition() {
 Expression *expr;
  
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

Expression* CParser::parse_expression(int min_prec) {
 Expression *left = parse_factor(), *right, *middle;
 Token next_token = peek();
 TokenType type = next_token.type;

 while (is_binop(type) && precedence[type] >= min_prec) {
  consume();
  BinaryOp op = parse_binop(next_token);

  if (type >= TokenType::Equal) {
   right = parse_expression(precedence[type]);
   left  = new Assignment(op, left, right);
  } else if (type == TokenType::Question) {
   middle = parse_conditional_middle();
   right  = parse_expression(precedence[type]);
   left   = new Conditional(left, middle, right);
  } else {
   right = parse_expression(precedence[type] + 1);
   left  = new Binary(op, left, right);
  }

  next_token = peek();
  type = next_token.type;
 }

 return left;
}

Expression* CParser::parse_conditional_middle() {
 Expression *expr = parse_expression();

 expect(TokenType::Colon, "Expected a ':' after first half of conditional expression.");
 return expr;
}

Expression* CParser::parse_factor() {
 Token cur = consume();
 TokenType token_type = cur.type;
 Expression *expr;
 
 if (TokenType::Number <= token_type && token_type <= TokenType::Unsigned_Long_Number) {
  Type type = Type::Int;
  string _const = cur.to_string();
  bool is_unsigned = false;
  switch (token_type) {
   case TokenType::Unsigned_Number: {
    type = Type::UInt;
    _const.pop_back();
    is_unsigned = true;
   } break;
   case TokenType::Long_Number: {
    type = Type::Long;
    _const.pop_back();
   } break;
   case TokenType::Unsigned_Long_Number: {
    type = Type::ULong;
    _const.pop_back();
    _const.pop_back();
    is_unsigned = true;
   } break;
   default: {}
  }

  size_t val = std::stoull(_const);
  if (is_unsigned) {
   if (val > UINT64_MAX) {
    error_at_line(cur.line, "Constant is too big to be an unsigned int or unsigned long.");
   } else if (val > UINT32_MAX) {
    type = Type::ULong;
   }
  } else {
   if (val > INT64_MAX) {
    error_at_line(cur.line, "Constant is too big to be an int or long.");
   } else if (val > INT32_MAX) {
    type = Type::Long;
   }
  }

  expr = new Constant(_const);
  expr->type = type;
 } else if (token_type == TokenType::Identifier) {
  if (did_consume(TokenType::Left_Paren)) {
   FunctionCall *call = new FunctionCall(cur);
   if (!did_consume(TokenType::Right_Paren)) {
    do {
     Expression *arg = parse_expression();
     
     call->args.push_back(arg);
    } while (did_consume(TokenType::Comma));
    
    expect(TokenType::Right_Paren, "Expected ')' after function arguments.");
   }

   expr = call;
  } else {
   expr = new Var(cur);
  }
 } else if (is_unop(token_type)) {
  Unary *un = new Unary();
  un->postfix = false;
  un->op = parse_unop(cur);
  un->expr = parse_factor();

  expr = un;
 } else if (token_type == TokenType::Left_Paren) {
  if (is_specifier(peek().type)) {
   Cast *cast = new Cast();
   std::vector<Type> types;
   bool has_signed = false, has_unsigned = false;
   for (TokenType type = peek().type; is_type(type); consume(), type = peek().type) {
    switch (type) {
     case TokenType::Int:  types.push_back(Type::Int); break;
     case TokenType::Long: types.push_back(Type::Long); break;
     case TokenType::Signed: {
      if (has_signed || has_unsigned) error_at_line(peek().line, "Invalid Type");

      has_signed = true;
     } break;
     case TokenType::Unsigned: {
      if (has_signed || has_unsigned) error_at_line(peek().line, "Invalid Type");

      has_unsigned = true;
     } break;
    }
   }

   if (types.size() == 0 && (has_signed || has_unsigned)) {
    types.push_back(Type::Int);
   }
  
   for (int i = 0; has_unsigned && i < types.size(); i++) {
    types[i] = types[i] == Type::Long ? Type::ULong : Type::UInt;
   }
   
   expect(TokenType::Right_Paren, "Expected closing parenthesis.");
   cast->type = parse_type(types);
   cast->expr = parse_factor();

   expr = cast;
  } else {
   expr = parse_expression();
   expect(TokenType::Right_Paren, "Expected closing parenthesis.");
  }
 } else {
  error_at_line(cur.line, "Not an expression! " + cur.to_string());
 }
  
 TokenType next_token_type = peek().type;
 while (next_token_type == TokenType::Plus_Plus || next_token_type == TokenType::Minus_Minus) {
  consume();
  Unary *unary = new Unary();
  unary->postfix = true;
  unary->op = next_token_type == TokenType::Plus_Plus ? UnaryOp::Increment : UnaryOp::Decrement;
  unary->expr = expr;

  expr = unary;
  next_token_type = peek().type;
 }

 return expr;
}