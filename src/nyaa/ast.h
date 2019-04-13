#ifndef MAI_NYAA_AST_AST_H_
#define MAI_NYAA_AST_AST_H_

#include "nyaa/nyaa-values.h"
#include "nyaa/builtin.h"
#include "base/arena-utils.h"
#include "base/arena.h"
#include "base/base.h"
#include "mai-lang/handles.h"

struct NYAA_YYLTYPE;

namespace mai {
    
namespace nyaa {
class Object;
namespace ast {
    
#define DECL_AST_NODES(V) \
    V(Block) \
    V(VarDeclaration) \
    V(Assignment) \
    V(Multiple) \
    V(StringLiteral) \
    V(ApproxLiteral) \
    V(SmiLiteral) \
    V(IntLiteral) \
    V(LambdaLiteral) \
    V(Variable) \
    V(Index) \
    V(DotField) \
    V(Call) \
    V(SelfCall) \
    V(FunctionDefinition) \
    V(Return)

class Factory;
class Statement;
class Expression;
class LValue;
#define DEFINE_PRE_DECL(name) class name;
DECL_AST_NODES(DEFINE_PRE_DECL)
#undef DEFINE_PRE_DECL
    
using String = base::ArenaString;
    
struct Location {
    int begin_line   = 0;
    int begin_column = 0;
    int end_line     = 0;
    int end_column   = 0;
    
    Location() {}
    Location(const NYAA_YYLTYPE &yyl);

    static Location Concat(const NYAA_YYLTYPE &first, const NYAA_YYLTYPE &last);
};
    
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
    
#define DEFINE_METHOD(name) virtual IVal Visit##name(name *node, VisitorContext *) { return IVal::Void(); }
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
    
    bool IsInvoke() const { return  IsCall() || IsSelfCall(); }
    
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
    using NameList = base::ArenaVector<const String *>;
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
    
class Assignment : public Statement {
public:
    using LValList = base::ArenaVector<LValue *>;
    using RValList = base::ArenaVector<Expression *>;
    
    DEF_PTR_GETTER_NOTNULL(LValList, lvals);
    DEF_PTR_GETTER_NOTNULL(RValList, rvals);
    DEFINE_AST_NODE(Assignment);
private:
    Assignment(int line, LValList *lvals, RValList *rvals)
        : Statement(line)
        , lvals_(DCHECK_NOTNULL(lvals))
        , rvals_(DCHECK_NOTNULL(rvals)) {}
    
    LValList *lvals_;
    RValList *rvals_;
}; // class Assignment

class Return : public Statement {
public:
    using ExprList = base::ArenaVector<Expression *>;
    
    DEF_PTR_GETTER(ExprList, rets);
    
    inline int GetNRets() const;

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
    
    
class Expression : public Statement {
public:
    virtual bool is_expression() const override { return true; }
    virtual bool is_literal() const { return false; }
    virtual bool is_lval() const { return false; }
    virtual bool is_rval() const { return true; }
    
protected:
    Expression(int line) : Statement(line) {}
}; // class Expression
    
    
class LValue : public Expression {
public:
    virtual bool is_lval() const { return true; }
protected:
    LValue(int line) : Expression(line) {}
}; // class LVal
    
    
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
    
    
class SmiLiteral : public Literal<int64_t> {
public:
    DEFINE_AST_NODE(SmiLiteral);
private:
    SmiLiteral(int line, int64_t value) : Literal<int64_t>(line, value) {}
}; // class SmiLiteral


class IntLiteral : public Literal<const String *> {
public:
    DEFINE_AST_NODE(IntLiteral);
private:
    IntLiteral(int line, const String *value) : Literal<const String *>(line, value) {}
}; // class SmiLiteral
    
class LambdaLiteral : public Literal<Block *> {
public:
    using ParameterList = base::ArenaVector<const String *>;
    
    DEF_VAL_GETTER(int, end_line);
    DEF_VAL_GETTER(bool, vargs);
    DEF_PTR_GETTER(ParameterList, params);
    
    DEFINE_AST_NODE(LambdaLiteral);
private:
    LambdaLiteral(int begin_line, int end_line, ParameterList *params, bool vargs, Block *body)
        : Literal<Block*>(begin_line, body)
        , end_line_(end_line)
        , vargs_(vargs)
        , params_(params) {}
    
    int end_line_;
    bool vargs_;
    ParameterList *params_;
}; // class LambdaLiteral
    
class Call : public Expression {
public:
    using ArgumentList = base::ArenaVector<Expression *>;
    
    DEF_PTR_GETTER_NOTNULL(Expression, callee);
    DEF_PTR_GETTER(ArgumentList, args);

    inline int GetNArgs() const;

    DEFINE_AST_NODE(Call);
protected:
    Call(int line, Expression *callee, ArgumentList *args)
        : Expression(line)
        , callee_(DCHECK_NOTNULL(callee))
        , args_(args) {}
    
private:
    Expression *callee_;
    ArgumentList *args_;
}; // class Call
    
class SelfCall : public Call {
public:
    DEF_PTR_GETTER_NOTNULL(const String, method);
    DEFINE_AST_NODE(SelfCall);
private:
    SelfCall(int line, Expression *callee, const String *method, ArgumentList *args)
        : Call(line, callee, args)
        , method_(DCHECK_NOTNULL(method)) {}

    const String *method_;
}; // class SelfCall
    
class Variable : public LValue {
public:
    DEF_PTR_GETTER_NOTNULL(const String, name);
    DEFINE_AST_NODE(Variable);
private:
    Variable(int line, const String *name)
        : LValue(line)
        , name_(DCHECK_NOTNULL(name)) {}
    
