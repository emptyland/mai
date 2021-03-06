#ifndef MAI_LANG_HIR_H_
#define MAI_LANG_HIR_H_

#include "base/arena-utils.h"
#include "base/arenas.h"
#include "base/queue-macros.h"
#include "base/base.h"
#include <type_traits>

namespace mai {

namespace lang {

class Class;
class CompilationInfo;
class HOperatorFactory;
class HOperator;
class HValue;
class HNode;
class Used;

struct OperatorOps;
struct NodeOps;

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
    DECLARE_HIR_CONTROL(V) \
    DECLARE_HIR_INNER(V) \
    DECLARE_HIR_CONSTANT(V) \
    DECLARE_HIR_LOAD_STORE_FIELD(V) \
    DECLARE_HIR_LOAD_STORE_ARRAY_AT(V) \
    DECLARE_HIR_BINARY_ARITHMETIC(V) \
    DECLARE_HIR_COMPAROR(V) \
    DECLARE_HIR_CALLING(V)

#define DECLARE_HIR_CONTROL(V) \
    V(Begin) \
    V(End) \
    V(Return) \
    V(Throw) \
    V(Loop) \
    V(LoopExit) \
    V(TailCall) \
    V(Branch) \
    V(IfTrue) \
    V(IfFalse) \
    V(Merge) \
    V(CheckStack) \
    V(Deoptimize) \
    V(DeoptimizeIf) \
    V(DeoptimizeUnless) \
    V(Terminate)

#define DECLARE_HIR_INNER(V) \
    V(FrameState) \
    V(Phi) \
    V(Guard) \
    V(Argument) \
    V(Parameter)

#define DECLARE_HIR_CONSTANT(V) \
    V(Word32Constant) \
    V(Word64Constant) \
    V(StringConstant) \
    V(Float32Constant) \
    V(Float64Constant)

#define DECLARE_HIR_BINARY_ARITHMETIC(V) \
    V(Word8Add) \
    V(Word16Add) \
    V(Word32Add) \
    V(Word64Add) \
    V(Float32Add) \
    V(Float64Add) \
    V(Word8Sub) \
    V(Word16Sub) \
    V(Word32Sub) \
    V(Word64Sub) \
    V(Float32Sub) \
    V(Float64Sub) \
    V(Int8Mul) \
    V(Int16Mul) \
    V(Int32Mul) \
    V(Int64Mul) \
    V(UInt8Mul) \
    V(UInt16Mul) \
    V(UInt32Mul) \
    V(UInt64Mul) \
    V(Float32Mul) \
    V(Float64Mul) \
    V(Int8Div) \
    V(Int16Div) \
    V(Int32Div) \
    V(Int64Div) \
    V(UInt8Div) \
    V(UInt16Div) \
    V(UInt32Div) \
    V(UInt64Div) \
    V(Float32Div) \
    V(Float64Div) \
    V(Word8Mod) \
    V(Word16Mod) \
    V(Word32Mod) \
    V(Word64Mod)

#define DECLARE_HIR_COMPAROR(V) \
    V(Word8Equal) \
    V(Word8NotEqual) \
    V(Word8LessThan) \
    V(Word8LessOrEqual) \
    V(Word8GreaterThan) \
    V(Word8GreaterOrEqual) \
    V(Word16Equal) \
    V(Word16NotEqual) \
    V(Word16LessThan) \
    V(Word16LessOrEqual) \
    V(Word16GreaterThan) \
    V(Word16GreaterOrEqual) \
    V(Word32Equal) \
    V(Word32NotEqual) \
    V(Word32LessThan) \
    V(Word32LessOrEqual) \
    V(Word32GreaterThan) \
    V(Word32GreaterOrEqual) \
    V(Word64Equal) \
    V(Word64NotEqual) \
    V(Word64LessThan) \
    V(Word64LessOrEqual) \
    V(Word64GreaterThan) \
    V(Word64GreaterOrEqual) \
    V(Float32Equal) \
    V(Float32NotEqual) \
    V(Float32LessThan) \
    V(Float32LessOrEqual) \
    V(Float32GreaterThan) \
    V(Float32GreaterOrEqual) \
    V(Float64Equal) \
    V(Float64NotEqual) \
    V(Float64LessThan) \
    V(Float64LessOrEqual) \
    V(Float64GreaterThan) \
    V(Float64GreaterOrEqual) \
    V(StringEQ) \
    V(StringNE) \
    V(StringLT) \
    V(StringLE) \
    V(StringGT) \
    V(StringGE)


#define DECLARE_HIR_LOAD_STORE_FIELD(V) \
    V(LoadField) \
    V(LoadWord8Field) \
    V(LoadWord16Field) \
    V(LoadWord32Field) \
    V(LoadWord64Field) \
    V(UncheckedLoadField) \
    V(UncheckedLoadWord8Field) \
    V(UncheckedLoadWord16Field) \
    V(UncheckedLoadWord32Field) \
    V(UncheckedLoadWord64Field) \
    V(StoreField) \
    V(StoreWord8Field) \
    V(StoreWord16Field) \
    V(StoreWord32Field) \
    V(StoreWord64Field) \
    V(UncheckedStoreField) \
    V(UncheckedStoreWord8Field) \
    V(UncheckedStoreWord16Field) \
    V(UncheckedStoreWord32Field) \
    V(UncheckedStoreWord64Field)

#define DECLARE_HIR_LOAD_STORE_ARRAY_AT(V) \
    V(LoadArrayAt) \
    V(LoadWord8ArrayAt) \
    V(LoadWord16ArrayAt) \
    V(LoadWord32ArrayAt) \
    V(LoadWord64ArrayAt) \
    V(UncheckedLoadArrayAt) \
    V(UncheckedLoadWord8ArrayAt) \
    V(UncheckedLoadWord16ArrayAt) \
    V(UncheckedLoadWord32ArrayAt) \
    V(UncheckedLoadWord64ArrayAt) \
    V(StoreArrayAt) \
    V(StoreWord8ArrayAt) \
    V(StoreWord16ArrayAt) \
    V(StoreWord32ArrayAt) \
    V(StoreWord64ArrayAt) \
    V(UncheckedStoreArrayAt) \
    V(UncheckedStoreWord8ArrayAt) \
    V(UncheckedStoreWord16ArrayAt) \
    V(UncheckedStoreWord32ArrayAt) \
    V(UncheckedStoreWord64ArrayAt)

#define DECLARE_HIR_CALLING(V) \
    V(CallInline) \
    V(CallBytecode) \
    V(CallVtab) \
    V(CallNative) \
    V(NewObject)

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
    
