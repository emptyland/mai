#ifndef MAI_NYAA_HIGH_LEVEL_IR_H_
#define MAI_NYAA_HIGH_LEVEL_IR_H_

#include "base/arena-utils.h"
#include "base/arena.h"
#include "base/slice.h"
#include "base/base.h"
#include "glog/logging.h"
#include <set>

namespace mai {
    
namespace nyaa {
class NyMap;
class Object;
namespace hir {
    
#define DECL_DAG_NODES(V) \
    V(Parameter) \
    V(Alloca) \
    V(Load) \
    V(LoadUp) \
    V(LoadGlobal) \
    V(BaseOfStack) \
    V(Constant) \
    V(Invoke) \
    V(Branch) \
    V(NoCondBranch) \
    V(Phi) \
    V(Ret) \
    V(ICmp) \
    V(FCmp) \
    DECL_HIR_STORE(V) \
    DECL_HIR_CAST(V) \
    DECL_HIR_UNARY(V) \
    DECL_HIR_BINARY(V)
    
#define DECL_HIR_STORE(V) \
    V(StoreGlobal) \
    V(StoreUp) \
    V(Store)

#define DECL_HIR_UNARY(V) \
    V(IMinus)
    
#define DECL_HIR_BINARY(V) \
    V(IAdd) \
    V(ISub) \
    V(IMul) \
    V(IDiv) \
    V(IMod) \
    V(LAdd) \
    V(LSub) \
    V(LMul) \
    V(LDiv) \
    V(LMod) \
    V(FAdd) \
    V(FSub) \
    V(FMul) \
    V(FDiv) \
    V(FMod) \
    V(OAdd) \
    V(OSub) \
    V(OMul) \
    V(ODiv) \
    V(OMod)
    
#define DECL_HIR_CAST(V) \
    V(Inbox) \
    V(IntToLong) \
    V(LongToInt) \
    V(IntToFloat) \
    V(FloatToInt) \
    V(LongToFloat) \
    V(FloatToLong)

#define DECL_DAG_TYPES(V) \
    V(Void,    "void") \
    V(Int,     "int") \
    V(Long,    "long") \
    V(Float,   "float") \
    V(String,  "string") \
    V(Array,   "array") \
    V(Map,     "map") \
    V(UDO,     "udo") \
    V(Closure, "closure") \
    V(Object,  "object")
    
struct Type {
    enum ID {
    #define DEFINE_ENUM(name, literal) k##name,
        DECL_DAG_TYPES(DEFINE_ENUM)
    #undef DEFINE_ENUM
        kMaxTypes,
    };
    
    static const char *kNames[];
}; // struct Type
    
struct Compare {
    enum Op {
        kEQ,
        kNE,
        kLT,
        kLE,
        kGT,
        kGE,
    };
    
    static const char *kNames[];
};
    
class Value;
class Function;
class BasicBlock;
class UnaryInst;
class BinaryInst;
#define DEFINE_DECL(name) class name;
    DECL_DAG_NODES(DEFINE_DECL)
#undef DEFINE_DECL
    
using String = base::ArenaString;
    
struct UpvalDesc {
    Type::ID type_hint;
    int slot;
};
    
// Sea-of-Nodes IR:
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
    
    DEF_PTR_PROP_RW(BasicBlock, entry);
    DEF_PTR_GETTER(base::Arena, arena);
    
    inline BasicBlock *NewBB(BasicBlock *in_edge, bool dont_insert = false);
    inline Value *Parameter(Type::ID type, int line);
    inline Value *Alloca(Type::ID type, int line);
    inline Value *Load(BasicBlock *bb, Value *src, int line);
    inline Value *Store(BasicBlock *bb, Value *dst, Value *src, int line);
    inline Value *LoadUp(BasicBlock *bb, Type::ID type, int slot, int line);
    inline Value *StoreUp(BasicBlock *bb, int slot, Value *val, int line);
    inline Value *LoadGlobal(BasicBlock *bb, Type::ID type, const String *name, int line);
    inline Value *StoreGlobal(BasicBlock *bb, const String *name, Value *val, int line);
    inline Invoke *Invoke(BasicBlock *bb, Type::ID type, Value *callee, int argc, int wanted,
                          int line);
    inline Value *BaseOfStack(BasicBlock *bb, Value *base, int offset, int line);
    inline Constant *Constant(Type::ID type, int line);
    inline Value *Nil(int line);
    inline Branch *Branch(BasicBlock *bb, Value *cond, int line);
    inline Value *NoCondBranch(BasicBlock *bb, BasicBlock *target, int line);
    inline Phi *Phi(BasicBlock *bb, Type::ID type, int line);
    inline Ret *Ret(BasicBlock *bb, int line);
    
