#ifndef MAI_LANG_HIR_H_
#define MAI_LANG_HIR_H_

#include "base/arena-utils.h"
#include "base/arenas.h"
#include "base/queue-macros.h"
#include "base/base.h"
#include <type_traits>

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
    V(Begin) \
    V(End) \
    V(Return) \
    V(Loop) \
    V(LoopExit) \
    V(Branch) \
    V(IfTrue) \
    V(IfFalse) \
    V(Merge) \
    V(FrameState) \
    V(Phi) \
    V(Guard) \
    V(Argument) \
    V(Parameter) \
    V(CheckStack) \
    DECLARE_HIR_CONSTANT(V) \
    DECLARE_HIR_LOAD_STORE(V) \
    DECLARE_HIR_BINARY_ARITHMETIC(V) \
    DECLARE_HIR_COMPAROR(V) \
    DECLARE_HIR_CALLING(V)

#define DECLARE_HIR_CONSTANT(V) \
    V(Constant32) \
    V(Constant64) \
    V(ConstantString) \
    V(FConstant32) \
    V(FConstant64)

#define DECLARE_HIR_BINARY_ARITHMETIC(V) \
    V(Add8) \
    V(Add16) \
    V(Add32) \
    V(Add64) \
    V(FAdd32) \
    V(FAdd64) \
    V(Sub8) \
    V(Sub16) \
    V(Sub32) \
    V(Sub64) \
    V(FSub32) \
    V(FSub64) \
    V(Mul8) \
    V(Mul16) \
    V(Mul32) \
    V(Mul64) \
    V(IMul8) \
    V(IMul16) \
    V(IMul32) \
    V(IMul64) \
    V(FMul32) \
    V(FMul64) \
    V(Div8) \
    V(Div16) \
    V(Div32) \
    V(Div64) \
    V(IDiv8) \
    V(IDiv16) \
    V(IDiv32) \
    V(IDiv64) \
    V(FDiv32) \
    V(FDiv64) \
    V(Mod8) \
    V(Mod16) \
    V(Mod32) \
    V(Mod64)

#define DECLARE_HIR_COMPAROR(V) \
    V(Equal8) \
    V(NotEqual8) \
    V(LessThan8) \
    V(LessOrEqual8) \
    V(GreaterThan8) \
    V(GreaterOrEqual8) \
    V(Equal16) \
    V(NotEqual16) \
    V(LessThan16) \
    V(LessOrEqual16) \
    V(GreaterThan16) \
    V(GreaterOrEqual16) \
    V(Equal32) \
    V(NotEqual32) \
    V(LessThan32) \
    V(LessOrEqual32) \
    V(GreaterThan32) \
    V(GreaterOrEqual32) \
    V(Equal64) \
    V(NotEqual64) \
    V(LessThan64) \
    V(LessOrEqual64) \
    V(GreaterThan64) \
    V(GreaterOrEqual64) \
    V(FEqual32) \
    V(FNotEqual32) \
    V(FLessThan32) \
    V(FLessOrEqual32) \
    V(FGreaterThan32) \
    V(FGreaterOrEqual32) \
    V(FEqual64) \
    V(FNotEqual64) \
    V(FLessThan64) \
    V(FLessOrEqual64) \
    V(FGreaterThan64) \
    V(FGreaterOrEqual64) \
    V(StringEQ) \
    V(StringNE) \
    V(StringLT) \
    V(StringLE) \
    V(StringGT) \
    V(StringGE) \

#define DECLARE_HIR_LOAD_STORE(V) \
    V(LoadField) \
    V(LoadField8) \
    V(LoadField16) \
    V(LoadField32) \
    V(LoadField64) \
    V(StoreField) \
    V(StoreField8) \
    V(StoreField16) \
    V(StoreField32) \
    V(StoreField64)

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

// graphviz for Graph output
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
    HOperator(Value value, uint64_t properties, int control_in, int value_in, int effect_in,
              int control_out, int value_out, int effect_out)
        : value_(value)
        , properties_(properties)
        , control_in_(control_in)
        , value_in_(value_in)
        , effect_in_(effect_in)
        , control_out_(control_out)
        , value_out_(value_out)
        , effect_out_(effect_out) {}

    Value value_;
    uint64_t properties_;
    int control_in_;
    int value_in_;
    int effect_in_;
    int control_out_;
    int value_out_;
    int effect_out_;
    
    static const char *kNames[HMaxOpcode];
}; // class HOperator


