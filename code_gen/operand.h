#pragma once
#include "../lexer/tokens.h"
#include "../parser/parser.h"
#include "../tacky/value.h"
#include "../helpers.h"
#include <variant>

namespace Gen {
 enum Register {
  AX,
  CX,
  DX,
  SI,
  DI,
  R8,
  R9,
  R10,
  R11,
  SP,
  Reg_Count
 };
 
 struct Immediate {
  size_t val;
 };
 
 struct Pseudo {
  Token name;
 };
 
 struct Data {
  Token name;
 };
 
 struct StackOffset {
  ptrdiff_t offset;
 };

 enum class AssemblyType {
  Byte,
  Word,
  Longword,
  Quadword
 };

 using Operand_Variant = std::variant<Immediate, Register, Pseudo, Data, StackOffset>; 
 struct Operand {
  bool is_signed;
  AssemblyType type;
  Operand_Variant var;

  Operand() {}
  Operand(Register var):  var(var) {}
  Operand(Immediate var): var(var) {}
  Operand(size_t var): var(Immediate{.val = var}), type(AssemblyType::Quadword) {}
  bool operator==(Operand &op) {
   if (var.index() != op.var.index()) return false;

   return std::visit(overloaded{
    [&](Immediate &imm) -> bool {
     return std::get<Immediate>(var).val == imm.val;
    },
    [&](Register &reg) -> bool {
     return std::get<Register>(var) == reg;
    },
    [&](Pseudo &pseudo) -> bool {
     return std::get<Pseudo>(var).name.to_string() == pseudo.name.to_string();
    },
    [&](Data &data) -> bool {
     return std::get<Data>(var).name.to_string() == data.name.to_string();
    },
    [&](StackOffset &offset) -> bool {
     return std::get<StackOffset>(var).offset == offset.offset;
    }
   }, op.var);
  }

  void set_type(Parser::Type type, TACKY::Value val) {
   switch (type) {
    case Parser::Type::Int:  {
     is_signed = true;
     this->type = AssemblyType::Longword;
    } break;
    case Parser::Type::Long:  {
     is_signed = true;
     this->type = AssemblyType::Quadword;
    } break;
    case Parser::Type::UInt:  {
     is_signed = false;
     this->type = AssemblyType::Longword;
    } break;
    case Parser::Type::ULong:  {
     is_signed = false;
     this->type = AssemblyType::Quadword;
    } break;
    default: {
     std::visit(overloaded{
      [&](TACKY::Constant value) {
       error("code gen error: " + std::to_string(value._const));
      },
      [&](TACKY::Var value) {
       error("code gen error: " + value.name.to_string());
      },
     }, val);
    };
   }
  }
 };
}

static const char* one_byte_regs[Gen::Register::Reg_Count] = {
 [Gen::Register::AX]  = "%al",
 [Gen::Register::CX]  = "%cl",
 [Gen::Register::DX]  = "%dl",
 [Gen::Register::SI]  = "%sil",
 [Gen::Register::DI]  = "%dil",
 [Gen::Register::R8]  = "%r8b",
 [Gen::Register::R9]  = "%r9b",
 [Gen::Register::R10] = "%r10b",
 [Gen::Register::R11] = "%r11b",
 [Gen::Register::SP]  = "%rspb",
};

static const char* two_byte_regs[Gen::Register::Reg_Count] = {
 [Gen::Register::AX]  = "%ax",
 [Gen::Register::CX]  = "%cx",
 [Gen::Register::DX]  = "%dx",
 [Gen::Register::SI]  = "%si",
 [Gen::Register::DI]  = "%di",
 [Gen::Register::R8]  = "%r8w",
 [Gen::Register::R9]  = "%r9w",
 [Gen::Register::R10] = "%r10w",
 [Gen::Register::R11] = "%r11w",
 [Gen::Register::SP]  = "%rspw",
};

static const char* four_byte_regs[Gen::Register::Reg_Count] = {
 [Gen::Register::AX]  = "%eax",
 [Gen::Register::CX]  = "%ecx",
 [Gen::Register::DX]  = "%edx",
 [Gen::Register::SI]  = "%esi",
 [Gen::Register::DI]  = "%edi",
 [Gen::Register::R8]  = "%r8d",
 [Gen::Register::R9]  = "%r9d",
 [Gen::Register::R10] = "%r10d",
 [Gen::Register::R11] = "%r11d",
 [Gen::Register::SP]  = "%esp",
};

static const char* eight_byte_regs[Gen::Register::Reg_Count] = {
 [Gen::Register::AX]  = "%rax",
 [Gen::Register::CX]  = "%rcx",
 [Gen::Register::DX]  = "%rdx",
 [Gen::Register::SI]  = "%rsi",
 [Gen::Register::DI]  = "%rdi",
 [Gen::Register::R8]  = "%r8",
 [Gen::Register::R9]  = "%r9",
 [Gen::Register::R10] = "%r10",
 [Gen::Register::R11] = "%r11",
 [Gen::Register::SP]  = "%rsp",
};