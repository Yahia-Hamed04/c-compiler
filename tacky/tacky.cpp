#include "tacky.h"
#include "../helpers.h"
using namespace TACKY;

TACKYifier::TACKYifier(Parser::CParser &parser) {
 this->parser = &parser;
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

 return Var{.name = tmp};
}

Var TACKYifier::make_label(string prefix) {
 string format_str = prefix + "%d";

 Token tmp = {.type = TokenType::Identifier, .start = new char[prefix.size() + 10], .line = 0};
 tmp.length = (size_t)sprintf(tmp.start, format_str.data(), label_count++);

 return Var{.name = tmp};
}

Var TACKYifier::make_label(string prefix, string suffix) {
 string format_str = prefix + suffix;

 Token tmp = {.type = TokenType::Identifier, .start = new char[prefix.size() + suffix.size()], .line = 0};
 tmp.length = (size_t)sprintf(tmp.start, "%s", format_str.data());

 return Var{.name = tmp};
}

void TACKYifier::tackyify() {
 Parser::Program parser_program = parser->get_program();

 for (Parser::FuncDecl func : parser_program.functions) {
  tackyify(func);
 }
}

void TACKYifier::tackyify(Parser::FuncDecl function) {
 program.func.name = function.name;

 tackyify(*function.body);
 tackyify((Parser::Statement)Parser::Return{.expr = Parser::Constant{._const = "0"}});
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
   tackyify(stmt);
  }
 }, item);
}

void TACKYifier::tackyify(Parser::Declaration decl) {
 std::visit(overloaded{
  [&](Parser::FuncDecl &decl) {},
  [&](Parser::VarDecl &decl) {
   if (decl.init == std::nullopt) return;
  
   Value val = tackyify(&*decl.init);
   program.func.body.push_back(Copy(val, decl.name));
  }
 }, decl);
}

void TACKYifier::tackyify(Parser::ForInit init) {
 std::visit(overloaded{
  [&](Parser::VarDecl *decl) {
   tackyify(*(Parser::Declaration *)decl);
  },
  [&](std::optional<Parser::Expression> &expr) {
   tackyify(expr);
  }
 }, init);
}

void TACKYifier::tackyify(Parser::Statement stmt) {
 std::vector<Instruction> &function_body = program.func.body;

 std::visit(overloaded{
  [&](Parser::EmptyStatement _) {},
  [&](Parser::CompoundStatement stmts) {
   tackyify(*stmts.block);
  },
  [&](Parser::Return stmt) {
   TACKY::Return inst;
   inst.val = tackyify(&stmt.expr);

   function_body.push_back(inst);
  },
  [&](Parser::If if_stmt) {
   Value cond_res = tackyify(&if_stmt.condition);
   Var cond_var = make_temporary();
   Var end_label = make_label();
   Var else_label = make_label("else_");

   function_body.push_back(Copy(cond_res, cond_var));
   function_body.push_back(JumpIfZero(cond_var, if_stmt._else != nullptr ? else_label : end_label));
   tackyify(*if_stmt.then);
  
   if (if_stmt._else != nullptr) {
    function_body.push_back(Jump(end_label));
    function_body.push_back(TACKY::Label(else_label));
    tackyify(*if_stmt._else);
   } else label_count--; // don't count else_label if there is no else block.

   function_body.push_back(TACKY::Label(end_label));
  },
  [&](Parser::While &while_stmt) {
   string loop_name = while_stmt.label.to_string();
   Var continue_label = make_label("continue_", loop_name);
   Var break_label = make_label("break_", loop_name);
   Var cond_var = make_temporary();
   
   function_body.push_back(TACKY::Label(continue_label));
   Value cond_res = tackyify(&while_stmt.condition);
   function_body.push_back(Copy(cond_res, cond_var));
   function_body.push_back(JumpIfZero(cond_var, break_label));
   tackyify(*while_stmt.body);
   function_body.push_back(Jump(continue_label));
   function_body.push_back(TACKY::Label(break_label));
  },
  [&](Parser::DoWhile &do_while_stmt) {
   string loop_name = do_while_stmt.label.to_string();
   Var start_label = make_label();
   Var continue_label = make_label("continue_", loop_name);
   Var break_label = make_label("break_", loop_name);
   Var cond_var = make_temporary();

   function_body.push_back(TACKY::Label(start_label));
   tackyify(*do_while_stmt.body);
   function_body.push_back(TACKY::Label(continue_label));
   Value cond_res = tackyify(&do_while_stmt.condition);
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
   if (int *konst = std::get_if<int>(&cond_res); konst == nullptr || *konst == 0) {
    Var cond_var = make_temporary();
    function_body.push_back(Copy(cond_res, cond_var));
    function_body.push_back(JumpIfZero(cond_var, break_label));
   }

   tackyify(*for_stmt.body);
   function_body.push_back(TACKY::Label(continue_label));
   tackyify(for_stmt.post);
   function_body.push_back(Jump(start_label));
   function_body.push_back(TACKY::Label(break_label));
  },
  [&](Parser::Switch &swtch) {
   string switch_name = swtch.label.to_string();
   Var cond_var = make_temporary();
   Var eq_var = make_temporary();
   Value cond_res = tackyify(&swtch.expr);
   Var break_label = make_label("break_", switch_name);
   Var default_label = make_label("case_", "_" + switch_name);
   bool has_default = false;

   function_body.push_back(Copy(cond_res, cond_var));
   for (Parser::Case *_case : swtch.cases) {
    string const_str = std::get<Parser::Constant>(_case->expr)._const;
    if (const_str == "") {
     has_default = true;
     continue;
    }

    Var case_label = make_label("case_", const_str + "_" + switch_name);
    Binary eq(cond_var, std::stoi(const_str), eq_var);
    eq.op = Parser::BinaryOp::Equal;

    function_body.push_back(eq);
    function_body.push_back(JumpIfNotZero(eq_var, case_label));
   }

   function_body.push_back(Jump(has_default ? default_label : break_label));
   tackyify(*swtch.body);
   function_body.push_back(TACKY::Label(break_label));
  },
  [&](Parser::Case &_case) {
   string const_str = std::get<Parser::Constant>(_case.expr)._const;
   string case_suffix = const_str + "_" + _case.label.to_string();
   Var case_label = make_label("case_", case_suffix);

   program.func.body.push_back(TACKY::Label(case_label));
   tackyify(*_case.stmt);
  },
  [&](Parser::Continue &cont) {
   Var continue_label = make_label("continue_", cont.label.to_string());

   function_body.push_back(Jump(continue_label));
  },
  [&](Parser::Break &brk) {
   Var break_label = make_label("break_", brk.label.to_string());

   function_body.push_back(Jump(break_label));
  },
  [&](Parser::Goto goto_stmt) {
   Var target = {.name = goto_stmt.target.name};

   function_body.push_back(Jump(target));
  },
  [&](Parser::Label &label) {
   Var label_var = {.name = label.name};

   program.func.body.push_back(TACKY::Label(label_var));
   tackyify(*label.stmt);
  },
  [&](Parser::Expression expr) {
   tackyify(&expr);
  }
 }, stmt);
}

