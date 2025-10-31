#include "tacky.h"
#include "../helpers.h"
using namespace TACKY;

TACKYifier::TACKYifier(Parser::CParser &parser) {
 this->parser = &parser;
 this->symbols = &parser.symbols;
 this->temp_var_count = parser.get_var_count();
 this->label_count = 0;

 tackyify();
}

TACKY::Program TACKYifier::get_program() {
 return program;
}

Var TACKYifier::make_temporary(bool increment_var_count) {
 Token tmp = {.type = TokenType::Identifier, .start = new char[10], .line = 0};
 tmp.length = (size_t)sprintf(tmp.start, "tmp.%d", temp_var_count);
 if (increment_var_count) temp_var_count++;

 return Var(tmp);
}

Var TACKYifier::make_tacky_var(Parser::Type type, bool increment_var_count) {
 Var var = make_temporary(increment_var_count);
 var.type = type;
 
 (*symbols)[var.name.to_string()] = {.type = type, .attr_type = Parser::AttrType::Local};

 return var;
}

Var TACKYifier::make_label(string prefix) {
 string format_str = prefix + "%d";

 Token tmp = {.type = TokenType::Identifier, .start = new char[prefix.size() + 10], .line = 0};
 tmp.length = (size_t)sprintf(tmp.start, format_str.data(), label_count++);

 return Var(tmp);
}

Var TACKYifier::make_label(string prefix, string suffix) {
 string format_str = prefix + suffix;

 Token tmp = {.type = TokenType::Identifier, .start = new char[prefix.size() + suffix.size()], .line = 0};
 tmp.length = (size_t)sprintf(tmp.start, "%s", format_str.data());

 return Var(tmp);
}

void TACKYifier::tackyify() {
 Parser::Program parser_program = parser->get_program();
 for (Parser::Declaration decl : parser_program.decls) {
  if (Parser::FuncDecl *func = std::get_if<Parser::FuncDecl>(&decl); func) {
   tackyify(*func);
  }
 }

 for (auto& [name, entry] : *symbols) {
  if (entry.attr_type != Parser::AttrType::Static) continue;
  Token tmp = {.type = TokenType::Identifier, .start = new char[name.length() + 1], .line = 0};
  tmp.length = (size_t)sprintf(tmp.start, "%s", name.data());
   
  StaticVariable var = {
   .name = tmp,
   .type = entry.type,
   .global = entry.global
  };
  
  var.init_val_type = entry.init_val_type;
  switch (entry.init_val_type) {
    case Parser::InitValType::InitInt: 
    case Parser::InitValType::InitLong: {
     var.init = std::stoull(entry.init_val);
     program.statics.push_back(var);
    } break;
    case Parser::InitValType::Tentative: {
     if (entry.type == Parser::Type::Long) {
      var.init_val_type = Parser::InitValType::InitLong;
     } else {
      var.init_val_type = Parser::InitValType::InitInt;
     }
     var.init = 0;
     program.statics.push_back(var);
    } break;
    case Parser::InitValType::None: {}
   }
 }
}


void TACKYifier::tackyify(Parser::FuncDecl function) {
 if (function.body == nullptr) return;
 
 Function func;
 func.name = function.name;
 func.global = (*symbols)[function.name.to_string()].global;
 for (int i = 0; i < function.params.size(); i++) {
  Var param(function.params[i]);
  param.type = function.param_types[i];

  func.params.push_back(param);
 }

 current_function = &func;

 tackyify(*function.body);
 func.body.push_back(TACKY::Return(0));

 program.funcs.push_back(func);
}

void TACKYifier::tackyify(Parser::Block block) {
 for (Parser::Block_Item &item : block.items) {
  tackyify(item);
 }
}

void TACKYifier::tackyify(Parser::Block_Item item) {
 std::visit(overloaded{
  [&](Parser::Declaration &decl) {
   tackyify(decl);
  },
  [&](Parser::Statement &stmt) {
   tackyify(&stmt);
  }
 }, item);
}

void TACKYifier::tackyify(Parser::Declaration decl) {
 std::visit(overloaded{
  [&](Parser::FuncDecl &decl) {},
  [&](Parser::VarDecl &decl) {
   if (decl.init == nullptr || decl.tasc.storage_class != Parser::StorageClass::None) return;
  
   Value val = tackyify(decl.init);
   current_function->body.push_back(Copy(val, decl));
  }
 }, decl);
}

