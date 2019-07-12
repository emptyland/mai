#ifndef MAI_NYAA_HIGH_LEVEL_IR_H_
#define MAI_NYAA_HIGH_LEVEL_IR_H_

#include "base/arena-utils.h"
#include "base/arena.h"
#include "base/slice.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {
class NyMap;
class Object;
namespace hir {
    
#define DECL_DAG_NODES(V) \
    V(Parameter) \
    V(LoadUp) \
    V(StoreUp) \
    V(LoadGlobal) \
    V(StoreGlobal) \
    V(BaseOfStack) \
    V(Constant) \
    V(Inbox) \
    V(IntToLong) \
    V(LongToInt) \
    V(IntToFloat) \
    V(FloatToInt) \
    V(LongToFloat) \
    V(FloatToLong) \
    V(IMinus) \
    V(IAdd) \
    V(ISub) \
    V(Branch) \
    V(NoCondBranch) \
    V(Phi) \
    V(Ret)
    
#define DECL_DAG_TYPES(V) \
    V(Void,   "void") \
    V(Int,    "int") \
    V(Long,   "long") \
    V(Float,  "float") \
    V(String, "string") \
    V(Array,  "array") \
    V(Map,    "map") \
    V(Object, "object")
    
struct Type {
    enum ID {
    #define DEFINE_ENUM(name, literal) k##name,
        DECL_DAG_TYPES(DEFINE_ENUM)
    #undef DEFINE_ENUM
        kMaxTypes,
    };
    
    static const char *kNames[];
}; // struct Type
    
class Value;
class Function;
class BasicBlock;
class Unary;
class Binary;
#define DEFINE_DECL(name) class name;
    DECL_DAG_NODES(DEFINE_DECL)
#undef DEFINE_DECL
    
using String = base::ArenaString;
    
struct UpvalDesc {
    Type::ID type_hint;
    int slot;
};
    
// https://www.oracle.com/technetwork/java/javase/tech/c2-ir95-150110.pdf
class HIRNode {
public:
    void *operator new (size_t size, base::Arena *arena) {
        return arena->Allocate(size);
    }
    
    void operator delete (void *p) = delete;
};
    
class Function : public HIRNode {
public:
    using ValueSet = base::ArenaVector<Value *>;
    using BasicBlockSet = base::ArenaVector<BasicBlock *>;
    
    DEF_PTR_GETTER(base::Arena, arena);
    
    inline BasicBlock *NewBB(BasicBlock *in_edge);
    inline Value *Parameter(Type::ID type, int line);
    inline Value *LoadUp(BasicBlock *bb, Type::ID type, int slot, int line);
    inline Value *StoreUp(BasicBlock *bb, int slot, Value *val, int line);
    inline Value *LoadGlobal(BasicBlock *bb, Type::ID type, const String *name, int line);
    inline Value *StoreGlobal(BasicBlock *bb, const String *name, Value *val, int line);
    inline Value *Inbox(BasicBlock *bb, Value *from, int line);
    inline Value *IntToLong(BasicBlock *bb, Value *from, int line);
    inline Value *IntToFloat(BasicBlock *bb, Value *from, int line);
    inline Value *LongToInt(BasicBlock *bb, Value *from, int line);
    inline Value *LongToFloat(BasicBlock *bb, Value *from, int line);
    inline Value *FloatToInt(BasicBlock *bb, Value *from, int line);
    inline Value *FloatToLong(BasicBlock *bb, Value *from, int line);
    inline Value *BaseOfStack(BasicBlock *bb, Value *base, int offset, int line);
    inline Constant *Constant(Type::ID type, int line);
    inline Value *Nil(int line);
    inline Value *IMinus(BasicBlock *bb, Value *operand, int line);
    inline Value *IAdd(BasicBlock *bb, Value *lhs, Value *rhs, int line);
    inline Value *ISub(BasicBlock *bb, Value *lhs, Value *rhs, int line);
    inline Branch *Branch(BasicBlock *bb, Value *cond, int line);
    inline Value *NoCondBranch(BasicBlock *bb, BasicBlock *target, int line);
    inline Phi *Phi(BasicBlock *bb, Type::ID type, int line);
    inline Ret *Ret(BasicBlock *bb, int line);

    void PrintTo(std::string *buf, size_t limit) const;
    inline void PrintTo(FILE *fp) const;

    static Function *New(base::Arena *arena) { return new (arena) Function(arena); }
    
