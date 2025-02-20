#include "code_gen.h"
#include "../helpers.h"
#include <unordered_map>
#include <type_traits>
using namespace Gen;

Generator::Generator(TACKYifier &tackyifier) {
 this->tackyifier = &tackyifier;
 this->symbols = tackyifier.symbols;
 
 generate();
}

Gen::Program Generator::get_program() {
 return program;
}

bool add_var(std::unordered_map<std::string, size_t> &vars, size_t &stack_alloc_amount, Operand &operand) {
 Pseudo *op = std::get_if<Pseudo>(&operand);
 if (op != nullptr)  {
  std::string op_name = op->name.to_string();
 
  if (!vars.count(op_name)) {
   vars[op_name] = stack_alloc_amount;
   stack_alloc_amount += 4;
  }
 
  operand = StackOffset{.offset = -ptrdiff_t(vars[op_name]) - 4};
 }

 return op != nullptr;
}

void Generator::add_inst (
 std::unordered_map<string, size_t> &vars,
 Gen::StackAlloc &stack_alloc,
 Gen::Instructions &insts,
 Gen::Instruction &&instruction
) {
 size_t &stack_alloc_amount = stack_alloc.amount;

 std::visit(overloaded{
   [&](auto &inst) {insts.push_back(inst);},
   [&](Mov &mov) {
    bool src = add_var(vars, stack_alloc_amount, mov.src);
    bool dst = add_var(vars, stack_alloc_amount, mov.dst);
    bool both_pseudo = src && dst;
    bool both_offset = std::holds_alternative<StackOffset>(mov.src) && std::holds_alternative<StackOffset>(mov.dst);

    if (both_pseudo || both_offset) {
     Operand dst = mov.dst;
     mov.dst = Register::R10;
     
     insts.push_back(mov);
     insts.push_back(Mov{.src = Register::R10, .dst = dst});
    } else insts.push_back(mov);
   },
   [&](Gen::Unary &un) {
    add_var(vars, stack_alloc_amount, un.operand);

    insts.push_back(un);
   },
   [&](Idiv &idiv) {
    Immediate *imm = std::get_if<Immediate>(&idiv.operand);

    if (imm != nullptr) {
     Operand op = idiv.operand;
     idiv.operand = Register::R10;
     insts.push_back(Mov{.src = op, .dst = Register::R10});
    } else {
     add_var(vars, stack_alloc_amount, idiv.operand);
    }

    insts.push_back(idiv);
   },
   [&](Gen::Binary &bin) {
    bool src = add_var(vars, stack_alloc_amount, bin.src);
    bool dst = add_var(vars, stack_alloc_amount, bin.dst);
    if (!dst) {
     insts.push_back(bin);
     return;
    }

    if (bin.op == Gen::BinaryOp::Mult) {
     Operand dst = bin.dst;
     bin.dst = Register::R11;
     
     insts.insert(insts.end(), {
      Mov{.src = dst, .dst = Register::R11},
      bin,
      Mov{.src = Register::R11, .dst = dst}
     });
    } else if (src) {
     Operand src = bin.src;
     Register reg = bin.op == Gen::BinaryOp::Sal || bin.op == Gen::BinaryOp::Sar ? Register::CX : Register::R10;
     bin.src = reg;
     
     insts.push_back(Mov{.src = src, .dst = reg});
     insts.push_back(bin);
    } else insts.push_back(bin);
   },
   [&](Cmp &cmp) {
    bool p1 = add_var(vars, stack_alloc_amount, cmp.op1);
    bool p2 = add_var(vars, stack_alloc_amount, cmp.op2);
    bool imm = std::holds_alternative<Immediate>(cmp.op2);

    if (p1 && p2) {
     insts.push_back(Mov{.src = cmp.op1, .dst = Register::R10});
     cmp.op1 = Register::R10;
    } else if (imm) {
     insts.push_back(Mov{.src = cmp.op2, .dst = Register::R11});
     cmp.op2 = Register::R11;
    }

    insts.push_back(cmp);
   },
   [&](Set_Condition &set) {
    add_var(vars, stack_alloc_amount, set.operand);

    insts.push_back(set);
   },
   [&](Push &push) {
    add_var(vars, stack_alloc_amount, push.operand);

    insts.push_back(push);
   }
  }, instruction);
}