void TACKYifier::tackyify(Parser::ForInit &init) {
 std::visit(overloaded{
  [&](Parser::VarDecl *decl) {
   tackyify((Parser::Declaration)*decl);
  },
  [&](Parser::Expression *expr) {
   tackyify(expr);
  }
 }, init);
}

void TACKYifier::tackyify(Parser::Statement *stmt) {
 if (stmt == nullptr) return;
 std::vector<Instruction> &function_body = current_function->body;

 std::visit(overloaded{
  [&](Parser::EmptyStatement &_) {},
  [&](Parser::CompoundStatement &stmts) {
   tackyify(*stmts.block);
  },
  [&](Parser::Return &stmt) {
   TACKY::Return inst;
   inst.val = tackyify(stmt.expr);

   function_body.push_back(inst);
  },
  [&](Parser::If &if_stmt) {
   Value cond_res = tackyify(if_stmt.condition);
   Var cond_var = make_tacky_var(if_stmt.condition->type);
   Var end_label = make_label();
   Var else_label = make_label("else_");

   function_body.push_back(Copy(cond_res, cond_var));
   function_body.push_back(JumpIfZero(cond_var, if_stmt._else != nullptr ? else_label : end_label));
   tackyify(if_stmt.then);
  
   if (if_stmt._else != nullptr) {
    function_body.push_back(Jump(end_label));
    function_body.push_back(TACKY::Label(else_label));
    tackyify(if_stmt._else);
   } else label_count--; // don't count else_label if there is no else block.

   function_body.push_back(TACKY::Label(end_label));
  },
  [&](Parser::While &while_stmt) {
   string loop_name = while_stmt.label.to_string();
   Var continue_label = make_label("continue_", loop_name);
   Var break_label = make_label("break_", loop_name);
   Var cond_var = make_tacky_var();
   
   function_body.push_back(TACKY::Label(continue_label));
   Value cond_res = tackyify(while_stmt.condition);
   function_body.push_back(Copy(cond_res, cond_var));
   function_body.push_back(JumpIfZero(cond_var, break_label));
   tackyify(while_stmt.body);
   function_body.push_back(Jump(continue_label));
   function_body.push_back(TACKY::Label(break_label));
  },
  [&](Parser::DoWhile &do_while_stmt) {
   string loop_name = do_while_stmt.label.to_string();
   Var start_label = make_label();
   Var continue_label = make_label("continue_", loop_name);
   Var break_label = make_label("break_", loop_name);
   Var cond_var = make_tacky_var();

   function_body.push_back(TACKY::Label(start_label));
   tackyify(do_while_stmt.body);
   function_body.push_back(TACKY::Label(continue_label));
   Value cond_res = tackyify(do_while_stmt.condition);
   function_body.insert(function_body.end(), {
    Copy(cond_res, cond_var),
    JumpIfNotZero(cond_var, start_label),
    TACKY::Label(break_label),
   });
  },
  [&](Parser::For &for_stmt) {
   string loop_name = for_stmt.label.to_string();
   Var start_label = make_label();
   Var continue_label = make_label("continue_", loop_name);
   Var break_label = make_label("break_", loop_name);
   
   tackyify(for_stmt.init);
   function_body.push_back(TACKY::Label(start_label));

   Value cond_res = tackyify(for_stmt.condition);
   if (Constant *konst = std::get_if<Constant>(&cond_res); konst == nullptr || konst->_const == 0) {
    Var cond_var = make_tacky_var();
    function_body.push_back(Copy(cond_res, cond_var));
    function_body.push_back(JumpIfZero(cond_var, break_label));
   }

   tackyify(for_stmt.body);
   function_body.push_back(TACKY::Label(continue_label));
   tackyify(for_stmt.post);
   function_body.push_back(Jump(start_label));
   function_body.push_back(TACKY::Label(break_label));
  },
  [&](Parser::Switch &swtch) {
   string switch_name = swtch.label.to_string();
   Var cond_var = make_tacky_var(swtch.expr->type);
   Var eq_var = make_tacky_var(swtch.expr->type);
   Value cond_res = tackyify(swtch.expr);
   Var break_label = make_label("break_", switch_name);
   Var default_label = make_label("case_", "_" + switch_name);
   bool has_default = false;

   function_body.push_back(Copy(cond_res, cond_var));
   for (Parser::Case *_case : swtch.cases) {
    Parser::Constant *_const = static_cast<Parser::Constant *>(_case->expr);
    if (_const == nullptr) {
     has_default = true;
     continue;
    }

    string const_str = _const->_const;
    if (const_str[0] == '-') const_str[0] = 'n';
    Var case_label = make_label("case_", const_str + "_" + switch_name);
    Binary eq(cond_var, _const, eq_var);
    eq.op = Parser::BinaryOp::Equal;

    function_body.push_back(eq);
    function_body.push_back(JumpIfNotZero(eq_var, case_label));
   }

   function_body.push_back(Jump(has_default ? default_label : break_label));
   tackyify(swtch.body);
   function_body.push_back(TACKY::Label(break_label));
  },
  [&](Parser::Case &_case) {
   string const_str;
   if (_case.expr != nullptr) {
    const_str = static_cast<Parser::Constant *>(_case.expr)->_const;
    if (const_str[0] == '-') const_str[0] = 'n';
   } else const_str = "";
  
   string case_suffix = const_str + "_" + _case.label.to_string();
   Var case_label = make_label("case_", case_suffix);
   current_function->body.push_back(TACKY::Label(case_label));
   tackyify(_case.stmt);
  },
  [&](Parser::Continue &cont) {
   Var continue_label = make_label("continue_", cont.label.to_string());

   function_body.push_back(Jump(continue_label));
  },
  [&](Parser::Break &brk) {
   Var break_label = make_label("break_", brk.label.to_string());

   function_body.push_back(Jump(break_label));
  },
  [&](Parser::Goto &goto_stmt) {
   Var target(goto_stmt.target.name);

   function_body.push_back(Jump(target));
  },
  [&](Parser::Label &label) {
   Var label_var(label.name);

   current_function->body.push_back(TACKY::Label(label_var));
   tackyify(label.stmt);
  },
  [&](Parser::ExpressionStatement &expr) {
   tackyify(expr.expr);
  }
 }, *stmt);
}

