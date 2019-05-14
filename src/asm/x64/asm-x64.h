#ifndef MAI_ASM_X64_H_
#define MAI_ASM_X64_H_

#include "base/base.h"
#include "glog/logging.h"
#include <string>

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
    
    Carry    = Below,
    NotCarry = AboveEqual,
    Zero     = Equal,
    NotZero  = NotEqual,
    Sign     = Negative,
    NotSign  = Positive,
    LastCond = Greater,
}; // Cond;
    
struct Register {
    int code;
}; // struct Register
    
extern const Register rax; // 0
extern const Register rcx; // 1
extern const Register rdx; // 2
extern const Register rbx; // 3
extern const Register rsp; // 4

extern const Register rbp; // 5
extern const Register rsi; // 6
extern const Register rdi; // 7
extern const Register r8;  // 8
extern const Register r9;  // 9

extern const Register r10; // 10
extern const Register r11; // 11
extern const Register r12; // 12
extern const Register r13; // 13
extern const Register r14; // 14

extern const Register r15; // 15
extern const Register rNone; // -1

struct Xmm {
    int code;
}; // struct Xmm
    
extern const Xmm xmm0; // 0
extern const Xmm xmm1; // 1
extern const Xmm xmm2; // 2
extern const Xmm xmm3; // 3
extern const Xmm xmm4; // 4

extern const Xmm xmm5; // 5
extern const Xmm xmm6; // 6
extern const Xmm xmm7; // 7
extern const Xmm xmm8; // 8
extern const Xmm xmm9; // 9

extern const Xmm xmm10; // 10
extern const Xmm xmm11; // 11
extern const Xmm xmm12; // 12
extern const Xmm xmm13; // 13
extern const Xmm xmm14; // 14

extern const Xmm xmm15; // 15
    
static const constexpr int kMaxRegArgs = 8;
static const constexpr int kMaxXmmArgs = 8;
static const constexpr int kMaxAllocRegs = 11;
static const constexpr int kMaxAllocXmms = 15;
    
extern const Register kRegArgv[kMaxRegArgs];
extern const Xmm kXmmArgv[kMaxXmmArgs];
extern const Register kRegAlloc[kMaxAllocRegs];
    
#define RegBits(reg)   (1 << (reg).code)
#define RegHiBit(reg)  ((reg).code >> 3)
#define RegLoBits(reg) ((reg).code & 0x7)
#define RegIsByte(reg) ((reg).code <= 3)
    
#define XmmHiBit(reg)  RegHiBit(reg)
#define XmmLoBits(reg) RegLoBits(reg)
    
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
    
    // [base + index * scale + disp/r]
    Operand(Register base, Register index, ScaleFactor scale,
            int32_t disp);
    
    // [index * scale + disp/r]
    Operand(Register index, ScaleFactor scale, int32_t disp);
    
    DEF_VAL_GETTER(uint8_t, rex);
    DEF_VAL_GETTER(uint8_t, len);
    const uint8_t *buf() const { return buf_; }