struct FrameState {
    int32_t  bci;
    int32_t  offset;
}; // struct FrameState

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
    inline HOperatorWith(Value value, uint64_t properties, int control_in, int value_in,
                         int effect_in, int control_out, int value_out, int effect_out, T data)
    : HOperator(value, properties, control_in, value_in, effect_in,
                control_out, value_out, effect_out)
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
    
    const HOperator *Begin() {
        return new (arena_) HOperator(HBegin, 0, 0, 0, 0, 0, 0, 0);
    }

    const HOperator *End(int control_in, int value_in) {
        return new (arena_) HOperator(HEnd, 0, control_in, value_in, 0, 0, 0, 0);
    }

    const HOperator *CheckStack() {
        return new (arena_) HOperator(HCheckStack, 0, 0, 0, 0, 0, 0, 0);
    }

    const HOperator *Branch(int control_in, int value_in) {
        return new (arena_) HOperator(HBranch, 0, control_in, value_in, 0, 0, 0, 0);
    }
    
    const HOperator *IfTrue(int hint) {
        return new (arena_) HOperator(HIfTrue, hint, 1, 0, 0, 0, 0, 0);
    }
    
    const HOperator *IfFalse(int hint) {
        return new (arena_) HOperator(HIfFalse, hint, 1, 0, 0, 0, 0, 0);
    }
    
    const HOperator *Merge(int control_in) {
        return new (arena_) HOperator(HMerge, 0, control_in, 0, 0, 0, 0, 0);
    }

    const HOperator *Phi(int control_in, int value_in) {
        return new (arena_) HOperator(HPhi, 0, control_in, value_in, 0, 0, 0, 0);
    }
    
    const HOperator *Return(int control_in, int value_in) {
        return new (arena_) HOperator(HReturn, 0, control_in, value_in, 0, 0, 0, 0);
    }
    
    const HOperator *Loop() {
        return new (arena_) HOperator(HLoop, 0, 1, 0, 0, 0, 0, 0);
    }
    
    const HOperator *LoopExit(int control_in) {
        return new (arena_) HOperator(HLoopExit, 0, control_in, 0, 0, 0, 0, 0);
    }

    const HOperator *Guard(uint64_t hint) {
        return new (arena_) HOperator(HGuard, hint, 0, 0, 0, 0, 0, 0);
    }
    
    const HOperator *Parameter(int index) {
        return new (arena_) HOperatorWith<int>(HParameter, 0, 0, 0, 0, 0, 0, 0, index);
    }

    const HOperator *Constant32(uint32_t data) {
        return new (arena_) HOperatorWith<uint32_t>(HConstant32, 0, 0, 0, 0, 0, 0, 0, data);
    }
    
    const HOperator *Constant64(uint64_t data) {
        return new (arena_) HOperatorWith<uint64_t>(HConstant64, 0, 0, 0, 0, 0, 0, 0, data);
    }
    
    const HOperator *FConstant32(float data) {
        return new (arena_) HOperatorWith<float>(HFConstant32, 0, 0, 0, 0, 0, 0, 0, data);
    }
    
    const HOperator *FConstant64(double data) {
        return new (arena_) HOperatorWith<double>(HFConstant64, 0, 0, 0, 0, 0, 0, 0, data);
    }

    const HOperator *Argument(uint64_t hint) {
        return new (arena_) HOperator(HArgument, hint, 0, 0, 0, 0, 0, 1);
    }

    const HOperator *FrameState(int control_in, int value_in, int32_t bci, int32_t offset) {
        return new (arena_) HOperatorWith<struct FrameState>(HFrameState, 0, control_in,
                                                             value_in, 0, 0, 1, 0,
                                                             {bci, offset});
    }

#define DEFINE_LOAD_STORE_FIELD(name) \
    const HOperator *name(int control_in, int value_in, int32_t offset) { \
        return new (arena_) HOperatorWith<int32_t>(H##name, 0, control_in, value_in, 0, 0, 1, 0, \
                                                   offset); \
    }
    DECLARE_HIR_LOAD_STORE(DEFINE_LOAD_STORE_FIELD)