    inline Value *ICmp(BasicBlock *bb, Compare::Op op, Value *lhs, Value *rhs, int line);
    inline Value *FCmp(BasicBlock *bb, Compare::Op op, Value *lhs, Value *rhs, int line);
    
#define DECL_CAST_NEW(name) \
    inline Value *name(BasicBlock *bb, Value *from, int line);
    
    DECL_HIR_CAST(DECL_CAST_NEW)
    
#undef DECL_CAST_NEW

#define DECL_UNARY_NEW(name) \
    inline Value *name(BasicBlock *bb, Value *operand, int line);
    
    DECL_HIR_UNARY(DECL_UNARY_NEW)
    
#undef DECL_UNARY_NEW
    
#define DECL_BINARY_NEW(name) \
    inline Value *name(BasicBlock *bb, Value *lhs, Value *rhs, int line);

    DECL_HIR_BINARY(DECL_BINARY_NEW)
    
#undef DECL_BINARY_NEW
    
    void AddParameter(Value *val) { parameters_.push_back(DCHECK_NOTNULL(val)); }

    inline void AddBasicBlock(BasicBlock *bb);
    
    int NextBBId() { return next_bb_id_++; }

    void PrintTo(std::string *buf, size_t limit) const;
    inline void PrintTo(FILE *fp) const;
    
    inline int ReplaceAllUses(Value *val, Value *new_val);

    static Function *New(base::Arena *arena) { return new (arena) Function(arena); }
    
    friend class Value;
    friend class BasicBlock;
private:
    Function(base::Arena *arena)
        : arena_(arena)
        , parameters_(arena)
        , basic_blocks_(arena) {}
    
    int NextValId() { return next_val_id_++; }

    base::Arena *arena_;
    ValueSet parameters_;
    BasicBlockSet basic_blocks_;
    BasicBlock *entry_ = nullptr;
    int next_bb_id_ = 0;
    int next_val_id_ = 0;
}; // class Function
    
class BasicBlock : public HIRNode {
public:
    using EdgeSet = base::ArenaVector<BasicBlock *>;
    
    DEF_PTR_GETTER_NOTNULL(Function, owns);
    DEF_VAL_PROP_RW(int, label);
    DEF_VAL_PROP_RMW(EdgeSet, in_edges);
    DEF_VAL_PROP_RMW(EdgeSet, out_edges);
    
    inline bool insts_empty() const;
    inline Value *insts_begin() const;
    inline Value *insts_end() const;
    
    inline Value *insts_head() const;
    inline Value *insts_tail() const;
    
    inline void AddInEdge(BasicBlock *bb) {
        in_edges_.push_back(bb);
        bb->out_edges_.push_back(this);
    }
    
    void GetAfterAll(std::set<BasicBlock *> *all) {
        if (all->find(this) != all->end()) {
            return;
        }
        all->insert(this);
        for (auto edge: out_edges_) {
            edge->GetAfterAll(all);
        }
    }
    
    void InsertHead(Value *val) { InsertValueAfter(dummy_, val); }
    inline void InsertValueAfter(Value *pos, Value *val);
    void InsertTail(Value *val) { InsertValueBefore(dummy_, val); }
    inline void InsertValueBefore(Value *pos, Value *val);
    inline void RemoveValue(Value *val);
    
    inline int ReplaceAllUses(Value *old_val, Value *new_val);
    inline int ReplaceAllUsesAfter(Value *old_val, Value *new_val);
    
    void PrintTo(FILE *fp) const;
    
    friend class Value;
    friend class Function;
private:
    inline BasicBlock(Function *top, int label);

    int label_;
    Function *owns_;
    EdgeSet in_edges_;
    EdgeSet out_edges_;
    Value *dummy_; // instructions list dummy
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
    enum InstID {
    #define DEFINE_ENUM(name) k##name,
        DECL_DAG_NODES(DEFINE_ENUM)
    #undef DEFINE_ENUM
        kMaxInsts,
    };

