/*
 * Copyright (c) 2017 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/debug_stub/debug_stepi.h"
#include "native_client/src/trusted/debug_stub/platform.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"
#include "native_client/src/trusted/validator_arm/gen/arm32_decode.h"
#include "native_client/src/trusted/validator_arm/model.h"

namespace {

bool Bit(uint32_t reg, int index) {
  return (reg >> index) & 1;
}

// This should be called to perform stepi instruction, so it will
// be accessed from a single thread
const nacl_arm_dec::Arm32DecoderState& GetDecoderState() {
  static const nacl_arm_dec::Arm32DecoderState decode_state;
  return decode_state;
}

bool IsInstructionExecuted(port::Thread* thread,
                           nacl_arm_dec::Instruction inst) {
  using nacl_arm_dec::Instruction;

  // ARM processor conditional bits APSR - NZCV
  uint32_t cpsr = thread->GetContext()->cpsr;
  bool V = Bit(cpsr, 28);
  bool C = Bit(cpsr, 29);
  bool Z = Bit(cpsr, 30);
  bool N = Bit(cpsr, 31);

  // See description of nacl_arm_dec::Instruction::Condition enum in model.h
  switch (inst.GetCondition()) {
    case Instruction::EQ:
      return Z;
    case Instruction::NE:
      return !Z;
    case Instruction::CS:
      return C;
    case Instruction::CC:
      return !C;
    case Instruction::MI:
      return N;
    case Instruction::PL:
      return !N;
    case Instruction::VS:
      return V;
    case Instruction::VC:
      return !V;
    case Instruction::HI:
      return C && !Z;
    case Instruction::LS:
      return !C || Z;
    case Instruction::GE:
      return N == V;
    case Instruction::LT:
      return N != V;
    case Instruction::GT:
      return !Z && N == V;
    case Instruction::LE:
      return Z || N != V;
    case Instruction::AL:
      return true;
    case Instruction::UNCONDITIONAL:
      return true;
  }

  return true;
}

uint32_t ProcessIndirectBranch(port::Thread* thread,
                               const nacl_arm_dec::ClassDecoder& decoder,
                               nacl_arm_dec::Instruction inst) {
  nacl_arm_dec::Register reg = decoder.branch_target_register(inst);
  NaClSignalContext* ctx = thread->GetContext();

  switch (reg.number()) {
    case 0:
      return ctx->r0;
    case 1:
      return ctx->r1;
    case 2:
      return ctx->r2;
    case 3:
      return ctx->r3;
    case 4:
      return ctx->r4;
    case 5:
      return ctx->r5;
    case 6:
      return ctx->r6;
    case 7:
      return ctx->r7;
    case 8:
      return ctx->r8;
    case 9:
      return ctx->r9;
    case 10:
      return ctx->r10;
    case 11:
      return ctx->r11;
    case 12:
      return ctx->r12;
    case 13:
      return ctx->stack_ptr;
    case 14:
      return ctx->lr;
    case 15:
      return ctx->prog_ctr;
    default:
      return 0;
  }
}

}  // namespace

// In NaCl ARM we don't need to support all available instructions
// but only one allowed by sandbox and checked by validator.
// See:
// https://developer.chrome.com/native-client/reference/sandbox_internals/arm-32-bit-sandbox
uint32_t NaClNextInstructionAddr(port::Thread* thread) {
  uint32_t curr_pc = thread->GetContext()->prog_ctr;
  uint32_t curr_inst;

  if (!port::IPlatform::GetMemory(curr_pc, sizeof(curr_inst), &curr_inst))
    return 0;

  uint32_t next_pc_default = curr_pc + sizeof(curr_inst);
  nacl_arm_dec::Instruction inst(curr_inst);
  if (inst.Bits() == nacl_arm_dec::kBreakpoint ||
      inst.Bits() == nacl_arm_dec::kHaltFill ||
      inst.Bits() == nacl_arm_dec::kAbortNow) {
    // Special instruction - already breakpoint?
    return 0;
  }

  if (!IsInstructionExecuted(thread, inst))
    return next_pc_default;

  const nacl_arm_dec::ClassDecoder& decoder = GetDecoderState().decode(inst);

  nacl_arm_dec::Register target_register = decoder.branch_target_register(inst);
  if (!target_register.Equals(nacl_arm_dec::Register::None()))
    return ProcessIndirectBranch(thread, decoder, inst);

  if (decoder.is_relative_branch(inst))
    return curr_pc + decoder.branch_target_offset(inst);

  // TODO(a.bujalski) What about bundles?
  return next_pc_default;
}
