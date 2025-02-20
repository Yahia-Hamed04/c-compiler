#include "emitter.h"
#include "helpers.h"
using namespace Gen;

Emitter::Emitter(Generator &gen) {
 this->gen = &gen;
 this->symbols = gen.symbols;
 this->code = "";

 emit();
}

std::string Emitter::get_code() {
 return code;
}

void Emitter::emit() {
 Gen::Program program = gen->get_program();

 for (Gen::Function &function : program.funcs) {
  std::string function_name = function.name.to_string();
  code += "    .globl " + function_name + '\n';
  code += function_name + ":\n";
  code += "    pushq %rbp\n";
  code += "    movq %rsp, %rbp\n";
  
  for (Gen::Instruction &inst : function.instructions) {
   emit_instruction(inst);
  }
 }

 code += "\n.section .note.GNU-stack,\"\",@progbits\n";
}

string cond_code(Condition cond) {
 string code;
 
 switch (cond) {
  case Condition::Equal:         code = "e"; break;
  case Condition::Greater:       code = "g"; break;
  case Condition::Less:          code = "l"; break;
  case Condition::Not_Equal:     code = "ne"; break;
  case Condition::Less_Equal:    code = "le"; break;
  case Condition::Greater_Equal: code = "ge"; break;
 }

 return code;
}

void Emitter::emit_instruction(Gen::Instruction &inst) {
 bool is_label = std::get_if<Gen::Label>(&inst) != nullptr; 
 if (!is_label) code += "    ";

 std::visit(overloaded{
  [&](Mov &mov) {
   code += "movl "; emit_operand(mov.src);
   code += ", ";    emit_operand(mov.dst, true);
  },
  [&](Gen::Unary &un) {
   switch (un.op) {
    case UnaryOp::Neg: code += "negl"; break;
    case UnaryOp::Not: code += "notl"; break;
    case UnaryOp::Inc: code += "incl"; break;
    case UnaryOp::Dec: code += "decl"; break;
   }

   code += ' '; emit_operand(un.operand);
  },
  [&](Gen::Binary &bin) {
   switch (bin.op) {
    case BinaryOp::And:  code += "andl";  break;
    case BinaryOp::Or:   code += "orl";   break;
    case BinaryOp::Xor:  code += "xorl";  break;
    case BinaryOp::Sal:  code += "sall";  break;
    case BinaryOp::Sar:  code += "sarl";  break;
    case BinaryOp::Add:  code += "addl";  break;
    case BinaryOp::Sub:  code += "subl";  break;
    case BinaryOp::Mult: code += "imull"; break;
   }

   code += ' ';  emit_operand(bin.src);
   code += ", "; emit_operand(bin.dst, true);
  },
  [&](Idiv &idiv) {
   code += "idivl "; emit_operand(idiv.operand);
  },
  [&](Call &call) {
   string fun_name = call.name.to_string();
   code += "call " + fun_name;
   if (!(*symbols)[fun_name].defined) {
    code += "@PLT";
   }
  },
  [&](Push &push) {
   code += "pushq "; emit_operand(push.operand, false, 8);
  },
  [&](StackAlloc &alloc) {
   code += "subq $" + std::to_string(alloc.amount) + ", %rsp";
  },
  [&](StackFree &free) {
   code += "addq $" + std::to_string(free.amount) + ", %rsp";
  },
  [&](Cmp &cmp) {
   code += "cmpl "; emit_operand(cmp.op1);
   code += ", ";   emit_operand(cmp.op2);
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
   emit_operand(set.operand, true, 1);
  },
  [&](Ret &_) {
   code += "movq %rbp, %rsp\n";
   code += "    popq %rbp\n";
   code += "    ret";
  },
  [&](Cdq &_) {
   code += "cdq";
  }
 }, inst);

 code += '\n';
}

void Emitter::emit_operand(Operand &operand, bool is_dst, int num_bytes) {
 std::visit(overloaded{
  [&](Pseudo pseudo) {
   error(
    "Code Gen Error: pseudo-register \"" +
    pseudo.name.to_string() +
    "\" was not turned into a memory address!");
  },
  [&](Immediate imm) {
   if (is_dst)
    error("Code Emission Error: destination cannot be an immediate value (how'd this even happen?!)");

   code += "$" + std::to_string(imm.val);
  },
  [&](Register reg) {
   switch (num_bytes) {
    case 1: code += one_byte_regs[reg]; break;
    case 2: code += two_byte_regs[reg]; break;
    case 4: code += four_byte_regs[reg]; break;
    case 8: code += eight_byte_regs[reg]; break;
    default: error("Invalid number of bytes.");
   }
  },
  [&](StackOffset offset) {
   code += std::to_string(offset.offset) + "(%rbp)";
  }
 }, operand);
}