#undef DEFINE_LOAD_STORE_FIELD
    

#define DEFINE_CACHED_OP(name) const HOperator *name() { return cache_[H##name]; }
    DECLARE_HIR_BINARY_ARITHMETIC(DEFINE_CACHED_OP)
    DECLARE_HIR_COMPAROR(DEFINE_CACHED_OP);
#undef DEFINE_CACHED_OP

    DISALLOW_IMPLICIT_CONSTRUCTORS(HOperatorFactory);
private:
    void Initialize();
    
    base::Arena *arena_;
    const HOperator *cache_[HMaxOpcode];
}; // class HOperatorFactory

class HValue {
public:
    DEF_VAL_GETTER(int, vid);
    DEF_VAL_GETTER(HType, type);
    DEF_VAL_PROP_RW(uint32_t, mark);

    DISALLOW_IMPLICIT_CONSTRUCTORS(HValue);
protected:
    HValue(int vid, HType type)
        : vid_(vid)
        , type_(type) {}

    int vid_;
    HType type_;
    uint32_t mark_ = 0;
}; // class HValue


class HNode : public HValue {
public:
    struct Use : public base::ArenaObject {
        Use *next_;
        Use *prev_;
        HNode *user;
        int input_index;
    }; // struct Used

    static inline HNode *New(base::Arena *arena, int vid, HType type, const HOperator *op,
                             int inputs_capacity, int inputs_size, HNode **inputs) {
        DCHECK_GE(inputs_size, 0);
        DCHECK_GE(inputs_capacity, inputs_size);
        size_t request_size = sizeof(HNode) + sizeof(*inputs) * inputs_capacity;
        void *chunk = arena->Allocate(request_size);
        return new (chunk) HNode(arena, vid, type, op, inputs_capacity, inputs_size, inputs);
    }
    
    static HNode *Clone(base::Arena *arena, HNode *src, int inputs_capacity) {
        for (int i = 0; i < src->inputs_size_; i++) {
            src->input(i)->RemoveUser(src);
        }
        HNode *dest = New(arena, src->vid(), src->type(), src->op(), inputs_capacity,
                          src->inputs_size_, src->inputs());
        dest->use_ = src->use_;
        return dest;
    }
    
    bool IsDead() const { return inputs_size() > 0 && inputs()[0] == nullptr; }
    
    void Kill() { ::memset(inputs(), 0, sizeof(*inputs()) * inputs_size()); }

    DEF_PTR_GETTER(const HOperator, op);
    HOperator::Value opcode() const { return op_->value(); }
    
    View<HNode*> inputs_view() const { return {inputs(), static_cast<size_t>(inputs_size())}; }
    
    HNode **inputs() {
        return is_inline_inputs_ ? inline_inputs_ : out_of_line_inputs_->inputs_;
    }
    
    HNode *const * inputs() const {
        return is_inline_inputs_ ? inline_inputs_ : out_of_line_inputs_->inputs_;
    }

    bool is_inline_inputs() const { return is_inline_inputs_; }

    int inputs_size() const {
        return is_inline_inputs_ ? inputs_size_ : out_of_line_inputs_->inputs_size_;
    }
    
    int inputs_capacity() const {
        return is_inline_inputs_ ? inputs_capacity_ : out_of_line_inputs_->inputs_capacity_;
    }

