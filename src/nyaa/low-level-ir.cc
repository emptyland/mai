#include "nyaa/low-level-ir.h"
#include "nyaa/runtime.h"
#if defined(MAI_ARCH_X64)
#include "asm/x64/asm-x64.h"
#endif // defined(MAI_ARCH_X64)

namespace mai {
    
namespace nyaa {

namespace lir {
    
#if defined(MAI_ARCH_X64)

constexpr RegisterOperand kAllX64Regs[] = {
    RegisterOperand(0), // RAX
    RegisterOperand(1), // RCX
    RegisterOperand(2), // RDX
    RegisterOperand(3), // RBX
    
    RegisterOperand(4), // RSP
    RegisterOperand(5), // RBP
    RegisterOperand(6), // RSI
    RegisterOperand(7), // RDI
    
    RegisterOperand(8),  // R8
    RegisterOperand(9),  // R9
    RegisterOperand(10), // R10
    RegisterOperand(11), // R11
    
    RegisterOperand(12), // R12
    RegisterOperand(13), // R13
    RegisterOperand(14), // R14
    RegisterOperand(15), // R15
};
    
const RegisterOperand *Architecture::kAllocatableRegisters[kMaxAllocatableRegisters] = {
    &kAllX64Regs[x64::kRAX],
    &kAllX64Regs[x64::kRBX],
    &kAllX64Regs[x64::kRDX],
    &kAllX64Regs[x64::kRCX],
    &kAllX64Regs[x64::kRSI],
    &kAllX64Regs[x64::kRDI],
    &kAllX64Regs[x64::kR8],
    &kAllX64Regs[x64::kR9], // r10,r11,r12,r13 be used
    &kAllX64Regs[x64::kR14],
    &kAllX64Regs[x64::kR15],
};
    
constexpr FPRegisterOperand kAllX64FPRegs[] = {
    FPRegisterOperand(0), // xmm0
    FPRegisterOperand(1), // xmm1
    FPRegisterOperand(2), // xmm2
    FPRegisterOperand(3), // xmm3
    
    FPRegisterOperand(4), // xmm4
    FPRegisterOperand(5), // xmm5
    FPRegisterOperand(6), // xmm6
    FPRegisterOperand(7), // xmm7
    
    FPRegisterOperand(8), // xmm8
    FPRegisterOperand(9), // xmm9
    FPRegisterOperand(10), // xmm10
    FPRegisterOperand(11), // xmm11
    
    FPRegisterOperand(12), // xmm12
    FPRegisterOperand(13), // xmm13
    FPRegisterOperand(14), // xmm14
    FPRegisterOperand(15), // xmm15
};

const RegisterOperand *Architecture::kAllRegisters[kMaxRegisters] = {
    &kAllX64Regs[0],  &kAllX64Regs[1],  &kAllX64Regs[2],  &kAllX64Regs[3],
    &kAllX64Regs[4],  &kAllX64Regs[5],  &kAllX64Regs[6],  &kAllX64Regs[7],
    &kAllX64Regs[8],  &kAllX64Regs[9],  &kAllX64Regs[10], &kAllX64Regs[11],
    &kAllX64Regs[12], &kAllX64Regs[13], &kAllX64Regs[14], &kAllX64Regs[15],
};

const FPRegisterOperand *Architecture::kAllFPRegisters[kMaxFPRegisters] = {
    &kAllX64FPRegs[0],  &kAllX64FPRegs[1],  &kAllX64FPRegs[2],  &kAllX64FPRegs[3],
    &kAllX64FPRegs[4],  &kAllX64FPRegs[5],  &kAllX64FPRegs[6],  &kAllX64FPRegs[7],
    &kAllX64FPRegs[8],  &kAllX64FPRegs[9],  &kAllX64FPRegs[10], &kAllX64FPRegs[11],
    &kAllX64FPRegs[12], &kAllX64FPRegs[13], &kAllX64FPRegs[14], &kAllX64FPRegs[15],
};

//    kRAX = 0,
//    kRCX,
//    kRDX,
//    kRBX,
//    kRSP, // 4
//
//    kRBP, // 5
//    kRSI,
//    kRDI,
//    kR8,
//    kR9,  // 9
//
//    kR10, // 10
//    kR11,
//    kR12,
//    kR13,
//    kR14, // 14
//
//    kR15,
const char *Architecture::kRegisterNames[kMaxRegisters] = {
    "rax", "rcx", "rdx", "rbx",
    "rsp", "rbp", "rsi", "rdi",
    "r8",  "r9",  "r10", "r11",
    "r12", "r13", "r14", "r15",
};

const char *Architecture::kFPRegisterNames[kMaxFPRegisters] = {
    "xmm0",  "xmm1",  "xmm2",  "xmm3",
    "xmm4",  "xmm5",  "xmm6",  "xmm7",
    "xmm8",  "xmm9",  "xmm10", "xmm11",
    "xmm12", "xmm13", "xmm14", "xmm15",
};

#endif // defined(MAI_ARCH_X64)
    
const char *kIRCodeNames[kMaxIRCodes] = {
#define DEFINE_IR_CODE_NAME(name, ...) #name,
    DECL_LIR_CODE(DEFINE_IR_CODE_NAME)
#undef DEFINE_IR_CODE_NAME
};
    
constexpr MemoryOperand kL32StackSlots[] = {
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 0),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 1),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 2),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 3),
    
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 4),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 5),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 6),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 7),
    
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 8),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 9),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 10),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 11),
    
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 12),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 13),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 14),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 15),
    
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 16),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 17),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 18),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 19),
    
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 20),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 21),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 22),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 23),
    
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 24),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 25),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 26),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 27),
    
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 28),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 29),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 30),
    MemoryOperand(Architecture::kVMBPCode, kPointerSize * 31),
};
    
