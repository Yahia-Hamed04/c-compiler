#pragma once
#include "parser.h"
#include "../helpers.h"
#include <unordered_set>
using namespace Parser;

void CParser::resolve_labels() {
 for (FuncDecl func : program.functions) {
  resolve_labels(*func.body);
 }
}

void CParser::resolve_labels(Block &block) {
 for (Block_Item &block_item : block.items) {
  Statement *stmt = std::get_if<Statement>(&block_item);
  if (stmt != nullptr) {
   resolve_labels(*stmt);
  }
 }
}

void CParser::resolve_labels(Statement &stmt) {
 std::visit(overloaded{
  [](auto _) {},
  [&](CompoundStatement stmts) {
   resolve_labels(*stmts.block);
  },
  [&](Label &label) {
   resolve_labels(label);
   resolve_labels(*label.stmt);
  },
  [&](If &if_stmt) {
   resolve_labels(*if_stmt.then);
   if (if_stmt._else != nullptr) {
    resolve_labels(*if_stmt._else);
   }
  },
  [&](While &while_stmt) {
   resolve_labels(*while_stmt.body);
  },
  [&](DoWhile &do_while_stmt) {
   resolve_labels(*do_while_stmt.body);
  },
  [&](For &for_stmt) {
   resolve_labels(*for_stmt.body);
  },
  [&](Switch &swtch) {
   resolve_labels(*swtch.body);
  },
  [&](Case &_case) {
   resolve_labels(*_case.stmt);
  }
 }, stmt);
}

void CParser::resolve_labels(Label &label) {
 string label_name = label.name.to_string();
 label_count++;
 
 if (labels.count(label_name)) {
  error_at_line(label.name.line, "Duplicate label declaration for \"" + label_name + "\".");
 }

 labels.insert(label_name);
}

void CParser::resolve_idents() {
 for (FuncDecl func : program.functions) {
  resolve_idents(*func.body);
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
 if (vars.count(func_name) && vars[func_name].from_current_block) {
  error_at_line(decl.name.line, "Duplicate function declaration for \"" + func_name + "\".");
 }

 MapEntry new_func = make_var(func_name, true);
 vars[func_name] = new_func;
 decl.name = new_func.name;

 if (decl.body != nullptr) {
  if (in_block) {
   error_at_line(decl.name.line, "Functions cannot be defined in a local scope.");
  }

  resolve_idents(*decl.body);
 }
}

void CParser::resolve_idents(VarDecl &decl) {
 string var_name = decl.name.to_string();
 if (vars.count(var_name) && vars[var_name].from_current_block) {
  error_at_line(decl.name.line, "Duplicate variable declaration for \"" + var_name + "\".");
 }

 MapEntry new_var = make_var(var_name);
 vars[var_name] = new_var;
 decl.name = new_var.name;

 if (decl.init != std::nullopt) {
  resolve_idents(decl.init);
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
   std::unordered_map<string, MapEntry> old_vars;
   old_vars = vars;
   
   for (auto& [_, var] : vars) {
    var.from_current_block = false;
   }

   for (Block_Item &item : stmts.block->items) {
    resolve_idents(item);
   }

   vars = old_vars;
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
   std::unordered_map<string, MapEntry> old_vars;
   old_vars = vars;
   
   for (auto& [_, var] : vars) {
    var.from_current_block = false;
   }

   resolve_idents(for_stmt.init);
   resolve_idents(for_stmt.condition);
   resolve_idents(for_stmt.post);
   resolve_idents(for_stmt.body);

   vars = old_vars;   
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

   if (!labels.count(target_name)) {
    error_at_line(goto_stmt.target.name.line, "Undeclared label \"" + target_name + "\"!");
   }
  },
  [&](Label &label) {
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

 resolve_idents(opt_expr);
}

void CParser::resolve_idents(Expression *expr) {
 if (expr == nullptr) return;

 resolve_idents(expr);
}

void CParser::resolve_idents(Expression &expr) {
 std::visit(overloaded{
  [&](Constant _) {},
  [&](Var &var) {
   string var_name = var.name.to_string();

   if (!vars.count(var_name)) {
    error_at_line(var.name.line, "Undeclared variable \"" + var_name + "\"!");
   }

   var.name = vars[var_name].name;
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
   if (!vars.count(func_name)) {
    error_at_line(call.name.line, "Undeclared function \"" + func_name + "\"!");
   } else if (!vars[func_name].is_function) {
    error_at_line(call.name.line, "\"" + func_name + "\" is not a function!");
   }
   
   for (Expression *arg : call.args) {
    resolve_idents(arg);
   }
  }
 }, expr);
}

void CParser::label_statement() {
 Token null_token = {.length = 0};
 
 for (FuncDecl func : program.functions) {
  label_statement(*func.body, null_token);
 }
}

void CParser::label_statement(Block &block, Token current_label, Switch *curr_swtch, bool in_switch) {
 for (Block_Item &block_item : block.items) {
  Statement *stmt = std::get_if<Statement>(&block_item);
  if (stmt != nullptr) {
   label_statement(*stmt, current_label, curr_swtch, in_switch);
  }
 }
}

void CParser::label_statement(Statement *stmt, Token current_label, Switch *curr_swtch, bool in_switch) {
 if (stmt == nullptr) return;

 label_statement(*stmt, current_label, curr_swtch, in_switch);
}
void CParser::label_statement(Statement &stmt, Token current_label, Switch *curr_swtch, bool in_switch) {
 std::visit(overloaded{
  [&](auto _) {},
  [&](CompoundStatement stmts) {
   label_statement(*stmts.block, current_label, curr_swtch, in_switch);
  },
  [&](If &if_stmt) {
   label_statement(if_stmt.then, current_label, curr_swtch, in_switch);
   label_statement(if_stmt._else, current_label, curr_swtch, in_switch);
  },
  [&](While &while_stmt) {
   Token new_label = make_label();

   while_stmt.label = new_label;
   label_statement(while_stmt.body, new_label, curr_swtch);
  },
  [&](DoWhile &do_while_stmt) {
   Token new_label = make_label();

   do_while_stmt.label = new_label;
   label_statement(do_while_stmt.body, new_label, curr_swtch);
  },
  [&](For &for_stmt) {
   Token new_label = make_label();

   for_stmt.label = new_label;
   label_statement(for_stmt.body, new_label, curr_swtch);
  },
  [&](Switch &swtch) {
   swtch.label = make_label();

   label_statement(swtch.body, current_label, &swtch, true);

   std::unordered_set<string> case_consts;

   for (Case *_case : swtch.cases) {
    string const_str = std::get<Constant>(_case->expr)._const;
    
    if (case_consts.count(const_str)) {
     error("Duplicate case");
    }

    case_consts.insert(const_str);
   }
  },
  [&](Case &_case) {
   if (curr_swtch == nullptr) {
    error("case statement outside of switch.");
   }

   _case.label = curr_swtch->label;
   curr_swtch->cases.push_back(&_case);
   label_statement(_case.stmt, current_label, curr_swtch, true);
  },
  [&](Continue &cont) {
   if (current_label.length == 0) {
    error("continue statement outside of loop.");
   }

   cont.label = current_label;
  },
  [&](Break &brk) {
   if (current_label.length == 0 && curr_swtch == nullptr) {
    error("break statement outside of loop or switch.");
   }

   brk.label = curr_swtch != nullptr && in_switch ? curr_swtch->label : current_label;
  },
  [&](Label &label) {
   label_statement(label.stmt, current_label, curr_swtch);
  }
 }, stmt);
}