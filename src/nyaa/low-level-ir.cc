#include "nyaa/low-level-ir.h"
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
    
} // namespace lir

} // namespace nyaa
    
} // namespace mai
