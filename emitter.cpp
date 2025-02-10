#include "emitter.h"
#include "helpers.h"

Emitter::Emitter(Generator &gen) {
 this->gen = &gen;

 emit();
}

std::string Emitter::get_code() {
 return code;
}

void Emitter::emit() {
 Gen::Program program = gen->get_program();
 std::string function_name = program.func.name.to_string();
 code  = "    .globl " + function_name + '\n';
 code += function_name + ":\n";
 code += "    pushq %rbp\n";
 code += "    movq %rsp, %rbp\n";
 
 for (Gen::Instruction &inst : program.func.instructions) {
  emit_instruction(inst);
 }

 code += "\n.section .note.GNU-stack,\"\",@progbits\n";
}

string cond_code(Gen::Condition cond) {
 string code;
 
 switch (cond) {
  case Gen::Condition::Equal:         code = "e"; break;
  case Gen::Condition::Greater:       code = "g"; break;
  case Gen::Condition::Less:          code = "l"; break;
  case Gen::Condition::Not_Equal:     code = "ne"; break;
  case Gen::Condition::Less_Equal:    code = "le"; break;
  case Gen::Condition::Greater_Equal: code = "ge"; break;
 }

 return code;
}

void Emitter::emit_instruction(Gen::Instruction &inst) {
 bool is_label = std::get_if<Gen::Label>(&inst) != nullptr; 
 if (!is_label) code += "    ";

 std::visit(overloaded{
  [&](Gen::Mov &mov) {
   code += "movl "; emit_operand(mov.src);
   code += ", ";    emit_operand(mov.dst, true);
  },
  [&](Gen::Unary &un) {
   switch (un.op) {
    case Gen::UnaryOp::Neg: code += "negl"; break;
    case Gen::UnaryOp::Not: code += "notl"; break;
    case Gen::UnaryOp::Inc: code += "incl"; break;
    case Gen::UnaryOp::Dec: code += "decl"; break;
   }

   code += ' '; emit_operand(un.operand);
  },
  [&](Gen::Binary &bin) {
   switch (bin.op) {
    case Gen::BinaryOp::And:  code += "andl";  break;
    case Gen::BinaryOp::Or:   code += "orl";   break;
    case Gen::BinaryOp::Xor:  code += "xorl";  break;
    case Gen::BinaryOp::Sal:  code += "sall";  break;
    case Gen::BinaryOp::Sar:  code += "sarl";  break;
    case Gen::BinaryOp::Add:  code += "addl";  break;
    case Gen::BinaryOp::Sub:  code += "subl";  break;
    case Gen::BinaryOp::Mult: code += "imull"; break;
   }

   code += ' ';  emit_operand(bin.src);
   code += ", "; emit_operand(bin.dst, true);
  },
  [&](Gen::Idiv &idiv) {
   code += "idivl "; emit_operand(idiv.operand);
  },
  [&](Gen::StackAlloc &alloc) {
   code += "subq $" + std::to_string(alloc.amount) + ", %rsp";
  },
  [&](Gen::Cmp &cmp) {
   code += "cmpl "; emit_operand(cmp.op1);
   code += ", ";   emit_operand(cmp.op2);
  },
  [&](Gen::Label &label) {
   code += ".L" + label.name.to_string() + ":";
  },
  [&](Gen::Jmp &jmp) {
   code += "jmp .L" + jmp.target.to_string();
  },
  [&](Gen::Conditional_Jmp &jmp) {
   code += "j" + cond_code(jmp.condition) + " .L" + jmp.target.to_string();
  },
  [&](Gen::Set_Condition &set) {
   code += "set" + cond_code(set.condition) + " ";
   emit_operand(set.operand, true);
  },
  [&](Gen::Ret &_) {
   code += "movq %rbp, %rsp\n";
   code += "    popq %rbp\n";
   code += "    ret";
  },
  [&](Gen::Cdq &_) {
   code += "cdq";
  }
 }, inst);

 code += '\n';
}

void Emitter::emit_operand(Gen::Operand &operand, bool is_dst, bool is_set) {
 std::visit(overloaded{
  [&](Gen::Pseudo pseudo) {
   error(
    "Code Gen Error: pseudo-register \"" +
    pseudo.name.to_string() +
    "\" was not turned into a memory address!");
  },
  [&](Gen::Immediate imm) {
   if (is_dst)
    error("Code Emission Error: destination cannot be an immediate value (how'd this even happen?!)");

   code += "$" + std::to_string(imm.val);
  },
  [&](Gen::Register reg) {
   if (is_set) {
    switch(reg) {
     case Gen::Register::AX:  code += "%al"; break;
     case Gen::Register::CX:  code += "%cl"; break;
     case Gen::Register::DX:  code += "%dl"; break;
     case Gen::Register::R10: code += "%r10b"; break;
     case Gen::Register::R11: code += "%r11b"; break;
    }
   } else {
    switch(reg) {
     case Gen::Register::AX:  code += "%eax"; break;
     case Gen::Register::CX:  code += "%ecx"; break;
     case Gen::Register::DX:  code += "%edx"; break;
     case Gen::Register::R10: code += "%r10d"; break;
     case Gen::Register::R11: code += "%r11d"; break;
    }
   }
  },
  [&](Gen::StackOffset offset) {
   code += "-" + std::to_string(offset.offset) + "(%rbp)";
  }
 }, operand);
}
