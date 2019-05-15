#include "asm/x64/asm-x64.h"

namespace mai {
    
namespace x64 {
    
const Register rax{kRAX}; // 0
const Register rcx{kRCX}; // 1
const Register rdx{kRDX}; // 2
const Register rbx{kRBX}; // 3
const Register rsp{kRSP}; // 4

const Register rbp{kRBP}; // 5
const Register rsi{kRSI}; // 6
const Register rdi{kRDI}; // 7
const Register r8{kR8};  // 8
const Register r9{kR9};  // 9

const Register r10{kR10}; // 10
const Register r11{kR11}; // 11
const Register r12{kR12}; // 12
const Register r13{kR13}; // 13
const Register r14{kR14}; // 14

const Register r15{kR15}; // 15
const Register rNone{-1}; // -1
    
// 128bit xmm registers:
const Xmm xmm0  = {0}; // 0
const Xmm xmm1  = {1}; // 1
const Xmm xmm2  = {2}; // 2
const Xmm xmm3  = {3}; // 3
const Xmm xmm4  = {4}; // 4

const Xmm xmm5  = {5}; // 5
const Xmm xmm6  = {6}; // 6
const Xmm xmm7  = {7}; // 7
const Xmm xmm8  = {8}; // 8
const Xmm xmm9  = {9}; // 9

const Xmm xmm10 = {10}; // 10
const Xmm xmm11 = {11}; // 11
const Xmm xmm12 = {12}; // 12
const Xmm xmm13 = {13}; // 13
const Xmm xmm14 = {14}; // 14

const Xmm xmm15 = {15}; // 15
    
const Register kRegArgv[kMaxRegArgs] {
    rdi, // {kRDI},
    rsi, // {kRSI},
    rdx, // {kRDX},
    rcx, // {kRCX},
    r8,  // {kR8},
    r9,  // {kR9},
    r10, // {kR10},
    r11, // {kR11},
};

const Xmm kXmmArgv[kMaxXmmArgs] {
    xmm0,
    xmm1,
    xmm2,
    xmm3,
    xmm4,
    xmm5,
    xmm6,
    xmm7,
};

// The non-allocatable registers are:
// rsp - stack pointer
// rbp - frame pointer
// r10 - fixed scratch register
// r12 - smi constant register
// r13 - root register
//
const Register kRegAlloc[kMaxAllocRegs] {
    {kRAX}, // 0
    {kRBX},
    {kRDX},
    {kRCX},
    {kRSI}, // 4
    {kRDI},
    {kR8},
    {kR9},
    {kR11}, // 8
    {kR14},
    {kR15},
};

// [base + disp/r]
Operand::Operand(Register base, int32_t disp)
    : rex_(0)
    , len_(1) {
    if (base.code == rsp.code || base.code == r12.code) {
        // From v8:
        // SIB byte is needed to encode (rsp + offset) or (r12 + offset)
        SetSIB(times_1, rsp, base);
    }
    if (disp == 0 && base.code != rbp.code && base.code != r13.code) {
        SetModRM(0, base);
    } else if (IsIntN(disp, 8)) {
        SetModRM(1, base);
        SetDisp8(disp);
    } else {
        SetModRM(2, base);
        SetDisp32(disp);
    }
}

// [base + index * scale + disp/r]
Operand::Operand(Register base, Register index, ScaleFactor scale,
                 int32_t disp)
    : rex_(0)
    , len_(1) {
    DCHECK(index.code != rsp.code);

    SetSIB(scale, index, base);
    if (disp == 0 && base.code != rbp.code && base.code != r13.code) {
        SetModRM(0, rsp);
    } else if (IsIntN(disp, 8)) {
        SetModRM(1, rsp);
        SetDisp8(disp);
    } else {
        SetModRM(2, rsp);
        SetDisp32(disp);
    }
}

// [index * scale + disp/r]
Operand::Operand(Register index, ScaleFactor scale, int32_t disp)
    : rex_(0)
    , len_(1) {
    SetModRM(0, rsp);
    SetSIB(scale, index, rbp);
    SetDisp32(disp);
}

    
void Assembler::Emit_movq(Register dst, Register src, int size) {
    if (RegLoBits(dst) == 4) {
        EmitRex(src, dst, size);
        EmitB(0x89);
        EmitModRM(src, dst);
    } else {
        EmitRex(dst, src, size);
        EmitB(0x8B);
        EmitModRM(dst, src);
    }
}
    
void Assembler::Emit_movq(Register dst, Immediate src, int size) {
    EmitRex(dst, size);
    if (size == 8) {
        EmitB(0xC7);
        EmitModRM(0x0, dst);
    } else {
        DCHECK_EQ(size, 4);
        EmitB(0xB8 + RegLoBits(dst));
    }
    EmitDW(src.value());
}

void Assembler::Emit_movb(Register dst, Register src) {
    if (!RegIsByte(dst)) {
        EmitRex32(src, dst);
    } else {
        EmitOptionalRex32(src, dst);
    }
    EmitB(0x88);
    EmitModRM(src, dst);
}
    
void Assembler::Emit_movb(Register dst, Operand src) {
    if (!RegIsByte(dst)) {
        // Register is not one of al, bl, cl, dl. Its encoding needs REX
        EmitRex32(src);
    } else {
        EmitOptionalRex32(src);
    }
    EmitB(0x8A);
    EmitOperand(dst, src);
}

void Assembler::Emit_movb(Operand dst, Register src) {
    if (!RegIsByte(src)) {
        EmitRex32(src, dst);
    } else {
        EmitOptionalRex32(src, dst);
    }
    EmitB(0x88);
    EmitOperand(src, dst);
}
    
void Assembler::Emit_movaps(Xmm dst, Xmm src) {
    if (XmmLoBits(src) == 4) {
        EmitOptionalRex32(dst, src);
        EmitB(0x0F);
        EmitB(0x29);
        EmitOperand(src, dst);
    } else {
        EmitOptionalRex32(dst, src);
        EmitB(0x0F);
        EmitB(0x28);
        EmitOperand(dst, src);
    }
}

void Assembler::Emit_movapd(Xmm dst, Xmm src) {
    EmitB(0x66);
    if (XmmLoBits(src) == 4) {
        EmitOptionalRex32(dst, src);
        EmitB(0x0F);
        EmitB(0x29);
        EmitOperand(src, dst);
    } else {
        EmitOptionalRex32(dst, src);
        EmitB(0x0F);
        EmitB(0x28);
        EmitOperand(dst, src);
    }
}

void Assembler::Emit_call(Label *l) {
    EmitB(0xE8);
    if (l->IsBound()) {
        int off = l->GetPosition() - pc() - sizeof(uint32_t);
        DCHECK_LE(off, 0);
        EmitDW(off);
    } else if (l->IsLinked()) {
        EmitDW(l->GetPosition());
        l->LinkTo(pc() - sizeof(uint32_t), true);
    } else {
        DCHECK(l->IsUnused());
        int32_t curr = pc();
        EmitDW(curr);
        l->LinkTo(curr, true);
    }
}

void Assembler::Emit_ret(int val) {
    DCHECK(IsUintN(val, 16));
    if (val == 0) {
        EmitB(0xC3);
    } else {
        EmitB(0xC2);
        EmitB(val & 0xFF);
        EmitB((val >> 8) & 0xFF);
    }
}

void Assembler::Emit_jmp(Label *l, bool is_far) {
    static const int kShortSize = 1;
    static const int kLongSize = 4;
    
    if (l->IsBound()) {
        int off = l->GetPosition() - pc() - 1;
        DCHECK_LE(off, 0);
        
        if (IsIntN(off - kShortSize, 8)) {
            // 1110 1011 #8-bit disp
            EmitB(0xEB);
            EmitB((off - kShortSize) & 0xFF);
        } else {
            // 1110 1001 #32-bit disp
            EmitB(0xE9);
            EmitDW(off - kLongSize);
        }
    } else if (!is_far) { // near
        EmitB(0xEB);
        uint8_t disp = 0x0;
        
        if (l->IsNearLinked()) {
            int off = l->GetNearLinkPosition() - pc();
            DCHECK(IsIntN(off, 8));
            disp = static_cast<uint8_t>(off & 0xFF);
        }
        l->LinkTo(pc(), false);
        EmitB(disp);
    } else if (l->IsLinked()) {
        // 1110 1001 #32-bit disp
        EmitB(0xE9);
        EmitDW(l->GetPosition());
        l->LinkTo(pc() - kLongSize, true);
    } else {
        DCHECK(l->IsUnused());
        EmitB(0xE9);
        
        int32_t curr = pc();
        EmitDW(curr);
        l->LinkTo(curr, true);
    }
}
    
void Assembler::Emit_jcc(Cond cc, Label *l, bool is_far) {
    if (cc == Always) {
        Emit_jmp(l, is_far);
        return;
    }
    if (cc == Never) {
        return;
    }
    
    DCHECK(IsUintN(cc, 4));
    if (l->IsBound()) {
        static const int kShortSize = 2;
        static const int kLongSize  = 6;
    
        int off = l->GetPosition() - pc();
        DCHECK_LE(off, 0);
        
        if (IsIntN(off - kShortSize, 8)) {
            // 0111 tttn #8-bit disp
            EmitB(0x70 | cc);
            EmitB((off - kShortSize) & 0xFF);
        } else {
            // 0000 1111 1000 tttn #32-bit disp
            EmitB(0x0F);
            EmitB(0x80 | cc);
            EmitDW((off - kLongSize));
        }
    } else if (!is_far) { // near
        // 0111 tttn #8-bit disp
        EmitB(0x70 | cc);
        uint8_t disp = 0x0;
        
        if (l->IsNearLinked()) {
            int off = l->GetNearLinkPosition() - pc();
            DCHECK(IsIntN(off, 8));
            disp = static_cast<uint8_t>(off & 0xFF);
        }
        l->LinkTo(pc(), false);
        EmitB(disp);
    } else if (l->IsLinked()) {
        // 0000 1111 1000 tttn #32-bit disp
        EmitB(0x0F);
        EmitB(0x80 | cc);
        EmitDW(l->GetPosition());
        l->LinkTo(pc() - sizeof(uint32_t), true);
    } else {
        DCHECK(l->IsUnused());
        EmitB(0x0F);
        EmitB(0x80 | cc);
        
        int32_t curr = pc();
        EmitDW(curr);
        l->LinkTo(curr, true);
    }
}
    
void Assembler::Emit_test(Register dst, Register src, int size) {
    if (RegLoBits(src) == 4) {
        EmitRex(src, dst, size);
        EmitB(0x85);
        EmitModRM(src, dst);
    } else {
        EmitRex(dst, src, size);
        EmitB(0x85);
        EmitModRM(dst, src);
    }
}

void Assembler::Emit_test(Register dst, Immediate mask, int size) {
    if (IsUintN(mask.value(), 8)) {
        // testb(reg, mask);
        return;
    }
    if (dst.code == rax.code) {
        EmitRex(rax, size);
        EmitB(0xA9);
        EmitDW(mask.value());
    } else {
        EmitRex(dst, size);
        EmitB(0xF7);
        EmitModRM(0x0, dst);
        EmitDW(mask.value());
    }
}
    
void Assembler::Emit_test(Operand dst, Immediate mask, int size) {
    if (IsUintN(mask.value(), 8)) {
        // testb(op, mask);
        return;
    }
    EmitRex(rax, dst, size);
    EmitB(0xF7);
    EmitOperand(rax, dst);
    EmitDW(mask.value());
}
    
void Assembler::Emit_cmp(Register dst, int32_t val, int size) {
    if (dst.code == kRAX) {
        EmitRex(dst, size);
        EmitB(0x3D);
        EmitDW(val);
    } else {
        EmitRex(dst, size);
        EmitB(0x81);
        EmitModRM(7, dst);
        EmitDW(val);
    }
}

void Assembler::BindTo(Label *l, int pos) {
    DCHECK(!l->IsBound()); // Label may only be bound once.
    DCHECK_GE(pos, 0);
    DCHECK_LE(pos, pc());
    
    if (l->IsLinked()) {
        int curr = l->GetPosition();
        int next = LongAt(curr);
        
        while (next != curr) {
            int i32 = pos - (curr + sizeof(uint32_t));
            LongPut(curr, i32);
            curr = next;
            next = LongAt(next);
        }
        
        int last_i32 = pos - (curr + sizeof(uint32_t));
        LongPut(curr, last_i32);
    }
    
    while (l->IsNearLinked()) {
        int fixup_pos = l->GetNearLinkPosition();
        int off_to_next = static_cast<int8_t>(*AddrAt(fixup_pos));
        //int off_to_next = LongAt(fixup_pos);
        DCHECK_LE(off_to_next, 0);
        
        int disp = pos - (fixup_pos + sizeof(int8_t));
        DCHECK(IsIntN(disp, 8));
        
        //state->code[fixup_pos] = disp;
        buf_[fixup_pos] = disp;
        if (off_to_next < 0) {
            //YILabelLinkTo(l, fixup_pos + off_to_next, 0);
            l->LinkTo(fixup_pos + off_to_next, false);
        } else {
            l->set_near_link_pos(0);
        }
    }

    //YILabelBindTo(l, pos);
    l->BindTo(pos);
}

void Assembler::Emit_shift(Register dst, Immediate amount, int subcode, int size) {
    DCHECK(size == sizeof(uint64_t) ? IsUintN(amount.value(), 6)
           : IsUintN(amount.value(), 5));
    if (amount.value() == 1) {
        EmitRex(dst, size);
        EmitB(0xD1);
        EmitModRM(subcode, dst);
    } else {
        EmitRex(dst, size);
        EmitB(0xC1);
        EmitModRM(subcode, dst);
        EmitB(amount.value());
    }
}

void Assembler::Emit_nop(int n) {
    switch (n) {
        case 0:
            break;
        case 1:
            Emit_nop();
            break;
        case 2:
            EmitB(0x66);
            EmitB(0x90);
            break;
        case 3:
            // 0F 1F 00
            EmitB(0x0F);
            EmitB(0x1F);
            EmitB(0x00);
            break;
        case 4:
            // 0F 1F 40 00
            EmitB(0x0F);
            EmitB(0x1F);
            EmitB(0x40);
            EmitB(0x00);
            break;
        case 5:
            // 0F 1F 44 00 00
            EmitB(0x0F);
            EmitB(0x1F);
            EmitB(0x44);
            EmitW(0); // 00 00
            break;
        case 6:
            // 66 0F 1F 44 00 00
            EmitB(0x66);
            EmitB(0x0F);
            EmitB(0x1F);
            EmitB(0x44);
            EmitW(0); // 00 00
            break;
        case 7:
            // 0F 1F 80 00 00 00 00
            EmitB(0x0F);
            EmitB(0x1F);
            EmitB(0x80);
            EmitDW(0); // 00 00 00 00
            break;
        case 8:
            // 0F 1F 84 00 00 00 00 00
            EmitB(0x0F);
            EmitB(0x1F);
            EmitB(0x84);
            EmitB(0x00);
            EmitDW(0); // 00 00 00 00
            break;
        case 9:
            // 66 0F 1F 84 00 00 00 00 00
            EmitB(0x66);
            EmitB(0x0F);
            EmitB(0x1F);
            EmitB(0x84);
            EmitB(0x00);
            EmitDW(0); // 00 00 00 00
            break;
        default:
            DCHECK_GT(n, 0);
            break;
    }
}

} // namespace x64
    
} // namespace mai
