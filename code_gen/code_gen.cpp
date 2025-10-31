#include "code_gen.h"
#include "../helpers.h"
#include <unordered_map>
#include <type_traits>
using namespace Gen;

Generator::Generator(TACKYifier &tackyifier) {
 this->tackyifier = &tackyifier;
 for (auto [name, entry] : *tackyifier.symbols) {
  asm_table[name] = {
   .type = entry.type == Parser::Type::Int ? AssemblyType::Longword : AssemblyType::Quadword,
   .is_static = entry.attr_type == Parser::AttrType::Static,
   .defined = entry.defined,
  };
 }
 
 generate();
}

Gen::Program Generator::get_program() {
 return program;
}

int get_alignment(AssemblyType type) {
 switch (type) {
  case AssemblyType::Longword: return 4;
  case AssemblyType::Quadword: return 8;
  default: error("Unhandled assembly type");
 }

 return 0;
}

bool Generator::add_var(
 std::unordered_map<std::string, size_t> &vars,
 size_t &stack_alloc_amount,
 AssemblyType type,
 Operand &operand
) {
 Pseudo *op = std::get_if<Pseudo>(&operand.var);
 if (op == nullptr) return std::holds_alternative<StackOffset>(operand.var);

 std::string op_name = op->name.to_string();
 if (asm_table[op_name].is_static) {
  operand.var = Data{.name = op->name};
  return true;
 }
 
 if (!vars.count(op_name)) {
  int op_alignment = get_alignment(type);
  stack_alloc_amount += op_alignment;
  if (stack_alloc_amount % op_alignment != 0) {
   stack_alloc_amount += op_alignment - (stack_alloc_amount % op_alignment);
  }
  
  vars[op_name] = stack_alloc_amount;
 }

 operand.var = StackOffset{.offset = -ptrdiff_t(vars[op_name])};
 return true;
}

