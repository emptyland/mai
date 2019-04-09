#ifndef MAI_NYAA_AST_AST_H_
#define MAI_NYAA_AST_AST_H_

#include "nyaa/nyaa-values.h"
#include "nyaa/builtin.h"
#include "base/arena-utils.h"
#include "base/arena.h"
#include "base/base.h"
#include "mai-lang/handles.h"

namespace mai {
    
namespace nyaa {
class Object;
namespace ast {
    
#define DECL_AST_NODES(V) \
    V(Block) \
    V(VarDeclaration) \
    V(Multiple) \
    V(StringLiteral) \
    V(ApproxLiteral) \
    V(Variable) \
    V(Call) \
    V(Return)

class Factory;
class Statement;
class Expression;
#define DEFINE_PRE_DECL(name) class name;
DECL_AST_NODES(DEFINE_PRE_DECL)
#undef DEFINE_PRE_DECL
    
using String = base::ArenaString;
    
class VisitorContext {
public:
    VisitorContext() {}
#if defined(DEBUG) || defined(_DEBUG)
    virtual ~VisitorContext() {}
#else
    ~VisitorContext() {}
#endif
}; // class VisitorContext
    
class Visitor {
public:
    Visitor() {}
    virtual ~Visitor() {}
    
#define DEFINE_METHOD(name) virtual IVal Visit##name(name *node, VisitorContext *) { return IVal::None(); }
    DECL_AST_NODES(DEFINE_METHOD);
#undef DEFINE_METHOD
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Visitor);
}; // class AstVisitor

class AstNode {
public:
    enum Kind {
#define DEFINE_ENUM(name) k##name,
        DECL_AST_NODES(DEFINE_ENUM)
#undef DEFINE_ENUM
    };

