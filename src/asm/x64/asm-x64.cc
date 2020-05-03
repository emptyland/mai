#include "asm/x64/asm-x64.h"

namespace mai {
    
namespace x64 {
    
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

const XMMRegister kXmmArgv[kMaxXmmArgs] {
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
    rax, // 0
    rbx,
    rdx,
    rcx,
    rsi, // 4
    rdi,
    r8,
    r9,
    r14,
    r15,
};

// [base + disp/r]
Operand::Operand(Register base, int32_t disp)
    : rex_(0)
    , len_(1) {
    if (base == rsp || base == r12) {
        // From v8:
        // SIB byte is needed to encode (rsp + offset) or (r12 + offset)
        SetSIB(times_1, rsp, base);
    }
    if (disp == 0 && base != rbp && base != r13) {
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
    DCHECK(index != rsp);

    SetSIB(scale, index, base);
    if (disp == 0 && base != rbp && base != r13) {
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

    
void Assembler::movq(Register dst, Register src) {
    if (dst.lo_bits() == 4) {
        EmitRex(src, dst, 8);
        EmitB(0x89);
        EmitModRM(src, dst);
    } else {
        EmitRex(dst, src, 8);
        EmitB(0x8B);
        EmitModRM(dst, src);
    }
}

void Assembler::movb(Register dst, Register src) {
    if (!dst.is_byte()) {
        EmitRex32(src, dst);
    } else {
        EmitOptionalRex32(src, dst);
    }
    EmitB(0x88);
    EmitModRM(src, dst);
}
    
void Assembler::movb(Register dst, Operand src) {
    if (!dst.is_byte()) {
        // Register is not one of al, bl, cl, dl. Its encoding needs REX
        EmitRex32(src);
    } else {
        EmitOptionalRex32(src);
    }
    EmitB(0x8A);
    EmitOperand(dst, src);
}

void Assembler::movb(Operand dst, Register src) {
    if (!src.is_byte()) {
        EmitRex32(src, dst);
    } else {
        EmitOptionalRex32(src, dst);
    }
    EmitB(0x88);
    EmitOperand(src, dst);
}
    
void Assembler::movaps(XMMRegister dst, XMMRegister src) {
    if (src.lo_bits() == 4) {
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

void Assembler::movapd(XMMRegister dst, XMMRegister src) {
    EmitB(0x66);
    if (src.lo_bits() == 4) {
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

void Assembler::call(Label *l) {
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

void Assembler::ret(int val) {
    DCHECK(IsUintN(val, 16));
    if (val == 0) {
        EmitB(0xC3);
    } else {
        EmitB(0xC2);
        EmitB(val & 0xFF);
        EmitB((val >> 8) & 0xFF);
    }
}

void Assembler::jmp(Label *l, bool is_far) {
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
    
void Assembler::j(Cond cc, Label *label, bool is_far) {
    if (cc == Always) {
        jmp(label, is_far);
        return;
    }
    if (cc == Never) {
        return;
    }
    
    DCHECK(IsUintN(cc, 4));
    if (label->IsBound()) {
        static const int kShortSize = 2;
        static const int kLongSize  = 6;
    
        int off = label->GetPosition() - pc();
        DCHECK_LE(off, 0);
        
        if (IsIntN(off - kShortSize, 8)) {
            // 0111 tttn #8-bit disp
            if (cc == RCXZero) {
                EmitB(0xE3);
            } else {
                EmitB(0x70 | cc);
            }
            EmitB((off - kShortSize) & 0xFF);
        } else {
            // 0000 1111 1000 tttn #32-bit disp
            DCHECK_NE(cc, RCXZero);
            EmitB(0x0F);
            EmitB(0x80 | cc);
            EmitDW((off - kLongSize));
        }
    } else if (!is_far) { // near
        // 0111 tttn #8-bit disp
        if (cc == RCXZero) {
            EmitB(0xE3);
        } else {
            EmitB(0x70 | cc);
        }
        uint8_t disp = 0x0;
        
        if (label->IsNearLinked()) {
            int off = label->GetNearLinkPosition() - pc();
            DCHECK(IsIntN(off, 8));
            disp = static_cast<uint8_t>(off & 0xFF);
        }
        label->LinkTo(pc(), false);
        EmitB(disp);
    } else if (label->IsLinked()) {
        // 0000 1111 1000 tttn #32-bit disp
        DCHECK_NE(cc, RCXZero);
        EmitB(0x0F);
        EmitB(0x80 | cc);
        EmitDW(label->GetPosition());
        label->LinkTo(pc() - sizeof(uint32_t), true);
    } else {
        DCHECK(label->IsUnused());
        DCHECK_NE(cc, RCXZero);
        EmitB(0x0F);
        EmitB(0x80 | cc);

        int32_t curr = pc();
        EmitDW(curr);
        label->LinkTo(curr, true);
    }
}
    
void Assembler::test(Register dst, Register src, int size) {
    if (src.lo_bits() == 4) {
        EmitRex(src, dst, size);
        EmitB(0x85);
        EmitModRM(src, dst);
    } else {
        EmitRex(dst, src, size);
        EmitB(0x85);
        EmitModRM(dst, src);
    }
}

void Assembler::test(Register dst, Immediate mask, int size) {
//    if (IsUintN(mask.value(), 8)) {
//        // testb(reg, mask);
//        return;
//    }
    if (dst == rax) {
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
    
void Assembler::test(Operand dst, Immediate mask, int size) {
    if (IsUintN(mask.value(), 8)) {
        // testb(op, mask);
        return;
    }
    EmitRex(rax, dst, size);
    EmitB(0xF7);
    EmitOperand(rax, dst);
    EmitDW(mask.value());
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
        int off_to_next = *reinterpret_cast<int8_t *>(AddrAt(fixup_pos));
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

void Assembler::nop(int n) {
    switch (n) {
        case 0:
            break;
        case 1:
            nop();
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

void Assembler::EmitShift(Register dst, uint8_t amount, int subcode, int size) {
    DCHECK(size == sizeof(uint64_t) ? IsUintN(amount, 6) : IsUintN(amount, 5));
    if (size == 2) {
        EmitB(0x66);
    }
    if (amount == 1) {
        EmitRex(dst, size);
        if (size == 1) {
            EmitB(0xD0);
        } else {
            DCHECK(size == 2 || size == 4 || size == 8);
            EmitB(0xD1);
        }
        EmitModRM(subcode, dst);
    } else {
        EmitRex(dst, size);
        if (size == 1) {
            EmitB(0xC0);
        } else {
            DCHECK(size == 2 || size == 4 || size == 8);
            EmitB(0xC1);
        }
        EmitModRM(subcode, dst);
        EmitB(amount);
    }
}

void Assembler::EmitArith(uint8_t op, Register reg, Register rm_reg, int size) {
    DCHECK((op & 0xC6) == 2);
    if (size == 2) {
        EmitB(0x66);
    }
    if (rm_reg.lo_bits() == 4) { // Forces SIB byte.
        // Swap reg and rm_reg and change opcode operand order.
        EmitRex(rm_reg, reg, size);
        EmitB(op ^ 0x20);
        EmitModRM(rm_reg, reg);
    } else {
        EmitRex(reg, rm_reg, size);
        EmitB(op);
        EmitModRM(reg, rm_reg);
    }
}

void Assembler::EmitArith(uint8_t subcode, Register lhs, int32_t imm, int size) {
    if (size == 2) {
        EmitB(0x66);
    }
    EmitRex(lhs, size);
    if (IsIntN(imm, 8)) {
        EmitB(0x83);
        EmitModRM(subcode, lhs);
        EmitB(imm);
    } else if (lhs == rax) {
        EmitB(0x05 | (subcode << 3));
        EmitDW(imm);
    } else {
        EmitB(0x81);
        EmitModRM(subcode, lhs);
        EmitDW(imm);
    }
}
    
void Assembler::EmitArith(uint8_t subcode, Operand lhs, int32_t imm, int size) {
    if (size == 2) {
        EmitB(0x66);
    }
    EmitRex(lhs, size);
    if (IsIntN(imm, 8)) {
        EmitB(0x83);
        EmitOperand(subcode, lhs);
        EmitB(imm);
    } else {
        EmitB(0x81);
        EmitOperand(subcode, lhs);
        EmitDW(imm);
    }
}

} // namespace x64
    
} // namespace mai