class TackyifyingExpressionVisitor : public Parser::ExpressionVisitor {
 private:
  TACKYifier &tackyifier;
  std::vector<Instruction> &function_body;
 public:
  Value val;

  TackyifyingExpressionVisitor(TACKYifier &tackyifier, std::vector<Instruction> &function_body):
  tackyifier(tackyifier),
  function_body(function_body) {}
 
  void visit(Parser::Constant *expr) {
   TACKY::Constant konst(std::stoull(expr->_const));
   konst.type = expr->type;

   val = konst;
  }
  
  void visit(Parser::Var *expr) {
   TACKY::Var var(expr->name);
   var.type = expr->type;
   
   val = var;
  }
  
  void visit(Parser::Unary *expr) {
   Unary inst;
   inst.op = expr->op;
   if (expr->op == Parser::UnaryOp::Increment || expr->op == Parser::UnaryOp::Decrement) {
    if (!expr->expr->is_var()) {
     if (expr->op == Parser::UnaryOp::Increment) {
      error("Cannot increment a literal.");
     } else {
      error("Cannot decrement a literal.");
     }
    }

    TACKY::Var var(static_cast<Parser::Var *>(expr->expr)->name);
    TACKY::Var res = tackyifier.make_tacky_var(expr->type, expr->postfix);
    var.type = expr->type;
    
    inst.src = var;
    inst.dst = var;

    if (expr->postfix) function_body.push_back(Copy(var, res));
    function_body.push_back(inst);
    val = expr->postfix ? res : var;
    return;
   }
   
   inst.src = tackyifier.tackyify(expr->expr);
   inst.dst = tackyifier.make_tacky_var(expr->type);

   function_body.push_back(inst);
   val = inst.dst;
  }
  
