#ifndef MAI_ASM_X64_H_
#define MAI_ASM_X64_H_

#include "base/base.h"
#include "glog/logging.h"
#include <string>
#include <type_traits>

namespace mai {

namespace x64 {
    
enum ScaleFactor {
    times_1 = 0,
    times_2 = 1,
    times_4 = 2,
    times_8 = 3,
    times_int_size = times_4,
    times_ptr_size = times_8,
}; // ScaleFactor

enum RegCode {
    kRAX = 0,
    kRCX,
    kRDX,
    kRBX,
    kRSP, // 4
    
    kRBP, // 5
    kRSI,
    kRDI,
    kR8,
    kR9,  // 9
    
    kR10, // 10
    kR11,
    kR12,
    kR13,
    kR14, // 14
    
    kR15,
}; // RegCode;

enum Cond {
    NoCond       = -1,
    
    Overflow     = 0,
    NoOverflow   = 1,
    Below        = 2,
    AboveEqual   = 3,
    Equal        = 4,
    NotEqual     = 5,
    BelowEqual   = 6,
    Above        = 7,
    Negative     = 8,
    Positive     = 9,
    ParityEven   = 10,
    ParityOdd    = 11,
    Less         = 12,
    GreaterEqual = 13,
    LessEqual    = 14,
    Greater      = 15,
    
    // Fake conditions
    Always       = 16,
    Never        = 17,
    RCXZero      = 18,
    
    Carry    = Below,
    NotCarry = AboveEqual,
    Zero     = Equal,
    NotZero  = NotEqual,
    Sign     = Negative,
    NotSign  = Positive,
    LastCond = Greater,
}; // Cond;


// Control Code:
// [1:0] Source Data Format:
// 00b: Unsigned byte
// 01b: Signed byte
// 10b: Unsigned word
// 11b: Signed word
//
// [3:2]Aggregation Operation:
// 00b: Equal any
// 01b: Ranges
// 10b: Equal each
// 11b: Equal ordered
//
// [5:4]Polarity:
// 00b: Positive Polarity (+)
// 01b: Negative Polarity (-)
// 10b: Masked (+)
// 11b: Masked (-)
//
// [6] Output Selection:
// For PCMPESTRI/PCMPISTRI
// 0b: Least significant index
// 1b: Most significant index
// For PCMPESTRM/PCMPISTRM
// 0b: Bit mask
// 1b: Byte/word mask
//
struct PCMP {
    static constexpr int8_t Byte = 0;
    static constexpr int8_t Word = 1;
    
    static constexpr int8_t Unsigned = 0;
    static constexpr int8_t Signed = 1 << 1;
    
    static constexpr int8_t EqualAny = 0;
    static constexpr int8_t Ranges = 1 << 2;
    static constexpr int8_t EqualEach = 2 << 2;
    static constexpr int8_t EqualOrdered = 3 << 2;
    
    static constexpr int8_t PositivePolarity = 0;
    static constexpr int8_t NegativePolarity = 1 << 4;
    static constexpr int8_t PositiveMasked = 2 << 4;
    static constexpr int8_t NegativeMasked = 3 << 4;
    
    static constexpr int8_t LeastSignificantIndex = 0;
    static constexpr int8_t MostSignificantIndex = 1 << 6;
    
    static constexpr int8_t BitMask = 0;
    static constexpr int8_t ByteWordMask = 1 << 6;
    // Least significant index
};
    
class Register {
public:
    constexpr explicit Register(int code) : code_(code) {}
    
    DEF_VAL_GETTER(int, code);

    inline uint32_t bits() const { return 1u << code_; }
    inline uint32_t hi_bits() const { return code_ >> 3; }
    inline uint32_t lo_bits() const { return code_ & 0x7; }
    inline bool is_byte() const { return code_ <= 3; }
    
    inline bool operator == (const Register &other) const { return code_ == other.code_; }
    inline bool operator != (const Register &other) const { return code_ != other.code_; }
private:
    const int code_;
}; // class Register
    
constexpr Register rax(kRAX); // 0
constexpr Register rcx(kRCX); // 1
constexpr Register rdx(kRDX); // 2
constexpr Register rbx(kRBX); // 3
constexpr Register rsp(kRSP); // 4

constexpr Register rbp(kRBP); // 5
constexpr Register rsi(kRSI); // 6
constexpr Register rdi(kRDI); // 7
constexpr Register r8(kR8);  // 8
constexpr Register r9(kR9);  // 9

constexpr Register r10(kR10); // 10
constexpr Register r11(kR11); // 11
constexpr Register r12(kR12); // 12
constexpr Register r13(kR13); // 13
constexpr Register r14(kR14); // 14

constexpr Register r15(kR15); // 15
//extern const Register rNone; // -1
    
constexpr struct RIPRegisterTag {} rip;

class XMMRegister {
public:
    constexpr explicit XMMRegister(int code) : code_(code) {}
    
    DEF_VAL_GETTER(int, code);
    
    //inline uint32_t bits() const { return 1u << code_; }
    inline uint8_t hi_bits() const { return code_ >> 3; }
    inline uint8_t lo_bits() const { return code_ & 0x7; }
    //inline bool is_byte() const { return code_ <= 3; }
    
    inline bool operator == (const XMMRegister &other) const { return code_ == other.code_; }
    inline bool operator != (const XMMRegister &other) const { return code_ != other.code_; }
private:
    const int code_;
}; // class XMMRegister
    
constexpr XMMRegister xmm0(0); // 0
constexpr XMMRegister xmm1(1); // 1
constexpr XMMRegister xmm2(2); // 2
constexpr XMMRegister xmm3(3); // 3
constexpr XMMRegister xmm4(4); // 4

constexpr XMMRegister xmm5(5); // 5
constexpr XMMRegister xmm6(6); // 6
constexpr XMMRegister xmm7(7); // 7
constexpr XMMRegister xmm8(8); // 8
constexpr XMMRegister xmm9(9); // 9

constexpr XMMRegister xmm10(10); // 10
constexpr XMMRegister xmm11(11); // 11
constexpr XMMRegister xmm12(12); // 12
constexpr XMMRegister xmm13(13); // 13
constexpr XMMRegister xmm14(14); // 14

constexpr XMMRegister xmm15(15); // 15


constexpr Register Argv_0 = rdi; // {kRDI},
constexpr Register Argv_1 = rsi; // {kRSI},
constexpr Register Argv_2 = rdx; // {kRDX},
constexpr Register Argv_3 = rcx; // {kRCX},
constexpr Register Argv_4 = r8;  // {kR8},
constexpr Register Argv_5 = r9;  // {kR9},
constexpr Register Argv_6 = r10; // {kR10},
constexpr Register Argv_7 = r11; // {kR11},

static constexpr int kMaxRegArgs = 8;
static constexpr int kMaxXmmArgs = 8;
static constexpr int kMaxAllocRegs = 10;
static constexpr int kMaxAllocXmms = 15;
static constexpr int kRegisterSize = 8;

extern const Register kRegArgv[kMaxRegArgs];
extern const XMMRegister kXmmArgv[kMaxXmmArgs];
extern const Register kRegAlloc[kMaxAllocRegs];

    
inline int IsIntN(int64_t x, uint32_t n) {
    DCHECK((0 < n) && (n < 64));
    int64_t limit = (int64_t)(1) << (n - 1);
    return (-limit <= x) && (x < limit);
}
    
inline bool IsUintN(int64_t x, uint32_t n) {
    DCHECK((0 < n) && (n < sizeof(x) * 8));
    return !(x >> n);
}
    
class Immediate {
public:
    explicit Immediate(int32_t value) : value_(value) {}
    
