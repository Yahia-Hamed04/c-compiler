#pragma once
#include "../parser.h"
#include "../../helpers.h"
using namespace Parser;

void CParser::resolve_idents() {
 for (FuncDecl &func : program.functions) {
  curr_func = &func;
  
  resolve_idents(func, false);
 }
}

void CParser::resolve_idents(Block &block) {
 for (Block_Item &item : block.items) {
  resolve_idents(item);
 }
}

void CParser::resolve_idents(Block_Item &item) {
 std::visit(overloaded{
  [&](Declaration &decl) {
   resolve_idents(decl);
  },
  [&](Statement &stmt) {
   resolve_idents(stmt);
  },
 }, item);
}

void CParser::resolve_idents(Declaration &decl) {
 std::visit(overloaded{
  [&](FuncDecl &decl) {
   resolve_idents(decl);
  },
  [&](VarDecl &decl) {
   resolve_idents(decl);
  }
 }, decl);
}

void CParser::resolve_idents(FuncDecl &decl, bool in_block) {
 string func_name = decl.name.to_string();
 if (idents.count(func_name)) {
  MapEntry entry = idents[func_name];

  if (entry.from_current_scope && !entry.has_linkage) {
   error_at_line(decl.name.line, "Duplicate function declaration for \"" + func_name + "\".");
  }
 }

 MapEntry new_func = make_var(func_name, true);
 idents[func_name] = new_func;
 decl.name = new_func.name;

 std::unordered_map<string, MapEntry> old_idents;
 old_idents = idents;
 new_scope();

 for (Token &param : decl.params) {
  VarDecl param_decl = {.name = param, .init = std::nullopt};

  resolve_idents(param_decl);
  param = param_decl.name;
 }

 if (decl.body != nullptr) {
  if (in_block) {
   error_at_line(decl.name.line, "Functions cannot be defined in a local scope.");
  }

  resolve_idents(*decl.body);
 }

 idents = old_idents;
}

void CParser::resolve_idents(VarDecl &decl) {
 string var_name = decl.name.to_string();
 if (idents.count(var_name) && idents[var_name].from_current_scope) {
  error_at_line(decl.name.line, "Duplicate variable declaration for \"" + var_name + "\".");
 }

 MapEntry new_var = make_var(var_name);
 idents[var_name] = new_var;
 decl.name = new_var.name;

 resolve_idents(decl.init);
}

void CParser::resolve_idents(Statement *stmt) {
 if (stmt == nullptr) return;

 resolve_idents(*stmt);
}

void CParser::resolve_idents(Statement &stmt) {
 std::visit(overloaded{
  [&](auto _) {},
  [&](CompoundStatement stmts) {
   std::unordered_map<string, MapEntry> old_idents;
   old_idents = idents;
   new_scope();

   resolve_idents(*stmts.block);

   idents = old_idents;   
  },
  [&](Return &ret) {
   resolve_idents(ret.expr);
  },
  [&](If &if_stmt) {
   resolve_idents(if_stmt.condition);
   resolve_idents(if_stmt.then);
   resolve_idents(if_stmt._else);
  },
  [&](While &while_stmt) {
   resolve_idents(while_stmt.condition);
   resolve_idents(while_stmt.body);
  },
  [&](DoWhile &do_while_stmt) {
   resolve_idents(do_while_stmt.body);
   resolve_idents(do_while_stmt.condition);
  },
  [&](For &for_stmt) {
   std::unordered_map<string, MapEntry> old_idents;
   old_idents = idents;
   new_scope();

   resolve_idents(for_stmt.init);
   resolve_idents(for_stmt.condition);
   resolve_idents(for_stmt.post);
   resolve_idents(for_stmt.body);

   idents = old_idents;   
  },
  [&](Switch &swtch) {
   resolve_idents(swtch.expr);
   resolve_idents(swtch.body);
  },
  [&](Case &_case) {
   if (!(_case.is_default || std::holds_alternative<Constant>(_case.expr))) {
    error("case argument must be a constant.");
   }

   resolve_idents(_case.stmt);
  },
  [&](Goto &goto_stmt) {
   string target_name = goto_stmt.target.name.to_string();

   if (!curr_func->labels.count(target_name)) {
    error_at_line(goto_stmt.target.name.line, "Undeclared label \"" + target_name + "\"!");
   }

   goto_stmt.target.name = curr_func->labels[target_name];
  },
  [&](Label &label) {
   label.name = curr_func->labels[label.name.to_string()];
   resolve_idents(label.stmt);
  },
  [&](Expression &expr) {
   resolve_idents(expr);
  },
 }, stmt);
}

void CParser::resolve_idents(ForInit &init) {
 std::visit(overloaded{
  [&](VarDecl *decl) {
   resolve_idents(*decl);
  },
  [&](std::optional<Expression> &expr) {
   resolve_idents(expr);
  }
 }, init);
}

void CParser::resolve_idents(std::optional<Expression> &opt_expr) {
 if (opt_expr == std::nullopt) return;

 resolve_idents(*opt_expr);
}

void CParser::resolve_idents(Expression *expr) {
 if (expr == nullptr) return;

 resolve_idents(*expr);
}

void CParser::resolve_idents(Expression &expr) {
 std::visit(overloaded{
  [&](Constant _) {},
  [&](Var &var) {
   string var_name = var.name.to_string();

   if (!idents.count(var_name)) {
    error_at_line(var.name.line, "Undeclared variable \"" + var_name + "\"!");
   }

   var.name = idents[var_name].name;
  },
  [&](Unary &un) {
   bool valid_lvalue = std::holds_alternative<Var>(*un.expr);
   if ((un.op == UnaryOp::Increment || un.op == UnaryOp::Decrement) && !valid_lvalue) {
    error("Invalid lvalue");
   }

   resolve_idents(un.expr);
  },
  [&](Binary &bin) {
   resolve_idents(bin.left);
   resolve_idents(bin.right);
  },
  [&](Assignment &assign) {
   bool valid_lvalue = std::holds_alternative<Var>(*assign.left);
   if (!valid_lvalue) error("Invalid lvalue");

   resolve_idents(assign.left);
   resolve_idents(assign.right);
  },
  [&](Conditional &conditional) {
   resolve_idents(conditional.condition);
   resolve_idents(conditional.left);
   resolve_idents(conditional.right);
  },
  [&](FunctionCall &call) {
   string func_name = call.name.to_string();
   if (!idents.count(func_name)) {
    error_at_line(call.name.line, "Undeclared function \"" + func_name + "\"!");
   }
   
   call.name = idents[func_name].name;
   for (Expression *arg : call.args) {
    resolve_idents(arg);
   }
  }
 }, expr);
}