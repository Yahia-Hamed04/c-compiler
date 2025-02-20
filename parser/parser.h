#pragma once
#include <unordered_map>
#include <unordered_set>
#include "../lexer/lexer.h"
#include "types.h"

namespace Parser {
 struct MapEntry {
  Token name;
  bool from_current_scope;
  bool has_linkage;
 };

 enum class Type {
  Int, 
  Function
 };

 struct TypeEntry {
  Type type;
  int params_count = 0;
  bool defined = false;
 };

 using SymbolTable = std::unordered_map<string, TypeEntry>;
 
 class CParser {
  private:
   Lexer *lexer;
   int token_index;
   int var_count, label_count;
   FuncDecl *curr_func;
   std::unordered_map<string, MapEntry> idents;
   Program program;
 
   Token peek(int n = 0);
   Token consume();
   bool did_consume(TokenType type);
   Token expect(TokenType type, string err_msg);

   MapEntry make_var(string prefix, bool has_linkage = false);
   Token make_label(string prefix = "");
 
   void parse();
   FuncDecl parse_function();
   Block parse_block();
   Block_Item parse_block_item();
   Declaration parse_declaration();
   Statement parse_statement();
   Expression parse_condition();
   Expression parse_expression(int min_prec = 0);
   Expression parse_conditional_middle();
   Expression parse_factor();

   void resolve_labels();
   void resolve_labels(Block &block);
   void resolve_labels(Label &label);
   void resolve_labels(Statement &stmt);

   void new_scope();
   void restore_scope();
   
   void resolve_idents();
   void resolve_idents(Block &item);
   void resolve_idents(Block_Item &item);
   void resolve_idents(Declaration &decl);
   void resolve_idents(FuncDecl &decl, bool in_block = true);
   void resolve_idents(VarDecl &decl);
   void resolve_idents(ForInit &init);
   void resolve_idents(Statement &stmt);
   void resolve_idents(Statement *stmt);
   void resolve_idents(std::optional<Expression> &opt_expr);
   void resolve_idents(Expression *expr);
   void resolve_idents(Expression &expr);

   void label_statement();
   void label_statement(Block &block, Token current_label, Switch *curr_swtch = nullptr, bool in_switch = false);
   void label_statement(Statement &stmt, Token current_label, Switch *curr_swtch = nullptr, bool in_switch = false);
   void label_statement(Statement *stmt, Token current_label, Switch *curr_swtch = nullptr, bool in_switch = false);
   
   void typecheck();
   void typecheck(Block &block);
   void typecheck(Block_Item &item);
   void typecheck(Declaration &decl);
   void typecheck(VarDecl &var);
   void typecheck(FuncDecl &func);
   void typecheck(Statement *stmt);
   void typecheck(Statement &stmt);
   void typecheck(ForInit &init);
   void typecheck(std::optional<Expression> &expr);
   void typecheck(Expression *expr);
   void typecheck(Expression &expr);

  public:
   SymbolTable symbols;
   CParser() = delete;
   CParser(Lexer &lexer, bool resolve);
 
   Program get_program();
   int get_var_count();
 };
}