    DEF_VAL_GETTER(int32_t, value);
private:
    int32_t value_;
}; // class Immediate

class Operand {
public:
    
    // [base + disp/r]
    Operand(Register reg, int32_t disp);
    
    // [index * scale + disp/r]
    Operand(Register index, ScaleFactor scale, int32_t disp);
    
    // [base + index * scale + disp/r]
    Operand(Register base, Register index, ScaleFactor scale,
            int32_t disp);
    
    // [rip * scale + disp/r]
    Operand(RIPRegisterTag, int32_t disp) {
        // mod = 00
        // r/m = 101
        buf_[0] = 0 << 6 | 5;
        rex_ = 1;
        len_ = 1;
        SetDisp32(disp);
    }

    // [rip * scale + disp/r]
    Operand(RIPRegisterTag, ScaleFactor scale, int32_t disp);

    DEF_VAL_GETTER(uint8_t, rex);
    DEF_VAL_GETTER(uint8_t, len);
    const uint8_t *buf() const { return buf_; }

private:
    void SetModRM(int mod, Register rmreg) {
        DCHECK(IsUintN(mod, 2));
        buf_[0] = mod << 6 | rmreg.lo_bits();
        rex_ |= rmreg.hi_bits();
    }

    void SetSIB(ScaleFactor scale, Register index, Register base) {
        DCHECK_EQ(len_, 1);
        DCHECK(IsUintN(scale, 2));
        DCHECK(index != rsp || base == rsp || base == r12);

        buf_[1] = ((static_cast<int>(scale)) << 6) | (index.lo_bits() << 3) | base.lo_bits();
        rex_ |= index.hi_bits() << 1 | base.hi_bits();
        len_ = 2;
    }
    
    void SetDisp8(int disp) {
        DCHECK(IsIntN(disp, 8));
        DCHECK(len_ == 1 || len_ == 2);

        int8_t *p = reinterpret_cast<int8_t *>(&buf_[len_]);
        *p = disp;
        len_ += sizeof(int8_t);
    }
    
    void SetDisp32(int disp) {
        DCHECK(len_ == 1 || len_ == 2);
        
        int32_t *p = reinterpret_cast<int32_t *>(&buf_[len_]);
        *p = disp;
        len_ += sizeof(int32_t);
    }

    uint8_t rex_;
    uint8_t buf_[6];
    uint8_t len_;
}; // class Operand

class Label {
public:
    Label() { DCHECK(IsUnused()); }
    
    ~Label() {
        DCHECK(!IsLinked());
        DCHECK(!IsNearLinked());
    }

    DEF_VAL_GETTER(int, pos);
    DEF_VAL_PROP_RW(int, near_link_pos);
    
    bool IsBound() const { return pos_ < 0; }
    bool IsUnused() const { return pos_ == 0 && near_link_pos_ == 0; }
    bool IsLinked() const { return pos_ > 0; }
    bool IsNearLinked() const { return near_link_pos_ > 0; }
    
    void BindTo(int for_bind) {
        pos_ = -(for_bind) - 1;
        DCHECK(IsBound());
    }
    
    void LinkTo(int for_link, bool is_far) {
        if (!is_far) {
            near_link_pos_ = for_link + 1;
            DCHECK(IsNearLinked());
        } else {
            pos_ = for_link + 1;
            DCHECK(IsLinked());
        }
    }

    int GetPosition() const {
        if (pos_ < 0)
            return -pos_ - 1;
        if (pos_ > 0)
            return pos_ - 1;
        NOREACHED();
        return 0;
    }
    
    int GetNearLinkPosition() const { return near_link_pos_ - 1; }
    
private:
    int pos_ = 0;
    int near_link_pos_ = 0;
}; // class Label

class Assembler {
public:
    static const int kDefaultSize = 8;
    
    Assembler() { Reset(); }
    
    DEF_VAL_GETTER(int, pc);
    DEF_VAL_GETTER(std::string, buf);
    
    void Reset() {
        pc_ = 0;
        buf_.clear();
    }
    
    void leaq(Register dst, Operand src) {
        EmitRex(dst, src, 8);
        EmitB(0x8D);
        EmitOperand(dst, src);
    }

    //----------------------------------------------------------------------------------------------
    // Stack operations:
    //----------------------------------------------------------------------------------------------
    void pushq(Register src) {
        EmitOptionalRex32(src);
        EmitB(0x50 | src.lo_bits());
    }
    
    void pushq(Operand opd) {
        EmitOptionalRex32(opd);
        EmitB(0xff);
        EmitOperand(6, opd);
    }
    
    void pushq(int32_t val) {
        EmitB(0x68);
        EmitDW(val);
    }
    
    void pushfq() { EmitB(0x9c); }
    
    void popq(Register dst) {
        EmitOptionalRex32(dst);
        EmitB(0x58 | dst.lo_bits());
    }
    
    void popq(Operand dst) {
        EmitOptionalRex32(dst);
        EmitB(0x8F);
        EmitOperand(0, dst);
    }
    
    void popfq() { EmitB(0x9D); }
    
    //----------------------------------------------------------------------------------------------
    // Movs
    //----------------------------------------------------------------------------------------------
    void movp(Register dst, Address val) {
        EmitRex(dst, sizeof(val));
        EmitB(0xB8 | dst.lo_bits());
        EmitP(val);
    }
    
    void movq(Register dst, Address val) {
        DCHECK_EQ(sizeof(val), sizeof(int64_t));
        EmitRex64(dst);
        EmitB(0xB8 | dst.lo_bits());
        EmitP(val);
    }
    
    void movq(Register dst, int64_t val) {
        EmitRex64(dst);
        EmitB(0xB8 | dst.lo_bits());
        EmitQW(val);
    }
    
    void movq(Register dst, Register src);
    
    void movq(Register dst, Operand src) {
        EmitRex(dst, src, 8);
        EmitB(0x8B);
        EmitOperand(dst, src);
    }
    
    void movq(Operand dst, Register src) {
        EmitRex(src, dst, 8);
        EmitB(0x89);
        EmitOperand(src, dst);
    }
    
    void movq(Register dst, int32_t src) {
        EmitRex(dst, 8);
        EmitB(0xC7);
        EmitModRM(0x0, dst);
        EmitDW(src);
    }
    
    void movq(Operand dst, int32_t src) {
        EmitRex(dst, 8);
        EmitB(0xC7);
        EmitOperand(0x0, dst);
        EmitDW(src);
    }
    
    void movl(Register dst, Register src) {
        EmitRex(src, dst, 4);
        EmitB(0x89);
        EmitModRM(src, dst);
    }
    
    void movl(Register dst, Operand src) {
        EmitRex(dst, src, 4);
        EmitB(0x8B);
        EmitOperand(dst, src);
    }
    
    void movl(Operand dst, Register src) {
        EmitRex(src, dst, 4);
        EmitB(0x89);
        EmitOperand(src, dst);
    }
    
    void movl(Register dst, int32_t src) {
        EmitRex(dst, 4);
        EmitB(0xB8 + dst.lo_bits());
        EmitDW(src);
    }
    
    void movl(Operand dst, int32_t src) {
        EmitRex(dst, 4);
        EmitB(0xC7);
        EmitOperand(0x0, dst);
        EmitDW(src);
    }
    