    virtual Kind kind() const = 0;
    virtual int line() const = 0;
    //virtual int end_line() const = 0;
    virtual bool is_statement() const = 0;
    virtual bool is_expression() const = 0;
    //virtual Object *Emit() const = 0;
    virtual IVal Accept(Visitor *, VisitorContext *) = 0;
    
#define DEFINE_TYPE_CHECK(name) bool Is##name() const { return kind() == k##name; }
    DECL_AST_NODES(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK
    
#define DEFINE_CAST_METHOD(name) \
    name *To##name() { return !Is##name() ? nullptr : reinterpret_cast<name *>(this); } \
    const name *To##name() const { return !Is##name() ? nullptr : reinterpret_cast<const name *>(this); }
    DECL_AST_NODES(DEFINE_CAST_METHOD)
#undef DEFINE_CAST_METHOD
    
    void *operator new (size_t size, base::Arena *arena) {
        return arena->Allocate(size);
    }
    
    void operator delete (void *p) = delete;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(AstNode);
protected:
    AstNode() {}
}; // class AstNode
    
#define DEFINE_AST_NODE(name) \
    virtual Kind kind() const override { return k##name; } \
    virtual IVal Accept(Visitor *v, VisitorContext *ctx) override { return v->Visit##name(this, ctx); } \
    friend class Factory; \
    DISALLOW_IMPLICIT_CONSTRUCTORS(name)
    

class Statement : public AstNode {
public:
    virtual int line() const override { return line_; }
    //virtual int end_line() const override { return line_; }
    virtual bool is_statement() const override { return true; }
    virtual bool is_expression() const override { return false; }
    //virtual Object *Emit() const override { return Object::kNil; }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Statement);
protected:
    Statement(int line) : line_(line) {}
    
private:
    int line_;
}; // class Statement
    
class VarDeclaration : public Statement {
public:
    using NameList = base::ArenaVector<String *>;
    using ExprList = base::ArenaVector<Expression *>;
    
    DEF_PTR_GETTER_NOTNULL(NameList, names);
    DEF_PTR_GETTER(ExprList, inits);
    DEFINE_AST_NODE(VarDeclaration);
private:
    VarDeclaration(int line, NameList *names, ExprList *inits)
        : Statement(line)
        , names_(DCHECK_NOTNULL(names))
        , inits_(inits) {}

    NameList *names_;
    ExprList *inits_;
}; // class VarDeclaration

class Expression : public Statement {
public:
    virtual bool is_expression() const override { return true; }
    virtual bool is_literal() const { return false; }

protected:
    Expression(int line) : Statement(line) {}
}; // class Expression


class Return : public Statement {
public:
    using ExprList = base::ArenaVector<Expression *>;
    
    DEF_PTR_GETTER(ExprList, rets);
    
    int GetNRets() const {
        if (!rets_) {
            return 0;
        }
        if (rets_->size() == 1 && rets_->at(0)->IsCall()) {
            return -1;
        } else {
            return static_cast<int>(rets_->size());
        }
    }
    DEFINE_AST_NODE(Return);
private:
    Return(int line, ExprList *rets)
    : Statement(line)
    , rets_(rets) {}
    
    ExprList *rets_;
}; // class Return
    
    
class Block : public Statement {
public:
    using StmtList = base::ArenaVector<Statement *>;
    
    int end_line() const { return end_line_; }
    DEF_PTR_GETTER_NOTNULL(StmtList, stmts);
    
    DEFINE_AST_NODE(Block);
private:
    Block(int begin_line, int end_line, StmtList *stmts)
        : Statement(begin_line)
        , end_line_(end_line)
        , stmts_(DCHECK_NOTNULL(stmts)) {}

    int end_line_;
    StmtList *stmts_;
}; // class Block
    
    
class Multiple : public Expression {
public:
    static constexpr const int kLimitOperands = 5;
    
    DEF_VAL_GETTER(Operator::ID, op);
    DEF_VAL_GETTER(int, n_operands);
    
    Expression *operand(int i) const {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, n_operands_);
        return operands_[i];
    }
    
    Expression *operand() const { return DCHECK_NOTNULL(operand(0)); }
    Expression *lhs() const { return DCHECK_NOTNULL(operand(0)); }
    Expression *rhs() const { return DCHECK_NOTNULL(operand(1)); }
    
    DEFINE_AST_NODE(Multiple);
private:
    Multiple(int line, Operator::ID op, Expression *expr)
        : Expression(line)
        , op_(op)
        , n_operands_(1) {
        ::memset(operands_, 0, sizeof(Expression *) * kLimitOperands);
        operands_[0] = expr;
    }
    
    Multiple(int line, Operator::ID op, Expression *e1, Expression *e2)
        : Expression(line)
        , op_(op)
        , n_operands_(2) {
        ::memset(operands_, 0, sizeof(Expression *) * kLimitOperands);
        operands_[0] = e1;
        operands_[1] = e2;
    }
    
    Operator::ID op_;
    int n_operands_;
    Expression *operands_[kLimitOperands];
};

template<class T>
class Literal : public Expression {
public:
    DEF_VAL_GETTER(T, value);
    virtual bool is_literal() const { return true; }
    
protected:
    Literal(int line, T value) : Expression(line), value_(value) {}
    T value_;
}; // class Literal
    
    
class StringLiteral : public Literal<const String *> {
public:
    DEFINE_AST_NODE(StringLiteral);
private:
    StringLiteral(int line, const String *value) : Literal<const String *>(line, value) {}
}; // class StringLiteral


class ApproxLiteral : public Literal<f64_t> {
public:
    DEFINE_AST_NODE(ApproxLiteral);
private:
    ApproxLiteral(int line, f64_t value) : Literal<f64_t>(line, value) {}
}; // class ApproxLiteral

    
class Variable : public Expression {
public:
    DEF_PTR_GETTER_NOTNULL(const String, name);
    DEFINE_AST_NODE(Variable);
private:
    Variable(int line, const String *name)
        : Expression(line)
        , name_(DCHECK_NOTNULL(name)) {}

    const String *name_;
}; // class Variable
    
class Call : public Expression {
public:
    using ArgumentList = base::ArenaVector<Expression *>;
    
    DEF_PTR_GETTER_NOTNULL(Expression, callee);
    DEF_PTR_GETTER(ArgumentList, args);
    
    int GetNArgs() const {
        if (!args_) {
            return 0;
        }
        if (args_->size() == 1 && args_->at(0)->IsCall()) {
            return -1;
        } else {
            return static_cast<int>(args_->size());
        }
    }
    DEFINE_AST_NODE(Call);
private:
    Call(int line, Expression *callee, ArgumentList *args)
        : Expression(line)
        , callee_(DCHECK_NOTNULL(callee))
        , args_(args) {}
    
    Expression *callee_;
    ArgumentList *args_;
}; // class Call
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// Factory
////////////////////////////////////////////////////////////////////////////////////////////////////
class Factory final {
public:
    Factory(base::Arena *arena)
        : arena_(DCHECK_NOTNULL(arena)) {}
    
    Block *NewBlock(Block::StmtList *stmts) {
        return new (arena_) Block(0, 0, stmts);
    }
    
    VarDeclaration *NewVarDeclaration(VarDeclaration::NameList *names,
                                      VarDeclaration::ExprList *inits) {
        return new (arena_) VarDeclaration(0, names, inits);
    }
    
    StringLiteral *NewStringLiteral(const String *val) {
        return new (arena_) StringLiteral(0, val);
    }
    
    ApproxLiteral *NewApproxLiteral(f64_t val) {
        return new (arena_) ApproxLiteral(0, val);
    }
    
    Variable *NewVariable(const String *name) {
        return new (arena_) Variable(0, name);
    }
    
    Call *NewCall(Expression *callee, Call::ArgumentList *args) {
        return new (arena_) Call(0, callee, args);
    }
    
    Return *NewReturn(Return::ExprList *rets) {
        return new (arena_) Return(0, rets);
    }
    
    Multiple *NewUnary(Operator::ID op, Expression *operand) {
        return new (arena_) Multiple(0, op, operand);
    }
    
    Multiple *NewBinary(Operator::ID op, Expression *lhs, Expression *rhs) {
        return new (arena_) Multiple(0, op, lhs, rhs);
    }

    template<class T>
    base::ArenaVector<T> *NewList(T init) {
        auto rv = new (arena_) base::ArenaVector<T>(arena_);
        if (init) {
            rv->push_back(init);
        }
        return rv;
    }
private:
    base::Arena *arena_;
}; // class Factory
    
} //namespace ast

} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_AST_AST_H_
