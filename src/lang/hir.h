#ifndef MAI_LANG_HIR_H_
#define MAI_LANG_HIR_H_

#include "base/arenas.h"
#include "base/queue-macros.h"
#include "base/base.h"

namespace mai {

namespace lang {

class HOperatorFactory;
class HOperator;
class HValue;
class HNode;
class Used;

#define DECLARE_HIR_TYPES(V) \
    V(Word8,   8, HType::kNumberBit) \
    V(Word16,  16, HType::kNumberBit) \
    V(Word32,  32, HType::kNumberBit) \
    V(Word64,  64, HType::kNumberBit) \
    V(Int8,    8,  HType::kSignedBit|HType::kNumberBit) \
    V(Int16,   16, HType::kSignedBit|HType::kNumberBit) \
    V(Int32,   32, HType::kSignedBit|HType::kNumberBit) \
    V(Int64,   64, HType::kSignedBit|HType::kNumberBit) \
    V(UInt8,   8,  HType::kNumberBit) \
    V(UInt16,  16, HType::kNumberBit) \
    V(UInt32,  32, HType::kNumberBit) \
    V(UInt64,  64, HType::kNumberBit) \
    V(Float32, 32, HType::kSignedBit|HType::kNumberBit) \
    V(Float64, 64, HType::kSignedBit|HType::kNumberBit|HType::kFloatBit) \
    V(Any,     kPointerSize * 8, HType::kReferenceBit)

class HType {
public:
    enum Kind {
    #define DEFINE_KINDS(name, ...) k##name,
        DECLARE_HIR_TYPES(DEFINE_KINDS)
    #undef DEFINE_KINDS
    };
    
    static constexpr uint32_t kSignedBit = 1u;
    static constexpr uint32_t kNumberBit = 1u << 1;
    static constexpr uint32_t kFloatBit = 1u << 2;
    static constexpr uint32_t kReferenceBit = 1u << 3;
    
    constexpr HType(Kind kind, int bits, uint32_t flags)
        : kind_(kind)
        , bits_(bits)
        , flags_(flags) {}
    
    DEF_VAL_GETTER(Kind, kind);
    DEF_VAL_GETTER(int, bits);
    int bytes() const { return bits_ / 8; }
    
    bool is_signed() const { return flags_ & kSignedBit; }
    bool is_unsigned() const { return !is_signed(); }
    bool is_float() const { return flags_ & kFloatBit; }
    bool is_number() const { return flags_ & kNumberBit; }
    bool is_reference() const { return flags_ & kReferenceBit; }
    
private:
    const Kind kind_;
    const int bits_;
    const uint32_t flags_;
}; // class HType

struct HTypes {
#define DEFINE_TYPES(name, bits, flags) \
    static constexpr HType name = HType(HType::k##name, bits, flags);
    DECLARE_HIR_TYPES(DEFINE_TYPES)
#undef DEFINE_TYPES
};

#define DECLARE_HIR_OPCODES(V) \
    V(Phi) \
    V(Guard) \
    V(Constant) \
    V(Argument) \
    V(Branch) \
    DECLARE_HIR_COMPAROR(V) \
    DECLARE_HIR_CALLING(V)

#define DECLARE_HIR_COMPAROR(V) \
    V(Equal) \
    V(NotEqual) \
    V(LessThan) \
    V(LessOrEqual) \
    V(GreaterThan) \
    V(GreaterOrEqual) \
    V(StringEQ) \
    V(StringNE) \
    V(StringLT) \
    V(StringLE) \
    V(StringGT) \
    V(StringGE) \

#define DECLARE_HIR_CALLING(V) \
    V(CallInline) \
    V(CallBytecode) \
    V(CallVtab) \
    V(CallNative)

enum HOperatorCode {
#define DEFINE_CODE(name, ...) H##name,
    DECLARE_HIR_OPCODES(DEFINE_CODE)
#undef DEFINE_CODE
    HMaxOpcode,
};

class HOperator {
public:
    using Value = HOperatorCode;
    
    static constexpr uint64_t kWordBits = 1;
    static constexpr uint64_t kSignedBits = 2;
    static constexpr uint64_t kUnsignedBits = 3;
    static constexpr uint64_t kFloatingBits = 4;
    static constexpr uint64_t kSimpledMask = 0xf;
    
    static constexpr uint64_t kLikelyBits = 0x10;
    static constexpr uint64_t kUnlikelyBits = 0x20;
    static constexpr uint64_t kConditionMask = 0xf0;
    
    DEF_VAL_GETTER(Value, value);
    DEF_VAL_GETTER(int, control_in);
    DEF_VAL_GETTER(int, effect_in);
    DEF_VAL_GETTER(int, value_in);
    DEF_VAL_GETTER(int, control_out);
    DEF_VAL_GETTER(int, effect_out);
    DEF_VAL_GETTER(int, value_out);
    
    bool WordHint() const { return (properties_ & kSimpledMask) == kWordBits; }
    bool SignedHint() const { return (properties_ & kSimpledMask) == kSignedBits; }
    bool UnsignedHint() const { return (properties_ & kSimpledMask) == kUnsignedBits; }
    bool FloatingHint() const { return (properties_ & kSimpledMask) == kFloatingBits; }
    
    bool LikelyHint() const { return ConditionHint() == kLikelyBits; }
    bool UnlikelyHint() const { return ConditionHint() == kUnlikelyBits; }
    
    uint64_t ConditionHint() const { return (properties_ & kConditionMask) >> 4; }
    
    const char *name() const { return kNames[value_]; }
    
