#pragma once
#include "../parser.h"
#include "../../helpers.h"
using namespace Parser;

void CParser::typecheck() {
 for (Declaration &decl : program.decls) {
   std::visit(overloaded{
    [&](VarDecl &var) {
     typecheck_file_scope(var);
    },
    [&](FuncDecl &func) {
     curr_func = &func;
     typecheck(func);
    }
   }, decl);
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

void convert_to(Expression *&expr, Type type) {
 if (expr == nullptr || expr->type == type) {
  return;
 }
 
 Cast *cast = new Cast();
 cast->type = type;
 cast->expr = expr;

 expr = cast;
}

void CParser::typecheck(VarDecl &var) {
 TypeEntry entry = {.type = var.tasc.type};
 string var_name = var.name.to_string();

 switch (var.tasc.storage_class) {
  case StorageClass::Extern: {
   if (var.init != nullptr) {
    error_at_line(var.name.line, "Initializer on local extern variable declaration.");
   }

   entry.attr_type = AttrType::Static;
   entry.global = true;
   entry.init_val_type = InitValType::None;

   if (!symbols.count(var_name)) {
    symbols[var_name] = entry;
   } else if (symbols[var_name].type == Type::Function) {
    error_at_line(var.name.line, "Function redeclared as variable!");
   } else if (symbols[var_name].type != entry.type) {
    error_at_line(var.name.line, "Extern variable redeclared with different type!");
   }
  } break;
  case StorageClass::Static: {
   entry.attr_type = AttrType::Static;
   entry.global = false;
   entry.init_val_type = !var.init || var.init->type == Type::Int ? InitValType::InitInt : InitValType::InitLong;

   if (var.init == nullptr) {
    entry.init_val = "0";
   } else if (var.init->is_const()) {
    entry.init_val = static_cast<Constant *>(var.init)->_const;
   } else {
    error_at_line(var.name.line, "Non-constant initializer on local static variable!");
   }

   symbols[var_name] = entry;
  } break;
  default: {
   entry.global = false;
   entry.init_val_type = !var.init || var.init->type == Type::Int ? InitValType::InitInt : InitValType::InitLong;

   symbols[var_name] = entry;
   typecheck(var.init);
   convert_to(var.init, var.tasc.type);
  }
 }
}

bool is_initial(InitValType type) {
 return type == InitValType::InitInt || type == InitValType::InitLong;
}

void CParser::typecheck_file_scope(VarDecl &var) {
 TypeEntry entry;
 entry.type = var.tasc.type;
 entry.global = var.tasc.storage_class != StorageClass::Static;

 if (var.init == nullptr) {
  entry.init_val_type = var.tasc.storage_class == StorageClass::Extern ? InitValType::None : InitValType::Tentative; 
 } else if (var.init->is_const()) {
  entry.init_val = static_cast<Constant *>(&*var.init)->_const;
  entry.init_val_type = var.init->type == Type::Int ? InitValType::InitInt : InitValType::InitLong;
 } else {
  error_at_line(var.name.line, "Non-constant initializer!");
 }

 if (symbols.count(var.name.to_string())) {
  TypeEntry old_entry = symbols[var.name.to_string()];

  if (old_entry.type == Type::Function) {
   error_at_line(var.name.line, "Function redeclared as variable!");
  } else if (old_entry.type != entry.type) {
   error_at_line(var.name.line, "Global variable redeclared with different type!");
  }

  if (var.tasc.storage_class == StorageClass::Extern) {
   entry.global = old_entry.global;
  } else if (entry.global != old_entry.global) {
   error_at_line(var.name.line, "Conflicting variable linkage!");
  }

  if (is_initial(old_entry.init_val_type)) {
   if (is_initial(entry.init_val_type)) {
    error_at_line(var.name.line, "Conflicting file scope variable definitions.");
   } else {
    entry.init_val_type = old_entry.init_val_type;
    entry.init_val = old_entry.init_val;
   }
  } else if (!is_initial(entry.init_val_type) && old_entry.init_val_type == InitValType::Tentative) {
   entry.init_val_type = InitValType::Tentative;
  }
 }

 entry.attr_type = AttrType::Static;
 symbols[var.name.to_string()] = entry;
}

void CParser::typecheck(FuncDecl &func) {
 std::vector<Type> param_types = func.param_types;
 Type ret_type = func.ret.type;
 bool has_body = func.body != nullptr;
 bool already_defined = false;
 bool global = func.ret.storage_class != StorageClass::Static;
 string func_name = func.name.to_string(); 

 if (symbols.count(func_name)) {
  TypeEntry old_decl = symbols[func_name];

  if (!(old_decl.type == Type::Function && old_decl.ret_type == ret_type && old_decl.param_types == param_types)) {
   error_at_line(func.name.line, "Incompatible function declarations.");
  }

  already_defined = old_decl.defined;
  if (already_defined && has_body) {
   error_at_line(func.name.line, func_name + " is already defined.");
  }

  if (old_decl.global && func.ret.storage_class == StorageClass::Static) {
   error_at_line(func.name.line, "Static function declaration follows non-static");
  }

  global = old_decl.global;
 }

 symbols[func_name] = {
  .type = Type::Function,
  .ret_type = func.ret.type,
  .param_types = param_types,
  .global = global,
  .defined = already_defined || has_body,
 };

 if (has_body) {
  for (int i = 0; i < func.params.size(); i++) {
   Token param = func.params[i];
   Type type = func.param_types[i];
   
   symbols[param.to_string()] = TypeEntry{.type = type};
  }

  typecheck(*func.body);
 }
}

void CParser::typecheck(Statement *stmt) {
 if (stmt == nullptr) return;

 typecheck(*stmt);
}

bool is_signed(Type t) {
 return t > Type::ULong;
}

int get_type_size(Type t) {
 switch (t) {
  case Type::Int:
  case Type::UInt: return 1;
  case Type::Long:
  case Type::ULong: return 2;
  default: return -1;
 }
}

Type get_common_type(Type t1, Type t2) {
 if (t1 == t2) return t1;
 if (get_type_size(t1) == get_type_size(t2)) {
  return is_signed(t1) ? t2 : t1;
 }

 return get_type_size(t1) > get_type_size(t2) ? t1 : t2;
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

   convert_to(ret.expr, curr_func->ret.type);
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
   for (Case *_case : swtch.cases) {
    _case->expr->type = swtch.expr->type;
   }

   typecheck(swtch.body);
  },
  [&](Case &_case) {
   typecheck(_case.expr);
   typecheck(_case.stmt);
  },
  [&](ExpressionStatement &expr) {
   typecheck(expr.expr);
  }
 }, stmt);
}