Value TACKYifier::tackyify(std::optional<Parser::Expression> expr) {
 if (expr == std::nullopt) return 1;

 return tackyify(&*expr);
}

Value TACKYifier::tackyify(Parser::Expression *expr) {
 std::vector<Instruction> &function_body = program.func.body;
 
 return std::visit(overloaded{
  [&](Parser::Constant expr) -> Value {
   return std::stoi(expr._const);
  },
  [&](Parser::Var expr) -> Value {
   return TACKY::Var{.name = expr.name};
  },
  [&](Parser::Unary unary) -> Value {
   Unary inst;
   inst.op = unary.op;
   if (unary.op == Parser::UnaryOp::Increment || unary.op == Parser::UnaryOp::Decrement) {
    TACKY::Var var = {.name = std::get_if<Parser::Var>(unary.expr)->name};
    TACKY::Var res = make_temporary(unary.postfix);
    if (unary.postfix) function_body.push_back(Copy(var, res));
    
    inst.src = var;
    inst.dst = var;

    function_body.push_back(inst);
    return unary.postfix ? res : var;
   }
   
   inst.src = tackyify(unary.expr);
   inst.dst = make_temporary();

   function_body.push_back(inst);
   return inst.dst;
  },
  [&](Parser::Binary binary) -> Value {
   if (!(binary.op == Parser::BinaryOp::And || binary.op == Parser::BinaryOp::Or)) {
    Binary inst;
    inst.op = binary.op;
    inst.src1 = tackyify(binary.left);
    inst.src2 = tackyify(binary.right);
    inst.dst = make_temporary();
    
    function_body.push_back(inst);
    return inst.dst;
   } else {
    Var v1 = make_temporary();
    Var v2 = make_temporary();
    Var result = make_temporary();
    Var end = make_label();
    Value e1, e2;

    if (binary.op == Parser::BinaryOp::And) {
     Var false_label = make_label("false_");

     e1 = tackyify(binary.left);
     function_body.push_back(Copy(e1, v1));
     function_body.push_back(JumpIfZero(v1, false_label));
     e2 = tackyify(binary.right);
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
     Var true_label = make_label("true_");

     e1 = tackyify(binary.left);
     function_body.push_back(Copy(e1, v1));
     function_body.push_back(JumpIfNotZero(v1, true_label));
     e2 = tackyify(binary.right);
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

    return result;
   }
  },
  [&](Parser::Assignment expr) -> Value {
   TACKY::Var var = {.name = std::get<Parser::Var>(*expr.left).name}; 
   Value val = tackyify(expr.right);
   if (expr.op != Parser::BinaryOp::Equal) {
    Binary inst;
    inst.op   = expr.op;
    inst.src1 = var;
    inst.src2 = val;
    inst.dst  = var;
    
    function_body.push_back(inst);
   } else {
    function_body.push_back(Copy(val, var));
   }

   return var;
  },
  [&](Parser::Conditional expr) -> Value {
   Value cond_res = tackyify(expr.condition);
   Var cond_var = make_temporary();
   Var result = make_temporary();
   Var else_label = make_label();
   Var end_label = make_label();
   Value v1, v2;

   function_body.push_back(Copy(cond_res, cond_var));
   function_body.push_back(JumpIfZero(cond_var, else_label));
   v1 = tackyify(expr.left);
   function_body.push_back(Copy(v1, result));
   function_body.push_back(Jump(end_label));
   function_body.push_back(TACKY::Label(else_label));
   v2 = tackyify(expr.right);
   function_body.push_back(Copy(v2, result));
   function_body.push_back(TACKY::Label(end_label));

   return result;
  },
  [&](Parser::FunctionCall expr) -> Value {
   return 0;
  }
}, *expr);
}