    void *operator new(size_t n, base::Arena *arena) { return arena->Allocate(n); }
    
    friend class HOperatorFactory;
    DISALLOW_IMPLICIT_CONSTRUCTORS(HOperator);
private:
    HOperator(Value value, uint64_t properties, int control_in, int effect_in, int value_in,
              int control_out, int effect_out, int value_out)
        : value_(value)
        , properties_(properties)
        , control_in_(control_in)
        , effect_in_(effect_in)
        , value_in_(value_in)
        , control_out_(control_out)
        , effect_out_(effect_out)
        , value_out_(value_out) {}

    Value value_;
    uint64_t properties_;
    int control_in_;
    int effect_in_;
    int value_in_;
    int control_out_;
    int effect_out_;
    int value_out_;
    
    static const char *kNames[HMaxOpcode];
}; // class HOperator

class HOperatorFactory final {
public:
    HOperatorFactory(base::Arena *arena)
        : arena_(arena) {
        ::memset(cache_, 0, sizeof(cache_[0]) * HMaxOpcode);
        Initialize();
    }

    const HOperator *Phi(int value_in) {
        return new (arena_) HOperator(HPhi, 0, 0, 0, value_in, 0, 0, 1);
    }

    const HOperator *Guard(uint64_t hint) {
        return new (arena_) HOperator(HGuard, hint, 0, 0, 0, 0, 0, 0);
    }

    const HOperator *Constant(uint64_t hint) {
        return new (arena_) HOperator(HConstant, hint, 0, 0, 0, 0, 0, 1);
    }

    const HOperator *Argument(uint64_t hint) {
        return new (arena_) HOperator(HArgument, hint, 0, 0, 0, 0, 0, 1);
    }
    
//    const HOperator *Branch() {
//        
//    }

    const HOperator *Equal(uint64_t hint) {
        return new (arena_) HOperator(HEqual, hint, 0, 0, 2, 0, 0, 1);
    }
    
    const HOperator *NotEqual(uint64_t hint) {
        return new (arena_) HOperator(HNotEqual, hint, 0, 0, 2, 0, 0, 1);
    }
    
    const HOperator *LessThan(uint64_t hint) {
        return new (arena_) HOperator(HLessThan, hint, 0, 0, 2, 0, 0, 1);
    }
    
    const HOperator *LessOrEqual(uint64_t hint) {
        return new (arena_) HOperator(HLessOrEqual, hint, 0, 0, 2, 0, 0, 1);
    }
    
    const HOperator *GreaterThan(uint64_t hint) {
        return new (arena_) HOperator(HGreaterThan, hint, 0, 0, 2, 0, 0, 1);
    }
    
    const HOperator *GreaterOrEqual(uint64_t hint) {
        return new (arena_) HOperator(HGreaterOrEqual, hint, 0, 0, 2, 0, 0, 1);
    }
    

    DISALLOW_IMPLICIT_CONSTRUCTORS(HOperatorFactory);
private:
    void Initialize();
    
    base::Arena *arena_;
    const HOperator *cache_[HMaxOpcode];
}; // class HOperatorFactory

class HValue {
public:
    DEF_VAL_GETTER(int, vid);
    DEF_VAL_GETTER(int, hint);
    DEF_VAL_GETTER(HType, type);

    DISALLOW_IMPLICIT_CONSTRUCTORS(HValue);
protected:
    HValue(int vid, int hint, HType type)
        : vid_(vid)
        , hint_(hint)
        , type_(type) {}

    int vid_;
    int hint_;
    HType type_;
}; // class HValue


struct Used {
    void *operator new(size_t n, base::Arena *arena) { return arena->Allocate(n); }

    Used *next_;
    Used *prev_;
    HNode *user;
}; // struct Used

class HNode : public HValue {
public:
    static inline HNode *New(base::Arena *arena, int vid, int hint, HType type, const HOperator *op,
                             HNode **inputs, int inputs_size) {
        size_t request_size = sizeof(HNode) + sizeof(*inputs) * inputs_size;
        void *chunk = arena->Allocate(request_size);
        return new (chunk) HNode(arena, vid, hint, type, op, inputs_size, inputs);
    }
    
    DEF_PTR_GETTER(const HOperator, op);
    HOperator::Value opcode() const { return op_->value(); }
    
    DEF_VAL_GETTER(int, inputs_size);

    HNode *input(int i) const {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, inputs_size());
        return inputs_[i];
    }
    
    HNode *lhs() const {
        DCHECK_EQ(op_->value_in(), 2);
        return input(0);
    }
    
    HNode *rhs() const {
        DCHECK_EQ(op_->value_in(), 2);
        return input(1);
    }
    
    Used *BeUsed(base::Arena *arena, HNode *user) {
        Used *used = new (arena) Used{};
        used->next_ = used;
        used->prev_ = used;
        used->user  = user;
        QUEUE_INSERT_TAIL(&used_, used);
        return used;
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(HNode);
private:
    HNode(base::Arena *arena, int vid, int hint, HType type, const HOperator *op, int inputs_size,
          HNode **inputs)
        : HValue(vid, hint, type)
        , op_(op)
        , inputs_size_(inputs_size) {
        used_.next_ = &used_;
        used_.prev_ = &used_;
        for (int i = 0; i < inputs_size_; i++) {
            inputs_[i] = inputs[i];
            BeUsed(arena, inputs_[i]);
        }
    }
    
    const HOperator *op_;
    Used used_;
    int inputs_size_;
    HNode *inputs_[0];
}; // class HNode




} // namespace lang

} // namespace mai

#endif // MAI_LANG_HIR_H_
