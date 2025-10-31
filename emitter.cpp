#include "emitter.h"
#include "helpers.h"
using namespace Gen;

Emitter::Emitter(Generator &gen): gen(&gen), symbols(gen.asm_table) {
 emit();
}

std::string Emitter::get_code() {
 return code;
}

void Emitter::emit() {
 Gen::Program program = gen->get_program();

 for (Gen::StaticVariable &var : program.statics) {
  emit_var(var);
 }

 code += "    .text\n";
 for (Gen::Function &function : program.funcs) {
  std::string function_name = function.name.to_string();
  code += function.global ? "    .globl " + function_name + '\n' : "";
  code += function_name + ":\n";
  code += "    pushq %rbp\n";
  code += "    movq %rsp, %rbp\n";
  
  for (Gen::Instruction &inst : function.instructions) {
   emit_instruction(inst);
  } code += '\n';
 }

 code += "\n.section .note.GNU-stack,\"\",@progbits\n";
}

string cond_code(Condition cond) {
 string code;
 
 switch (cond) {
  case Condition::Equal:         code = "e"; break;
  case Condition::Greater:       code = "g"; break;
  case Condition::Less:          code = "l"; break;
  case Condition::Above:         code = "a"; break;
  case Condition::Below:         code = "b"; break;
  case Condition::Not_Equal:     code = "ne"; break;
  case Condition::Greater_Equal: code = "ge"; break;
  case Condition::Less_Equal:    code = "le"; break;
  case Condition::Above_Equal:   code = "ae"; break;
  case Condition::Below_Equal:   code = "be"; break;
 }

 return code;
}

void Emitter::emit_type(Gen::AssemblyType &type) {
 switch (type) {
   case Gen::AssemblyType::Longword: code += 'l'; break;
   case Gen::AssemblyType::Quadword: code += 'q'; break;
   default: error("invalid type");
 }
}

void Emitter::emit_var(Gen::StaticVariable &var) {
 std::string var_name = var.name.to_string();
 code += var.global ? "    .globl " + var_name + '\n' : "";
 code += var.init == 0 ? "    .bss\n" : "    .data\n";
 code += "    .align " + std::to_string(var.alignment) + '\n';
 code += var_name + ":\n";
 if (var.init == 0) {
  code += "    .zero " + std::to_string(var.alignment) + "\n";
 } else {
  switch (var.init_val_type) {
   case Parser::InitValType::InitInt:  code += "    .long "; break;
   case Parser::InitValType::InitLong: code += "    .quad "; break;
   default: error(std::to_string(static_cast<int>(var.init_val_type)));
  }
  
  code += std::to_string(var.init) + '\n';
 }

 code += '\n';
}

void Emitter::emit_instruction(Gen::Instruction &inst) {
 bool is_label = std::get_if<Gen::Label>(&inst) != nullptr; 
 if (!is_label) code += "    ";

 std::visit(overloaded{
  [&](Mov &mov) {
   if (static_cast<int>(mov.type) > 3) {
    return;
   }
   
   code += "mov"; emit_type(mov.type);
   code += ' ';   emit_operand(mov.src, mov.type);
   code += ", ";  emit_operand(mov.dst, mov.type, true);
  },
  [&](Movsx &mov) {
   code += "movslq "; emit_operand(mov.src, Gen::AssemblyType::Longword);
   code += ", ";      emit_operand(mov.dst, Gen::AssemblyType::Quadword, true);
  },
  [&](Movzx &mov) {/* "movzx" does not correspond to any real instruction yet */},
  [&](Gen::Unary &un) {
   switch (un.op) {
    case UnaryOp::Neg: code += "neg"; break;
    case UnaryOp::Not: code += "not"; break;
    case UnaryOp::Inc: code += "inc"; break;
    case UnaryOp::Dec: code += "dec"; break;
   }

   emit_type(un.type);
   code += ' '; emit_operand(un.operand, un.type);
  },
  [&](Gen::Binary &bin) {
   switch (bin.op) {
    case BinaryOp::And:  code += "and";  break;
    case BinaryOp::Or:   code += "or";   break;
    case BinaryOp::Xor:  code += "xor";  break;
    case BinaryOp::Sal:  code += "sal";  break;
    case BinaryOp::Sar:  code += "sar";  break;
    case BinaryOp::Shl:  code += "shl";  break;
    case BinaryOp::Shr:  code += "shr";  break;
    case BinaryOp::Add:  code += "add";  break;
    case BinaryOp::Sub:  code += "sub";  break;
    case BinaryOp::Mult: code += "imul"; break;
   }

   emit_type(bin.type);
   code += ' ';  emit_operand(bin.src, bin.type);
   code += ", "; emit_operand(bin.dst, bin.type, true);
  },
  [&](Div &div) {
   if (div.operand.is_signed) code += 'i';
   code += "div"; emit_type(div.type);
   code += ' '; emit_operand(div.operand, div.type);
  },
  [&](Call &call) {
   string fun_name = call.name.to_string();
   code += "call " + fun_name;
   if (!symbols[fun_name].defined) {
    code += "@PLT";
   }
  },
  [&](Push &push) {
   code += "pushq "; emit_operand(push.operand, Gen::AssemblyType::Quadword);
  },
  [&](Cmp &cmp) {
   code += "cmp"; emit_type(cmp.type);
   code += ' ';   emit_operand(cmp.op1, cmp.type);
   code += ", ";  emit_operand(cmp.op2, cmp.type);
  },
  [&](Gen::Label &label) {
   code += ".L" + label.name.to_string() + ":";
  },
  [&](Jmp &jmp) {
   code += "jmp .L" + jmp.target.to_string();
  },
  [&](Conditional_Jmp &jmp) {
   code += "j" + cond_code(jmp.condition) + " .L" + jmp.target.to_string();
  },
  [&](Set_Condition &set) {
   code += "set" + cond_code(set.condition) + " ";
   emit_operand(set.operand, Gen::AssemblyType::Byte, true);
  },
  [&](Ret &_) {
   code += "movq %rbp, %rsp\n";
   code += "    popq %rbp\n";
   code += "    ret";
  },
  [&](Cdq &cdq) {
   switch (cdq.type) {
    case Gen::AssemblyType::Longword: code += "cdq"; break;
    case Gen::AssemblyType::Quadword: code += "cqo"; break;
   }   
  }
 }, inst);

 code += '\n';
}

void Emitter::emit_operand(Operand &operand, Gen::AssemblyType type, bool is_dst) {
 std::visit(overloaded{
  [&](Pseudo pseudo) {
   error(
    "Code Gen Error: pseudo-register \"" +
    pseudo.name.to_string() +
    "\" was not turned into a memory address!");
  },
  [&](Data data) {
   code += data.name.to_string() + "(%rip)";
  },
  [&](Immediate imm) {
   if (is_dst)
    error("Code Emission Error: destination cannot be an immediate value (how'd this even happen?!)");

   code += "$" + std::to_string(imm.val);
  },
  [&](Register reg) {
   switch (type) {
    case Gen::AssemblyType::Byte:     code += one_byte_regs[reg];   break;
    case Gen::AssemblyType::Word:     code += two_byte_regs[reg];   break;
    case Gen::AssemblyType::Longword: code += four_byte_regs[reg];  break;
    case Gen::AssemblyType::Quadword: code += eight_byte_regs[reg]; break;
    default: error("Invalid register type.");
   }
  },
  [&](StackOffset offset) {
   code += std::to_string(offset.offset) + "(%rbp)";
  }
 }, operand.var);
}