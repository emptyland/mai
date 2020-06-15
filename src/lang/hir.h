#ifndef MAI_LANG_HIR_H_
#define MAI_LANG_HIR_H_

#include "base/arenas.h"
#include "base/queue-macros.h"
#include "base/base.h"

namespace mai {

namespace lang {

class CompilationInfo;
class HOperatorFactory;
class HOperator;
class HValue;
class HNode;
class Used;

#define DECLARE_HIR_TYPES(V) \
    V(Void,    0, 0) \
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
    V(Start) \
    V(End) \
    V(Merge) \
    V(FrameState) \
    V(Phi) \
    V(Guard) \
    V(Constant) \
    V(Argument) \
    V(Branch) \
    V(CheckStack) \
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

class HOperator : public base::ArenaObject {
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
    
    static void UpdateControlIn(const HOperator *op, int value) {
        const_cast<HOperator *>(op)->control_in_ = value;
    }
    static void UpdateEffectIn(const HOperator *op, int value) {
        const_cast<HOperator *>(op)->effect_in_ = value;
    }
    static void UpdateValueIn(const HOperator *op, int value) {
        const_cast<HOperator *>(op)->value_in_ = value;
    }

    friend class HOperatorFactory;
    DISALLOW_IMPLICIT_CONSTRUCTORS(HOperator);
protected:
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


template<class T>
class HOperatorWith : public HOperator {
public:
    T data() const { return data_; }
    
    static inline T Data(const HNode *node);
    
    static inline T Data(const HOperator *op) { return DCHECK_NOTNULL(Cast(op))->data(); }
    
    static inline const HOperatorWith *Cast(const HOperator *op) {
        return static_cast<const HOperatorWith *>(op);
    }

    friend class HOperatorFactory;
    DISALLOW_IMPLICIT_CONSTRUCTORS(HOperatorWith);
protected:
    inline HOperatorWith(Value value, uint64_t properties, int control_in, int effect_in,
                         int value_in, int control_out, int effect_out, int value_out, T data)
    : HOperator(value, properties, control_in, effect_in, value_in,
                control_out, effect_out, value_out)
    , data_(data) {}
    
private:
    T data_;
}; // template<class T> class HOperatorWith

class HOperatorFactory final {
public:
    HOperatorFactory(base::Arena *arena)
        : arena_(arena) {
        ::memset(cache_, 0, sizeof(cache_[0]) * HMaxOpcode);
        Initialize();
    }
    
    const HOperator *Start() {
        return new (arena_) HOperator(HStart, 0, 0, 0, 0, 0, 0, 0);
    }
    
    const HOperator *End(int control_in) {
        return new (arena_) HOperator(HEnd, 0, 0, control_in, 0, 0, 0, 0);
    }
    
    const HOperator *CheckStack() {
        return new (arena_) HOperator(HCheckStack, 0, 0, 0, 0, 0, 0, 0);
    }

    const HOperator *Phi(int control_in, int value_in) {
        return new (arena_) HOperator(HPhi, 0, control_in, 0, value_in, 0, 0, 0);
    }

    const HOperator *Guard(uint64_t hint) {
        return new (arena_) HOperator(HGuard, hint, 0, 0, 0, 0, 0, 0);
    }

    const HOperator *ConstantWord32(uint32_t data) {
        return new (arena_) HOperatorWith<uint32_t>(HConstant, 0, 0, 0, 0, 0, 0, 0, data);
    }
    
    const HOperator *ConstantWord64(uint64_t data) {
        return new (arena_) HOperatorWith<uint64_t>(HConstant, 0, 0, 0, 0, 0, 0, 0, data);
    }

    const HOperator *Argument(uint64_t hint) {
        return new (arena_) HOperator(HArgument, hint, 0, 0, 0, 0, 0, 1);
    }

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
    static inline HNode *New(base::Arena *arena, int vid, HType type, const HOperator *op,
                             int inputs_capacity, int inputs_size, HNode **inputs) {
        DCHECK_GE(inputs_size, 0);
        DCHECK_GE(inputs_capacity, inputs_size);
        size_t request_size = sizeof(HNode) + sizeof(*inputs) * inputs_capacity;
        void *chunk = arena->Allocate(request_size);
        return new (chunk) HNode(arena, vid, 0, type, op, inputs_capacity, inputs_size, inputs);
    }
    
