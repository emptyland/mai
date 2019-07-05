#ifndef MAI_NYAA_IR_DAG_IR_H_
#define MAI_NYAA_IR_DAG_IR_H_

#include "base/arena-utils.h"
#include "base/arena.h"
#include "base/slice.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {
    
namespace dag {
    
#define DECL_DAG_NODES(V) \
    V(Parameter) \
    V(Constant) \
    V(IMinus) \
    V(IAdd) \
    V(ISub) \
    V(Branch) \
    V(Phi)
    
#define DECL_DAG_TYPES(V) \
    V(Void) \
    V(Nil) \
    V(Int) \
    V(Float) \
    V(String) \
    V(Array) \
    V(Map)
    
struct Type {
    enum ID {
    #define DEFINE_ENUM(name) k##name,
        DECL_DAG_TYPES(DEFINE_ENUM)
    #undef DEFINE_ENUM
    };
}; // struct Type
    
class Value;
class Function;
class BasicBlock;
class Unary;
class Binary;
#define DEFINE_DECL(name) class name;
    DECL_DAG_NODES(DEFINE_DECL)
#undef DEFINE_DECL
    

// https://www.oracle.com/technetwork/java/javase/tech/c2-ir95-150110.pdf
class DAGNode {
public:
    void *operator new (size_t size, base::Arena *arena) {
        return arena->Allocate(size);
    }
    
    void operator delete (void *p) = delete;
};
    
class Function : public DAGNode {
public:
    using ValueSet = base::ArenaVector<Value *>;
    using BasicBlockSet = base::ArenaVector<BasicBlock *>;
    
    DEF_PTR_GETTER(base::Arena, arena);
    
    inline BasicBlock *NewBB(BasicBlock *in_edge);
    inline Value *Parameter(Type::ID type, int line);
    inline Value *Constant(BasicBlock *bb, Type::ID type, int line);
    inline Value *IMinus(BasicBlock *bb, Value *operand, int line);
    inline Value *IAdd(BasicBlock *bb, Value *lhs, Value *rhs, int line);
    inline Value *ISub(BasicBlock *bb, Value *lhs, Value *rhs, int line);
    inline Value *Branch(BasicBlock *bb, Value *cond, int line);
    inline Value *Phi(BasicBlock *bb, Type::ID type, int line);
    
    static Function *New(base::Arena *arena) { return new (arena) Function(arena); }
    
    friend class Value;
private:
    Function(base::Arena *arena)
        : arena_(arena)
        , values_(arena) {}
    
    int NextValId() { return static_cast<int>(values_.size()); }
    int NextBBId() { return next_bb_id_++; }
    
    void AddValue(Value *v) { values_.push_back(DCHECK_NOTNULL(v)); }

    base::Arena *arena_;
    ValueSet values_;
    BasicBlock *entry_ = nullptr;
    int next_bb_id_ = 0;
}; // class Function
    
class BasicBlock : public DAGNode {
public:
    using EdgeSet = base::ArenaVector<BasicBlock *>;
    
    DEF_VAL_GETTER(int, label);
    DEF_PTR_GETTER(Value, root);
    DEF_PTR_GETTER(Value, last);
    DEF_VAL_PROP_RMW(EdgeSet, in_edges);
    DEF_VAL_PROP_RMW(EdgeSet, out_edges);
    
    friend class Value;
    friend class Function;
private:
    BasicBlock(int label, base::Arena *arena)
        : label_(label)
        , in_edges_(arena)
        , out_edges_(arena) {}
    
    inline void AddValue(Value *v);

    int label_;
    EdgeSet in_edges_;
    EdgeSet out_edges_;
    Value *root_ = nullptr;
    Value *last_ = nullptr;
}; // class BasicBlock

class Value : public DAGNode {
public:
    enum Kind {
    #define DEFINE_ENUM(name) k##name,
        DECL_DAG_NODES(DEFINE_ENUM)
    #undef DEFINE_ENUM
    };

