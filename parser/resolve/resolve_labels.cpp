#pragma once
#include "../parser.h"
#include "../../helpers.h"
using namespace Parser;

void CParser::resolve_labels() {
 for (Declaration &decl : program.decls) {
  FuncDecl *func = std::get_if<FuncDecl>(&decl);
  if (func == nullptr || func->body == nullptr) continue;

  curr_func = func;
  resolve_labels(*func->body);
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
 
 if (curr_func->labels.count(label_name)) {
  error_at_line(label.name.line, "Duplicate label declaration for \"" + label_name + "\".");
 }

 curr_func->labels[label_name] = make_label(label_name);
}