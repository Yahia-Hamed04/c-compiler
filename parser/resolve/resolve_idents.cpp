#pragma once
#include "../parser.h"
#include "../../helpers.h"
using namespace Parser;

void CParser::resolve_idents() {
 for (Declaration &decl : program.decls) {
  resolve_idents(decl, false);
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

void CParser::resolve_idents(Declaration &decl, bool in_block) {
 std::visit(overloaded{
  [&](FuncDecl &decl) {
   curr_func = &decl;
   
   resolve_idents(decl, in_block);
  },
  [&](VarDecl &decl) {
   resolve_idents(decl, in_block);
  }
 }, decl);
}

void CParser::resolve_idents(FuncDecl &decl, bool in_block) {
 if (in_block) {
  if (decl.ret.storage_class == StorageClass::Static) {
   error_at_line(decl.name.line, "Static function declarations must be global.");
  } else if (decl.body != nullptr) {
   error_at_line(decl.name.line, "Functions cannot be defined in a local scope.");
  }  
 }

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
  VarDecl param_decl = {.name = param, .init = nullptr};

  resolve_idents(param_decl, true);
  param = param_decl.name;
 }

 if (decl.body != nullptr) {
  resolve_idents(*decl.body);
 }

 idents = old_idents;
}

void CParser::resolve_idents(VarDecl &decl, bool in_block) {
 string var_name = decl.name.to_string();
 if (!in_block) {
  idents[var_name] = {
   .name = decl.name,
   .from_current_scope = true,
   .has_linkage = true
  };
 } else {
  if (idents.count(var_name)) {
   MapEntry entry = idents[var_name];

   if (entry.from_current_scope && !(entry.has_linkage && decl.tasc.storage_class == StorageClass::Extern)) {
    error_at_line(decl.name.line, "conflicting local variable declaration for \"" + var_name + "\".");
   }
  }

  if (decl.tasc.storage_class == StorageClass::Extern) {
   idents[var_name] =  {
    .name = decl.name,
    .from_current_scope = true,
    .has_linkage = true
   };
  } else {
   MapEntry new_var = make_var(var_name);
   idents[var_name] = new_var;
   decl.name = new_var.name;
   resolve_idents(decl.init);
  }
 }
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
   if (!(_case.expr == nullptr || _case.expr->is_const())) {
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
  [&](ExpressionStatement expr) {
   resolve_idents(expr.expr);
  },
 }, stmt);
}

void CParser::resolve_idents(ForInit &init) {
 std::visit(overloaded{
  [&](VarDecl *decl) {
   resolve_idents(*decl, true);
  },
  [&](Expression *expr) {
   resolve_idents(expr);
  }
 }, init);
}

class ResolvingExpessionVisitor : public ExpressionVisitor {
 private:
  std::unordered_map<string, MapEntry> *idents;

 public:
  ResolvingExpessionVisitor(std::unordered_map<string, MapEntry> &idents) {
   this->idents = &idents;
  }

  void visit(Constant *_) {}

  void visit(Var *var) {
   string var_name = var->name.to_string();

   if (!idents->count(var_name)) {
    error_at_line(var->name.line, "Undeclared variable \"" + var_name + "\"!");
   }

   var->name = (*idents)[var_name].name;
  }

  void visit(Unary *un) {
   bool valid_lvalue = un->expr->is_var();
   if ((un->op == UnaryOp::Increment || un->op == UnaryOp::Decrement) && !valid_lvalue) {
    error("Invalid lvalue");
   }

   un->expr->accept(this);
  }

  void visit(Binary *bin) {
   bin->left->accept(this);
   bin->right->accept(this);
  }

  void visit(Assignment *assign) {
   bool valid_lvalue = assign->left->is_var();
   if (!valid_lvalue) error("Invalid lvalue");

   assign->left->accept(this);
   assign->right->accept(this);
  }

  void visit(Conditional *conditional) {
   conditional->condition->accept(this);
   conditional->left->accept(this);
   conditional->right->accept(this);
  }

  void visit(FunctionCall *call) {
   string func_name = call->name.to_string();
   if (!idents->count(func_name)) {
    error_at_line(call->name.line, "Undeclared function \"" + func_name + "\"!");
   }
   
   call->name = (*idents)[func_name].name;
   for (Expression *arg : call->args) {
    arg->accept(this);
   }
  }

  void visit(Cast *cast) {
   cast->expr->accept(this);
  }
};

void CParser::resolve_idents(Expression *expr) {
 if (expr == nullptr) return;
 ResolvingExpessionVisitor visitor(idents);

 expr->accept(&visitor);
}