private:
    void SetModRM(int mod, Register rmreg) {
        DCHECK(IsUintN(mod, 2));
        buf_[0] = mod << 6 | RegLoBits(rmreg);
        rex_ |= RegHiBit(rmreg);
    }

    void SetSIB(ScaleFactor scale, Register index, Register base) {
        DCHECK_EQ(len_, 1);
        DCHECK(IsUintN(scale, 2));
        DCHECK(index.code != rsp.code || base.code == rsp.code ||
               base.code == r12.code);
        
        buf_[1] = (((int)scale) << 6) | (RegLoBits(index) << 3) | RegLoBits(base);
        rex_ |= RegHiBit(index) << 1 | RegHiBit(base);
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
    Label() = default;

    DEF_VAL_GETTER(int, pos);
    DEF_VAL_PROP_RW(int, near_link_pos);
    
    bool IsBound() const { return pos_ < 0; }
    bool IsUnused() const { return pos_ == 0 && near_link_pos_ == 0; }
    bool IsLinked() const { return pos_ > 0; }
    bool IsNearLinked() const { return near_link_pos_ > 0; }
    bool GetNearLinkPosition() const { return near_link_pos_ - 1; }
    
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
        DLOG(FATAL) << "noreached";
        return 0;
    }
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
    
    void Emit_lea(Register dst, Operand src, int size = kDefaultSize) {
        EmitRex(dst, src, size);
        EmitB(0x8D);
        EmitOperand(dst, src);
    }
    
    void Emit_rdrand(Register dst, int size = kDefaultSize) {
        EmitRex(dst, size);
        EmitB(0x0F);
        EmitB(0xC7);
        EmitModRM(6, dst);
    }
    
    //----------------------------------------------------------------------------------------------
    // Stack operations:
    //----------------------------------------------------------------------------------------------
    void Emit_pushq(Register src) {
        EmitOptionalRex32(src);
        EmitB(0x50 | RegLoBits(src));
    }
    
    void Emit_pushq(Operand opd) {
        EmitOptionalRex32(opd);
        EmitB(0xff);
        EmitOperand(6, opd);
    }
    
    void Emit_pushq(int32_t val) {
        EmitB(0x68);
        EmitDW(val);
    }
    
    void Emit_pushfq() { EmitB(0x9c); }
    
    void Emit_popq(Register dst) {
        EmitOptionalRex32(dst);
        EmitB(0x58 | RegLoBits(dst));
    }
    
    void Emit_popq(Operand dst) {
        EmitOptionalRex32(dst);
        EmitB(0x8F);
        EmitOperand(0, dst);
    }
    
    void Emit_popfq() { EmitB(0x9D); }
    
    //----------------------------------------------------------------------------------------------
    // Movs
    //----------------------------------------------------------------------------------------------
    void Emit_movp(Register dst, Address val) {
        EmitRex(dst, sizeof(val));
        EmitB(0xB8 | RegLoBits(dst));
        EmitP(val);
    }
    
    void Emit_movq(Register dst, Address val) {
        DCHECK_EQ(sizeof(val), sizeof(int64_t));
        EmitRex64(dst);
        EmitB(0xB8 | RegLoBits(dst));
        EmitP(val);
    }
    
    void Emit_movq(Register dst, int64_t val) {
        EmitRex64(dst);
        EmitB(0xB8 | RegLoBits(dst));
        EmitQW(val);
    }
    
    void Emit_movq(Register dst, Register src, int size = kDefaultSize);
    
    void Emit_movq(Register dst, Operand src, int size = kDefaultSize) {
        EmitRex(dst, src, size);
        EmitB(0x8B);
        EmitOperand(dst, src);
    }
    
    void Emit_movq(Operand dst, Register src, int size = kDefaultSize) {
        EmitRex(src, dst, size);
        EmitB(0x89);
        EmitOperand(src, dst);
    }
    
    void Emit_movq(Register dst, Immediate src, int size = kDefaultSize);
    
    void Emit_movq(Operand dst, Immediate src, int size = kDefaultSize) {
        EmitRex(dst, size);
        EmitB(0xC7);
        EmitOperand(0x0, dst);
        EmitDW(src.value());
    }
    
    //----------------------------------------------------------------------------------------------
    // Jmps
    //----------------------------------------------------------------------------------------------
    
    // Calling
    void Emit_call(Label *l);

    void Emit_call(Address addr) {
        // 1110 1000 #32-bit disp
        EmitB(0xE8);
        Address src = AddrAt(pc() + 4);
        int32_t disp = static_cast<int32_t>(addr - src);
        DCHECK(IsIntN(disp, 32)); // near call; only to 32bit
        EmitDW(disp);
    }

    void Emit_call(Register addr) {
        // opcode: FF /2 r64
        EmitOptionalRex32(addr);
        EmitB(0xff);
        EmitModRM(0x2, addr);
    }

    void Emit_call(Operand addr) {
        // opcode: FF /2 m64
        EmitOptionalRex32(addr);
        EmitB(0xff);
        EmitOperand(0x2, addr);
    }

    void Emit_ret(int val) {
        DCHECK(IsUintN(val, 16));
        if (val == 0) {
            EmitB(0xC3);
        } else {
            EmitB(0xC2);
            EmitB(val & 0xFF);
            EmitB((val >> 8) & 0xFF);
        }
    }
    
    // Jumping
    void Emit_jmp(Label *l, bool is_far);

    void Emit_jmp(Address addr); // always near jmp
    
    // Jump near, absolute indirect, RIP = 64-Bit offset from register or memory
    void Emit_jmp(Register addr) {
        // opcode: FF /4 r64
        EmitOptionalRex32(addr);
        EmitB(0xff);
        EmitModRM(0x4, addr);
    }

    void Emit_jmp(Operand addr) { // always far jmp
        // opcode: FF /4 m64
        EmitOptionalRex32(addr);
        EmitB(0xff);
        EmitOperand(0x4, addr);
    }
    
    void Emit_jcc(Cond cond, Label *l, bool is_far);
    
    void Bind(Label *l) { BindTo(l, pc()); }
    
    void BindTo(Label *l, int pos);
    
    //----------------------------------------------------------------------------------------------
    // Tests
    //----------------------------------------------------------------------------------------------
    // Logic Compare
    void Emit_test(Register dst, Register src, int size = kDefaultSize);
    void Emit_test(Register dst, Immediate mask, int size = kDefaultSize);

    void Emit_test(Operand dst, Register src, int size = kDefaultSize) {
        EmitRex(src, dst, size);
        EmitB(0x85);
        EmitOperand(src, dst);
    }

    void Emit_test(Operand dst, Immediate mask, int size = kDefaultSize);
    
    //----------------------------------------------------------------------------------------------
    // Neg/Not/Shift
    //----------------------------------------------------------------------------------------------
    // Neg
    void Emit_neg(Register dst, int size = kDefaultSize) {
        EmitRex(dst, size);
        EmitB(0xF7);
        EmitModRM(0x3, dst);
    }

    void Emit_neg(Operand dst, int size = kDefaultSize) {
        EmitRex(dst, size);
        EmitB(0xF7);
        EmitOperand(0x3, dst);
    }

    // Not
    void Emit_not(Register dst, int size = kDefaultSize) {
        EmitRex(dst, size);
        EmitB(0xF7);
        EmitModRM(0x2, dst);
    }

    void Emit_not(Operand dst, int size = kDefaultSize) {
        EmitRex(dst, size);
        EmitB(0xF7);
        EmitOperand(0x2, dst);
    }

    // Shift
    void Emit_shift(Register dst, Immediate amount, int subcode, int size = kDefaultSize);

    void Emit_shift(Register dst, int subcode, int size = kDefaultSize) {
        EmitRex(dst, size);
        EmitB(0xD3);
        EmitModRM(subcode, dst);
    }

    // shift dst:src left by cl bits, affecting only dst.
    void Emit_shld(Register dst, Register src);

    // shift src:dst right by cl bits, affecting only dst.
    void Emit_shrd(Register dst, Register src);

    //----------------------------------------------------------------------------------------------
    // Debug
    //----------------------------------------------------------------------------------------------
    void EmitBreakpoint() { Emit_int3(); }

    void Emit_int3() { EmitB(0xCC); }
    
    void Emit_nop() {
        //0x90; 0x0f 0x1f
        EmitB(0x90);
        EmitB(0x0F);
        EmitB(0x1F);
    }

    DISALLOW_IMPLICIT_CONSTRUCTORS(Assembler);