    DEF_VAL_GETTER(Type::ID, type);
    DEF_VAL_GETTER(int, index);
    DEF_VAL_GETTER(int, line);
    DEF_PTR_PROP_RW(Value, prev);
    DEF_PTR_PROP_RW(Value, next);
    DEF_PTR_PROP_RW(BasicBlock, owns);
    
#define DEFINE_IS(name, literal) bool Is##name##Ty() const { return type_ == Type::k##name; }
    DECL_DAG_TYPES(DEFINE_IS)
#undef DEFINE_IS
    
#define DEFINE_IS(name) bool Is##name() const { return kind() == k##name; }
    DECL_DAG_NODES(DEFINE_IS)
#undef DEFINE_IS
    
    Use *uses_begin() const { return use_dummy_.next; }
    Use *uses_end() const { return const_cast<Use *>(&use_dummy_); }
    bool uses_empty() const { return use_dummy_.next == &use_dummy_; }
    void clear_uses() {
        use_dummy_.next = &use_dummy_;
        use_dummy_.prev = &use_dummy_;
    }
    
    virtual InstID kind() const { return kMaxInsts; }
    virtual Type::ID hint(int i) const { DLOG(FATAL) << "No implement"; return Type::kVoid; }
    
    virtual bool ReplaceUse(Value *old_val, Value *new_val) { return false; }
    virtual void PrintOperator(FILE *fp) const { DLOG(FATAL) << "Not implements"; }

    inline void PrintTo(FILE *fp) const;
    inline void PrintValue(FILE *fp) const;
    
    void AddUse(Use *use) { use->AddToList(&use_dummy_); }
    
    inline void RemoveFromOwns();

    friend class BasicBlock;
    friend class Function;
protected:
    Value(Function *top, BasicBlock *bb, Type::ID type, int line)
        : type_(type)
        , index_(type != Type::kVoid ? top->NextValId() : 0)
        , line_(line)
        , use_dummy_(nullptr)
        , owns_(bb) {
            use_dummy_.next = &use_dummy_;
            use_dummy_.prev = &use_dummy_;
            if (bb) {
                bb->InsertTail(this);
            }
        }
    
    // pure value
    Value(Type::ID type, int line)
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
    
    void RemoveFromList() {
        prev_->next_ = next_;
        next_->prev_ = prev_;
#if defined(DEBUG) || defined(_DEBUG)
        prev_ = nullptr;
        next_ = nullptr;
#endif
    }
    
    Type::ID type_;
    int index_;
    int line_;
    Use use_dummy_;
    BasicBlock *owns_; // Which basic-block has in.
    Value *prev_ = nullptr;
    Value *next_ = nullptr;
};
    
#define DEFINE_DAG_INST_NODE(name) \
    virtual InstID kind() const override { return k##name; } \
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
    int64_t int_val() const { DCHECK(IsIntTy()); return smi_; }
    void set_int_val(int64_t val) { DCHECK(IsIntTy()); smi_ = val; }
    
    double float_val() const { DCHECK(IsFloatTy()); return f64_; }
    void set_float_val(double val) { DCHECK(IsFloatTy()); f64_ = val; }
    
    const String *string_val() const { DCHECK(IsStringTy()); return str_; }
    void set_string_val(const String *val) { DCHECK(IsStringTy()); str_ = val; }
    
    NyMap *map_val() const { DCHECK(IsMapTy()); return map_; }
    void set_map_val(NyMap *val) { DCHECK(IsMapTy()); map_ = val; }
    
    bool is_nil() const { return nil_stub_ == nullptr; }
    void set_nil() { nil_stub_ = nullptr; }
    
    virtual void PrintOperator(FILE *fp) const override;

    DEFINE_DAG_INST_NODE(Constant);
private:
    Constant(Type::ID type, int line) : Value(type, line) {}
    
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

    virtual void PrintOperator(FILE *fp) const override { ::fprintf(fp, "upval $%d", slot_); }
    
    DEFINE_DAG_INST_NODE(LoadUp);
private:
    LoadUp(Function *top, BasicBlock *bb, Type::ID type, int slot, int line)
        : Value(top, bb, type, line)
        , slot_(slot) {}
    
    int slot_; // upvalue slot in closure object
}; // class LoadUp
    
class StoreInst : public Value {
public:
    DEF_PTR_PROP_RW_NOTNULL2(Value, src);

    virtual bool ReplaceUse(Value *old_val, Value *new_val) override {
        if (old_val == src_ && new_val->type() == src_->type()) {
            src_ = new_val;
            return true;
        }
        return false;
    }

protected:
    StoreInst(Function *top, BasicBlock *bb, Value *src, int line)
        : Value(top, bb, Type::kVoid, line)
        , src_(DCHECK_NOTNULL(src)) {
        Set(src, top->arena());
    }
    
