#include "resolve_labels.cpp"
#include "resolve_idents.cpp"
#include "label_statements.cpp"
#include "typecheck.cpp"

void CParser::new_scope() {
 for (auto& [_, ident] : idents) {
  ident.from_current_scope = false;
 }
}