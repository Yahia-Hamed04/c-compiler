#pragma once
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
  Reg_Count
 };
 
 struct Immediate {
  int val;
 };
 
 struct Pseudo {
  Token name;
 };
 
 struct StackOffset {
  ptrdiff_t offset;
 };
 
 typedef std::variant<Immediate, Register, Pseudo, StackOffset> Operand;
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
};