    Value *src_;
}; // class StoreInst
    
class Alloca : public Value {
public:
    virtual bool ReplaceUse(Value *old_val, Value *new_val) override { return true; }
    
    virtual void PrintOperator(FILE *fp) const override { ::fprintf(fp, "alloca"); }
    
    DEFINE_DAG_INST_NODE(Alloca);
private:
    Alloca(Function *top, Type::ID type, int line);
}; // class Alloca
    
class Load : public Value {
public:
    DEF_PTR_GETTER(Value, src);
    
    virtual bool ReplaceUse(Value *old_val, Value *new_val) override {
        if (old_val == src_ && new_val->type() == src_->type()) {
            src_ = new_val;
            return true;
        }
        return false;
    }
    
    virtual void PrintOperator(FILE *fp) const override {
        ::fprintf(fp, "load ");
        src_->PrintValue(fp);
    }
    
    DEFINE_DAG_INST_NODE(Load);
private:
    Load(Function *top, BasicBlock *bb, Value *src, int line)
        : Value(top, bb, src->type(), line)
        , src_(src) {}

    Value *src_;
}; // class Load
    
class Store : public StoreInst {
public:
    DEF_PTR_GETTER(Value, dst);
    
    virtual bool ReplaceUse(Value *old_val, Value *new_val) override {
        bool ok = StoreInst::ReplaceUse(old_val, new_val);
        if (ok) {
            return ok;
        }
        if (old_val == dst_ && new_val->type() == dst_->type()) {
            dst_ = new_val;
            return true;
        }
        return false;
    }
    
    virtual void PrintOperator(FILE *fp) const override {
        ::fprintf(fp, "store ");
        dst_->PrintValue(fp);
        ::fprintf(fp, ", ");
        src_->PrintValue(fp);
    }
    
    DEFINE_DAG_INST_NODE(Store);
private:
    Store(Function *top, BasicBlock *bb, Value *dst, Value *src, int line)
        : StoreInst(top, bb, src, line)
        , dst_(dst) {}
    
    Value *dst_;
}; // class Store
    
class StoreUp : public StoreInst {
public:
    DEF_VAL_GETTER(int, slot);
    
    virtual void PrintOperator(FILE *fp) const override {
        ::fprintf(fp, "store upval $%d, ", slot_);
        src_->PrintValue(fp);
    }

    DEFINE_DAG_INST_NODE(StoreUp);
private:
    StoreUp(Function *top, BasicBlock *bb, int slot, Value *src, int line)
        : StoreInst(top, bb, src, line)
        , slot_(slot) {}
    
    int slot_; // upvalue slot in closure object
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
    
class StoreGlobal : public StoreInst {
public:
    DEF_PTR_GETTER(const String, name);

    virtual void PrintOperator(FILE *fp) const override {
        ::fprintf(fp, "store global $%s, ", name_->data());
        src_->PrintValue(fp);
    }
    
    DEFINE_DAG_INST_NODE(StoreGlobal);
private:
    StoreGlobal(Function *top, BasicBlock *bb, const String *name, Value *src, int line)
        : StoreInst(top, bb, src, line)
        , name_(DCHECK_NOTNULL(name)) {
    }

    const String *name_;
}; // class LoadGlobal


class CastInst : public Value {
public:
    DEF_PTR_PROP_RW_NOTNULL2(Value, from);
    virtual const char *cast_name() const = 0;
    
    virtual bool ReplaceUse(Value *old_val, Value *new_val) override {
        if (old_val == from_ && new_val->type() == from_->type()) {
            from_ = new_val;
            return true;
        }
        return false;
    }
    
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
    
class Invoke : public Value {
public:
    using RetSet = base::ArenaVector<Type::ID>;
    
    DEF_PTR_PROP_RW(Value, callee);

    Value *argument(int i) const {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, argc_);
        return args_[i];
    }

    inline void SetArgument(int i, Value *v);
    
    inline int GetArgument(Value *val) const;
    
    DEF_VAL_PROP_RMW(RetSet, rets);
    DEF_VAL_GETTER(int, argc);
    DEF_VAL_GETTER(int, wanted);
    
    void AddRetType(Type::ID type) { rets_.push_back(type); }
    
    virtual Type::ID hint(int i) const override {
        DCHECK_GE(i, 0);
        DCHECK_LT(i - 1, rets_.size());
        return i == 0 ? type() : rets_[i - 1];
    }
    