    HNode *input(int i) const {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, inputs_size());
        return DCHECK_NOTNULL(inputs()[i]);
    }
    
    HNode *lhs() const {
        DCHECK_EQ(inputs_size(), 2);
        return input(0);
    }
    
    HNode *rhs() const {
        DCHECK_EQ(inputs_size(), 2);
        return input(1);
    }

    void AppendInput(base::Arena *arena, HNode *node);
    
    void ReplaceInput(base::Arena *arena, int i, HNode *node) {
        HNode *old = input(i);
        if (old == node) {
            return;
        }
        Use *use = DCHECK_NOTNULL(old->FindUse(this));
        old->RemoveUse(use);
        inputs()[i] = node;
        DCHECK_EQ(i, use->input_index);
        DCHECK_EQ(this, use->user);
        node->AppendUse(use);
    }
    
    void InsertInput(base::Arena *arena, int after, HNode *node);
    
    // TODO:
    bool Verify() { return true; }
    
    void AppendUse(Use *use) { QUEUE_INSERT_TAIL(&use_, use); }
    
    void RemoveUse(Use *use) {
        use = DCHECK_NOTNULL(FindUse(use));
        QUEUE_REMOVE(use);
    }

    Use *AppendUser(base::Arena *arena, int input_index, HNode *used);
    
    void RemoveUser(HNode *user) {
        Use *u = DCHECK_NOTNULL(FindUse(user));
        QUEUE_REMOVE(u);
    }
    
    Use *FindUse(HNode *user) const {
        for (Use *u = use_.next_; u != &use_; u = u->next_) {
            if (u->user == user) {
                return u;
            }
        }
        return nullptr;
    }
    
    Use *FindUse(Use *use) const {
        for (Use *u = use_.next_; u != &use_; u = u->next_) {
            if (u == use) {
                return u;
            }
        }
        return nullptr;
    }

    int UseCount() {
        int n = 0;
        UseIterator iter(this);
        for (iter.SeekToFirst(); iter.Valid(); iter.Next())  {
            n++;
        }
        return n;
    }
    
    class UseIterator {
    public:
        UseIterator(HNode *owns): owns_(owns) {}
        void SeekToFirst() { use_ = owns_->use_.next_; }
        void SeekToLast() { use_ = owns_->use_.prev_; }
        void Next() {
            DCHECK(Valid());
            use_ = use_->next_;
        }
        void Prev() {
            DCHECK(Valid());
            use_ = use_->prev_;
        }
        void Remove() {
            DCHECK(Valid());
            QUEUE_REMOVE(use_);
        }
        bool Valid() const { return use_ && use_ != &owns_->use_; }
        HNode *operator *() { return user(); }
        HNode *operator -> () { return user(); }
        HNode *user() const {
            DCHECK(Valid());
            return use_->user;
        }
        HNode *from() const { return user(); }
        HNode *to() const { return use_->user->input(use_->input_index); }
        
        void set_to(HNode *new_to) { use_->user->inputs()[use_->input_index] = new_to; }
        
        bool UpdateTo(HNode *new_to) {
            DCHECK(Valid());
            HNode *old_to = to();
            if (old_to != new_to) {
                Use *next = use_->next_;
                if (old_to) {
                    old_to->RemoveUse(use_);
                }
                set_to(new_to);
                if (new_to) {
                    new_to->AppendUse(use_);
                }
                use_ = next;
            }
            return old_to != new_to;
        }
    private:
        HNode *const owns_;
        Use *use_ = nullptr;
    }; // class UsedIterator
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(HNode);
private:
    struct OutOfLineInputs {
        HNode *owns_;
        int inputs_capacity_;
        int inputs_size_;
        HNode *inputs_[0];
        
        static OutOfLineInputs *New(base::Arena *arena, HNode *owns, uint32_t inputs_capacity,
                                    uint32_t inputs_size, HNode **inputs) {
            size_t request_size = sizeof(OutOfLineInputs) + sizeof(HNode *) * inputs_capacity;
            OutOfLineInputs *ool = static_cast<OutOfLineInputs *>(arena->Allocate(request_size));
            ool->owns_ = owns;
            ool->inputs_capacity_ = inputs_capacity;
            ool->inputs_size_ = inputs_size;
            ::memcpy(ool->inputs_, inputs, sizeof(HNode *) * inputs_size);
            return ool;
        }
    };
    
    HNode(base::Arena *arena, int vid, HType type, const HOperator *op, uint32_t inputs_capacity,
          uint32_t inputs_size, HNode **inputs);

    const HOperator *op_;
    uint32_t is_inline_inputs_ : 1;
    uint32_t padding0_         : 3;
    uint32_t inputs_capacity_  : 14;
    uint32_t inputs_size_      : 14;
    Use use_;
    union {
        OutOfLineInputs *out_of_line_inputs_;
        HNode *inline_inputs_[1];
    }; // Tail
}; // class HNode


class HGraphDecorator {
public:
    HGraphDecorator() = default;
    virtual ~HGraphDecorator() = default;
    
    virtual void Decorate(HNode *node) = 0;

    DISALLOW_IMPLICIT_CONSTRUCTORS(HGraphDecorator);
}; // class HGraphDecorator