    void movw(Register dst, Register src) {
        EmitB(0x66);
        EmitOptionalRex32(src, dst);
        EmitB(0x89);
        EmitModRM(src, dst);
    }
    
    void movw(Register dst, Operand src) {
        EmitB(0x66);
        EmitOptionalRex32(dst, src);
        EmitB(0x8B);
        EmitOperand(dst, src);
    }
    
    void movw(Operand dst, Register src) {
        EmitB(0x66);
        EmitOptionalRex32(src, dst);
        EmitB(0x89);
        EmitOperand(src, dst);
    }
    
    void movw(Register dst, int32_t val) {
        EmitB(0x66);
        EmitRex32(dst);
        //EmitB(0xB8 + RegLoBits(dst));
        EmitB(0xB8 + dst.lo_bits());
        EmitW(val);
    }
    
    void movw(Operand dst, int32_t val) {
        EmitB(0x66);
        EmitOptionalRex32(dst);
        EmitB(0xC7);
        EmitOperand(0x00, dst);
        EmitB(static_cast<uint8_t>(val & 0xff));
        EmitB(static_cast<uint8_t>(val >> 8));
    }
    
    void movb(Register dst, Register src);
    
    void movb(Register dst, Operand src);
    
    void movb(Operand dst, Register src);
    
    void movb(Register dst, int32_t val) {
        if (!dst.is_byte()) {
            EmitRex32(dst);
        }
        EmitB(0xB0 + dst.lo_bits());
        EmitB(val);

    }
    
    void movb(Operand dst, int32_t val) {
        EmitOptionalRex32(dst);
        EmitB(0xC6);
        EmitOperand(0x00, dst);
        EmitB(val);
    }
    
    // NOTICE: only AH, BH, CH, DH can be extend
    // byte -> dword
    void movzxb(Register dst, Register src) {
        DCHECK(src == rax || src == rbx || src == rcx || src == rdx);
        EmitOptionalRex32(dst, src);
        EmitB(0x0F);
        EmitB(0xB6);
        EmitModRM(dst, src);
    }

    void movzxb(Register dst, Operand src) {
        EmitOptionalRex32(dst, src);
        EmitB(0x0F);
        EmitB(0xB6);
        EmitOperand(dst, src);
    }

    // word -> dword
    void movzxw(Register dst, Register src) {
        EmitOptionalRex32(dst, src);
        EmitB(0x0F);
        EmitB(0xB7);
        EmitModRM(dst, src);
    }

    void movzxw(Register dst, Operand src) {
        EmitOptionalRex32(dst, src);
        EmitB(0x0F);
        EmitB(0xB7);
        EmitOperand(dst, src);
    }

    // byte -> dword
    void movsxb(Register dst, Register src) {
        DCHECK(src == rax || src == rbx || src == rcx || src == rdx);
        EmitOptionalRex32(dst, src);
        EmitB(0x0F);
        EmitB(0xBE);
        EmitModRM(dst, src);
    }

    void movsxb(Register dst, Operand src) {
        EmitOptionalRex32(dst, src);
        EmitB(0x0F);
        EmitB(0xBE);
        EmitOperand(dst, src);
    }

    // word -> dword
    void movsxw(Register dst, Register src) {
        EmitOptionalRex32(dst, src);
        EmitB(0x0F);
        EmitB(0xBF);
        EmitModRM(dst, src);
    }

    void movsxw(Register dst, Operand src) {
        EmitOptionalRex32(dst, src);
        EmitB(0x0F);
        EmitB(0xBF);
        EmitOperand(dst, src);
    }
    
    //
    // SSE
    //
    // MOVAPS—Move Aligned Packed Single-Precision Floating-Point Values
    void movaps(XMMRegister dst, XMMRegister src);

    void movaps(XMMRegister dst, Operand src) { EmitSSEArith(0, 0x28, dst, src); }

    void movaps(Operand dst, XMMRegister src) { EmitSSEArith(0, 0x29, src, dst); }
    
    // MOVSS—Move Scalar Single-Precision Floating-Point Values
    void movss(XMMRegister dst, XMMRegister src) { EmitSSEArith(0xF3, 0x10, dst, src); }
    
    void movss(XMMRegister dst, Operand src) { EmitSSEArith(0xF3, 0x10, dst, src); }

    void movss(Operand dst, XMMRegister src) { EmitSSEArith(0xF3, 0x11, src, dst); }
    
    //
    // SSE2
    //
    // MOVAPD—Move Aligned Packed Double-Precision Floating-Point Values
    void movapd(XMMRegister dst, XMMRegister src);

    void movapd(XMMRegister dst, Operand src) { EmitSSEArith(0x66, 0x28, dst, src); }

    void movapd(Operand dst, XMMRegister src) { EmitSSEArith(0x66, 0x29, src, dst); }

    // Move Scalar Double-Precision Floating-Point Value
    void movsd(XMMRegister dst, XMMRegister src) { EmitSSEArith(0xF2, 0x10, dst, src); }
    
    void movsd(XMMRegister dst, Operand src) { EmitSSEArith(0xF2, 0x10, dst, src); }

    void movsd(Operand dst, XMMRegister src) { EmitSSEArith(0xF2, 0x11, src, dst); }
    
    // Move Unaligned Double Quadword
    void movdqu(XMMRegister dst, XMMRegister src) { EmitSSEArith(0xF3, 0x6F, dst, src); }
    
    void movdqu(XMMRegister dst, Operand src) { EmitSSEArith(0xF3, 0x6F, dst, src); }
    
    void movdqu(Operand dst, XMMRegister src) { EmitSSEArith(0xF3, 0x7F, src, dst); }
    
    // CMOVcc—Conditional Move
    void cmov(Cond cond, Register dst, Register src, int size) {
        EmitRex(dst, src, size);
        EmitB(0x0F);
        EmitB(0x40 | cond);
        EmitModRM(dst, src);
    }
    
    void cmov(Cond cond, Register dst, Operand src, int size) {
        EmitRex(dst, src, size);
        EmitB(0x0F);
        EmitB(0x40 | cond);
        EmitOperand(dst, src);
    }
    
    void cmovq(Cond cond, Register dst, Register src) { cmov(cond, dst, src, 8); }
    void cmovq(Cond cond, Register dst, Operand src) { cmov(cond, dst, src, 8); }
    
    void cmovl(Cond cond, Register dst, Register src) { cmov(cond, dst, src, 4); }
    void cmovl(Cond cond, Register dst, Operand src) { cmov(cond, dst, src, 4); }
    
    //----------------------------------------------------------------------------------------------
    // Jmps
    //----------------------------------------------------------------------------------------------
    
    // Calling
    void call(Label *l);

    void call(Address addr) {
        // 1110 1000 #32-bit disp
        EmitB(0xE8);
        Address src = AddrAt(pc() + 4);
        int32_t disp = static_cast<int32_t>(addr - src);
        DCHECK(IsIntN(disp, 32)); // near call; only to 32bit
        EmitDW(disp);
    }

    void call(Register addr) {
        // opcode: FF /2 r64
        EmitOptionalRex32(addr);
        EmitB(0xff);
        EmitModRM(0x2, addr);
    }

    void call(Operand addr) {
        // opcode: FF /2 m64
        EmitOptionalRex32(addr);
        EmitB(0xff);
        EmitOperand(0x2, addr);
    }

    void ret(int val);
    
    // Jumping
    void jmp(Label *l, bool is_far);

    void jmp(Address addr); // always near jmp
    