void CParser::typecheck(ForInit &init) {
 std::visit(overloaded{
  [&](VarDecl *var) {
   if (var->tasc.storage_class == StorageClass::Static) {
    error_at_line(var->name.line, "init decl cannot have external linkage.");
   }
   
   typecheck(*var);
  },
  [&](Expression* expr) {
   typecheck(expr);
  }
 }, init);
}

class TypecheckingExpressionVisitor : public ExpressionVisitor {
 private:
  SymbolTable *symbols;

 public:
  TypecheckingExpressionVisitor(SymbolTable &symbols) {
   this->symbols = &symbols;
  }

  void visit(Constant *_const) {
   if (!(_const->type == Type::Int || _const->type == Type::UInt)) return;

   _const->_const = std::to_string(truncate(std::stoull(_const->_const)));
  }
  
  void visit(Var *var) {
   var->type = (*symbols)[var->name.to_string()].type;
   if (var->type == Type::Function) {
    error_at_line(var->name.line, "Function name used as a variable.");
   }
  }
  
  void visit(FunctionCall *call) {
   TypeEntry entry = (*symbols)[call->name.to_string()];
   call->type = entry.ret_type;
   int line = call->name.line;

   if (entry.type != Type::Function) error_at_line(line, "Variable used as function name."); 
   if (entry.param_types.size() != call->args.size()) {
    error_at_line(line,
     call->name.to_string()
     + " called with "
     + std::to_string(call->args.size())
     + " arguments instead of "
     + std::to_string(entry.param_types.size())
    );
   }

   for (int i = 0; i < call->args.size(); i++) {
    Expression *&arg = call->args[i];
    Type type = entry.param_types[i];
    
    arg->accept(this);
    convert_to(arg, type);
    arg->type = type;
   } 
  }
  
  void visit(Unary *un) {
   un->expr->accept(this);

   if (un->op == UnaryOp::Not) {
    un->type = Type::Int;
   } else un->type = un->expr->type;
  }
  
  void visit(Binary *bin) {
   bin->left->accept(this);
   bin->right->accept(this);

   if (bin->op == BinaryOp::And || bin->op == BinaryOp::Or) {
    bin->type = Type::Int;
   } else if (bin->op == BinaryOp::Shift_Left || bin->op == BinaryOp::Shift_Right) {
    bin->type = bin->left->type;
   } else {
    Type common_type = get_common_type(bin->left->type, bin->right->type);
    convert_to(bin->left, common_type);
    convert_to(bin->right, common_type);

    bin->type = common_type;
   }
  }
  
  void visit(Assignment *assign) {
   assign->left->accept(this);
   assign->right->accept(this);

   if (assign->op == BinaryOp::Equal || assign->op == BinaryOp::Shift_Left || assign->op == BinaryOp::Shift_Right) {
    convert_to(assign->right, assign->left->type);
    assign->type = assign->left->type;
   } else {
    Type common_type = get_common_type(assign->left->type, assign->right->type);
    convert_to(assign->left, common_type);
    convert_to(assign->right, common_type);

    assign->type = common_type;
   }
  }
  
  void visit(Conditional *cond) {
   cond->condition->accept(this);
   cond->left->accept(this);
   cond->right->accept(this);

   cond->type = get_common_type(cond->left->type, cond->right->type);
  }
  
  void visit(Cast *cast) {
   cast->expr->accept(this);
  }
};

void CParser::typecheck(Expression *expr) {
 if (expr == nullptr) return;
 TypecheckingExpressionVisitor visitor(symbols);

 expr->accept(&visitor);
}