    virtual bool ReplaceUse(Value *old_val, Value *new_val) override;
    
    virtual void PrintOperator(FILE *fp) const override;

    DEFINE_DAG_INST_NODE(Invoke);
private:
    Invoke(Function *top, BasicBlock *bb, Type::ID type, Value *callee, int nargs, int wanted,
           int line)
        : Value(top, bb, type, line)
        , owns_(top)
        , callee_(DCHECK_NOTNULL(callee))
        , args_(nullptr)
        , rets_(top->arena())
        , nargs_(nargs)
        , argc_(nargs < 0 ? 1 : nargs)
        , wanted_(wanted) {
        args_ = top->arena()->NewArray<Value *>(argc_);
        ::memset(args_, 0, sizeof(argc_) * sizeof(args_[0]));
        Set(callee, top->arena());
    }

    Function *owns_;
    Value *callee_;
    Value **args_;
    RetSet rets_;
    int argc_; // size of real args
    int nargs_; // size of accept
    int wanted_; // size of rets
}; // class Invoke

class BaseOfStack : public Value {
public:
    DEF_PTR_GETTER(Value, base);
    DEF_VAL_GETTER(int, offset);
    
    virtual bool ReplaceUse(Value *old_val, Value *new_val) override {
        if (old_val == base_ && new_val->type() == type()) {
            base_ = new_val;
            return true;
        }
        return false;
    }
    