const MemoryOperand *Architecture::kLowStackSlots[kMaxStackSlots] = {
    &kL32StackSlots[0],  &kL32StackSlots[1],  &kL32StackSlots[2],  &kL32StackSlots[3],
    &kL32StackSlots[4],  &kL32StackSlots[5],  &kL32StackSlots[6],  &kL32StackSlots[7],
    &kL32StackSlots[8],  &kL32StackSlots[9],  &kL32StackSlots[10], &kL32StackSlots[11],
    &kL32StackSlots[12], &kL32StackSlots[13], &kL32StackSlots[14], &kL32StackSlots[15],
    &kL32StackSlots[16], &kL32StackSlots[17], &kL32StackSlots[18], &kL32StackSlots[19],
    &kL32StackSlots[20], &kL32StackSlots[21], &kL32StackSlots[22], &kL32StackSlots[23],
    &kL32StackSlots[24], &kL32StackSlots[25], &kL32StackSlots[26], &kL32StackSlots[27],
    &kL32StackSlots[28], &kL32StackSlots[29], &kL32StackSlots[30], &kL32StackSlots[31],
};
    
/*virtual*/ void ImmediateOperand::PrintTo(FILE *fp) const {
    switch (bits_) {
        case 0:
            ::fprintf(fp, "%p", addr_value());
            break;
        case 1:
            ::fprintf(fp, "#%d", i1_value());
            break;
        case 8:
            ::fprintf(fp, "#%d", i8_value());
            break;
        case 16:
            ::fprintf(fp, "#%d", i16_value());
            break;
        case 32:
            ::fprintf(fp, "#%d", i32_value());
            break;
        case 64:
            ::fprintf(fp, "#%" PRId64, i64_value());
            break;
        default:
            break;
    }
}
    
void Instruction::PrintTo(FILE *fp) {
    ::fprintf(fp, "%s", kIRCodeNames[op_]);
    if (output_) {
        ::fprintf(fp, " ");
        output_->PrintTo(fp);
    }
    if (n_inputs_ > 0) {
        if (output_) {
            ::fprintf(fp, " <- ");
        } else {
            ::fprintf(fp, " ");
        }
        for (int i = 0; i < n_inputs_; ++i) {
            if (i > 0) {
                ::fprintf(fp, ", ");
            }
            input(i)->PrintTo(fp);
        }
    }
}

} // namespace lir

} // namespace nyaa
    
} // namespace mai