void Generator::add_inst (
 std::unordered_map<string, size_t> &vars,
 size_t &stack_alloc_amount,
 Gen::Instructions &insts,
 Gen::Instruction &&instruction
) {
 std::visit(overloaded{
   [&](auto &inst) {insts.push_back(inst);},
   [&](Mov &mov) {
    if (mov.src == mov.dst) return;

    bool src = add_var(vars, stack_alloc_amount, mov.type, mov.src);
    bool dst = add_var(vars, stack_alloc_amount, mov.type, mov.dst);
    bool both_pseudo = src && dst;
    bool src_is_quad_const = std::holds_alternative<Immediate>(mov.src.var) && mov.src.type == AssemblyType::Quadword;
    bool one_quad_const_one_offset = src_is_quad_const && dst;

    if (src_is_quad_const && mov.type == AssemblyType::Longword) {
     mov.src = truncate(std::get<Immediate>(mov.src.var).val);
    }

    if (both_pseudo || one_quad_const_one_offset) {
     Operand dst = mov.dst;
     mov.dst = Register::R10;
     
     insts.push_back(mov);
     insts.push_back(Mov{.type = mov.type, .src = Register::R10, .dst = dst});
    } else insts.push_back(mov);
   },
   [&](Movsx &mov) {
    add_var(vars, stack_alloc_amount, AssemblyType::Longword, mov.src);
    add_var(vars, stack_alloc_amount, AssemblyType::Quadword, mov.dst);
    bool src_is_not_reg = !std::holds_alternative<Register>(mov.src.var);
    bool dst_is_not_reg = !std::holds_alternative<Register>(mov.dst.var);

    if (src_is_not_reg) {
     insts.push_back(Mov{.type = AssemblyType::Longword, .src = mov.src, .dst = Register::R10});
     mov.src = Register::R10;
    }

    if (dst_is_not_reg) {
     Operand dst = mov.dst;
     mov.dst = Register::R11;

     insts.push_back(mov);
     insts.push_back(Mov{.type = AssemblyType::Quadword, .src = Register::R11, .dst = dst});
    } else insts.push_back(mov);
   },
   [&](Movzx &mov) {
    add_var(vars, stack_alloc_amount, AssemblyType::Longword, mov.src);
    add_var(vars, stack_alloc_amount, AssemblyType::Quadword, mov.dst);
    bool dst_is_reg = std::holds_alternative<Register>(mov.dst.var);

    if (dst_is_reg) {
     insts.push_back(Mov{.type = AssemblyType::Longword, .src = mov.src, .dst = mov.dst});
    } else {
     insts.push_back(Mov{.type = AssemblyType::Longword, .src = mov.src, .dst = Register::R11});
     insts.push_back(Mov{.type = AssemblyType::Quadword, .src = Register::R11, .dst = mov.dst});
    }
   },
   [&](Gen::Unary &un) {
    add_var(vars, stack_alloc_amount, un.type, un.operand);
    bool is_quad_const = std::holds_alternative<Immediate>(un.operand.var) && un.operand.type == AssemblyType::Quadword;
    
    insts.push_back(un);
   },
   [&](Div &div) {
    Immediate *imm = std::get_if<Immediate>(&div.operand.var);

    if (imm != nullptr) {
     Operand op = div.operand;
     div.operand = Register::R10;
     insts.push_back(Mov{.type = div.type, .src = op, .dst = Register::R10});
    } else {
     add_var(vars, stack_alloc_amount, div.type, div.operand);
    }

    insts.push_back(div);
   },
   [&](Gen::Binary &bin) {
    bool src = add_var(vars, stack_alloc_amount, bin.type, bin.src);
    bool dst = add_var(vars, stack_alloc_amount, bin.type, bin.dst);
    if (!dst) {
     insts.push_back(bin);
     return;
    }

    bool src_is_quad_const = std::holds_alternative<Immediate>(bin.src.var) && bin.src.type == AssemblyType::Quadword;

    if (bin.op == Gen::BinaryOp::Mult) {
     Operand dst = bin.dst;
     bin.dst = Register::R11;
     
     if (src_is_quad_const) {
      insts.push_back(Mov{.type = bin.type, .src = bin.src, .dst = Register::R10});
      bin.src = Register::R10;
     }
     
     insts.insert(insts.end(), {
      Mov{.type = dst.type, .src = dst, .dst = Register::R11},
      bin,
      Mov{.type = dst.type, .src = Register::R11, .dst = dst}
     });
    } else if (src || src_is_quad_const) {
     Operand src = bin.src;
     Register reg = 
       bin.op == Gen::BinaryOp::Sal || 
       bin.op == Gen::BinaryOp::Sar || 
       bin.op == Gen::BinaryOp::Shl || 
       bin.op == Gen::BinaryOp::Shr ?  Register::CX : Register::R10;
     bin.src = reg;
     
     insts.push_back(Mov{.type = bin.type, .src = src, .dst = reg});
     insts.push_back(bin);
    } else insts.push_back(bin);
   },
   [&](Cmp &cmp) {
    bool p1 = add_var(vars, stack_alloc_amount, cmp.type, cmp.op1);
    bool p2 = add_var(vars, stack_alloc_amount, cmp.type, cmp.op2);
    bool i1 = std::holds_alternative<Immediate>(cmp.op1.var);
    bool i2 = std::holds_alternative<Immediate>(cmp.op2.var);
    bool op1_is_quad_const = i1 && cmp.op1.type == AssemblyType::Quadword;

    if ((p1 && p2) || op1_is_quad_const || (i1 && cmp.type == AssemblyType::Quadword)) {
     insts.push_back(Mov{.type = cmp.op1.type, .src = cmp.op1, .dst = Register::R10});
     cmp.op1 = Register::R10;
    } 
    
    if (i2) {
     insts.push_back(Mov{.type = cmp.op2.type, .src = cmp.op2, .dst = Register::R11});
     cmp.op2 = Register::R11;
    }

    insts.push_back(cmp);
   },
   [&](Set_Condition &set) {
    add_var(vars, stack_alloc_amount, set.operand.type, set.operand);

    insts.push_back(set);
   },
   [&](Push &push) {
    add_var(vars, stack_alloc_amount, push.operand.type, push.operand);
    bool is_quad_const = std::holds_alternative<Immediate>(push.operand.var) && push.operand.type == AssemblyType::Quadword;

    if (is_quad_const) {
     insts.push_back(Mov{.type = push.operand.type, .src = push.operand, .dst = Register::R10});
     push.operand = Register::R10;
    }

    insts.push_back(push);
   },
  }, instruction);
}