    // Jump near, absolute indirect, RIP = 64-Bit offset from register or memory
    void jmp(Register addr) {
        // opcode: FF /4 r64
        EmitOptionalRex32(addr);
        EmitB(0xff);
        EmitModRM(0x4, addr);
    }

    void jmp(Operand addr) { // always far jmp
        // opcode: FF /4 m64
        EmitOptionalRex32(addr);
        EmitB(0xff);
        EmitOperand(0x4, addr);
    }
    
    void j(Cond cond, Label *l, bool is_far);
    
    void LikelyJ(Cond cond, Label *l, bool is_far) {
        PrefixLikely();
        j(cond, l, is_far);
    }
    
    void UnlikelyJ(Cond cond, Label *l, bool is_far) {
        PrefixUnlikely();
        j(cond, l, is_far);
    }
    
    void Bind(Label *l) { BindTo(l, pc()); }
    
    void BindTo(Label *l, int pos);
    
    //----------------------------------------------------------------------------------------------
    // Tests
    //----------------------------------------------------------------------------------------------
    // Logic Compare
    void test(Register dst, Register src, int size = kDefaultSize);
    void test(Register dst, Immediate mask, int size = kDefaultSize);

    void test(Operand dst, Register src, int size = kDefaultSize) {
        EmitRex(src, dst, size);
        EmitB(0x85);
        EmitOperand(src, dst);
    }

    void test(Operand dst, Immediate mask, int size = kDefaultSize);
    
    void testl(Register dst, Register src) { test(dst, src, 4); }
    void testl(Register dst, int32_t mask) { test(dst, Immediate(mask), 4); }
    void testl(Operand dst, Register src) { test(dst, src, 4); }
    void testl(Operand dst, int32_t mask) { test(dst, Immediate(mask), 4); }
    
    void testq(Register dst, Register src) { test(dst, src, 8); }
    void testq(Register dst, int32_t mask) { test(dst, Immediate(mask), 8); }
    void testq(Operand dst, Register src) { test(dst, src, 8); }
    void testq(Operand dst, int32_t mask) { test(dst, Immediate(mask), 8); }

    //
    // SSE Floating Comparation
    //
    // CMPPD—Compare Packed Double-Precision Floating-Point Values
    void cmppd(XMMRegister lhs, XMMRegister rhs, int8_t precision) {
        //66 0F C2 /r ib
        EmitB(0x66);
        EmitOptionalRex32(lhs, rhs);
        EmitB(0x0F);
        EmitB(0xC2);
        EmitOperand(lhs, rhs); // RMI
        EmitB(precision);
    }
    
    void cmppd(XMMRegister lhs, Operand rhs, int8_t precision) {
        //66 0F C2 /r ib
        EmitB(0x66);
        EmitOptionalRex32(lhs, rhs);
        EmitB(0x0F);
        EmitB(0xC2);
        EmitOperand(lhs, rhs); // RMI
        EmitB(precision);
    }
    
    // Compare Packed Single-Precision Floating-Point Values
    void cmpps(XMMRegister lhs, XMMRegister rhs, int8_t precision) {
        // 0F C2 /r ib
        EmitOptionalRex32(lhs, rhs);
        EmitB(0x0F);
        EmitB(0xC2);
        EmitOperand(lhs, rhs); // RMI
        EmitB(precision);
    }
    
    void cmpps(XMMRegister lhs, Operand rhs, int8_t precision) {
        // 0F C2 /r ib
        EmitOptionalRex32(lhs, rhs);
        EmitB(0x0F);
        EmitB(0xC2);
        EmitOperand(lhs, rhs); // RMI
        EmitB(precision);
    }
    
    // Compare Scalar Double-Precision Floating-Point Values (SSE2)
    void cmpsd(XMMRegister lhs, XMMRegister rhs, int8_t precision) {
        // F2 0F C2 /r ib
        EmitB(0xF2);
        EmitOptionalRex32(lhs, rhs);
        EmitB(0x0F);
        EmitB(0xC2);
        EmitOperand(lhs, rhs); // RMI
        EmitB(precision);
    }

    void cmpsd(XMMRegister lhs, Operand rhs, int8_t precision) {
        // F2 0F C2 /r ib
        EmitB(0xF2);
        EmitOptionalRex32(lhs, rhs);
        EmitB(0x0F);
        EmitB(0xC2);
        EmitOperand(lhs, rhs); // RMI
        EmitB(precision);
    }
    
    // Compare Scalar Ordered Double-Precision Floating-Point Values and Set EFLAGS
    // 66 0F 2F /r
    void comisd(XMMRegister lhs, XMMRegister rhs) { EmitSSEArith(0x66, 0x2F, lhs, rhs); }
    void comisd(XMMRegister lhs, Operand rhs) { EmitSSEArith(0x66, 0x2F, lhs, rhs); }

    // UCOMISD—Unordered Compare Scalar Double-Precision Floating-Point Values and Set EFLAGS
    // 66 0F 2E /r
    void ucomisd(XMMRegister lhs, XMMRegister rhs) { EmitSSEArith(0x66, 0x2E, lhs, rhs); }
    void ucomisd(XMMRegister lhs, Operand rhs) { EmitSSEArith(0x66, 0x2E, lhs, rhs); }
    
    // Compare Scalar Ordered Single-Precision Floating-Point Values and Set EFLAGS
    // 0F 2F /r
    void comiss(XMMRegister lhs, XMMRegister rhs) { EmitSSEArith(0, 0x2F, lhs, rhs); }
    void comiss(XMMRegister lhs, Operand rhs) { EmitSSEArith(0, 0x2F, lhs, rhs); }
    
    // UCOMISS—Unordered Compare Scalar Single-Precision Floating-Point Values and Set EFLAGS
    // 0F 2E /r
    void ucomiss(XMMRegister lhs, XMMRegister rhs) { EmitSSEArith(0, 0x2E, lhs, rhs); }
    void ucomiss(XMMRegister lhs, Operand rhs) { EmitSSEArith(0, 0x2E, lhs, rhs); }
    
    //----------------------------------------------------------------------------------------------
    // Neg/Not/Shift
    //----------------------------------------------------------------------------------------------
    // Neg
    void negl(Register dst) { neg(dst, 4); }
    void negl(Operand dst) { neg(dst, 4); }
    void negq(Register dst) { neg(dst, 8); }
    void negq(Operand dst) { neg(dst, 8); }
    
    void neg(Register dst, int size) {
        EmitRex(dst, size);
        EmitB(0xF7);
        EmitModRM(0x3, dst);
    }

    void neg(Operand dst, int size) {
        EmitRex(dst, size);
        EmitB(0xF7);
        EmitOperand(0x3, dst);
    }
    
    void notl(Register dst) { not_(dst, 4); }
    void notl(Operand dst) { not_(dst, 4); }
    void notq(Register dst) { not_(dst, 8); }
    void notq(Operand dst) { not_(dst, 8); }

    // Not
    void not_(Register dst, int size) {
        EmitRex(dst, size);
        EmitB(0xF7);
        EmitModRM(0x2, dst);
    }

    void not_(Operand dst, int size) {
        EmitRex(dst, size);
        EmitB(0xF7);
        EmitOperand(0x2, dst);
    }

    // Shift
#define BITS_SHIFT(V) \
    V(rol, 0x0) \
    V(ror, 0x1) \
    V(rcl, 0x2) \
    V(rcr, 0x3) \
    V(shl, 0x4) \
    V(shr, 0x5) \
    V(sal, 0x4) \
    V(sar, 0x7)
    
#define DEF_SHIFT(name, subcode) \
    void name##l(Register dst, uint8_t imm8) { EmitShift(dst, imm8, subcode, 4); } \
    void name##l(Register dst) { EmitShift(dst, subcode, 4); } \
    void name##q(Register dst, uint8_t imm8) { EmitShift(dst, imm8, subcode, 8); } \
    void name##q(Register dst) { EmitShift(dst, subcode, 8); }
    