    DEF_VAL_GETTER(Value, value);
    DEF_VAL_GETTER(int, control_in);
    DEF_VAL_GETTER(int, effect_in);
    DEF_VAL_GETTER(int, value_in);
    DEF_VAL_GETTER(int, control_out);
    DEF_VAL_GETTER(int, effect_out);
    DEF_VAL_GETTER(int, value_out);
    DEF_VAL_GETTER(uint64_t, properties);
    
    const char *name() const { return kNames[value_]; }
    
    friend struct OperatorOps;
    friend class HOperatorFactory;
    DISALLOW_IMPLICIT_CONSTRUCTORS(HOperator);
protected:
    HOperator(Value value, uint64_t properties, int value_in, int effect_in, int control_in,
              int value_out, int effect_out, int control_out)
        : value_(value)
        , properties_(properties)
        , value_in_(value_in)
        , effect_in_(effect_in)
        , control_in_(control_in)
        , value_out_(value_out)
        , effect_out_(effect_out)
        , control_out_(control_out) {}

    Value value_;
    uint64_t properties_;

    int value_in_;
    int effect_in_;
    int control_in_;
    int value_out_;
    int effect_out_;
    int control_out_;
    
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
    inline HOperatorWith(Value value, uint64_t properties,
                         int value_in,  int effect_in,  int control_in,
                         int value_out, int effect_out, int control_out, T data)
    : HOperator(value, properties, value_in, effect_in, control_in,
                value_out, effect_out, control_out)
    , data_(data) {}
    
private:
    T data_;
}; // template<class T> class HOperatorWith


struct BranchHint final {
    enum Code {
        kNone,
        kLikely,
        kUnlikely,
    };
    
    static Code Of(const HOperator *op) { return static_cast<Code>(op->properties()); }
  