    friend class Value;
    friend class BasicBlock;
private:
    Function(base::Arena *arena)
        : arena_(arena)
        , basic_blocks_(arena) {}
    
    int NextValId() { return next_val_id_++; }
    int NextBBId() { return next_bb_id_++; }

    void AddBasicBlock(BasicBlock *bb) { basic_blocks_.push_back(DCHECK_NOTNULL(bb)); }

    base::Arena *arena_;
    BasicBlockSet basic_blocks_;
    BasicBlock *entry_ = nullptr;
    int next_bb_id_ = 0;
    int next_val_id_ = 0;
}; // class Function
    
class BasicBlock : public HIRNode {
public:
    using EdgeSet = base::ArenaVector<BasicBlock *>;
    
    DEF_VAL_GETTER(int, label);
    DEF_PTR_GETTER(Value, root);
    DEF_PTR_GETTER(Value, last);
    DEF_VAL_PROP_RMW(EdgeSet, in_edges);
    DEF_VAL_PROP_RMW(EdgeSet, out_edges);
    
    inline void AddInEdge(BasicBlock *bb) {
        in_edges_.push_back(bb);
        bb->out_edges_.push_back(this);
    }
    
    void PrintTo(FILE *fp) const;
    
    friend class Value;
    friend class Function;
private:
    BasicBlock(Function *top, int label, base::Arena *arena)
        : label_(label)
        , in_edges_(arena)
        , out_edges_(arena) { top->AddBasicBlock(this); }
    
    inline void AddValue(Value *v);

    int label_;
    EdgeSet in_edges_;
    EdgeSet out_edges_;
    Value *root_ = nullptr;
    Value *last_ = nullptr;
}; // class BasicBlock
    
struct Use : public HIRNode {
public:
    Use(Value *v) : val(v) {}
    
    void RemoveFromList() {
        prev->next = next;
        next->prev = prev;
    }
    
    void AddToList(Use *list) {
        next = list;
        prev = list->prev;
        list->prev->next = this;
        list->prev = this;
    }

    Value *val;
    Use *next;
    Use *prev;
}; // class Use

class Value : public HIRNode {
public:
    enum Kind {
    #define DEFINE_ENUM(name) k##name,
        DECL_DAG_NODES(DEFINE_ENUM)
    #undef DEFINE_ENUM
        kMaxInsts,
    };

    DEF_VAL_GETTER(Type::ID, type);
    DEF_VAL_GETTER(int, index);
    DEF_VAL_GETTER(int, line);
    DEF_PTR_PROP_RW(Value, next);
    
#define DEFINE_IS(name, literal) bool Is##name() const { return type_ == Type::k##name; }
    DECL_DAG_TYPES(DEFINE_IS)
#undef DEFINE_IS
    
    virtual Kind kind() const = 0;
    virtual void PrintOperator(FILE *fp) const = 0;
    virtual Type::ID hint(int i) { DLOG(FATAL) << "No implement"; return Type::kVoid; }

    void PrintTo(FILE *fp) const {
        PrintValue(fp);
        if (!IsVoid()) {
            fprintf(fp, " = ");
        }
        PrintOperator(fp);
    }
    
    void PrintValue(FILE *fp) const {
        if (!IsVoid()) {
            if (kind() == kConstant) {
                PrintOperator(fp);
            } else {
                fprintf(fp, "%s %%v%d", Type::kNames[type_], index_);
            }
        }
    }

    friend class Function;
protected:
    Value(Function *top, BasicBlock *bb, Type::ID type, int line)
        : type_(type)
        , index_(type != Type::kVoid ? top->NextValId() : 0)
        , line_(line)
        , use_dummy_(nullptr) {
            use_dummy_.next = &use_dummy_;
            use_dummy_.prev = &use_dummy_;
            if (bb) {
                bb->AddValue(this);
            }
        }
    
    // pure constant value
    Value(Function *top, Type::ID type, int line)
        : type_(type)
        , index_(0)
        , line_(line)
        , use_dummy_(nullptr) {
        use_dummy_.next = &use_dummy_;
        use_dummy_.prev = &use_dummy_;
    }
    
    void Set(Value *v, base::Arena *arena) {
        if (v) {
            v->AddUse(new (arena) Use(this));
        }
    }
    
    void AddUse(Use *use) { use->AddToList(&use_dummy_); }
    