Operand Generator::generate_operand(Value &value, bool print) {
 Operand operand;
 std::visit(overloaded{
  [&](Constant value) {
   operand = value._const;
   operand.set_type(value.type, value);

   if (print) std::cout << static_cast<int>(operand.type) << '\n';
  },
  [&](Var value) {
   operand.set_type(value.type, value);
   operand.var = Pseudo{.name = value.name};

   if (print) std::cout << static_cast<int>(operand.type) << '\n';
  },
 }, value);

 return operand;
}

bool is_relational_op(Parser::BinaryOp op) {
 return Parser::BinaryOp::Equal <= op && op <= Parser::BinaryOp::Greater_Or_Equal;
}

void Generator::generate() {
 TACKY::Program program = tackyifier->get_program();
 for (TACKY::Function &func : program.funcs) {
  Gen::Function function;
  function.name = func.name;
  function.global = func.global;
  function.instructions = generate(func);

  this->program.funcs.push_back(function);
 }

 for (TACKY::StaticVariable &var : program.statics) {
  Gen::StaticVariable variable;
  variable.name = var.name;
  variable.init = var.init;
  variable.init_val_type = var.init_val_type;
  variable.global = var.global;
  variable.alignment = var.type == Parser::Type::Int ? 4 : 8;

  this->program.statics.push_back(variable);
 }
}

Gen::Binary stack_alloc(size_t amount) {
 return Gen::Binary{.type = AssemblyType::Quadword, .op = BinaryOp::Sub, .src = amount, .dst = Register::SP};
}

Gen::Binary stack_free(size_t amount) {
 return Gen::Binary{.type = AssemblyType::Quadword, .op = BinaryOp::Add, .src = amount, .dst = Register::SP};
}