    static HNode *Clone(base::Arena *arena, HNode *src, int inputs_capacity) {
        for (int i = 0; i < src->inputs_size(); i++) {
            src->input(i)->Unused(src);
        }
        HNode *dest = New(arena, src->vid(), src->type(), src->op(), inputs_capacity,
                          src->inputs_size(), src->inputs_);
        dest->used_ = src->used_;
        return dest;
    }

    DEF_PTR_GETTER(const HOperator, op);
    HOperator::Value opcode() const { return op_->value(); }

    DEF_VAL_GETTER(int, inputs_capacity);
    DEF_VAL_GETTER(int, inputs_size);

    HNode *input(int i) const {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, inputs_size());
        return DCHECK_NOTNULL(inputs_[i]);
    }
    
    HNode *lhs() const {
        DCHECK_EQ(inputs_size(), 2);
        return input(0);
    }
    
    HNode *rhs() const {
        DCHECK_EQ(inputs_size(), 2);
        return input(1);
    }
    
    void ReplaceInput(base::Arena *arena, int i, HNode *node) {
        HNode *old = input(i);
        inputs_[i] = node;
        BeUsed(arena, node);
        old->Unused(this);
    }

    HNode *AppendInput(base::Arena *arena, HNode *node) {
        HNode *n = this;
        if (n->inputs_size() + 1 > n->inputs_capacity()) {
            n = Clone(arena, n, n->inputs_capacity() << 1);
        }
        n->inputs_[n->inputs_size_++] = node;
        n->BeUsed(arena, node);
        return n;
    }

    Used *BeUsed(base::Arena *arena, HNode *used);
    
    Used *FindUsed(HNode *user) const {
        for (Used *u = DCHECK_NOTNULL(used_)->next_; u != used_; u = u->next_) {
            if (u->user == user) {
                return u;
            }
        }
        return nullptr;
    }

    void Unused(HNode *user) {
        Used *u = DCHECK_NOTNULL(FindUsed(user));
        QUEUE_REMOVE(u);
    }
    
    int UsedCount() {
        int n = 0;
        UsedIterator iter(this);
        for (iter.SeekToFirst(); iter.Valid(); iter.Next())  {
            n++;
        }
        return n;
    }
    
    class UsedIterator {
    public:
        UsedIterator(HNode *owns): owns_(owns) {}
        void SeekToFirst() { used_ = !owns_->used_ ? nullptr : owns_->used_->next_; }
        void SeekToLast() { used_ = !owns_->used_ ? nullptr : owns_->used_->prev_; }
        void Next() {
            DCHECK(Valid());
            used_ = used_->next_;
        }
        void Prev() {
            DCHECK(Valid());
            used_ = used_->prev_;
        }
        void Remove() {
            DCHECK(Valid());
            QUEUE_REMOVE(used_);
        }
        bool Valid() const { return used_ && used_ != owns_->used_; }
        HNode *operator *() { return user(); }
        HNode *operator -> () { return user(); }
        HNode *user() const {
            DCHECK(Valid());
            return used_->user;
        }
    private:
        HNode *const owns_;
        Used *used_ = nullptr;
    }; // class UsedIterator
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(HNode);
private:
    HNode(base::Arena *arena, int vid, int hint, HType type, const HOperator *op,
          int inputs_capacity, int inputs_size, HNode **inputs);

    const HOperator *op_;
    int inputs_capacity_;
    int inputs_size_;
    Used *used_ = nullptr;
    HNode *inputs_[0];
}; // class HNode


class HGraph final : public base::ArenaObject {
public:
    HGraph(base::Arena *arena): arena_(arena) {}
    
    DEF_PTR_GETTER(base::Arena, arena);
    DEF_PTR_PROP_RW(HNode, start);
    DEF_PTR_PROP_RW(HNode, end);
    DEF_VAL_GETTER(int, next_node_id);
    
    int NextNodeId() { return next_node_id_++; }

private:
    base::Arena *const arena_;
    HNode *start_ = nullptr;
    HNode *end_ = nullptr;
    int next_node_id_ = 0;
}; // class HGraph


template<class T>
/*static*/ inline T HOperatorWith<T>::Data(const HNode *node) {
    return Data(DCHECK_NOTNULL(node)->op());
}

// Export functions:
HGraph *GenerateHIRGraph(const CompilationInfo *compilation_info, base::Arena *arena);

} // namespace lang

} // namespace mai

#endif // MAI_LANG_HIR_H_