Operand generate_operand(Value &value) {
 return std::visit(overloaded{
  [](/* TACKY::Constant */ int value) -> Operand {
   return Immediate{.val = value};
  },
  [](TACKY::Var value) -> Operand {
   return Pseudo{.name = value.name};
  },
 }, value);
}

bool is_relational_op(Parser::BinaryOp op) {
 return Parser::BinaryOp::Equal <= op && op <= Parser::BinaryOp::Greater_Or_Equal;
}

void Generator::generate() {
 TACKY::Program program = tackyifier->get_program();
 for (TACKY::Function &func : program.funcs) {
  Gen::Function function;
  function.name = func.name;
  function.instructions = generate(func);

  this->program.funcs.push_back(function);
 }
}

static const Register regs[6] = {DI, SI, DX, CX, R8, R9};
Gen::Instructions Generator::generate(TACKY::Function function) {
 Gen::Instructions insts;
 insts.push_back(Ret{});

 StackAlloc stack_alloc = {.amount = 0};
 std::unordered_map<std::string, size_t> vars;

 for (size_t i = 0; i < function.params.size(); i++) {
  TACKY::Value param = TACKY::Var{.name = function.params[i]};
  Gen::Mov mov;
  if (i < 6) {
   mov.src = regs[i];
  } else mov.src = StackOffset{.offset = ptrdiff_t(i - 4) * 8};

  mov.dst = generate_operand(param);

  add_inst(vars, stack_alloc, insts, mov);
 }
 
 for (TACKY::Instruction instruction : function.body) {
  std::visit(overloaded{
   [&](TACKY::Return inst) {    
    add_inst(vars, stack_alloc, insts, Mov{.src = generate_operand(inst.val), .dst = Register::AX});
    add_inst(vars, stack_alloc, insts, Ret{});
   },
   [&](TACKY::Unary inst) {;
    Operand src = generate_operand(inst.src);
    Operand dst = generate_operand(inst.dst);

    if (inst.op == Parser::UnaryOp::Not) {
     add_inst(vars, stack_alloc, insts, Cmp{.op1 = Immediate{.val = 0}, .op2 = src});
     add_inst(vars, stack_alloc, insts, Mov{.src = Immediate{.val = 0}, .dst = dst});
     add_inst(vars, stack_alloc, insts, Set_Condition{.condition = Condition::Equal, .operand = dst});
    } else {
     Gen::Unary un = {.operand = dst};
     switch (inst.op) {
      case Parser::UnaryOp::Complement: un.op = Gen::UnaryOp::Not; break;
      case Parser::UnaryOp::Negate:     un.op = Gen::UnaryOp::Neg; break;
      case Parser::UnaryOp::Increment:  un.op = Gen::UnaryOp::Inc; break;
      case Parser::UnaryOp::Decrement:  un.op = Gen::UnaryOp::Dec; break;
     }
     
     add_inst(vars, stack_alloc, insts, Mov{.src = src, .dst = dst});
     add_inst(vars, stack_alloc, insts, un);
    }
   },
   [&](TACKY::Binary inst) {
    Operand src1 = generate_operand(inst.src1);
    Operand src2 = generate_operand(inst.src2);
    Operand dst  = generate_operand(inst.dst);

    if (is_relational_op(inst.op)) {
     Set_Condition set = {.operand = dst};

     switch (inst.op) {
      case Parser::BinaryOp::Equal:            set.condition = Condition::Equal; break;
      case Parser::BinaryOp::Not_Equal:        set.condition = Condition::Not_Equal; break;
      case Parser::BinaryOp::Less_Than:        set.condition = Condition::Less; break;
      case Parser::BinaryOp::Less_Or_Equal:    set.condition = Condition::Less_Equal; break;
      case Parser::BinaryOp::Greater_Than:     set.condition = Condition::Greater; break;
      case Parser::BinaryOp::Greater_Or_Equal: set.condition = Condition::Greater_Equal; break;
     }
     
     add_inst(vars, stack_alloc, insts, Cmp{.op1 = src2, .op2 = src1});
     add_inst(vars, stack_alloc, insts, Mov{.src = Immediate{.val = 0}, .dst = dst});
     add_inst(vars, stack_alloc, insts, set);
    } else if (inst.op == Parser::BinaryOp::Divide || inst.op == Parser::BinaryOp::Remainder) {
     Register result_reg = inst.op == Parser::BinaryOp::Divide ? Register::AX : Register::DX;

     add_inst(vars, stack_alloc, insts, Mov{.src = src1, .dst = Register::AX});
     add_inst(vars, stack_alloc, insts, Cdq{});
     add_inst(vars, stack_alloc, insts, Idiv{.operand = src2});
     add_inst(vars, stack_alloc, insts, Mov{.src = result_reg, .dst = dst});
    } else {
     Gen::Binary bin = {.src = src2, .dst = dst};

     switch (inst.op) {
      case Parser::BinaryOp::Bitwise_And:  bin.op = Gen::BinaryOp::And;  break;
      case Parser::BinaryOp::Bitwise_Or:   bin.op = Gen::BinaryOp::Or;   break;
      case Parser::BinaryOp::Exclusive_Or: bin.op = Gen::BinaryOp::Xor;  break;
      case Parser::BinaryOp::Shift_Left:   bin.op = Gen::BinaryOp::Sal;  break;
      case Parser::BinaryOp::Shift_Right:  bin.op = Gen::BinaryOp::Sar;  break;
      case Parser::BinaryOp::Addition:     bin.op = Gen::BinaryOp::Add;  break;
      case Parser::BinaryOp::Subtract:     bin.op = Gen::BinaryOp::Sub;  break;
      case Parser::BinaryOp::Multiply:     bin.op = Gen::BinaryOp::Mult; break;
     }
     
     add_inst(vars, stack_alloc, insts, Mov{.src = src1, .dst = dst});
     add_inst(vars, stack_alloc, insts, bin);
    }
   },
   [&](TACKY::FunCall inst) {
    int args_len = inst.args.size();
    size_t padding = 8 * (args_len > 6 && args_len % 2);
    if (padding > 0) {
     add_inst(vars, stack_alloc, insts, StackAlloc{.amount = padding});
    }

    int i;
    for (i = 0; i < args_len && i < 6; i++) {
     Operand op = generate_operand(inst.args[i]);
     add_inst(vars, stack_alloc, insts, Mov{.src = op, .dst = regs[i]});
    }
    for (i = args_len - 1; i > 5; i--) {
     Operand op = generate_operand(inst.args[i]);
     bool reg_or_imm = std::holds_alternative<Register>(op) || std::holds_alternative<Immediate>(op);

     if (reg_or_imm) {
      add_inst(vars, stack_alloc, insts, Push{.operand = op});
     } else {
      add_inst(vars, stack_alloc, insts, Mov{.src = op, .dst = Register::AX});
      add_inst(vars, stack_alloc, insts, Push{.operand = Register::AX});
     }
    }

    add_inst(vars, stack_alloc, insts, Call{.name = inst.name});
    size_t bytes_to_remove = 8 * std::max(args_len - 6, 0) + padding;
    if (bytes_to_remove > 0) {
     add_inst(vars, stack_alloc, insts, StackFree{.amount = bytes_to_remove});
    }

    add_inst(vars, stack_alloc, insts, Mov{.src = Register::AX, .dst = generate_operand(inst.dst)});
   },
   [&](TACKY::Copy inst) {
    add_inst(vars, stack_alloc, insts, 
     Mov{.src = generate_operand(inst.src), .dst = generate_operand(inst.dst)}
    );
   },
   [&](TACKY::Label inst) {
    add_inst(vars, stack_alloc, insts, Gen::Label{.name = inst.name.name});
   },
   [&](TACKY::Jump inst) {
    add_inst(vars, stack_alloc, insts, Jmp{.target = inst.target.name});
   },
   [&](TACKY::JumpIfZero inst) {
    add_inst(vars, stack_alloc, insts, Cmp{.op1 = Immediate{.val = 0}, .op2 = generate_operand(inst.val)});
    add_inst(vars, stack_alloc, insts, Conditional_Jmp{.condition = Condition::Equal, .target = inst.target.name});
   },
   [&](TACKY::JumpIfNotZero inst) {
    add_inst(vars, stack_alloc, insts, Cmp{.op1 = Immediate{.val = 0}, .op2 = generate_operand(inst.val)});
    add_inst(vars, stack_alloc, insts, Conditional_Jmp{.condition = Condition::Not_Equal, .target = inst.target.name});
   }
  }, instruction);
 }

 stack_alloc.amount += 16 - (stack_alloc.amount % 16);
 insts[0] = stack_alloc;
 return insts;
}