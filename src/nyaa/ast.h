#ifndef MAI_NYAA_AST_AST_H_
#define MAI_NYAA_AST_AST_H_

#include "nyaa/nyaa-values.h"
#include "nyaa/builtin.h"
#include "base/arena-utils.h"
#include "base/arena.h"
#include "base/slice.h"
#include "base/base.h"
#include "mai-lang/handles.h"

struct NYAA_YYLTYPE;

namespace mai {
    
namespace nyaa {
class Object;
namespace ast {
    
#define DECL_AST_NODES(V) \
    V(Block) \
    V(ClassDefinition) \
    V(ObjectDefinition) \
    V(VarDeclaration) \
    V(PropertyDeclaration) \
    V(Assignment) \
    V(IfStatement) \
    V(WhileLoop) \
    V(ForIterateLoop) \
    V(ForStepLoop) \
    V(Multiple) \
    V(LogicSwitch) \
    V(Concat) \
    V(StringLiteral) \
    V(ApproxLiteral) \
    V(SmiLiteral) \
    V(IntLiteral) \
    V(NilLiteral) \
    V(LambdaLiteral) \
    V(MapInitializer) \
    V(VariableArguments) \
    V(Variable) \
    V(Index) \
    V(DotField) \
    V(Call) \
    V(SelfCall) \
    V(New) \
    V(FunctionDefinition) \
    V(Continue) \
    V(Break) \
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
    static name *Cast(AstNode *node) { return !node ? nullptr : node->To##name(); } \
    static const name *Cast(const AstNode *node) { return !node ? nullptr : node->To##name(); } \
    friend class Factory; \
    DISALLOW_IMPLICIT_CONSTRUCTORS(name)
    
using Attributes = base::ArenaVector<const String *>;

class Statement : public AstNode {
public:
    virtual int line() const override { return line_; }
    virtual bool is_statement() const override { return true; }
    virtual bool is_expression() const override { return false; }
    
    static Statement *Cast(AstNode *node) {
        return (!node || !node->is_statement()) ? nullptr : static_cast<Statement *>(node);
    }
    
    static const Statement *Cast(const AstNode *node) {
        return (!node || !node->is_statement()) ? nullptr : static_cast<const Statement *>(node);
    }
    
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
    inline int GetNWanted() const;
    DEFINE_AST_NODE(VarDeclaration);
protected:
    VarDeclaration(int line, NameList *names, ExprList *inits)
        : Statement(line)
        , names_(DCHECK_NOTNULL(names))
        , inits_(inits) {}
private:
    NameList *names_;
    ExprList *inits_;
}; // class VarDeclaration
    
class Assignment : public Statement {
public:
    using LValList = base::ArenaVector<LValue *>;
    using RValList = base::ArenaVector<Expression *>;
    
    DEF_PTR_GETTER_NOTNULL(LValList, lvals);
    DEF_PTR_GETTER_NOTNULL(RValList, rvals);
    inline int GetNWanted() const;
    DEFINE_AST_NODE(Assignment);
private:
    Assignment(int line, LValList *lvals, RValList *rvals)
        : Statement(line)
        , lvals_(DCHECK_NOTNULL(lvals))
        , rvals_(DCHECK_NOTNULL(rvals)) {}
    
    LValList *lvals_;
    RValList *rvals_;
}; // class Assignment
    
class IfStatement : public Statement {
public:
    DEF_VAL_PROP_RW(int, trace_id);
    DEF_PTR_GETTER_NOTNULL(Expression, cond);
    DEF_PTR_GETTER_NOTNULL(Block, then_clause);
    DEF_PTR_GETTER(Statement, else_clause);
    DEFINE_AST_NODE(IfStatement);
private:
    IfStatement(int line, int trace_id, Expression *cond, Block *then_clause, Statement *else_clause)
        : Statement(line)
        , trace_id_(trace_id)
        , cond_(DCHECK_NOTNULL(cond))
        , then_clause_(DCHECK_NOTNULL(then_clause))
        , else_clause_(else_clause) {}

    int trace_id_;
    Expression *cond_;
    Block *then_clause_;
    Statement *else_clause_;
}; // class IfStatement
    
class Loop : public Statement {
public:
    DEF_PTR_GETTER_NOTNULL(Block, body);
    DEF_VAL_GETTER(int, end_line);
protected:
    Loop(int begin_line, int end_line, Block *body)
        : Statement(begin_line)
        , end_line_(end_line)
        , body_(DCHECK_NOTNULL(body)) {}
private:
    int end_line_;
    Block *body_;
}; // class Loop
    
class WhileLoop : public Loop {
public:
    DEF_PTR_GETTER_NOTNULL(Expression, cond);
    DEFINE_AST_NODE(WhileLoop);
private:
    WhileLoop(int begin_line, int end_line, Expression *cond, Block *body)
        : Loop(begin_line, end_line, body)
        , cond_(DCHECK_NOTNULL(cond)) {}
    Expression *cond_;
}; // class WhileLoop
    
class ForIterateLoop : public Loop {
public:
    using NameList = base::ArenaVector<const String *>;
    DEF_VAL_GETTER(int, trace_id);
    DEF_PTR_GETTER_NOTNULL(NameList, names);
    DEF_PTR_GETTER_NOTNULL(Expression, init);
    DEFINE_AST_NODE(ForIterateLoop);
private:
    ForIterateLoop(int begin_line, int end_line, int trace_id, NameList *names, Expression *init,
                   Block *body)
        : Loop(begin_line, end_line, body)
        , trace_id_(trace_id)
        , names_(DCHECK_NOTNULL(names))
        , init_(DCHECK_NOTNULL(init)) {}
    
    int trace_id_;
    NameList *names_;
    Expression *init_;
}; // class ForIterateLoop
    
class ForStepLoop : public Loop {
public:
    int trace_id_base() const { return trace_id1(); }
    DEF_VAL_GETTER(int, trace_id1);
    DEF_VAL_GETTER(int, trace_id2);
    DEF_PTR_GETTER_NOTNULL(const ast::String, name);
    DEF_VAL_GETTER(bool, is_until);
    DEF_PTR_GETTER_NOTNULL(Expression, init);
    DEF_PTR_GETTER_NOTNULL(Expression, limit);
    DEF_PTR_GETTER(Expression, step);
    DEFINE_AST_NODE(ForStepLoop);
private:
    ForStepLoop(int begin_line, int end_line, int trace_id1, int trace_id2, const ast::String *name,
                Expression *init, bool is_until, Expression *limit, Expression *step, Block *body)
        : Loop(begin_line, end_line, body)
        , trace_id1_(trace_id1)
        , trace_id2_(trace_id2)
        , name_(DCHECK_NOTNULL(name))
        , init_(DCHECK_NOTNULL(init))
        , is_until_(is_until)
        , limit_(DCHECK_NOTNULL(limit))
        , step_(step) {}
    
    int trace_id1_;
    int trace_id2_;
    const ast::String *name_;
    Expression *init_;
    bool is_until_;
    Expression *limit_;
    Expression *step_;
}; // class ForStepLoop
    
class Continue : public Statement {
public:
    DEFINE_AST_NODE(Continue);
private:
    Continue(int line) : Statement(line) {}
}; // class Continue
    
class Break : public Statement {
public:
    DEFINE_AST_NODE(Break);
private:
    Break(int line) : Statement(line) {}
}; // class Break

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
    DEF_PTR_GETTER(StmtList, stmts);
    
    DEFINE_AST_NODE(Block);
private:
    Block(int begin_line, int end_line, StmtList *stmts)
        : Statement(begin_line)
        , end_line_(end_line)
        , stmts_(stmts) {}

    int end_line_;
    StmtList *stmts_;
}; // class Block
    
    
class Expression : public Statement {
public:
    virtual bool is_expression() const override { return true; }
    virtual bool is_literal() const { return false; }
    virtual bool is_lval() const { return false; }
    virtual bool is_rval() const { return true; }
    
    static Expression *Cast(AstNode *node) {
        return (!node || !node->is_expression()) ? nullptr : static_cast<Expression *>(node);
    }
    
    static const Expression *Cast(const AstNode *node) {
        return (!node || !node->is_expression()) ? nullptr : static_cast<const Expression *>(node);
    }
    
protected:
    Expression(int line) : Statement(line) {}
}; // class Expression
    
    
class LValue : public Expression {
public:
    virtual bool is_lval() const { return true; }
    
    static LValue *Cast(AstNode *node) {
        Expression *expr = Expression::Cast(node);
        return (!expr || !expr->is_lval()) ? nullptr : static_cast<LValue *>(node);
    }
    static const LValue *Cast(const AstNode *node) {
        const Expression *expr = Expression::Cast(node);
        return (!expr || !expr->is_lval()) ? nullptr : static_cast<const LValue *>(node);
    }
protected:
    LValue(int line) : Expression(line) {}
}; // class LVal
    
    
class Multiple : public Expression {
public:
    static constexpr const int kLimitOperands = 4;
    
    DEF_VAL_GETTER(int, trace_id);
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
    Expression *key() const { return operand(0); }
    Expression *value() const { return rhs(); }
    
    DEFINE_AST_NODE(Multiple);
protected:
    Multiple(int line, int trace_id, Operator::ID op, Expression *expr)
        : Expression(line)
        , op_(op)
        , n_operands_(1) {
        ::memset(operands_, 0, sizeof(Expression *) * kLimitOperands);
        operands_[0] = expr;
    }
    
    Multiple(int line, int trace_id, Operator::ID op, Expression *e1, Expression *e2)
        : Expression(line)
        , op_(op)
        , n_operands_(2) {
        ::memset(operands_, 0, sizeof(Expression *) * kLimitOperands);
        operands_[0] = e1;
        operands_[1] = e2;
    }

private:
    int trace_id_;
    Operator::ID op_;
    int n_operands_;
    Expression *operands_[kLimitOperands];
}; // class Multiple
    
class LogicSwitch : public Multiple {
public:
    DEFINE_AST_NODE(LogicSwitch);
private:
    LogicSwitch(int line, Operator::ID op, Expression *e1, Expression *e2)
        : Multiple(line, 0, op, e1, e2) {}
}; // class LogicSwitch
    
class Concat : public Expression {
public:
    using OperandList = base::ArenaVector<Expression *>;
    DEF_PTR_GETTER_NOTNULL(OperandList, operands);
    DEFINE_AST_NODE(Concat);
private:
    Concat(int line, OperandList *operands)
        : Expression(line)
        , operands_(DCHECK_NOTNULL(operands)) {}
    OperandList *operands_;
}; // class Concat

template<class T>
class Literal : public Expression {
public:
    DEF_VAL_GETTER(T, value);
    virtual bool is_literal() const { return true; }
    
protected:
    Literal(int line, T value) : Expression(line), value_(value) {}
    T value_;
}; // class Literal
    
class NilLiteral : public Literal<void *> {
public:
    DEFINE_AST_NODE(NilLiteral);
private:
    NilLiteral(int line) : Literal<void *>(line, nullptr) {}
}; // class NilLiteral

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
    
class MapInitializer : public Literal<base::ArenaVector<Multiple *> *> {
public:
    using EntryList = base::ArenaVector<Multiple *>;
    
    DEF_VAL_GETTER(int, end_line);
    DEFINE_AST_NODE(MapInitializer);
private:
    MapInitializer(int begin_line, int end_line, EntryList *entries)
        : Literal<base::ArenaVector<Multiple *> *>(begin_line, entries)
        , end_line_(end_line) {}
    int end_line_;
}; // class MapLiteral
    
class Call : public Expression {
public:
    using ArgumentList = base::ArenaVector<Expression *>;
    
    DEF_VAL_GETTER(int, trace_id);
    DEF_PTR_GETTER_NOTNULL(Expression, callee);
    DEF_PTR_GETTER(ArgumentList, args);

    inline int GetNArgs() const;

    DEFINE_AST_NODE(Call);
protected:
    Call(int line, int trace_id, Expression *callee, ArgumentList *args)
        : Expression(line)
        , callee_(DCHECK_NOTNULL(callee))
        , args_(args)
        , trace_id_(trace_id) {}
    
private:
    int trace_id_;
    Expression *callee_;
    ArgumentList *args_;
}; // class Call
    
class SelfCall : public Call {
public:
    DEF_PTR_GETTER_NOTNULL(const String, method);
    DEFINE_AST_NODE(SelfCall);
private:
    SelfCall(int line, int trace_id, Expression *callee, const String *method, ArgumentList *args)
        : Call(line, trace_id, callee, args)
        , method_(DCHECK_NOTNULL(method)) {}

    const String *method_;
}; // class SelfCall
    
class New : public Call {
public:
    DEFINE_AST_NODE(New);
private:
    New(int line, int trace_id, Expression *callee, ArgumentList *args)
        : Call(line, trace_id, callee, args) {}
}; // class NewCall
    
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

class VariableArguments : public Expression {
public:
    DEFINE_AST_NODE(VariableArguments);
private:
    VariableArguments(int line) : Expression(line) {}
}; // class VariableArguments

template<class T>
class GenericIndex : public LValue {
public:
    DEF_VAL_GETTER(int, trace_id);
    DEF_PTR_GETTER_NOTNULL(Expression, self);
    DEF_VAL_GETTER(T, index);
    DISALLOW_IMPLICIT_CONSTRUCTORS(GenericIndex);
protected:
    GenericIndex(int line, int trace_id, Expression *self, T index)
        : LValue(line)
        , trace_id_(trace_id)
        , self_(DCHECK_NOTNULL(self))
        , index_(index) {
    }

    int trace_id_;
    Expression *self_;
    T index_;
}; // class Index

class Index : public GenericIndex<Expression *> {
public:
    DEFINE_AST_NODE(Index);
private:
    Index(int line, int trace_id, Expression *self, Expression *index)
        : GenericIndex<Expression *>(line, trace_id, self, index) {}
}; // class Index
    
class DotField : public GenericIndex<const String *> {
public:
    DEFINE_AST_NODE(DotField);
private:
    DotField(int line, int trace_id, Expression *self, const String *index)
        : GenericIndex<const String *>(line, trace_id, self, index) {}
}; // class DotField
    
class PropertyDeclaration : public VarDeclaration {
public:
    DEF_VAL_GETTER(bool, readonly);
    DEFINE_AST_NODE(PropertyDeclaration);
private:
    PropertyDeclaration(int line, NameList *names, ExprList *inits, bool readonly)
        : VarDeclaration(line, names, inits)
        , readonly_(readonly) {}
    bool readonly_;
}; // class PropertyDeclaration
    
class ObjectDefinition : public Statement {
public:
    using MemberList = base::ArenaVector<Statement *>;

    DEF_VAL_GETTER(int, end_line);
    DEF_PTR_GETTER_NOTNULL(const String, name);
    DEF_VAL_GETTER(bool, local);
    DEF_PTR_GETTER(MemberList, members);
    DEFINE_AST_NODE(ObjectDefinition);
protected:
    ObjectDefinition(int begin_line, int end_line, const String *name, bool local,
                     MemberList *members)
        : Statement(begin_line)
        , end_line_(end_line)
        , name_(DCHECK_NOTNULL(name))
        , local_(local)
        , members_(members) {}
private:
    int end_line_;
    const String *name_;
    bool local_;
    MemberList *members_;
}; // class ObjectDefinition
    
class ClassDefinition : public ObjectDefinition {
public:
    DEF_PTR_GETTER(Expression, base);
    DEFINE_AST_NODE(ClassDefinition);
private:
    ClassDefinition(int begin_line, int end_line, const String *name, bool local, Expression *base,
                    MemberList *members)
        : ObjectDefinition(begin_line, end_line, name, local, members)
        , base_(base) {}

    Expression *base_;
}; // class ClassDefinition

class FunctionDefinition : public Statement {
public:
    DEF_VAL_GETTER(int, trace_id);
    DEF_PTR_GETTER(Expression, self);
    DEF_PTR_GETTER(const String, name);
    DEF_PTR_GETTER(LambdaLiteral, literal);

    DEFINE_AST_NODE(FunctionDefinition);
private:
    FunctionDefinition(int line, int trace_id, Expression *self, const String *name,
                       LambdaLiteral *literal)
        : Statement(line)
        , trace_id_(trace_id)
        , self_(self)
        , name_(name)
        , literal_(literal) {}

    int trace_id_;
    Expression *self_; // def foo.name(a,b) { retunr 1 }
    const String *name_;
    LambdaLiteral *literal_; // lambda (a,b) { return 1 }
}; // class FunctionDefinition
    
inline int VarDeclaration::GetNWanted() const {
    if (!inits_) {
        return 0;
    }
    if (inits_->size() == 1 && (inits_->at(0)->IsInvoke() || inits_->at(0)->IsVariableArguments())) {
        return -1;
    } else {
        return static_cast<int>(inits_->size());
    }
}
    
inline int Assignment::GetNWanted() const {
    DCHECK_NOTNULL(rvals_);
    if (rvals_->size() == 1 && (rvals_->at(0)->IsInvoke() || rvals_->at(0)->IsVariableArguments())) {
        return -1;
    } else {
        return static_cast<int>(rvals_->size());
    }
}
    
inline int Return::GetNRets() const {
    if (!rets_) {
        return 0;
    }
    if (rets_->size() == 1 && (rets_->at(0)->IsInvoke() || rets_->at(0)->IsVariableArguments())) {
        return -1;
    } else {
        return static_cast<int>(rets_->size());
    }
}

inline int Call::GetNArgs() const {
    if (!args_) {
        return 0;
    }
    if (args_->size() == 1 && (args_->at(0)->IsInvoke() || args_->at(0)->IsVariableArguments())) {
        return -1;
    } else {
        return static_cast<int>(args_->size());
    }
}

inline bool IsPlaceholder(const String *name) {
    return !name ? false : (name->size() == 1 && name->data()[0] == '_');
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Factory
////////////////////////////////////////////////////////////////////////////////////////////////////
class Factory final {
public:
    Factory(base::Arena *arena) : arena_(DCHECK_NOTNULL(arena)) {}

    Block *NewBlock(Block::StmtList *stmts, const Location &loc = Location{}) {
        return new (arena_) Block(loc.begin_line, loc.end_line, stmts);
    }

    VarDeclaration *NewVarDeclaration(VarDeclaration::NameList *names,
                                      VarDeclaration::ExprList *inits,
                                      const Location &loc = Location{}) {
        return new (arena_) VarDeclaration(loc.begin_line, names, inits);
    }

    PropertyDeclaration *NewPropertyDeclaration(Attributes *attrs,
                                                VarDeclaration::NameList *names,
                                                VarDeclaration::ExprList *inits,
                                                const Location &loc = Location{}) {
        bool ro = AttributesMatch(attrs, "ro", true, false);
        ro = AttributesMatch(attrs, "rw", false, ro);
        return new (arena_) PropertyDeclaration(loc.begin_line, names, inits, ro);
    }

    PropertyDeclaration *NewPropertyDeclaration(VarDeclaration::NameList *names,
                                                VarDeclaration::ExprList *inits, bool readonly,
                                                const Location &loc = Location{}) {
        return new (arena_) PropertyDeclaration(loc.begin_line, names, inits, readonly);
    }

    FunctionDefinition *NewFunctionDefinition(int trace_id, Expression *self, const String *name,
                                              LambdaLiteral *literal,
                                              const Location &loc = Location{}) {
        return new (arena_) FunctionDefinition(loc.begin_line, trace_id, self, name, literal);
    }

    ObjectDefinition *NewObjectDefinition(Attributes *attrs, const String *name,
                                          ObjectDefinition::MemberList *members,
                                          const Location &loc = Location{}) {
        bool local = AttributesMatch(attrs, "local", true, false);
        return new (arena_) ObjectDefinition(loc.begin_line, loc.end_line, name, local, members);
    }

    ObjectDefinition *NewObjectDefinition(const String *name, bool local,
                                          ObjectDefinition::MemberList *members,
                                          const Location &loc = Location{}) {
        return new (arena_) ObjectDefinition(loc.begin_line, loc.end_line, name, local, members);
    }

    ClassDefinition *NewClassDefinition(Attributes *attrs, const String *name, Expression *base,
                                        ObjectDefinition::MemberList *members,
                                        const Location &loc = Location{}) {
        bool local = AttributesMatch(attrs, "local", true, false);
        return new (arena_) ClassDefinition(loc.begin_line, loc.end_line, name, local, base,
                                            members);
    }

    ClassDefinition *NewClassDefinition(const String *name, bool local,
                                        ObjectDefinition::MemberList *members,
                                        const Location &loc = Location{}) {
        return new (arena_) ClassDefinition(loc.begin_line, loc.end_line, name, local, nullptr,
                                            members);
    }

    Assignment *NewAssignment(Assignment::LValList *lvals, Assignment::RValList *rvals,
                              const Location &loc = Location{}) {
        return new (arena_) Assignment(loc.begin_line, lvals, rvals);
    }

    IfStatement *NewIfStatement(int trace_id, Expression *cond, Block *then_clause,
                                Statement *else_clause, const Location &loc = Location{}) {
        return new (arena_) IfStatement(loc.begin_line, trace_id, cond, then_clause, else_clause);
    }

    WhileLoop *NewWhileLoop(Expression *cond, Block *body, const Location &loc = Location{}) {
        return new (arena_) WhileLoop(loc.begin_line, loc.end_line, cond, body);
    }

    ForIterateLoop *NewForIterateLoop(int trace_id, ForIterateLoop::NameList *names,
                                      Expression *init, Block *body,
                                      const Location &loc = Location{}) {
        return new (arena_) ForIterateLoop(loc.begin_line, loc.end_line, trace_id, names, init, body);
    }

    ForStepLoop *NewForStepLoop(int *next_trace_id, const ast::String *name, Expression *init,
                                bool is_until, Expression *limit, Expression *step, Block *body,
                                const Location &loc = Location{}) {
        return new (arena_) ForStepLoop(loc.begin_line, loc.end_line, (*next_trace_id)++,
                                        (*next_trace_id)++, name, init, is_until, limit, step,
                                        body);
    }

    NilLiteral *NewNilLiteral(const Location &loc = Location{}) {
        return new (arena_) NilLiteral(loc.begin_line);
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

    MapInitializer *NewMapInitializer(MapInitializer::EntryList *entries,
                                      const Location &loc = Location{}) {
        return new (arena_) MapInitializer(loc.begin_line, loc.end_line, entries);
    }

    Variable *NewVariable(const String *name, const Location &loc = Location{}) {
        return new (arena_) Variable(loc.begin_line, name);
    }

    VariableArguments *NewVariableArguments(const Location &loc = Location{}) {
        return new (arena_) VariableArguments(loc.begin_line);
    }

    Call *NewCall(int trace_id, Expression *callee, Call::ArgumentList *args,
                  const Location &loc = Location{}) {
        return new (arena_) Call(loc.begin_line, trace_id, callee, args);
    }
    
    SelfCall *NewSelfCall(int trace_id, Expression *callee, const String *method,
                          Call::ArgumentList *args, const Location &loc = Location{}) {
        return new (arena_) SelfCall(loc.begin_line, trace_id, callee, method, args);
    }

    New *NewNew(int trace_id, Expression *callee, Call::ArgumentList *args,
                const Location &loc = Location{}) {
        return new (arena_) New(loc.begin_line, trace_id, callee, args);
    }

    Continue *NewContinue(const Location &loc = Location{}) {
        return new (arena_) Continue(loc.begin_line);
    }

    Break *NewBreak(const Location &loc = Location{}) {
        return new (arena_) Break(loc.begin_line);
    }

    Return *NewReturn(Return::ExprList *rets, const Location &loc = Location{}) {
        return new (arena_) Return(loc.begin_line, rets);
    }

    Multiple *NewUnary(int trace_id, Operator::ID op, Expression *operand,
                       const Location &loc = Location{}) {
        return new (arena_) Multiple(loc.begin_line, trace_id, op, operand);
    }

    Multiple *NewBinary(int trace_id, Operator::ID op, Expression *lhs, Expression *rhs,
                        const Location &loc = Location{}) {
        return new (arena_) Multiple(loc.begin_line, trace_id, op, lhs, rhs);
    }

    Multiple *NewEntry(Expression *key, Expression *value, const Location &loc = Location{}) {
        return new (arena_) Multiple(loc.begin_line, 0, Operator::kEntry, key, value);
    }

    LogicSwitch *NewAnd(Expression *lhs, Expression *rhs, const Location &loc = Location{}) {
        return new (arena_) LogicSwitch(loc.begin_line, Operator::kAnd, lhs, rhs);
    }

    LogicSwitch *NewOr(Expression *lhs, Expression *rhs, const Location &loc = Location{}) {
        return new (arena_) LogicSwitch(loc.begin_line, Operator::kOr, lhs, rhs);
    }

    Concat *NewConcat(Concat::OperandList *ops, const Location &loc = Location{}) {
        return new (arena_) Concat(loc.begin_line, ops);
    }

    Index *NewIndex(int trace_id, Expression *self, Expression *index,
                    const Location &loc = Location{}) {
        return new (arena_) Index(loc.begin_line, trace_id, self, index);
    }

    DotField *NewDotField(int trace_id, Expression *self, const String *index,
                          const Location &loc = Location{}) {
        return new (arena_) DotField(loc.begin_line, trace_id, self, index);
    }

    String *NewString(const char *z, int quote) {
        if (quote == 0) {
            return String::New(arena_, z);
        }
        size_t len = ::strlen(z);
        return String::New(arena_, z + 1, len - 2);
    }

    String *NewRawString(const char *z, size_t n) { return String::New(arena_, z, n); }

    String *NewStringEscaped(const char *z, int quote, bool *ok) {
        std::string buf;
        if (quote == 0) {
            *ok = (base::Slice::ParseEscaped(z, &buf) == 0);
        } else {
            size_t len = ::strlen(z);
            *ok = (base::Slice::ParseEscaped(z + 1, len - 2, &buf) == 0);
        }
        return !*ok ? nullptr : String::New(arena_, buf.data(), buf.size());
    }

    template<class T>
    base::ArenaVector<T> *NewList(T init) {
        auto rv = new (arena_) base::ArenaVector<T>(arena_);
        if (init) {
            rv->push_back(init);
        }
        return rv;
    }

    template<class T>
    T AttributesMatch(Attributes *attrs, const char *z, T value, T def_val) {
        T rv = def_val;
        if (attrs) {
            for (auto attr : *attrs) {
                if (attr->Equal(z)) {
                    rv = value;
                }
            }
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