    const String *name_;
}; // class Variable


template<class T>
class GenericIndex : public LValue {
public:
    DEF_PTR_GETTER_NOTNULL(Expression, self);
    DEF_VAL_GETTER(T, index);
    DISALLOW_IMPLICIT_CONSTRUCTORS(GenericIndex);
protected:
    GenericIndex(int line, Expression *self, T index)
        : LValue(line)
        , self_(DCHECK_NOTNULL(self))
        , index_(index) {
    }

    Expression *self_;
    T index_;
}; // class Index

class Index : public GenericIndex<Expression *> {
public:
    DEFINE_AST_NODE(Index);
private:
    Index(int line, Expression *self, Expression *index)
        : GenericIndex<Expression *>(line, self, index) {}
}; // class Index
    
class DotField : public GenericIndex<const String *> {
public:
    DEFINE_AST_NODE(DotField);
private:
    DotField(int line, Expression *self, const String *index)
        : GenericIndex<const String *>(line, self, index) {}
}; // class DotField

class FunctionDefinition : public Statement {
public:
    DEF_PTR_GETTER(Expression, self);
    DEF_PTR_GETTER(const String, name);
    DEF_PTR_GETTER(LambdaLiteral, literal);

    DEFINE_AST_NODE(FunctionDefinition);
private:
    FunctionDefinition(int line, Expression *self, const String *name, LambdaLiteral *literal)
        : Statement(line)
        , self_(self)
        , name_(name)
        , literal_(literal) {}

    Expression *self_; // def foo.name(a,b) { retunr 1 }
    const String *name_;
    LambdaLiteral *literal_; // lambda (a,b) { return 1 }
}; // class FunctionDefinition
    
inline int Return::GetNRets() const {
    if (!rets_) {
        return 0;
    }
    if (rets_->size() == 1 && rets_->at(0)->IsInvoke()) {
        return -1;
    } else {
        return static_cast<int>(rets_->size());
    }
}
    
inline int Call::GetNArgs() const {
    if (!args_) {
        return 0;
    }
    if (args_->size() == 1 && args_->at(0)->IsInvoke()) {
        return -1;
    } else {
        return static_cast<int>(args_->size());
    }
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// Factory
////////////////////////////////////////////////////////////////////////////////////////////////////
class Factory final {
public:
    Factory(base::Arena *arena)
        : arena_(DCHECK_NOTNULL(arena)) {}

    Block *NewBlock(Block::StmtList *stmts, const Location &loc = Location{}) {
        return new (arena_) Block(loc.begin_line, loc.end_line, stmts);
    }

    VarDeclaration *NewVarDeclaration(VarDeclaration::NameList *names,
                                      VarDeclaration::ExprList *inits,
                                      const Location &loc = Location{}) {
        return new (arena_) VarDeclaration(loc.begin_line, names, inits);
    }
    
    FunctionDefinition *NewFunctionDefinition(Expression *self, const String *name,
                                              LambdaLiteral *literal,
                                              const Location &loc = Location{}) {
        return new (arena_) FunctionDefinition(loc.begin_line, self, name, literal);
    }
    
    Assignment *NewAssignment(Assignment::LValList *lvals, Assignment::RValList *rvals,
                              const Location &loc = Location{}) {
        return new (arena_) Assignment(loc.begin_line, lvals, rvals);
    }

    StringLiteral *NewStringLiteral(const String *val, const Location &loc = Location{}) {
        return new (arena_) StringLiteral(loc.begin_line, val);
    }

    ApproxLiteral *NewApproxLiteral(f64_t val, const Location &loc = Location{}) {
        return new (arena_) ApproxLiteral(loc.begin_line, val);
    }

    SmiLiteral *NewSmiLiteral(int64_t val, const Location &loc = Location{}) {
        return new (arena_) SmiLiteral(loc.begin_line, val);
    }

    IntLiteral *NewIntLiteral(const String *val, const Location &loc = Location{}) {
        return new (arena_) IntLiteral(loc.begin_line, val);
    }

    LambdaLiteral *NewLambdaLiteral(LambdaLiteral::ParameterList *params, bool vargs,
                                    Block *body, const Location &loc = Location{}) {
        return new (arena_) LambdaLiteral(loc.begin_line, loc.end_line, params, vargs, body);
    }

    Variable *NewVariable(const String *name, const Location &loc = Location{}) {
        return new (arena_) Variable(loc.begin_line, name);
    }

    Call *NewCall(Expression *callee, Call::ArgumentList *args, const Location &loc = Location{}) {
        return new (arena_) Call(loc.begin_line, callee, args);
    }
    
    SelfCall *NewSelfCall(Expression *callee, const String *method, Call::ArgumentList *args,
                          const Location &loc = Location{}) {
        return new (arena_) SelfCall(loc.begin_line, callee, method, args);
    }

    Return *NewReturn(Return::ExprList *rets, const Location &loc = Location{}) {
        return new (arena_) Return(loc.begin_line, rets);
    }

    Multiple *NewUnary(Operator::ID op, Expression *operand, const Location &loc = Location{}) {
        return new (arena_) Multiple(loc.begin_line, op, operand);
    }

    Multiple *NewBinary(Operator::ID op, Expression *lhs, Expression *rhs,
                        const Location &loc = Location{}) {
        return new (arena_) Multiple(loc.begin_line, op, lhs, rhs);
    }
    
    Index *NewIndex(Expression *self, Expression *index, const Location &loc = Location{}) {
        return new (arena_) Index(loc.begin_line, self, index);
    }
    
    DotField *NewDotField(Expression *self, const String *index, const Location &loc = Location{}) {
        return new (arena_) DotField(loc.begin_line, self, index);
    }

    String *NewString(const char *z, int quote) {
        if (quote == 0) {
            return String::New(arena_, z);
        }
        size_t len = ::strlen(z);
        return String::New(arena_, z + 1, len - 2);
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