    DISALLOW_ALL_CONSTRUCTORS(BranchHint);
};


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
        return new (arena_) HOperator(HEnd, 0, value_in, 0, control_in, 0, 0, 0);
    }

    const HOperator *CheckStack() {
        return new (arena_) HOperator(HCheckStack, 0, 0, 0, 0, 0, 0, 0);
    }

    const HOperator *Branch(int control_in, int value_in, BranchHint::Code hint = BranchHint::kNone) {
        return new (arena_) HOperator(HBranch, hint, value_in, 0, control_in, 0, 0, 0);
    }
    
    const HOperator *IfTrue(int hint) {
        return new (arena_) HOperator(HIfTrue, hint, 0, 0, 1, 0, 0, 0);
    }
    
    const HOperator *IfFalse(int hint) {
        return new (arena_) HOperator(HIfFalse, hint, 0, 0, 1, 0, 0, 0);
    }
    
    const HOperator *Merge(int control_in) {
        return new (arena_) HOperator(HMerge, 0, 0, 0, control_in, 0, 0, 0);
    }

    const HOperator *Phi(int control_in, int value_in) {
        return new (arena_) HOperator(HPhi, 0, value_in, 0, control_in, 0, 0, 0);
    }
    
    const HOperator *Return(int control_in, int value_in) {
        return new (arena_) HOperator(HReturn, 0, value_in, 0, control_in, 0, 0, 0);
    }

    const HOperator *Loop() {
        return new (arena_) HOperator(HLoop, 0, 0, 0, 1, 0, 0, 0);
    }
    
    const HOperator *LoopExit(int control_in) {
        return new (arena_) HOperator(HLoopExit, 0, 0, 0, control_in, 0, 0, 0);
    }

    const HOperator *Guard(uint64_t hint) {
        return new (arena_) HOperator(HGuard, hint, 0, 0, 0, 0, 0, 0);
    }
    
    const HOperator *Parameter(int index) {
        return new (arena_) HOperatorWith<int>(HParameter, 0, 0, 0, 0, 0, 0, 0, index);
    }

    const HOperator *Word32Constant(uint32_t data) {
        return new (arena_) HOperatorWith<uint32_t>(HWord32Constant, 0, 0, 0, 0, 0, 0, 0, data);
    }
    
    const HOperator *Word64Constant(uint64_t data) {
        return new (arena_) HOperatorWith<uint64_t>(HWord64Constant, 0, 0, 0, 0, 0, 0, 0, data);
    }
    
    const HOperator *Float32Constant(float data) {
        return new (arena_) HOperatorWith<float>(HFloat32Constant, 0, 0, 0, 0, 0, 0, 0, data);
    }
    
    const HOperator *Float64Constant(double data) {
        return new (arena_) HOperatorWith<double>(HFloat64Constant, 0, 0, 0, 0, 0, 0, 0, data);
    }

    const HOperator *Argument(uint64_t hint) {
        return new (arena_) HOperator(HArgument, hint, 0, 0, 0, 0, 0, 1);
    }

    const HOperator *FrameState(int control_in, int value_in, int32_t bci, int32_t offset) {
        return new (arena_) HOperatorWith<struct FrameState>(HFrameState, 0,
                                                             value_in, 0, control_in, 0, 0, 1,
                                                             {bci, offset});
    }
    
    const HOperator *NewObject(int control_in, int value_in, const Class *clazz) {
        return new (arena_) HOperatorWith<const Class *>(HNewObject, 0, value_in, 0, control_in, 0,
                                                         0, 1, clazz);
    }

#define DEFINE_LOAD_STORE_FIELD(name) \
    const HOperator *name(int control_in, int value_in, int32_t offset) { \
        return new (arena_) HOperatorWith<int32_t>(H##name, 0, value_in, 0, control_in, 0, 0, 0, \
                                                   offset); \
    }
    DECLARE_HIR_LOAD_STORE_FIELD(DEFINE_LOAD_STORE_FIELD)
#undef DEFINE_LOAD_STORE_FIELD

#define DEFINE_LOAD_STORE_ARRAY_AT(name) \
    const HOperator *name(int control_in, int value_in) { \
        return new (arena_) HOperator(H##name, 0, value_in, 0, control_in, 0, 0, 0); \
    }
    DECLARE_HIR_LOAD_STORE_ARRAY_AT(DEFINE_LOAD_STORE_ARRAY_AT)
#undef DEFINE_LOAD_STORE_ARRAY_AT

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
    
    void ReplaceInput(base::Arena *arena, int i, HNode *that);
    
    void ReplaceUses(HNode *that);
    
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
        Use *operator *() { return use_; }
        Use *operator -> () { return use_; }
        HNode *user() const {
            DCHECK(Valid());
            return use_->user;
        }
        HNode *from() const { return user(); }
        int index() const { return use_->input_index; }
        HNode *to() const { return use_->user->input(use_->input_index); }
        
        void set_from(HNode *new_from) { use_->user = new_from; }
        void set_to(HNode *new_to) { use_->user->inputs()[use_->input_index] = new_to; }
        
        bool UpdateTo(HNode *new_to);
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


struct OperatorOps {
    
    static bool IsNilChecked(const HOperator *op);
    
    static void UpdateControlIn(const HOperator *op, int value) {
        const_cast<HOperator *>(op)->control_in_ = value;
    }

    static void UpdateEffectIn(const HOperator *op, int value) {
        const_cast<HOperator *>(op)->effect_in_ = value;
    }

    static void UpdateValueIn(const HOperator *op, int value) {
        const_cast<HOperator *>(op)->value_in_ = value;
    }

    static void UpdateValue(const HOperator *op, HOperator::Value code) {
        const_cast<HOperator *>(op)->value_ = code;
    }
    
    DISALLOW_ALL_CONSTRUCTORS(OperatorOps);
}; // struct OperatorOps


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
    
    static int GetValueIndex(const HNode *node) { return 0; }
    
    static int GetEffectIndex(const HNode *node) {
        return GetValueIndex(node) + node->op()->value_in();
    }
    
    static int GetControlIndex(const HNode *node) {
        return GetEffectIndex(node) + node->op()->effect_in();
    }
    
    static bool IsControlEdge(const HNode::UseIterator &edge) {
        HNode *user = edge.user();
        if (user->op()->control_in() == 0) {
            return false;
        }
        int max = GetControlIndex(user) + user->op()->control_in();
        return edge.index() >= GetControlIndex(user) && edge.index() < max;
    }
    
    static void CollectControlProjections(HNode *node, HNode **projections, size_t count);
    
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