    // RCL : Rotate 9/17/33/65 bits (CF, r/m8) left
    // RCR : Rotate 9/17/33/65 bits (CF, r/m8) right
    // ROL : Rotate 8/16/32/64 bits (CF, r/m8) left
    // ROR : Rotate 8/16/32/64 bits (CF, r/m8) right
    // SAL : Multiply r/m8 by 2
    // SAR : Signed divide* r/m8 by 2
    // SHL : Multiply r/m8 by 2
    // SHR : Unsigned divide r/m8 by 2
    BITS_SHIFT(DEF_SHIFT)

#undef BITS_SHIFT
#undef DEF_SHIFT

    // shift dst:src left by cl bits, affecting only dst.
    void shld(Register dst, Register src);

    // shift src:dst right by cl bits, affecting only dst.
    void shrd(Register dst, Register src);

    //----------------------------------------------------------------------------------------------
    // Convert
    //----------------------------------------------------------------------------------------------
#define CONVERT_MODE_MX_X(name, prefix, subcode) \
    void cvt##name(XMMRegister dst, XMMRegister src) { EmitSSEArith(prefix, subcode, dst, src); } \
    void cvt##name(XMMRegister dst, Operand src) { EmitSSEArith(prefix, subcode, dst, src); }

#define CONVERT_MODE_MX_R(name, prefix, subcode) \
    void cvt##name##q(Register dst, XMMRegister src) { EmitSSEArith(prefix, subcode, dst, src, 8); } \
    void cvt##name##q(Register dst, Operand src) { EmitSSEArith(prefix, subcode, dst, src, 8); } \
    void cvt##name##l(Register dst, XMMRegister src) { EmitSSEArith(prefix, subcode, dst, src, 4); } \
    void cvt##name##l(Register dst, Operand src) { EmitSSEArith(prefix, subcode, dst, src, 4); }
    
#define CONVERT_MODE_MR_X(name, prefix, subcode) \
    void cvt##name##q(XMMRegister dst, Register src) { EmitSSEArith(prefix, subcode, dst, src, 8); } \
    void cvt##name##q(XMMRegister dst, Operand src) { EmitSSEArith(prefix, subcode, dst, src, 8); } \
    void cvt##name##l(XMMRegister dst, Register src) { EmitSSEArith(prefix, subcode, dst, src, 4); } \
    void cvt##name##l(XMMRegister dst, Operand src) { EmitSSEArith(prefix, subcode, dst, src, 4); }
    
#define DEF_CONVERT(name, mode, prefix, subcode) CONVERT_##mode(name, prefix, subcode)
    
#define SSE_CONVERT(V) \
    V(dq2pd,  MODE_MX_X, 0xF3, 0xE6) \
    V(dq2ps,  MODE_MX_X, 0,    0x5B) \
    V(pd2dq,  MODE_MX_X, 0xF2, 0xE6) \
    V(pd2ps,  MODE_MX_X, 0x66, 0x5A) \
    V(ps2pd,  MODE_MX_X, 0,    0x5A) \
    V(ps2dq,  MODE_MX_X, 0x66, 0x5A) \
    V(sd2si,  MODE_MX_R, 0xF2, 0x2D) \
    V(sd2ss,  MODE_MX_X, 0xF2, 0x5A) \
    V(si2sd,  MODE_MR_X, 0xF2, 0x2A) \
    V(si2ss,  MODE_MR_X, 0xF3, 0x2A) \
    V(ss2sd,  MODE_MX_X, 0xF3, 0x5A) \
    V(ss2si,  MODE_MX_R, 0xF3, 0x2D) \
    V(tpd2dq, MODE_MX_X, 0x66, 0xE6) \
    V(tps2dq, MODE_MX_X, 0xF3, 0x5B) \
    V(tsd2si, MODE_MX_R, 0xF2, 0x2C) \
    V(tss2si, MODE_MX_R, 0xF3, 0x2C)

    // Convert Packed Dword Integers to Packed Double-Precision FP Values
    // F3 0F E6
    // CVTDQ2PS—Convert Packed Dword Integers to Packed Single-Precision FP Values
    // 0F 5B /r
    // CVTPD2DQ—Convert Packed Double-Precision FP Values to Packed Dword Integers
    // F2 0F E6 /r
    // CVTPD2PS—Convert Packed Double-Precision FP Values to Packed Single-Precision FP Values
    // 66 0F 5A /r
    // CVTPS2PD—Convert Packed Single-Precision FP Values to Packed Double-Precision FP Values
    // 0F 5A /r
    // CVTSD2SI—Convert Scalar Double-Precision FP Value to Integer
    // F2 0F 2D /r
    // CVTSD2SS—Convert Scalar Double-Precision FP Value to Scalar Single-Precision FP Value
    // F2 0F 5A /r
    // CVTSI2SD—Convert Dword Integer to Scalar Double-Precision FP Value
    // F2 0F 2A /r
    // CVTSI2SS—Convert Dword Integer to Scalar Single-Precision FP Value
    // F3 0F 2A /r
    // CVTSS2SD—Convert Scalar Single-Precision FP Value to Scalar Double-Precision FP Value
    // F3 0F 5A /r
    // CVTSS2SI—Convert Scalar Single-Precision FP Value to Dword Integer
    // F3 0F 2D /r
    // CVTTPD2DQ—Convert with Truncation Packed Double-Precision FP Values to Packed Dword Integers
    // 66 0F E6 /r
    // CVTTPS2DQ—Convert with Truncation Packed Single-Precision FP Values to Packed Dword Integers
    // F3 0F 5B /r
    // CVTTSD2SI—Convert with Truncation Scalar Double-Precision FP Value to Signed Integer
    // F2 0F 2C /r
    // CVTTSS2SI—Convert with Truncation Scalar Single-Precision FP Value to Dword Integer
    // F3 0F 2C /r
    
    SSE_CONVERT(DEF_CONVERT)
    
#undef DEF_CONVERT
#undef SSE_CONVERT
#undef DEF_CONVERT
#undef CONVERT_MODE_MR_X
#undef CONVERT_MODE_MX_R
#undef CONVERT_MODE_MX_X

    //----------------------------------------------------------------------------------------------
    // Arith
    //----------------------------------------------------------------------------------------------
    void incq(Register dst) { inc(dst, 8); }
    void incq(Operand dst) { inc(dst, 8); }
    
    void incl(Register dst) { inc(dst, 4); }
    void incl(Operand dst) { inc(dst, 4); }
    
    void decq(Register dst) { dec(dst, 8); }
    void decq(Operand dst) { dec(dst, 8); }
    
    void decl(Register dst) { dec(dst, 4); }
    void decl(Operand dst) { dec(dst, 4); }
    
    void inc(Register dst, int size = kDefaultSize) {
        EmitRex(dst, size);
        EmitB(0xFF);
        EmitModRM(0, dst);
    }
    
    void inc(Operand dst, int size = kDefaultSize) {
        EmitRex(dst, size);
        EmitB(0xFF);
        EmitOperand(0, dst);
    }
    
    void dec(Register dst, int size = kDefaultSize) {
        EmitRex(dst, size);
        EmitB(0xFF);
        EmitModRM(1, dst);
    }
    