    virtual void PrintOperator(FILE *fp) const override {
        ::fprintf(fp, "baseof ");
        base_->PrintValue(fp);
        ::fprintf(fp, "[%d]", offset_);
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
    
class UnaryInst : public Value {
public:
    DEF_PTR_PROP_RW(Value, operand);
    
    virtual const char *op() const = 0;
    
    virtual bool ReplaceUse(Value *old_val, Value *new_val) override {
        if (old_val == operand_ && new_val->type() == operand_->type()) {
            operand_ = new_val;
            return true;
        }
        return false;
    }
    
    virtual void PrintOperator(FILE *fp) const override {
        fprintf(fp, "%s ", op());
        operand()->PrintValue(fp);
    }
protected:
    UnaryInst(Function *top, BasicBlock *bb, Type::ID type, Value *operand, int line)
        : Value(top, bb, type, line)
        , operand_(DCHECK_NOTNULL(operand)) {
        Set(operand, top->arena());
    }
private:
    Value *operand_;
}; // class Unary

class BinaryInst : public Value {
public:
    DEF_PTR_PROP_RW(Value, lhs);
    DEF_PTR_PROP_RW(Value, rhs);

    virtual const char *op() const = 0;
    
    virtual bool ReplaceUse(Value *old_val, Value *new_val) override;
    
    virtual void PrintOperator(FILE *fp) const override {
        lhs()->PrintValue(fp);
        fprintf(fp, " %s ", op());
        rhs()->PrintValue(fp);
    }
protected:
    BinaryInst(Function *top, BasicBlock *bb, Type::ID type, Value *lhs, Value *rhs, int line)
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
    
class Comparator : public BinaryInst {
public:
    DEF_VAL_GETTER(Compare::Op, op_code);
    
    virtual const char *op() const override {
        switch (op_code_) {
            case Compare::kEQ:
                return "eq";
            case Compare::kNE:
                return "ne";
            case Compare::kLT:
                return "lt";
            case Compare::kLE:
                return "le";
            case Compare::kGT:
                return "gt";
            case Compare::kGE:
                return "ge";
            default:
                break;
        }
    }
    
    virtual void PrintOperator(FILE *fp) const override {
        ::fprintf(fp, "cmp %s", op());
        lhs()->PrintValue(fp);
        ::fprintf(fp, " ");
        rhs()->PrintValue(fp);
    }
protected:
    Comparator(Function *top, BasicBlock *bb, Compare::Op op, Value *lhs, Value *rhs, int line)
        : BinaryInst(top, bb, Type::kInt, lhs, rhs, line)
        , op_code_(op) {}
private:
    Compare::Op op_code_;
}; // class Comparator
    
class IMinus : public UnaryInst {
public:
    virtual const char *op() const override { return "-"; }
    
    DEFINE_DAG_INST_NODE(IMinus);
private:
    IMinus(Function *top, BasicBlock *bb, Value *operand, int line)
        : UnaryInst(top, bb, operand->type(), operand, line) {
        DCHECK(operand->IsIntTy());
    }
}; // class IMinus

    
#define DEFINE_BINARY_INST(name, op_name) \
class name : public BinaryInst { \
public: \
    virtual const char *op() const override { return op_name; } \
    DEFINE_DAG_INST_NODE(name); \
private: \
    name(Function *top, BasicBlock *bb, Value *lhs, Value *rhs, int line) \
        : BinaryInst(top, bb, lhs->type(), lhs, rhs, line) { \
    } \
}

DEFINE_BINARY_INST(IAdd, "+");
DEFINE_BINARY_INST(ISub, "-");
DEFINE_BINARY_INST(IMul, "*");
DEFINE_BINARY_INST(IDiv, "/");
DEFINE_BINARY_INST(IMod, "mod");
DEFINE_BINARY_INST(LAdd, "+");
DEFINE_BINARY_INST(LSub, "-");
DEFINE_BINARY_INST(LMul, "*");
DEFINE_BINARY_INST(LDiv, "/");
DEFINE_BINARY_INST(LMod, "mod");
DEFINE_BINARY_INST(FAdd, "+");
DEFINE_BINARY_INST(FSub, "-");
DEFINE_BINARY_INST(FMul, "*");
DEFINE_BINARY_INST(FDiv, "/");
DEFINE_BINARY_INST(FMod, "mod");
DEFINE_BINARY_INST(OAdd, "+");
DEFINE_BINARY_INST(OSub, "-");
DEFINE_BINARY_INST(OMul, "*");
DEFINE_BINARY_INST(ODiv, "/");
DEFINE_BINARY_INST(OMod, "mod");
    
#undef DEFINE_BINARY_INST
    
class ICmp final : public Comparator {
public:
    DEFINE_DAG_INST_NODE(ICmp);
private:
    ICmp(Function *top, BasicBlock *bb, Compare::Op op, Value *lhs, Value *rhs, int line)
        : Comparator(top, bb, op, lhs, rhs, line) {
        DCHECK(lhs->IsIntTy());
        DCHECK(rhs->IsIntTy());
    }
}; // class ICmp
    
class FCmp final : public Comparator {
public:
    DEFINE_DAG_INST_NODE(FCmp);
private:
    FCmp(Function *top, BasicBlock *bb, Compare::Op op, Value *lhs, Value *rhs, int line)
        : Comparator(top, bb, op, lhs, rhs, line) {
        DCHECK(lhs->IsFloatTy());
        DCHECK(rhs->IsFloatTy());
    }
}; // class FCmp
    
class Branch : public Value {
public:
    DEF_PTR_PROP_RW(Value, cond);
    DEF_PTR_PROP_RW(BasicBlock, if_true);
    DEF_PTR_PROP_RW(BasicBlock, if_false);
    
    virtual bool ReplaceUse(Value *old_val, Value *new_val) override {
        if (old_val == cond_) {
            cond_ = new_val;
            return true;
        }
        return false;
    }
    
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
    DEF_VAL_SETTER(Type::ID, type);
    
    inline void AddIncoming(BasicBlock *bb, Value *value);

    inline Value *GetIncomingValue(BasicBlock *bb) const;
    
    inline BasicBlock *GetIncomingBasicBlock(Value *val) const;
    
    virtual bool ReplaceUse(Value *old_val, Value *new_val) override;
    
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
    
    Value *ret_val(size_t i) const {
        DCHECK_LT(i, ret_vals_.size());
        return ret_vals_[i];
    }
    
    void set_ret_val(size_t i, Value *val) {
        DCHECK_LT(i, ret_vals_.size());
        ret_vals_[i] = val;
    }
    
    size_t ret_vals_size() const { return ret_vals_.size(); }

    inline void AddRetVal(Value *val);
    
    bool RetValContain(Value *val) const {
        return std::find(ret_vals_.begin(), ret_vals_.end(), val) != ret_vals_.end();
    }
    
    virtual bool ReplaceUse(Value *old_val, Value *new_val) override;
    
    virtual void PrintOperator(FILE *fp) const override;
    
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
    
Value::InstID GetCastAction(Type::ID dst, Type::ID src);
    
Value::InstID TransformBinaryInst(Type::ID ty, Value::InstID src);

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Inline Functions:
////////////////////////////////////////////////////////////////////////////////////////////////////

inline BasicBlock *Function::NewBB(BasicBlock *in_edge, bool dont_insert) {
    BasicBlock *bb = new (arena_) BasicBlock(this, !dont_insert ? NextBBId() : 0);
    if (in_edge) {
        bb->in_edges_.push_back(in_edge);
        in_edge->out_edges_.push_back(bb);
    }
    if (!dont_insert) { AddBasicBlock(bb); }
    return bb;
}

inline Value *Function::Parameter(Type::ID type, int line) {
    return new (arena_) class Parameter(this, type, line);
}

inline Value *Function::Alloca(Type::ID type, int line) {
    return new (arena_) class Alloca(this, type, line);
}

inline Value *Function::Load(BasicBlock *bb, Value *src, int line) {
    return new (arena_) class Load(this, bb, src, line);
}

inline Value *Function::Store(BasicBlock *bb, Value *dst, Value *src, int line) {
    return new (arena_) class Store(this, bb, dst, src, line);
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
    
inline Invoke *Function::Invoke(BasicBlock *bb, Type::ID type, Value *callee, int argc, int wanted,
                                int line) {
    return new (arena_) class Invoke(this, bb, type, callee, argc, wanted, line);
}
    
inline Value *Function::BaseOfStack(BasicBlock *bb, Value *base, int offset, int line) {
    return new (arena_) class BaseOfStack(this, bb, base, offset, line);
}
    
inline Constant *Function::Constant(Type::ID type, int line) {
    return new (arena_) class Constant(type, line);
}
    
inline Value *Function::Nil(int line) {
    auto k = new (arena_) class Constant(Type::kObject, line);
    k->set_nil();
    return k;
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
    
inline Value *Function::ICmp(BasicBlock *bb, Compare::Op op, Value *lhs, Value *rhs, int line) {
    return new (arena_) class ICmp(this, bb, op, lhs, rhs, line);
}

inline Value *Function::FCmp(BasicBlock *bb, Compare::Op op, Value *lhs, Value *rhs, int line) {
    return new (arena_) class FCmp(this, bb, op, lhs, rhs, line);
}
    
#define DEFINE_CAST_NEW(name) \
inline Value *Function::name(BasicBlock *bb, Value *from, int line) { \
    return new (arena_) class name(this, bb, from, line); \
}
    
DECL_HIR_CAST(DEFINE_CAST_NEW)
    
#undef DEFINE_CAST_NEW
    
#define DEFINE_UNARY_NEW(name) \
inline Value *Function::name(BasicBlock *bb, Value *operand, int line) { \
    return new (arena_) class name(this, bb, operand, line); \
}
    
DECL_HIR_UNARY(DEFINE_UNARY_NEW)
    
#undef DEFINE_UNARY_NEW

#define DEFINE_BINARY_NEW(name) \
inline Value *Function::name(BasicBlock *bb, Value *lhs, Value *rhs, int line) { \
    return new (arena_) class name(this, bb, lhs, rhs, line); \
}

DECL_HIR_BINARY(DEFINE_BINARY_NEW)
    
#undef DEFINE_BINARY_NEW
    
inline void Function::AddBasicBlock(BasicBlock *bb) {
    //bb->label_ = NextBBId();
    basic_blocks_.push_back(DCHECK_NOTNULL(bb));
}
    
inline void Function::PrintTo(FILE *fp) const {
    for (auto bb : basic_blocks_) {
        bb->PrintTo(fp);
    }
}
    
inline int Function::ReplaceAllUses(Value *old_val, Value *new_val) {
    int n = 0;
    for (Use *i = old_val->uses_begin(); i != old_val->uses_end(); i = i->next) {
        bool ok = i->val->ReplaceUse(old_val, new_val);
        DCHECK(ok); (void)ok;
        new_val->AddUse(new (arena_) Use(i->val));
        ++n;
    }
    old_val->clear_uses();
    return n;
}
    
inline BasicBlock::BasicBlock(Function *owns, int label)
    : label_(label)
    , owns_(owns)
    , in_edges_(owns->arena())
    , out_edges_(owns->arena())
    , dummy_(new (owns->arena()) Value(Type::kVoid, 0)) {
    dummy_->next_ = dummy_;
    dummy_->prev_ = dummy_;
}
    
inline bool BasicBlock::insts_empty() const { return dummy_->prev_ == dummy_; }
inline Value *BasicBlock::insts_begin() const { return dummy_->next_; }
inline Value *BasicBlock::insts_end() const { return dummy_; }
inline Value *BasicBlock::insts_head() const { return dummy_->next_; }
inline Value *BasicBlock::insts_tail() const { return dummy_->prev_; }
    
inline int BasicBlock::ReplaceAllUses(Value *old_val, Value *new_val) {
    int n = 0;
    Use *p = old_val->uses_begin();
    while (p != old_val->uses_end()) {
        if (p->val->owns() != this) {
            p = p->next;
            continue;
        }
        
        Use *x = p;
        bool ok = x->val->ReplaceUse(old_val, new_val);
        DCHECK(ok); (void)ok;
        p = p->next;
        x->RemoveFromList();
        new_val->AddUse(new (owns_->arena_) Use(x->val));
        ++n;
    }
    return n;
}
    
inline int BasicBlock::ReplaceAllUsesAfter(Value *old_val, Value *new_val) {
    int n = ReplaceAllUses(old_val, new_val);
    for (auto edge : out_edges_) {
        n += edge->ReplaceAllUsesAfter(old_val, new_val);
    }
    return n;
}

//#define ngx_queue_insert_tail(h, x)
//(x)->prev = (h)->prev;
//(x)->prev->next = x;
//(x)->next = h;
//(h)->prev = x
inline void BasicBlock::InsertValueBefore(Value *h, Value *x) {
    //DCHECK(this == pos->owns() || pos == dummy_);
    (x)->prev_ = (h)->prev_;
    (x)->prev_->next_ = x;
    (x)->next_ = h;
    (h)->prev_ = x;
    
    x->owns_ = this;
}

//#define ngx_queue_insert_head(h, x)
//(x)->next = (h)->next;
//(x)->next->prev = x;
//(x)->prev = h;
//(h)->next = x
inline void BasicBlock::InsertValueAfter(Value *h, Value *x) {
//    DCHECK(this == pos->owns() || pos == dummy_);
//    val->next_ = pos->next_;
//    val->next_->prev_ = val;
//    val->prev_ = pos;
//    pos->next_ = val;
//    val->owns_ = this;
    (x)->next_ = (h)->next_;
    (x)->next_->prev_ = x;
    (x)->prev_ = h;
    (h)->next_ = x;

    x->owns_ = this;
}
    
inline void BasicBlock::RemoveValue(Value *val) {
    DCHECK_EQ(this, val->owns());
    val->RemoveFromList();
    val->set_owns(nullptr);
}
    
inline void Value::RemoveFromOwns() {
    if (owns_) {
        owns_->RemoveValue(this);
    }
}
    
inline void Value::PrintTo(FILE *fp) const {
    PrintValue(fp);
    if (!IsVoidTy()) {
        fprintf(fp, " = ");
    }
    PrintOperator(fp);
}

inline void Value::PrintValue(FILE *fp) const {
    if (!IsVoidTy()) {
        if (kind() == kConstant) {
            PrintOperator(fp);
        } else {
            fprintf(fp, "%s %%v%d", Type::kNames[type_], index_);
        }
    }
}
    
inline void Phi::AddIncoming(BasicBlock *bb, Value *value) {
    DCHECK(value->type() == type());
    if (!GetIncomingBasicBlock(value)) {
        Set(value, owns_->arena());
    }
    incoming_.push_back({bb, value});
}
    
inline Value *Phi::GetIncomingValue(BasicBlock *bb) const {
    auto iter = std::find_if(incoming_.begin(), incoming_.end(),
                             [bb](auto path) { return path.incoming_bb == bb; });
    return iter == incoming_.end() ? nullptr : iter->incoming_value;
}
    
inline BasicBlock *Phi::GetIncomingBasicBlock(Value *val) const {
    auto iter = std::find_if(incoming_.begin(), incoming_.end(),
                             [val](auto path) { return path.incoming_value == val; });
    return iter == incoming_.end() ? nullptr : iter->incoming_bb;
}
    
void Invoke::SetArgument(int i, Value *v) {
    DCHECK_GE(i, 0);
    DCHECK_LT(i, argc_);
    if (GetArgument(v) < argc_) {
        Set(v, owns_->arena());
    }
    args_[i] = v;
}
    
inline int Invoke::GetArgument(Value *val) const {
    int found = argc_;
    for (int i = 0; i < argc_; ++i) {
        if (args_[i] == val) {
            found = i;
            break;
        }
    }
    return found;
}
    
inline void Ret::AddRetVal(Value *val) {
    if (!RetValContain(val)) {
        Set(val, owns_->arena());
    }
    ret_vals_.push_back(val);
}

} // namespace hir
    
} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_HIGH_LEVEL_IR_H_
