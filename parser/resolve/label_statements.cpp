#pragma once
#include "../parser.h"
#include "../../helpers.h"
#include <unordered_set>
using namespace Parser;

void CParser::label_statement() {
 Token null_token = {.length = 0};
 
 for (Declaration &decl : program.decls) {
  FuncDecl *func = std::get_if<FuncDecl>(&decl);
  if (func == nullptr || func->body == nullptr) continue;
  
  label_statement(*func->body, null_token);
 }
}

void CParser::label_statement(Block &block, Token current_label, Switch *curr_swtch, bool in_switch) {
 for (Block_Item &block_item : block.items) {
  Statement *stmt = std::get_if<Statement>(&block_item);
  
  label_statement(stmt, current_label, curr_swtch, in_switch);
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
    string const_str;
    if (_case->expr != nullptr) {
     const_str = static_cast<Parser::Constant *>(_case->expr)->_const;
    } else {
     const_str = "";
    }

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
   if (_case.expr != nullptr) {
    _case.expr->type = curr_swtch->expr->type;
    typecheck(_case.expr);
   }

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