    void dec(Operand dst, int size = kDefaultSize) {
        EmitRex(dst, size);
        EmitB(0xFF);
        EmitOperand(1, dst);
    }
    
    // IDIV—Signed Divide
    void idiv(Register divsor, int size = kDefaultSize) {
        EmitRex(divsor, size);
        EmitB(0xF7);
        EmitModRM(7, divsor);
    }
    
    void idiv(Operand divsor, int size = kDefaultSize) {
        EmitRex(divsor, size);
        EmitB(0xF7);
        EmitOperand(7, divsor);
    }
    
    void idivl(Register divsor) { idiv(divsor, 4); }
    void idivl(Operand divsor) { idiv(divsor, 4); }
    
    void idivq(Register divsor) { idiv(divsor, 8); }
    void idivq(Operand divsor) { idiv(divsor, 8); }
    
    void imull(Register rhs) { imul(rhs, 4); }
    void imull(Operand rhs) { imul(rhs, 4); }
    void imull(Register dst, Register rhs) { imul(dst, rhs, 4); }
    void imull(Register dst, Operand rhs) { imul(dst, rhs, 4); }
    void imull(Register dst, Register lhs, int8_t rhs) { imul(dst, lhs, rhs, 4); }
    void imull(Register dst, Operand lhs, int8_t rhs) { imul(dst, lhs, rhs, 4); }
    void imull(Register dst, Register lhs, int32_t rhs) { imul(dst, lhs, rhs, 4); }
    void imull(Register dst, Operand lhs, int32_t rhs) { imul(dst, lhs, rhs, 4); }
    
    void imulq(Register rhs) { imul(rhs, 8); }
    void imulq(Operand rhs) { imul(rhs, 8); }
    void imulq(Register dst, Register rhs) { imul(dst, rhs, 8); }
    void imulq(Register dst, Operand rhs) { imul(dst, rhs, 8); }
    void imulq(Register dst, Register lhs, int8_t rhs) { imul(dst, lhs, rhs, 8); }
    void imulq(Register dst, Operand lhs, int8_t rhs) { imul(dst, lhs, rhs, 8); }
    void imulq(Register dst, Register lhs, int32_t rhs) { imul(dst, lhs, rhs, 8); }
    void imulq(Register dst, Operand lhs, int32_t rhs) { imul(dst, lhs, rhs, 8); }
    
    // RDX:RAX ← RAX ∗ r/m64.
    void imul(Register rhs, int size) {
        // REX.W + F7 /5
        EmitRex(rhs, size);
        EmitB(0xF7);
        EmitModRM(5, rhs);
    }
    
    void imul(Operand rhs, int size) {
        // REX.W + F7 /5
        EmitRex(rhs, size);
        EmitB(0xF7);
        EmitOperand(5, rhs);
    }
    
    // Quadword register ← Quadword register ∗ r/m64.
    void imul(Register dst, Register rhs, int size) {
        // REX.W + 0F AF /r
        EmitRex(dst, rhs, size);
        EmitB(0x0F);
        EmitB(0xAF);
        EmitModRM(dst, rhs);
    }
    
    void imul(Register dst, Operand rhs, int size) {
        // REX.W + 0F AF /r
        EmitRex(dst, rhs, size);
        EmitB(0x0F);
        EmitB(0xAF);
        EmitOperand(dst, rhs);
    }
    
    // Quadword register ← r/m64 ∗ sign-extended immediate byte.
    void imul(Register dst, Register lhs, int8_t rhs, int size) {
        // REX.W + 6B /r ib
        EmitRex(dst, lhs, size);
        EmitB(0x6B);
        EmitModRM(dst, lhs);
        EmitB(rhs);
    }
    
    void imul(Register dst, Operand lhs, int8_t rhs, int size) {
        // REX.W + 6B /r ib
        EmitRex(dst, lhs, size);
        EmitB(0x6B);
        EmitOperand(dst, lhs);
        EmitB(rhs);
    }
    
    // Quadword register ← r/m64 ∗ immediate doubleword
    void imul(Register dst, Register lhs, int32_t rhs, int size) {
        // REX.W + 69 /r id
        EmitRex(dst, lhs, size);
        EmitB(0x69);
        EmitModRM(dst, lhs);
        EmitDW(rhs);
    }
    
    void imul(Register dst, Operand lhs, int32_t rhs, int size) {
        // REX.W + 69 /r id
        EmitRex(dst, lhs, size);
        EmitB(0x69);
        EmitOperand(dst, lhs);
        EmitDW(rhs);
    }
    
    // Unsigned divide EDX:EAX by r/m32, with result stored in EAX ← Quotient, EDX ←
    // Remainder.
    void divl(Register divsor) { div(divsor, 4); }
    void divl(Operand divsor) { div(divsor, 4); }
    
    // Unsigned divide RDX:RAX by r/m64, with result stored in RAX ← Quotient, RDX ←
    // Remainder.
    void divq(Register divsor) { div(divsor, 8); }
    void divq(Operand divsor) { div(divsor, 8); }
    
    void div(Register divsor, int size) {
        // REX.W + F7 /6
        EmitRex(divsor, size);
        EmitB(0xF7);
        EmitModRM(6, divsor);
    }
    
    void div(Operand divsor, int size) {
        // REX.W + F7 /6
        EmitRex(divsor, size);
        EmitB(0xF7);
        EmitOperand(6, divsor);
    }
    
    // Unsigned multiply (EDX:EAX ← EAX ∗ r/m32).
    void mull(Register divsor) { mul(divsor, 4); }
    void mull(Operand divsor) { mul(divsor, 4); }
    
    // Unsigned multiply (RDX:RAX ← RAX ∗ r/m64.
    void mulq(Register divsor) { mul(divsor, 8); }
    void mulq(Operand divsor) { mul(divsor, 8); }
    
    void mul(Register divsor, int size) {
        // REX.W + F7 /4
        EmitRex(divsor, size);
        EmitB(0xF7);
        EmitModRM(4, divsor);
    }
    