  void visit(Parser::Binary *expr) {
   if (!(expr->op == Parser::BinaryOp::And || expr->op == Parser::BinaryOp::Or)) {
    Binary inst;
    inst.op = expr->op;
    inst.src1 = tackyifier.tackyify(expr->left);
    inst.src2 = tackyifier.tackyify(expr->right);
    inst.dst  = tackyifier.make_tacky_var(expr->type);

    function_body.push_back(inst);
    val = inst.dst;
   } else {
    Var v1 = tackyifier.make_tacky_var(expr->type);
    Var v2 = tackyifier.make_tacky_var(expr->type);
    Var result = tackyifier.make_tacky_var(expr->type);
    Var end = tackyifier.make_label();
    Value e1, e2;

    if (expr->op == Parser::BinaryOp::And) {
     Var false_label = tackyifier.make_label("false_");

     e1 = tackyifier.tackyify(expr->left);
     function_body.push_back(Copy(e1, v1));
     function_body.push_back(JumpIfZero(v1, false_label));
     e2 = tackyifier.tackyify(expr->right);
     function_body.insert(function_body.end(), {
      Copy(e2, v2),
      JumpIfZero(v2, false_label),
      Copy(1, result),
      Jump(end),
      TACKY::Label(false_label),
      Copy(0, result),
      TACKY::Label(end)
     });
    } else {
     Var true_label = tackyifier.make_label("true_");

     e1 = tackyifier.tackyify(expr->left);
     function_body.push_back(Copy(e1, v1));
     function_body.push_back(JumpIfNotZero(v1, true_label));
     e2 = tackyifier.tackyify(expr->right);
     function_body.insert(function_body.end(), {
      Copy(e2, v2),
      JumpIfNotZero(v2, true_label),
      Copy(0, result),
      Jump(end),
      TACKY::Label(true_label),
      Copy(1, result),
      TACKY::Label(end)
     });
    }

    val = result;
   }
  }
  
  void visit(Parser::Assignment *expr) {
   Value left = tackyifier.tackyify(expr->left);
   Value right = tackyifier.tackyify(expr->right);
   if (expr->op == Parser::BinaryOp::Equal) {
    function_body.push_back(Copy(right, left));
   } else {
    Binary inst;
    inst.op   = expr->op;
    inst.src1 = left;
    inst.src2 = right;
    inst.dst  = left;
    
    function_body.push_back(inst);
    if (!expr->left->is_var()) {
     Parser::Cast *cast = static_cast<Parser::Cast *>(expr->left);
     Var lval(static_cast<Parser::Var *>(cast->expr)->name);
     lval.type = cast->expr->type;

     function_body.push_back(Truncate(left, lval));
     val = lval;
     return;
    }
   }

   val = left;
  }
  
  void visit(Parser::Conditional *expr) {
   Value cond_res = tackyifier.tackyify(expr->condition);
   Var cond_var = tackyifier.make_tacky_var(expr->type);
   Var result = tackyifier.make_tacky_var(expr->type);
   Var else_label = tackyifier.make_label();
   Var end_label = tackyifier.make_label();
   Value v1, v2;

   function_body.push_back(Copy(cond_res, cond_var));
   function_body.push_back(JumpIfZero(cond_var, else_label));
   v1 = tackyifier.tackyify(expr->left);
   function_body.push_back(Copy(v1, result));
   function_body.push_back(Jump(end_label));
   function_body.push_back(TACKY::Label(else_label));
   v2 = tackyifier.tackyify(expr->right);
   function_body.push_back(Copy(v2, result));
   function_body.push_back(TACKY::Label(end_label));

   val = result;
  }
  
  void visit(Parser::FunctionCall *expr) {
   TACKY::FunCall call(expr->name);
   
   call.dst = tackyifier.make_tacky_var(expr->type);
   for (Parser::Expression *arg : expr->args) {
    Value val = tackyifier.tackyify(arg);
    Var   tmp = tackyifier.make_tacky_var(arg->type);
    
    function_body.push_back(Copy(val, tmp));
    call.args.push_back(tmp);
   }

   function_body.push_back(call);
   val = call.dst;
  }
  
  void visit(Parser::Cast *cast) {
   Value result = tackyifier.tackyify(cast->expr);

   if (cast->type != cast->expr->type){
    Var dst = tackyifier.make_tacky_var(cast->type);

    if (get_type_size(cast->type) == get_type_size(cast->expr->type)) {
     function_body.push_back(Copy(result, dst));
    } else if (get_type_size(cast->type) < get_type_size(cast->expr->type)) {
     function_body.push_back(Truncate(result, dst));
    } else if (is_signed(cast->expr->type)) {
    //  std::cout << static_cast<int>(cast->expr->type) << " -> " << static_cast<int>(cast->type) << std::endl;
     function_body.push_back(SignExtend(result, dst));
    } else {
     function_body.push_back(ZeroExtend(result, dst));
    } 
   
    val = dst;
   } else val = result;
  }
};

Value TACKYifier::tackyify(Parser::Expression *expr) {
 if (expr == nullptr) return 1;

 std::vector<Instruction> &function_body = current_function->body;
 TackyifyingExpressionVisitor visitor(*this, function_body);

 expr->accept(&visitor);
 return visitor.val;
}