static const Register regs[6] = {DI, SI, DX, CX, R8, R9};
Gen::Instructions Generator::generate(TACKY::Function function) {
 Gen::Instructions insts;
 insts.push_back(Ret{});

 size_t stack_alloc_amount = 0;
 std::unordered_map<std::string, size_t> vars;

 for (size_t i = 0; i < function.params.size(); i++) {
  TACKY::Value param = Var(function.params[i]);
  Mov mov;
  if (i < 6) {
   mov.src = regs[i];
  } else mov.src.var = StackOffset{.offset = ptrdiff_t(i - 4) * 8};

  mov.dst = generate_operand(param);
  mov.type = mov.dst.type;

  add_inst(vars, stack_alloc_amount, insts, mov);
 }
 
 for (TACKY::Instruction instruction : function.body) {
  std::visit(overloaded{
   [&](TACKY::Return inst) {    
    Operand src = generate_operand(inst.val);
    add_inst(vars, stack_alloc_amount, insts, Mov{.type = src.type, .src = src, .dst = Register::AX});
    add_inst(vars, stack_alloc_amount, insts, Ret{});
   },
   [&](TACKY::Unary inst) {
    Operand src = generate_operand(inst.src);
    Operand dst = generate_operand(inst.dst);

    if (inst.op == Parser::UnaryOp::Not) {
     add_inst(vars, stack_alloc_amount, insts, Cmp{.type = src.type, .op1 = 0, .op2 = src});
     add_inst(vars, stack_alloc_amount, insts, Mov{.type = dst.type, .src = 0, .dst = dst});
     add_inst(vars, stack_alloc_amount, insts, Set_Condition{.condition = Condition::Equal, .operand = dst});
    } else {
     Gen::Unary un = {.type = src.type, .operand = dst};
     switch (inst.op) {
      case Parser::UnaryOp::Complement: un.op = Gen::UnaryOp::Not; break;
      case Parser::UnaryOp::Negate:     un.op = Gen::UnaryOp::Neg; break;
      case Parser::UnaryOp::Increment:  un.op = Gen::UnaryOp::Inc; break;
      case Parser::UnaryOp::Decrement:  un.op = Gen::UnaryOp::Dec; break;
     }
     
     add_inst(vars, stack_alloc_amount, insts, Mov{.type = src.type, .src = src, .dst = dst});
     add_inst(vars, stack_alloc_amount, insts, un);
    }
   },
   [&](TACKY::Binary inst) {
    Operand src1 = generate_operand(inst.src1);
    Operand src2 = generate_operand(inst.src2);
    Operand dst  = generate_operand(inst.dst);
    bool src1_is_signed = src1.is_signed;
    bool is_signed = src1.is_signed || src2.is_signed;
    src1.is_signed = is_signed;
    src2.is_signed = is_signed;
    dst.is_signed = is_signed;

    if (is_relational_op(inst.op)) {
     Set_Condition set = {.operand = dst};

     switch (inst.op) {
      case Parser::BinaryOp::Equal:            set.condition = Condition::Equal; break;
      case Parser::BinaryOp::Not_Equal:        set.condition = Condition::Not_Equal; break;
      case Parser::BinaryOp::Less_Than:        set.condition = is_signed ? Condition::Less : Condition::Below; break;
      case Parser::BinaryOp::Less_Or_Equal:    set.condition = is_signed ? Condition::Less_Equal : Condition::Below_Equal; break;
      case Parser::BinaryOp::Greater_Than:     set.condition = is_signed ? Condition::Greater : Condition::Above; break;
      case Parser::BinaryOp::Greater_Or_Equal: set.condition = is_signed ? Condition::Greater_Equal : Condition::Above_Equal; break;
     }
     
     add_inst(vars, stack_alloc_amount, insts, Cmp{.type = src1.type, .op1 = src2, .op2 = src1});
     add_inst(vars, stack_alloc_amount, insts, Mov{.type = dst.type, .src = 0, .dst = dst});
     add_inst(vars, stack_alloc_amount, insts, set);
    } else if (inst.op == Parser::BinaryOp::Divide || inst.op == Parser::BinaryOp::Remainder) {
     Register result_reg = inst.op == Parser::BinaryOp::Divide ? Register::AX : Register::DX;

     add_inst(vars, stack_alloc_amount, insts, Mov{.type = src1.type, .src = src1, .dst = Register::AX});
     if (is_signed) {
      add_inst(vars, stack_alloc_amount, insts, Cdq{.type = src1.type});
     } else {
      add_inst(vars, stack_alloc_amount, insts, Mov{.type = src1.type, .src = 0, .dst = Register::DX});
     }
     add_inst(vars, stack_alloc_amount, insts, Div{.type = src1.type, .operand = src2});
     add_inst(vars, stack_alloc_amount, insts, Mov{.type = src1.type, .src = result_reg, .dst = dst});
    } else {
     Gen::Binary bin = {.type = dst.type, .src = src2, .dst = dst};

     switch (inst.op) {
      case Parser::BinaryOp::Bitwise_And:  bin.op = Gen::BinaryOp::And;  break;
      case Parser::BinaryOp::Bitwise_Or:   bin.op = Gen::BinaryOp::Or;   break;
      case Parser::BinaryOp::Exclusive_Or: bin.op = Gen::BinaryOp::Xor;  break;
      case Parser::BinaryOp::Shift_Left:   bin.op = src1_is_signed ? Gen::BinaryOp::Sal : Gen::BinaryOp::Shl;  break;
      case Parser::BinaryOp::Shift_Right:  bin.op = src1_is_signed ? Gen::BinaryOp::Sar : Gen::BinaryOp::Shr;  break;
      case Parser::BinaryOp::Addition:     bin.op = Gen::BinaryOp::Add;  break;
      case Parser::BinaryOp::Subtract:     bin.op = Gen::BinaryOp::Sub;  break;
      case Parser::BinaryOp::Multiply:     bin.op = Gen::BinaryOp::Mult; break;
     }
     
     add_inst(vars, stack_alloc_amount, insts, Mov{.type = src1.type, .src = src1, .dst = dst});
     add_inst(vars, stack_alloc_amount, insts, bin);
    }
   },
   [&](TACKY::FunCall inst) {
    int args_len = inst.args.size();
    size_t padding = 8 * (args_len > 6 && args_len % 2);
    if (padding > 0) {
     add_inst(vars, stack_alloc_amount, insts, stack_alloc(padding));
    }

    for (int i = 0; i < args_len && i < 6; i++) {
     Operand op = generate_operand(inst.args[i]);
     add_inst(vars, stack_alloc_amount, insts, Mov{.type = op.type, .src = op, .dst = regs[i]});
    }
    
    for (int i = args_len - 1; i > 5; i--) {
     Operand op = generate_operand(inst.args[i]);
     bool imm = std::holds_alternative<Immediate>(op.var);

     if (imm || op.type == AssemblyType::Quadword) {
      add_inst(vars, stack_alloc_amount, insts, Push{.operand = op});
     } else {
      add_inst(vars, stack_alloc_amount, insts, Mov{.type = AssemblyType::Longword, .src = op, .dst = Register::AX});
      add_inst(vars, stack_alloc_amount, insts, Push{.operand = Register::AX});
     }
    }

    add_inst(vars, stack_alloc_amount, insts, Call{.name = inst.name});
    size_t bytes_to_remove = 8 * std::max(args_len - 6, 0) + padding;
    if (bytes_to_remove > 0) {
     add_inst(vars, stack_alloc_amount, insts, stack_free(bytes_to_remove));
    }

    Operand dst = generate_operand(inst.dst);
    add_inst(vars, stack_alloc_amount, insts, Mov{.type = dst.type, .src = Register::AX, .dst = dst});
   },
   [&](TACKY::Copy inst) {
    Operand src = generate_operand(inst.src);
    add_inst(vars, stack_alloc_amount, insts, 
     Mov{.type = src.type, .src = src, .dst = generate_operand(inst.dst)}
    );
   },
   [&](TACKY::Label inst) {
    add_inst(vars, stack_alloc_amount, insts, Gen::Label{.name = inst.name.name});
   },
   [&](TACKY::Jump inst) {
    add_inst(vars, stack_alloc_amount, insts, Jmp{.target = inst.target.name});
   },
   [&](TACKY::JumpIfZero inst) {
    Operand op2 = generate_operand(inst.val);
    add_inst(vars, stack_alloc_amount, insts, Cmp{.type = op2.type, .op1 = 0, .op2 = op2});
    add_inst(vars, stack_alloc_amount, insts, Conditional_Jmp{.condition = Condition::Equal, .target = inst.target.name});
   },
   [&](TACKY::JumpIfNotZero inst) {
    Operand op2 = generate_operand(inst.val);
    add_inst(vars, stack_alloc_amount, insts, Cmp{.type = op2.type, .op1 = 0, .op2 = op2});
    add_inst(vars, stack_alloc_amount, insts, Conditional_Jmp{.condition = Condition::Not_Equal, .target = inst.target.name});
   },
   [&](Truncate inst) {
    add_inst(vars, stack_alloc_amount, insts, Mov{.type = AssemblyType::Longword, .src = generate_operand(inst.src), .dst = generate_operand(inst.dst)});
   },
   [&](SignExtend inst) {
    add_inst(vars, stack_alloc_amount, insts, Movsx{.src = generate_operand(inst.src), .dst = generate_operand(inst.dst)});
   },
   [&](ZeroExtend inst) {
    add_inst(vars, stack_alloc_amount, insts, Movzx{.src = generate_operand(inst.src), .dst = generate_operand(inst.dst)});
   }
  }, instruction);
 }

 stack_alloc_amount += 16 - (stack_alloc_amount % 16);
 insts[0] = stack_alloc(stack_alloc_amount);
 return insts;
}