    void mul(Operand divsor, int size) {
        // REX.W + F7 /4
        EmitRex(divsor, size);
        EmitB(0xF7);
        EmitOperand(4, divsor);
    }
    
    

#define ARITH_OP_LIST(V) \
    V(add, \
        0x3, 0x0, 0x3, 0x1, 0x0, \
        0x1, 0x0, 0x3, 0x1, 0x0, \
        0x0, 0x0, 0x2, 0x0, 0x0) \
    V(sub, \
        0x2B, 0x5, 0x2B, 0x29, 0x5, \
        0x29, 0x5, 0x2B, 0x29, 0x5, \
        0x28, 0x5, 0x2A, 0x29, 0x5) \
    V(cmp, \
        0x3B, 0x7, 0x3B, 0x39, 0x7, \
        0x39, 0x7, 0x3B, 0x39, 0x7, \
        0x3A, 0x7, 0x3A, 0x38, 0x7) \
    V(and, \
        0x23, 0x4, 0x23, 0x21, 0x4, \
        0x21, 0x4, 0x23, 0x21, 0x4, \
        0x22, 0x4, 0x22, 0x20, 0x4) \
    V(or, \
        0x0B, 0x1, 0x0B, 0x09, 0x1, \
        0x09, 0x1, 0x0B, 0x09, 0x1, \
        0x0A, 0x1, 0x0A, 0x08, 0x1) \
    V(xor, \
        0x33, 0x6, 0x33, 0x31, 0x6, \
        0x31, 0x6, 0x33, 0x31, 0x6, \
        0x32, 0x6, 0x32, 0x30, 0x6)
    
#define DEF_ARITH(name, q_r_r, q_r_i, q_r_o, q_o_r, q_o_i, w_r_r, w_r_i, w_r_o, w_o_r, w_o_i, b_r_r, b_r_i, b_r_o, b_o_r, b_o_i) \
    DEF_ARITH_LONG(name##q,   8, q_r_r, q_r_i, q_r_o, q_o_r, q_o_i) \
    DEF_ARITH_LONG(name##l,   4, q_r_r, q_r_i, q_r_o, q_o_r, q_o_i)
    
#define DEF_ARITH_LONG(name, size, rr, ri, ro, or, oi) \
    inline void name(Register lhs, Register rhs) { EmitArith(rr, lhs, rhs, size); } \
    inline void name(Register lhs, int32_t val) { EmitArith(ri, lhs, val, size); } \
    inline void name(Register lhs, Operand rhs) { EmitArith(ro, lhs, rhs, size); } \
    inline void name(Operand lhs, Register rhs) { EmitArith(or, rhs, lhs, size); } \
    inline void name(Operand lhs, int32_t val) { EmitArith(oi, lhs, val, size); }

    ARITH_OP_LIST(DEF_ARITH)
    
#undef ARITH_OP_LIST
#undef DEF_ARITH_LONG
#undef DEF_ARITH
    
    // ss, ps, sd, pd
#define ARITH_SSE_OP_LIST(V) \
    V(add, 0x58) \
    V(sub, 0x5C) \
    V(mul, 0x59) \
    V(div, 0x5E) \
    V(and, 0x54) \
    V(or,  0x56) \
    V(xor, 0x57)
    
#define DEF_SSE_ARITH(name, opcode) \
    inline void name##ss(XMMRegister lhs, XMMRegister rhs) { EmitSSEArith(0xF3, opcode, lhs, rhs); } \
    inline void name##ss(XMMRegister lhs, Operand rhs) { EmitSSEArith(0xF3, opcode, lhs, rhs); } \
    inline void name##ps(XMMRegister lhs, XMMRegister rhs) { EmitSSEArith(0, opcode, lhs, rhs); } \
    inline void name##ps(XMMRegister lhs, Operand rhs) { EmitSSEArith(0, opcode, lhs, rhs); } \
    inline void name##sd(XMMRegister lhs, XMMRegister rhs) { EmitSSEArith(0xF2, opcode, lhs, rhs); } \
    inline void name##sd(XMMRegister lhs, Operand rhs) { EmitSSEArith(0xF2, opcode, lhs, rhs); } \
    inline void name##pd(XMMRegister lhs, XMMRegister rhs) { EmitSSEArith(0x66, opcode, lhs, rhs); } \
    inline void name##pd(XMMRegister lhs, Operand rhs) { EmitSSEArith(0x66, opcode, lhs, rhs); }

    ARITH_SSE_OP_LIST(DEF_SSE_ARITH)

#undef DEF_SSE_ARITH
#undef ARITH_SSE_OP_LIST

    //----------------------------------------------------------------------------------------------
    // String Compare
    //----------------------------------------------------------------------------------------------
    // Packed Compare Explicit Length Strings, Return Index
    void pcmpestri(XMMRegister lhs, XMMRegister rhs, int8_t code) {
        // 66 0F 3A 61 /r imm8
        EmitB(0x66);
        EmitOptionalRex32(lhs, rhs);
        EmitB(0x0F);
        EmitB(0x3A);
        EmitB(0x61);
        EmitOperand(lhs, rhs);
        EmitB(code);
    }
    
    void pcmpestri(XMMRegister lhs, Operand rhs, int8_t code) {
        // 66 0F 3A 61 /r imm8
        EmitB(0x66);
        EmitOptionalRex32(lhs, rhs);
        EmitB(0x0F);
        EmitB(0x3A);
        EmitB(0x61);
        EmitOperand(lhs, rhs);
        EmitB(code);
    }
    
    // PCMPESTRM — Packed Compare Explicit Length Strings, Return Mask
    void pcmpestrm(XMMRegister lhs, XMMRegister rhs, int8_t code) {
        // 66 0F 3A 60 /r imm8
        EmitB(0x66);
        EmitOptionalRex32(lhs, rhs);
        EmitB(0x0F);
        EmitB(0x3A);
        EmitB(0x60);
        EmitOperand(lhs, rhs);
        EmitB(code);
    }
    
    void pcmpestrm(XMMRegister lhs, Operand rhs, int8_t code) {
        // 66 0F 3A 60 /r imm8
        EmitB(0x66);
        EmitOptionalRex32(lhs, rhs);
        EmitB(0x0F);
        EmitB(0x3A);
        EmitB(0x60);
        EmitOperand(lhs, rhs);
        EmitB(code);
    }
    
    //----------------------------------------------------------------------------------------------
    // Memory barrier
    //----------------------------------------------------------------------------------------------
    // Serializes load and store operations.
    void mfence() {
        // NP 0F AE F0
        EmitB(0x0F);
        EmitB(0xAE);
        EmitB(0xF0);
    }
    
    // Serializes load operations.
    void lfence() {
        // NP 0F AE E8
        EmitB(0x0F);
        EmitB(0xAE);
        EmitB(0xE8);
    }
    
    // Serializes store operations.
    void sfence() {
        // NP 0F AE F8
        EmitB(0x0F);
        EmitB(0xAE);
        EmitB(0xF8);
    }
    
    //----------------------------------------------------------------------------------------------
    // Utils
    //----------------------------------------------------------------------------------------------
    void rdrand(Register dst, int size = kDefaultSize) {
        EmitRex(dst, size);
        EmitB(0x0F);
        EmitB(0xC7);
        EmitModRM(6, dst);
    }
    
    void Breakpoint() { int3(); }
    
    // Returns processor identification and feature information to the EAX, EBX, ECX, and EDX
    // registers, as determined by input entered in EAX (in some cases, ECX as well).
    void cpuid() {
        // 0F A2
        EmitB(0x0F);
        EmitB(0xA2);
    }

    void int3() { EmitB(0xCC); }
    
    // nop for one byte
    void nop() { EmitB(0x90); }
    
    // nop n bytes, 1 < n < 9
    void nop(int n);
    
    //----------------------------------------------------------------------------------------------
    // Prefix
    //----------------------------------------------------------------------------------------------
    // Only for jcc
    void PrefixUnlikely() { EmitB(0x2E); }
    void PrefixLikely() { EmitB(0x3E); }
    
    // Atomic prefix
    void PrefixLock() { EmitB(0xF0); }

    DISALLOW_IMPLICIT_CONSTRUCTORS(Assembler);
private:
    void EmitShift(Register dst, uint8_t amount, int subcode, int size) {
        DCHECK(size == sizeof(uint64_t) ? IsUintN(amount, 6) : IsUintN(amount, 5));
        if (amount == 1) {
            EmitRex(dst, size);
            EmitB(0xD1);
            EmitModRM(subcode, dst);
        } else {
            EmitRex(dst, size);
            EmitB(0xC1);
            EmitModRM(subcode, dst);
            EmitB(amount);
        }
    }

    void EmitShift(Register dst, int subcode, int size) {
        EmitRex(dst, size);
        EmitB(0xD3);
        EmitModRM(subcode, dst);
    }
    
    void EmitArith(uint8_t op, Register lhs, Operand rhs, int size) {
        EmitRex(lhs, rhs, size);
        EmitB(op);
        EmitOperand(lhs, rhs);
    }
    
    void EmitArith(uint8_t op, Register lhs, Register rhs, int size);
    
    void EmitArith(uint8_t subcode, Register lhs, int32_t imm, int size);
    
    void EmitArith(uint8_t subcode, Operand lhs, int32_t imm, int size);

    template<class L, class R>
    inline void EmitSSEArith(uint8_t prefix, uint8_t subcode, L lhs, R rhs) {
        if (prefix) {
            EmitB(prefix);
        }
        EmitOptionalRex32(lhs, rhs);
        EmitB(0x0F);
        EmitB(subcode);
        EmitOperand(lhs, rhs);
    }
    
    template<class L, class R>
    inline void EmitSSEArith(uint8_t prefix, uint8_t subcode, L lhs, R rhs, int size) {
        if (prefix) {
            EmitB(prefix);
        }
        EmitRex(lhs, rhs, size);
        EmitB(0x0F);
        EmitB(subcode);
        EmitOperand(lhs, rhs);
    }
    
    template<class T, class S>
    inline void EmitRex(T dst, S src, int size) {
        if (size == 8) {
            EmitRex64(dst, src);
        } else {
            DCHECK_EQ(size, 4);
            EmitOptionalRex32(dst, src);
        }
    }
    
    template<class T>
    inline void EmitRex(T arg, int size) {
        if (size == 8) {
            EmitRex64(arg);
        } else {
            DCHECK_EQ(size, 4);
            EmitOptionalRex32(arg);
        }
    }
    
    void EmitRex64() { EmitB(0x48); }
    
    void EmitRex64(Register rmreg) {
        DCHECK((rmreg.code() & 0xf) == rmreg.code());
        EmitB(0x48 | rmreg.hi_bits());
    }

    void EmitRex64(Operand opd) { EmitB(0x48 | opd.rex()); }

    void EmitRex64(XMMRegister reg, Register rmreg) { EmitRex64(reg, XMMRegister{rmreg.code()}); }
    void EmitRex64(Register reg, XMMRegister rmreg) { EmitRex64(XMMRegister{reg.code()}, rmreg); }

    void EmitRex64(Register reg, Register rmreg) {
        EmitB(0x48 | reg.hi_bits() << 2 | rmreg.hi_bits());
    }
    
    void EmitRex64(XMMRegister reg, XMMRegister rmreg) {
        EmitB(0x48 | (reg.code() & 0x8) >> 1 | rmreg.code() >> 3);
    }

    void EmitRex64(Register reg, Operand opd) { EmitB(0x48 | reg.hi_bits() << 2 | opd.rex()); }
    void EmitRex64(XMMRegister reg, Operand opd) {
        EmitB(0x48 | (reg.code() & 0x8) >> 1 | opd.rex());
    }

    void EmitRex32(Register rmreg) { EmitB(0x40 | rmreg.hi_bits()); }

    void EmitRex32(Operand opd) { EmitB(0x40 | opd.rex()); }
    
    void EmitRex32(Register reg, Register rmreg) {
        EmitB(0x40 | reg.hi_bits() << 2 | rmreg.hi_bits());
    }
    
    void EmitRex32(Register reg, Operand opd) {
        EmitB(0x40 | reg.hi_bits() << 2 | opd.rex());
    }
    
    void EmitOptionalRex32(Register rmreg) { if (rmreg.hi_bits()) { EmitB(0x41); } }
    
    void EmitOptionalRex32(Operand opd) {
        if (opd.rex() != 0) { EmitB(0x40 | opd.rex()); }
    }
    
    void EmitOptionalRex32(Register reg, Register rmreg) {
        uint8_t rex_bits = reg.hi_bits() << 2 | rmreg.hi_bits();
        if (rex_bits != 0) { EmitB(0x40 | rex_bits); }
    }
    
    void EmitOptionalRex32(Register reg, Operand opd) {
        uint8_t rex_bits = reg.hi_bits() << 2 | opd.rex();
        if (rex_bits != 0) { EmitB(0x40 | rex_bits); }
    }

    void EmitOptionalRex32(XMMRegister reg, Operand opd) {
        uint8_t rex_bits = (reg.code() & 0x8) >> 1 | opd.rex();
        if (rex_bits != 0) { EmitB(0x40 | rex_bits); }
    }
    
    void EmitOptionalRex32(XMMRegister reg, XMMRegister base) {
        uint8_t rex_bits = (reg.code() & 0x8) >> 1 | (base.code() & 0x8) >> 3;
        if (rex_bits != 0) { EmitB(0x40 | rex_bits); }
    }
    
    void EmitOptionalRex32(XMMRegister reg, Register base) {
        EmitOptionalRex32(reg, XMMRegister{base.code()});
    }

    void EmitOptionalRex32(Register reg, XMMRegister base) {
        EmitOptionalRex32(XMMRegister{reg.code()}, base);
    }
    
    void EmitModRM(Register reg, Register rmreg) {
        EmitB(0xC0 | reg.lo_bits() << 3 | rmreg.lo_bits());
    }
    
    void EmitModRM(int n, Register rmreg) {
        EmitB(0xC0 | ((n) << 3) | rmreg.lo_bits());
    }

    void EmitOperand(Register reg, Operand opd) { EmitOperand(reg.lo_bits(), opd); }
    
    void EmitOperand(XMMRegister reg, Operand opd) { EmitOperand(Register(reg.code()), opd); }

    void EmitOperand(XMMRegister dst, XMMRegister src) {
        EmitB(0xC0 | (dst.lo_bits() << 3) | src.lo_bits());
    }

    void EmitOperand(XMMRegister dst, Register src) {
        EmitB(0xC0 | (dst.lo_bits() << 3) | src.lo_bits());
    }
    
    void EmitOperand(Register dst, XMMRegister src) {
        EmitB(0xC0 | (dst.lo_bits() << 3) | src.lo_bits());
    }
    
    void EmitOperand(int code, Operand addr) {
        DCHECK(IsUintN(code, 3));
        
        const unsigned len = addr.len();
        DCHECK_GT(len, 0);

        DCHECK_EQ((addr.buf()[0] & 0x38), 0);
        EmitB(addr.buf()[0] | code << 3);
        Emit(addr.buf() + 1, len - 1);
    }
    
    void EmitB(uint8_t x) { EmitT<uint8_t>(x); }
    void EmitW(uint16_t x) { EmitT<uint16_t>(x); }
    void EmitDW(uint32_t x) { EmitT<uint32_t>(x); }
    void EmitQW(uint64_t x) { EmitT<uint64_t>(x); }
    void EmitP(void *x) { EmitT<void *>(x); }
    
    template<class T>
    inline void EmitT(T x) {
        pc_ += sizeof(x);
        buf_.append(reinterpret_cast<char *>(&x), sizeof(x));
    }
    
    void Emit(const void *d, size_t n) {
        pc_ += n;
        buf_.append(static_cast<const char *>(d), n);
    }
    
    Address AddrAt(int pos) { return reinterpret_cast<Address>(&buf_[pos]); }
    uint32_t LongAt(int pos) { return *reinterpret_cast<uint32_t *>(AddrAt(pos)); }
    void LongPut(int pos, uint32_t x) { *reinterpret_cast<uint32_t *>(AddrAt(pos)) = x; }

    int pc_ = 0;
    std::string buf_;
}; // class Assembler

} // namespace x64

} // namespace mai

#endif // MAI_ASM_X64_H_