private:
    
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
        DCHECK(((rmreg).code & 0xf) == (rmreg).code);
        EmitB(0x48 | RegHiBit(rmreg));
    }

    void EmitRex64(Operand opd) { EmitB(0x48 | opd.rex()); }

    void EmitRex64(Xmm reg, Register rmreg) { EmitRex64(reg, Xmm{rmreg.code}); }
    void EmitRex64(Register reg, Xmm rmreg) { EmitRex64(Xmm{reg.code}, rmreg); }

    void EmitRex64(Register reg, Register rmreg) {
        EmitB(0x48 | RegHiBit(reg) << 2 | RegHiBit(rmreg));
    }
    
    void EmitRex64(Xmm reg, Xmm rmreg) {
        EmitB(0x48 | ((reg).code & 0x8) >> 1 | (rmreg).code >> 3);
    }
    
    void EmitRex64(Register reg, Operand opd) { EmitB(0x48 | RegHiBit(reg) << 2 | opd.rex()); }
    void EmitRex64(Xmm reg, Operand opd) { EmitB(0x48 | ((reg).code & 0x8) >> 1 | opd.rex()); }
    
    void EmitRex32(Register rmreg) { EmitB(0x40 | RegHiBit(rmreg)); }

    void EmitRex32(Operand opd) { EmitB(0x40 | opd.rex()); }
    
    void EmitRex32(Register reg, Register rmreg) {
        EmitB(0x40 | RegHiBit(reg) << 2 | RegHiBit(rmreg));
    }
    
    void EmitRex32(Register reg, Operand opd) {
        EmitB(0x40 | RegHiBit(reg) << 2 | opd.rex());
    }
    
    void EmitOptionalRex32(Register rmreg) {
        if (RegHiBit(rmreg)) { EmitB(0x41); }
    }
    
    void EmitOptionalRex32(Operand opd) {
        if (opd.rex() != 0) { EmitB(0x40 | opd.rex()); }
    }
    
    void EmitOptionalRex32(Register reg, Register rmreg) {
        uint8_t rex_bits = RegHiBit(reg) << 2 | RegHiBit(rmreg);
        if (rex_bits != 0) { EmitB(0x40 | rex_bits); }
    }
    
    void EmitOptionalRex32(Register reg, Operand opd) {
        uint8_t rex_bits = RegHiBit(reg) << 2 | opd.rex();
        if (rex_bits != 0) { EmitB(0x40 | rex_bits); }
    }

    void EmitOptionalRex32(Xmm reg, Operand opd) {
        uint8_t rex_bits = ((reg).code & 0x8) >> 1 | opd.rex();
        if (rex_bits != 0) { EmitB(0x40 | rex_bits); }
    }
    
    void EmitOptionalRex32(Xmm reg, Xmm base) {
        uint8_t rex_bits = ((reg).code & 0x8) >> 1 | ((base).code & 0x8) >> 3;
        if (rex_bits != 0) { EmitB(0x40 | rex_bits); }
    }
    
    void EmitOptionalRex32(Xmm reg, Register base) { EmitOptionalRex32(reg, Xmm{base.code}); }

    void EmitOptionalRex32(Register reg, Xmm base) { EmitOptionalRex32(Xmm{reg.code}, base); }
    
    void EmitModRM(Register reg, Register rmreg) {
        EmitB(0xc0 | RegLoBits(reg) << 3 | RegLoBits(rmreg));
    }
    
    void EmitModRM(int n, Register rmreg) {
        EmitB(0xc0 | ((n) << 3) | RegLoBits(rmreg));
    }
    
    void EmitOperand(Register reg, Operand opd) { EmitOperand(RegLoBits(reg), opd); }
    
    void EmitOperand(Xmm reg, Operand opd) { EmitOperand(Register{reg.code}, opd); }

    void EmitOperand(Xmm dst, Xmm src) { EmitB(0xC0 | (XmmLoBits(dst) << 3) | XmmLoBits(src)); }

    void EmitOperand(Xmm dst, Register src) {
        EmitB(0xC0 | (XmmLoBits(dst) << 3) | RegLoBits(src));
    }
    
    void EmitOperand(Register dst, Xmm src) {
        EmitB(0xC0 | (RegLoBits(dst) << 3) | XmmLoBits(src));
    }
    
    void EmitOperand(int code, Operand addr) {
        DCHECK(IsUintN(code, 3));
        
        const unsigned len = addr.len();
        DCHECK_GT(len, 0);
        
        DCHECK_EQ((addr.buf()[0] & 0x38), 0);
        EmitB(addr.buf()[0] | code << 3);

//        for (unsigned i = 1; i < len; i++) {
//            EmitB(addr.buf()[i]);
//        }
        Emit(addr.buf(), len);
        //pc_ += len;
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