    Type::ID type_;
    int index_;
    int line_;
    Use use_dummy_;
    Value *next_ = nullptr;
};
    
#define DEFINE_DAG_INST_NODE(name) \
    virtual Kind kind() const override { return k##name; } \
    static name *Cast(Value *node) { \
        return !node || node->kind() != k##name ? nullptr : reinterpret_cast<name *>(node); \
    } \
    static const name *Cast(const Value *node) { \
        return !node || node->kind() != k##name ? nullptr : reinterpret_cast<const name *>(node); \
    } \
    friend class Function; \
    DISALLOW_IMPLICIT_CONSTRUCTORS(name)
    
class Constant : public Value {
public:
    int64_t int_val() const { DCHECK(IsInt()); return smi_; }
    void set_int_val(int64_t val) { DCHECK(IsInt()); smi_ = val; }
    
    double float_val() const { DCHECK(IsFloat()); return f64_; }
    void set_float_val(double val) { DCHECK(IsFloat()); f64_ = val; }
    
    const String *string_val() const { DCHECK(IsString()); return str_; }
    void set_string_val(const String *val) { DCHECK(IsString()); str_ = val; }
    
    NyMap *map_val() const { DCHECK(IsMap()); return map_; }
    void set_map_val(NyMap *val) { DCHECK(IsMap()); map_ = val; }
    
    bool is_nil() const { return nil_stub_ == nullptr; }
    void set_nil() { nil_stub_ = nullptr; }
    
    virtual void PrintOperator(FILE *fp) const override;

    DEFINE_DAG_INST_NODE(Constant);
private:
    Constant(Function *top, Type::ID type, int line)
        : Value(top, type, line) {}
    
    union {
        int64_t smi_;
        double f64_;
        const String *str_;
        NyMap *map_;
        Object *nil_stub_;
    };
}; // class Constant
    
class Parameter : public Value {
public:
    virtual void PrintOperator(FILE *fp) const override { ::fprintf(fp, "parameter"); }
    
    DEFINE_DAG_INST_NODE(Parameter);
private:
    Parameter(Function *top, Type::ID type, int line)
        : Value(top, nullptr, type, line) {}
}; // class Argument
    
class LoadUp : public Value {
public:
    DEF_VAL_GETTER(int, slot);

    virtual void PrintOperator(FILE *fp) const override {
        ::fprintf(fp, "upval $%d", slot_);
    }
    
    DEFINE_DAG_INST_NODE(LoadUp);
private:
    LoadUp(Function *top, BasicBlock *bb, Type::ID type, int slot, int line)
        : Value(top, bb, type, line)
        , slot_(slot) {}
    
    int slot_; // upvalue slot in closure object
}; // class LoadUp
    
class StoreUp : public Value {
public:
    DEF_VAL_GETTER(int, slot);
    DEF_PTR_GETTER(Value, src);
    
    virtual void PrintOperator(FILE *fp) const override {
        ::fprintf(fp, "store upval $%d, ", slot_);
        src_->PrintValue(fp);
    }

    DEFINE_DAG_INST_NODE(StoreUp);
private:
    StoreUp(Function *top, BasicBlock *bb, int slot, Value *val, int line)
        : Value(top, bb, Type::kVoid, line)
        , slot_(slot)
        , src_(DCHECK_NOTNULL(val)) {
        Set(val, top->arena());
    }
    
    int slot_; // upvalue slot in closure object
    Value *src_;
}; // class StoreUp
    
class LoadGlobal : public Value {
public:
    DEF_PTR_PROP_RW_NOTNULL2(const String, name);
    
    virtual void PrintOperator(FILE *fp) const override {
        ::fprintf(fp, "global $%s", name_->data());
    }
    
    DEFINE_DAG_INST_NODE(LoadGlobal);
private:
    LoadGlobal(Function *top, BasicBlock *bb, Type::ID type, const String *name, int line)
        : Value(top, bb, type, line)
        , name_(name) {}

    const String *name_;
}; // class LoadGlobal
    
class StoreGlobal : public Value {
public:
    DEF_PTR_GETTER(const String, name);
    DEF_PTR_GETTER(Value, src);
    
    virtual void PrintOperator(FILE *fp) const override {
        ::fprintf(fp, "store global $%s, ", name_->data());
        src_->PrintValue(fp);
    }
    
    DEFINE_DAG_INST_NODE(StoreGlobal);
private:
    StoreGlobal(Function *top, BasicBlock *bb, const String *name, Value *src, int line)
        : Value(top, bb, Type::kVoid, line)
        , name_(DCHECK_NOTNULL(name))
        , src_(DCHECK_NOTNULL(src)) {
        Set(src, top->arena());
    }

    const String *name_;
    Value *src_;
}; // class LoadGlobal


class CastInst : public Value {
public:
    DEF_PTR_GETTER(Value, from);
    virtual const char *cast_name() const = 0;
    
    virtual void PrintOperator(FILE *fp) const override {
        ::fprintf(fp, "%s ", cast_name());
        from_->PrintValue(fp);
    }
protected:
    CastInst(Function *top, BasicBlock *bb, Type::ID to, Value *from, int line)
        : Value(top, bb, to, line)
        , from_(DCHECK_NOTNULL(from)) {
        Set(from, top->arena());
    }

    Value *from_;
}; // class CastInst

#define DEFINE_CAST_INST(name, short_name, dst_ty) \
class name : public CastInst { \
public: \
    virtual const char *cast_name() const override { return short_name; } \
    DEFINE_DAG_INST_NODE(name); \
private: \
    name(Function *top, BasicBlock *bb, Value *from, int line) \
        : CastInst(top, bb, dst_ty, from, line) { \
    } \
}
    
DEFINE_CAST_INST(Inbox,       "inbox", Type::kObject);
DEFINE_CAST_INST(IntToLong,   "itol",  Type::kLong);
DEFINE_CAST_INST(IntToFloat,  "itof",  Type::kFloat);
DEFINE_CAST_INST(LongToInt,   "ltoi",  Type::kInt);
DEFINE_CAST_INST(LongToFloat, "ltof",  Type::kFloat);
DEFINE_CAST_INST(FloatToInt,  "ftoi",  Type::kInt);
DEFINE_CAST_INST(FloatToLong, "ftol",  Type::kLong);

#undef DEFINE_CAST_INST

class BaseOfStack : public Value {
public:
    DEF_PTR_GETTER(Value, base);
    DEF_VAL_GETTER(int, offset);
    
    virtual void PrintOperator(FILE *fp) const override {
        ::fprintf(fp, "baseof ");
        base_->PrintValue(fp);
        ::fprintf(fp, " $%d", offset_);
    }
    
    DEFINE_DAG_INST_NODE(BaseOfStack);
private:
    BaseOfStack(Function *top, BasicBlock *bb, Value *base, int offset, int line)
        : Value(top, bb, DCHECK_NOTNULL(base)->hint(offset), line)
        , base_(DCHECK_NOTNULL(base))
        , offset_(offset) {
        Set(base, top->arena());
    }

    Value *base_;
    int offset_;
}; // class BaseOfStack
    
class Unary : public Value {
public:
    DEF_PTR_PROP_RW(Value, operand);
    
    virtual const char *op() const = 0;
    
    virtual void PrintOperator(FILE *fp) const override {
        fprintf(fp, "%s ", op());
        operand()->PrintValue(fp);
    }
protected:
    Unary(Function *top, BasicBlock *bb, Type::ID type, Value *operand, int line)
        : Value(top, bb, type, line)
        , operand_(DCHECK_NOTNULL(operand)) {
        Set(operand, top->arena());
    }
private:
    Value *operand_;
}; // class Unary

class Binary : public Value {
public:
    DEF_PTR_PROP_RW(Value, lhs);
    DEF_PTR_PROP_RW(Value, rhs);

    virtual const char *op() const = 0;
    
    virtual void PrintOperator(FILE *fp) const override {
        lhs()->PrintValue(fp);
        fprintf(fp, " %s ", op());
        rhs()->PrintValue(fp);
    }
protected:
    Binary(Function *top, BasicBlock *bb, Type::ID type, Value *lhs, Value *rhs, int line)
        : Value(top, bb, type, line)
        , lhs_(DCHECK_NOTNULL(lhs))
        , rhs_(DCHECK_NOTNULL(rhs)) {
        Set(lhs, top->arena());
        Set(rhs, top->arena());
    }
private:
    Value *lhs_;
    Value *rhs_;
}; // class Binary
    
class IMinus : public Unary {
public:
    virtual const char *op() const override { return "-"; }
    
    DEFINE_DAG_INST_NODE(IMinus);
private:
    IMinus(Function *top, BasicBlock *bb, Value *operand, int line)
        : Unary(top, bb, operand->type(), operand, line) {
        DCHECK(operand->IsInt());
    }
}; // class IMinus

class IAdd : public Binary {
public:
    virtual const char *op() const override { return "+"; }
    
    DEFINE_DAG_INST_NODE(IAdd);
private:
    IAdd(Function *top, BasicBlock *bb, Value *lhs, Value *rhs, int line)
        : Binary(top, bb, lhs->type(), lhs, rhs, line) {
        DCHECK(lhs->IsInt());
        DCHECK(rhs->IsInt());
    }
}; // class IAdd
    
class ISub : public Binary {
public:
    virtual const char *op() const override { return "-"; }
    
    DEFINE_DAG_INST_NODE(ISub);
private:
    ISub(Function *top, BasicBlock *bb, Value *lhs, Value *rhs, int line)
        : Binary(top, bb, lhs->type(), lhs, rhs, line) {
        DCHECK(lhs->IsInt());
        DCHECK(rhs->IsInt());
    }
}; // class ISub
    
class Branch : public Value {
public:
    DEF_PTR_PROP_RW(Value, cond);
    DEF_PTR_PROP_RW(BasicBlock, if_true);
    DEF_PTR_PROP_RW(BasicBlock, if_false);
    
    virtual void PrintOperator(FILE *fp) const override;
    
    DEFINE_DAG_INST_NODE(Branch);
private:
    Branch(Function *top, BasicBlock *bb, Value *cond, int line)
        : Value(top, bb, Type::kVoid, line)
        , cond_(cond) {
        Set(cond, top->arena());
    }
    
    Value *cond_;
    BasicBlock *if_true_ = nullptr;
    BasicBlock *if_false_ = nullptr;
}; // class Branch
    
class NoCondBranch : public Value {
public:
    DEF_PTR_GETTER(BasicBlock, target);
    
    virtual void PrintOperator(FILE *fp) const override {
        ::fprintf(fp, "br l%d", target_->label());
    }
    
    DEFINE_DAG_INST_NODE(NoCondBranch);
private:
    NoCondBranch(Function *top, BasicBlock *bb, BasicBlock *target, int line)
        : Value(top, bb, Type::kVoid, line)
        , target_(DCHECK_NOTNULL(target)) {
    }
    
    BasicBlock *target_;
}; // class NoCondBranch
    
class Phi : public Value {
public:
    struct Path {
        BasicBlock *incoming_bb;
        Value *incoming_value;
    };
    using PathSet = base::ArenaVector<Path>;
    
    DEF_VAL_PROP_RMW(PathSet, incoming);
    
    void AddIncoming(BasicBlock *bb, Value *value) {
        DCHECK(value->type() == type());
        incoming_.push_back({bb, value});
        Set(value, owns_->arena());
    }

    inline Value *GetIncomingValue(BasicBlock *bb);
    
    virtual void PrintOperator(FILE *fp) const override;
    
    DEFINE_DAG_INST_NODE(Phi);
private:
    Phi(Function *top, BasicBlock *bb, Type::ID type, int line)
        : Value(top, bb, type, line)
        , owns_(top)
        , incoming_(top->arena()) {}
    
    Function *owns_;
    PathSet incoming_;
}; // class Phi
    
class Ret : public Value {
public:
    using ValueSet = base::ArenaVector<Value *>;
    
    DEF_VAL_PROP_RMW(ValueSet, ret_vals);
    DEF_VAL_PROP_RW(int, wanted);
    
    void AddRetVal(Value *val) {
        ret_vals_.push_back(val);
        Set(val, owns_->arena());
    }
    
    virtual void PrintOperator(FILE *fp) const override {
        ::fprintf(fp, "ret(%d) ", wanted_);
        for (auto val : ret_vals_) {
            val->PrintValue(fp);
            ::fprintf(fp, " ");
        }
    }
    
    DEFINE_DAG_INST_NODE(Ret);
private:
    Ret(Function *top, BasicBlock *bb, int line)
        : Value(top, bb, Type::kVoid, line)
        , owns_(top)
        , ret_vals_(top->arena()) {}
    
    Function *owns_;
    ValueSet ret_vals_;
    int wanted_ = 0;
}; // class Ret
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// Utils:
////////////////////////////////////////////////////////////////////////////////////////////////////
struct CastPriority {
    enum How {
        kLHS,
        kRHS,
        kBoth,
        kKeep,
        kNever,
    };
    How how;
    Type::ID type;
};
    
CastPriority GetCastPriority(Type::ID lhs, Type::ID rhs);
    
Value::Kind GetCastAction(Type::ID dst, Type::ID src);

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Inline Functions:
////////////////////////////////////////////////////////////////////////////////////////////////////

inline BasicBlock *Function::NewBB(BasicBlock *in_edge) {
    BasicBlock *bb = new (arena_) BasicBlock(this, NextBBId(), arena_);
    if (in_edge) {
        bb->in_edges_.push_back(in_edge);
        in_edge->out_edges_.push_back(bb);
    }
    if (!entry_) {
        entry_ = bb;
    }
    return bb;
}

inline Value *Function::Parameter(Type::ID type, int line) {
    return new (arena_) class Parameter(this, type, line);
}
    
inline Value *Function::LoadUp(BasicBlock *bb, Type::ID type, int slot, int line) {
    return new (arena_) class LoadUp(this, bb, type, slot, line);
}
    
inline Value *Function::StoreUp(BasicBlock *bb, int slot, Value *val, int line) {
    return new (arena_) class StoreUp(this, bb, slot, val, line);
}
    
inline Value *Function::LoadGlobal(BasicBlock *bb, Type::ID type, const String *name, int line) {
    return new (arena_) class LoadGlobal(this, bb, type, name, line);
}
    
inline Value *Function::StoreGlobal(BasicBlock *bb, const String *name, Value *val, int line) {
    return new (arena_) class StoreGlobal(this, bb, name, val, line);
}

inline Value *Function::Inbox(BasicBlock *bb, Value *from, int line) {
    return new (arena_) class Inbox(this, bb, from, line);
}
    
inline Value *Function::IntToLong(BasicBlock *bb, Value *from, int line) {
    return new (arena_) class IntToLong(this, bb, from, line);
}

inline Value *Function::IntToFloat(BasicBlock *bb, Value *from, int line) {
    return new (arena_) class IntToLong(this, bb, from, line);
}

inline Value *Function::LongToInt(BasicBlock *bb, Value *from, int line) {
    return new (arena_) class LongToInt(this, bb, from, line);
}

inline Value *Function::LongToFloat(BasicBlock *bb, Value *from, int line) {
    return new (arena_) class LongToFloat(this, bb, from, line);
}

inline Value *Function::FloatToInt(BasicBlock *bb, Value *from, int line) {
    return new (arena_) class FloatToInt(this, bb, from, line);
}

inline Value *Function::FloatToLong(BasicBlock *bb, Value *from, int line) {
    return new (arena_) class FloatToLong(this, bb, from, line);
}
    
inline Value *Function::BaseOfStack(BasicBlock *bb, Value *base, int offset, int line) {
    return new (arena_) class BaseOfStack(this, bb, base, offset, line);
}
    
inline Constant *Function::Constant(Type::ID type, int line) {
    return new (arena_) class Constant(this, type, line);
}
    
inline Value *Function::Nil(int line) {
    auto k = new (arena_) class Constant(this, Type::kObject, line);
    k->set_nil();
    return k;
}
    
inline Value *Function::IMinus(BasicBlock *bb, Value *operand, int line) {
    return new (arena_) class IMinus(this, bb, operand, line);
}

inline Value *Function::IAdd(BasicBlock *bb, Value *lhs, Value *rhs, int line) {
    return new (arena_) class IAdd(this, bb, lhs, rhs, line);
}

inline Value *Function::ISub(BasicBlock *bb, Value *lhs, Value *rhs, int line) {
    return new (arena_) class ISub(this, bb, lhs, rhs, line);
}
    
inline Branch *Function::Branch(BasicBlock *bb, Value *cond, int line) {
    return new (arena_) class Branch(this, bb, cond, line);
}
    
inline Value *Function::NoCondBranch(BasicBlock *bb, BasicBlock *target, int line) {
    return new (arena_) class NoCondBranch(this, bb, target, line);
}

inline Phi *Function::Phi(BasicBlock *bb, Type::ID type, int line) {
    return new (arena_) class Phi(this, bb, type, line);
}
    
inline Ret *Function::Ret(BasicBlock *bb, int line) {
    return new (arena_) class Ret(this, bb, line);
}
    
inline void Function::PrintTo(FILE *fp) const {
    for (auto bb : basic_blocks_) {
        bb->PrintTo(fp);
    }
}
    
inline void BasicBlock::AddValue(Value *v) {
    if (!root_) {
        root_ = v;
        last_ = v;
    } else {
        last_->set_next(v);
        last_ = v;
    }
}
    
inline Value *Phi::GetIncomingValue(BasicBlock *bb) {
    for (auto path : incoming_) {
        if (path.incoming_bb == bb) {
            return path.incoming_value;
        }
    }
    return nullptr;
}

} // namespace hir
    
} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_HIGH_LEVEL_IR_H_