class HGraph final : public base::ArenaObject {
public:
    HGraph(base::Arena *arena)
        : arena_(arena)
        , decorators_(arena) {}

    DEF_PTR_GETTER(base::Arena, arena);
    DEF_PTR_PROP_RW(HNode, start);
    DEF_PTR_PROP_RW(HNode, end);
    DEF_VAL_GETTER(int, next_node_id);

    int node_count() const { return next_node_id_; }
    
    HNode *NewNode(const HOperator *op) {
        return NewNodeWithInputs(op, HTypes::Void, 0, nullptr);
    }
    
    HNode *NewNode(const HOperator *op, HType type) {
        return NewNodeWithInputs(op, type, 0, nullptr);
    }

    template<class ...Nodes>
    inline HNode *NewNode(const HOperator *op, HType type, Nodes... nodes) {
        HNode *inputs[] = {CheckNode(nodes)...};
        return NewNodeWithInputs(op, type, arraysize(inputs), inputs);
    }
    
    HNode *NewNodeWithInputs(const HOperator *op, HType type, int size, HNode **inputs) {
        HNode *node = HNode::New(arena_, NextNodeId(), type, op, size, size, inputs);
        Decorate(node);
        return node;
    }
    
    HNode *NewNodeReserved(const HOperator *op, HType type, int inputs_capacity) {
        HNode *node = HNode::New(arena_, NextNodeId(), type, op, inputs_capacity, 0, nullptr);
        Decorate(node);
        return node;
    }

    int NextNodeId() { return next_node_id_++; }

    void AddDecorator(HGraphDecorator *decorator) { decorators_.push_back(decorator); }
private:
    HNode *CheckNode(HNode *node) { return DCHECK_NOTNULL(node); }
    
    void Decorate(HNode *node) {
        for (HGraphDecorator *decorator : decorators_) {
            decorator->Decorate(node);
        }
    }
    
    base::Arena *const arena_;
    HNode *start_ = nullptr;
    HNode *end_ = nullptr;
    base::ArenaVector<HGraphDecorator *> decorators_;
    int next_node_id_ = 0;
}; // class HGraph


// 0, 4
template<class T, int M, int N>
class HNodeState final {
public:
    static constexpr uint32_t kMask = ((1u << N) - 1) << M;
    
    static_assert(M < N && M < 32 && N < 32, "incorrect M and N");
    
    inline HNodeState() = default;
    
    inline void Set(HNode *node, T state) const {
        node->set_mark((node->mark() & ~kMask) | (static_cast<uint32_t>(state) << M));
    }
    
    inline T Get(const HNode *node) const { return static_cast<T>(GetU32(node)); }
    
    inline uint32_t GetU32(const HNode *node) const { return (node->mark() & kMask) >> M; }
}; // class HNodeState


struct NodeOps {
    
    static bool IsConstant(const HNode *node);

    static HNode *GetControlInput(const HNode *node, int index) {
        DCHECK_GE(index, 0);
        DCHECK_LT(index, node->op()->control_in());
        return node->input(GetControlIndex(node) + index);
    }
    
    static HNode *GetValueInput(const HNode *node, int index) {
        DCHECK_GE(index, 0);
        DCHECK_LT(index, node->op()->value_in());
        return node->input(GetValueIndex(node) + index);
    }
    
    static HNode *GetEffectInput(const HNode *node, int index) {
        DCHECK_GE(index, 0);
        DCHECK_LT(index, node->op()->effect_in());
        return node->input(GetEffectIndex(node) + index);
    }
    
    static int GetControlIndex(const HNode *node) { return 0; }
    
    static int GetValueIndex(const HNode *node) { return node->op()->control_in(); }
    
    static int GetEffectIndex(const HNode *node) {
        return GetValueIndex(node) + node->op()->value_in();
    }
    
    DISALLOW_ALL_CONSTRUCTORS(NodeOps);
};

template<class T>
/*static*/ inline T HOperatorWith<T>::Data(const HNode *node) {
    return Data(DCHECK_NOTNULL(node)->op());
}

// Export functions:
HGraph *GenerateHIRGraph(const CompilationInfo *compilation_info, base::Arena *arena);

} // namespace lang

} // namespace mai

#endif // MAI_LANG_HIR_H_
