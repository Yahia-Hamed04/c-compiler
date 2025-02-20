#pragma once
#include "../parser.h"
#include "../../helpers.h"
using namespace Parser;

void CParser::typecheck() {
 for (FuncDecl &func : program.functions) {
  curr_func = &func;
  
  typecheck(func);
 }
}

void CParser::typecheck(Block &block) {
 for (Block_Item &item : block.items) {
  typecheck(item);
 }
}

void CParser::typecheck(Block_Item &item) {
 std::visit(overloaded{
  [&](Declaration &decl) {
   typecheck(decl);
  },
  [&](Statement &stmt) {
   typecheck(stmt);
  }
 }, item);
}

void CParser::typecheck(Declaration &decl) {
 std::visit(overloaded{
  [&](VarDecl &var) {
   typecheck(var);
  },
  [&](FuncDecl &func) {
   typecheck(func);
  }
 }, decl);
}

void CParser::typecheck(VarDecl &var) {
 symbols[var.name.to_string()] = TypeEntry{.type = Type::Int};

 typecheck(var.init);
}

void CParser::typecheck(FuncDecl &func) {
 int params_count = func.params.size();
 bool has_body = func.body != nullptr;
 bool already_defined = false;
 string func_name = func.name.to_string(); 

 if (symbols.count(func_name)) {
  TypeEntry old_decl = symbols[func_name];

  if (!(old_decl.type == Type::Function && old_decl.params_count == params_count)) {
   error_at_line(func.name.line, "Incompatible function declarations.");
  }

  already_defined = old_decl.defined;
  if (already_defined && has_body) {
   error_at_line(func.name.line, func_name + " is already defined.");
  }
 }

 symbols[func_name] = TypeEntry{.type = Type::Function, .params_count = params_count, .defined = already_defined || has_body};

 if (has_body) {
  for (Token &param : func.params) {
   symbols[param.to_string()] = TypeEntry{.type = Type::Int};
  }

  typecheck(*func.body);
 }
}

void CParser::typecheck(Statement *stmt) {
 if (stmt == nullptr) return;

 typecheck(*stmt);
}

void CParser::typecheck(Statement &stmt) {
 std::visit(overloaded{
  [](auto _) {},
  [&](CompoundStatement stmts) {
   typecheck(*stmts.block);
  },
  [&](Label &label) {
   typecheck(label.stmt);
  },
  [&](Return &ret) {
   typecheck(ret.expr);
  },
  [&](If &if_stmt) {
   typecheck(if_stmt.condition);
   typecheck(if_stmt.then);
   typecheck(if_stmt._else);
  },
  [&](While &while_stmt) {
   typecheck(while_stmt.condition);
   typecheck(while_stmt.body);
  },
  [&](DoWhile &do_while_stmt) {
   typecheck(do_while_stmt.condition);
   typecheck(do_while_stmt.body);
  },
  [&](For &for_stmt) {
   typecheck(for_stmt.init);
   typecheck(for_stmt.condition);
   typecheck(for_stmt.post);
   typecheck(for_stmt.body);
  },
  [&](Switch &swtch) {
   typecheck(swtch.expr);
   typecheck(swtch.body);
  },
  [&](Case &_case) {
   typecheck(_case.stmt);
  },
  [&](Expression &expr) {
   typecheck(expr);
  }
 }, stmt);
}

void CParser::typecheck(ForInit &init) {
 std::visit(overloaded{
  [&](VarDecl *var) {
   typecheck(*var);
  },
  [&](std::optional<Expression> expr) {
   typecheck(expr);
  }
 }, init);
}

void CParser::typecheck(std::optional<Expression> &expr) {
 if (expr == std::nullopt) return;

 typecheck(*expr);
}

void CParser::typecheck(Expression *expr) {
 if (expr == nullptr) return;

 typecheck(*expr);
}

void CParser::typecheck(Expression &expr) {
 std::visit(overloaded{
  [&](Constant _) {},
  [&](Var &var) {
   if (symbols[var.name.to_string()].type != Type::Int) {
    error_at_line(var.name.line, "Function name used as a variable.");
   }
  },
  [&](FunctionCall &call) {
   TypeEntry entry = symbols[call.name.to_string()];
   int line = call.name.line;

   if (entry.type != Type::Function) error_at_line(line, "Variable used as function name."); 
   if (entry.params_count != call.args.size()) {
    string err_str = "Function called with the wrong number of arguments.";
    err_str += " (" + std::to_string(call.args.size()) + " instead of " + std::to_string(entry.params_count) + ")";
    
    error_at_line(line, err_str);
   }

   for (Expression *arg : call.args) {
    typecheck(arg);
   }
  },
  [&](Unary &un) {
   typecheck(un.expr);
  },
  [&](Binary &bin) {
   typecheck(bin.left);
   typecheck(bin.right);
  },
  [&](Assignment &assign) {
   typecheck(assign.left);
   typecheck(assign.right);
  },
  [&](Conditional &cond) {
   typecheck(cond.condition);
   typecheck(cond.left);
   typecheck(cond.right);
  }
 }, expr);
}