    DEF_VAL_GETTER(Type::ID, type);
    DEF_VAL_GETTER(int, index);
    DEF_VAL_GETTER(int, line);
    DEF_PTR_PROP_RW(Value, next);
    
#define DEFINE_IS(name) bool Is##name() const { return type_ == Type::k##name; }
    DECL_DAG_TYPES(DEFINE_IS)
#undef DEFINE_IS
    
    virtual Kind kind() const = 0;

protected:
    Value(Function *top, BasicBlock *bb, Type::ID type, int line)
        : type_(type)
        , index_(top->NextValId())
        , line_(line) {
            top->AddValue(this);
            if (bb) {
                bb->AddValue(this);
            }
        }
    
    Type::ID type_;
    int index_;
    int line_;
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
    DEFINE_DAG_INST_NODE(Constant);
private:
    Constant(Function *top, BasicBlock *bb, Type::ID type, int line)
        : Value(top, bb, type, line) {}
}; // class Constant
    
class Parameter : public Value {
public:
    DEFINE_DAG_INST_NODE(Parameter);
private:
    Parameter(Function *top, BasicBlock *bb, Type::ID type, int line)
        : Value(top, bb, type, line) {}
}; // class Argument
    
class Unary : public Value {
public:
    DEF_PTR_PROP_RW(Value, operand);
protected:
    Unary(Function *top, BasicBlock *bb, Type::ID type, Value *operand, int line)
        : Value(top, bb, type, line)
        , operand_(DCHECK_NOTNULL(operand)) {}
private:
    Value *operand_;
}; // class Unary

class Binary : public Value {
public:
    DEF_PTR_PROP_RW(Value, lhs);
    DEF_PTR_PROP_RW(Value, rhs);
protected:
    Binary(Function *top, BasicBlock *bb, Type::ID type, Value *lhs, Value *rhs, int line)
        : Value(top, bb, type, line)
        , lhs_(DCHECK_NOTNULL(lhs))
        , rhs_(DCHECK_NOTNULL(rhs)) {}
private:
    Value *lhs_;
    Value *rhs_;
}; // class Binary
    
class IMinus : public Unary {
public:
    DEFINE_DAG_INST_NODE(IMinus);
private:
    IMinus(Function *top, BasicBlock *bb, Value *operand, int line)
        : Unary(top, bb, operand->type(), operand, line) {
        DCHECK(operand->IsInt());
    }
}; // class IMinus

class IAdd : public Binary {
public:
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
    
    DEFINE_DAG_INST_NODE(Branch);
private:
    Branch(Function *top, BasicBlock *bb, Value *cond, int line)
        : Value(top, bb, Type::kVoid, line) {
    }
    
    Value *cond_;
    BasicBlock *if_true_ = nullptr;
    BasicBlock *if_false_ = nullptr;
}; // class Branch
    
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
    }
    
    inline Value *GetIncomingValue(BasicBlock *bb);
    
    DEFINE_DAG_INST_NODE(Phi);
private:
    Phi(Function *top, BasicBlock *bb, Type::ID type, int line)
        : Value(top, bb, type, line)
        , incoming_(top->arena()) {}
    
    PathSet incoming_;
}; // class Phi

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Inline Functions:
////////////////////////////////////////////////////////////////////////////////////////////////////

inline BasicBlock *Function::NewBB(BasicBlock *in_edge) {
    BasicBlock *bb = new (arena_) BasicBlock(NextBBId(), arena_);
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
    return new (arena_) class Parameter(this, nullptr, type, line);
}
    
inline Value *Function::Constant(BasicBlock *bb, Type::ID type, int line) {
    return new (arena_) class Constant(this, bb, type, line);
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
    
inline Value *Function::Branch(BasicBlock *bb, Value *cond, int line) {
    return new (arena_) class Branch(this, bb, cond, line);
}

inline Value *Function::Phi(BasicBlock *bb, Type::ID type, int line) {
    return new (arena_) class Phi(this, bb, type, line);
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

} // namespace dag
    
} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_IR_DAG_IR_H_
