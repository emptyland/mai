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
    V(ClassDefinition) \
    V(ObjectDefinition) \
    V(VarDeclaration) \
    V(PropertyDeclaration) \
    V(Assignment) \
    V(IfStatement) \
    V(WhileLoop) \
    V(ForIterateLoop) \
    V(Multiple) \
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
    friend class Factory; \
    DISALLOW_IMPLICIT_CONSTRUCTORS(name)
    
using Attributes = base::ArenaVector<const String *>;

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
    DEF_PTR_GETTER_NOTNULL(Expression, cond);
    DEF_PTR_GETTER_NOTNULL(Block, then_clause);
    DEF_PTR_GETTER(Statement, else_clause);
    DEFINE_AST_NODE(IfStatement);
private:
    IfStatement(int line, Expression *cond, Block *then_clause, Statement *else_clause)
        : Statement(line)
        , cond_(DCHECK_NOTNULL(cond))
        , then_clause_(DCHECK_NOTNULL(then_clause))
        , else_clause_(else_clause) {}

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
    DEF_PTR_GETTER_NOTNULL(NameList, names);
    DEF_PTR_GETTER_NOTNULL(Expression, init);
    DEFINE_AST_NODE(ForIterateLoop);
private:
    ForIterateLoop(int begin_line, int end_line, NameList *names, Expression *init, Block *body)
        : Loop(begin_line, end_line, body)
        , names_(DCHECK_NOTNULL(names))
        , init_(DCHECK_NOTNULL(init)) {}
    NameList *names_;
    Expression *init_;
}; // class ForIterateLoop
    
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
    Expression *key() const { return operand(0); }
    Expression *value() const { return rhs(); }
    
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
    
class New : public Call {
public:
    DEFINE_AST_NODE(New);
private:
    New(int line, Expression *callee, ArgumentList *args) : Call(line, callee, args) {}
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
    
    FunctionDefinition *NewFunctionDefinition(Expression *self, const String *name,
                                              LambdaLiteral *literal,
                                              const Location &loc = Location{}) {
        return new (arena_) FunctionDefinition(loc.begin_line, self, name, literal);
    }
    
    ObjectDefinition *NewObjectDefinition(Attributes *attrs, const String *name,
                                          ObjectDefinition::MemberList *members,
                                          const Location &loc = Location{}) {
        bool local = AttributesMatch(attrs, "local", true, false);
        return new (arena_) ObjectDefinition(loc.begin_line, loc.end_line, name, local, members);
    }
    
    ClassDefinition *NewClassDefinition(Attributes *attrs, const String *name, Expression *base,
                                        ObjectDefinition::MemberList *members,
                                        const Location &loc = Location{}) {
        bool local = AttributesMatch(attrs, "local", true, false);
        return new (arena_) ClassDefinition(loc.begin_line, loc.end_line, name, local, base, members);
    }
    
    Assignment *NewAssignment(Assignment::LValList *lvals, Assignment::RValList *rvals,
                              const Location &loc = Location{}) {
        return new (arena_) Assignment(loc.begin_line, lvals, rvals);
    }
    
    IfStatement *NewIfStatement(Expression *cond, Block *then_clause, Statement *else_clause,
                                const Location &loc = Location{}) {
        return new (arena_) IfStatement(loc.begin_line, cond, then_clause, else_clause);
    }
    
    WhileLoop *NewWhileLoop(Expression *cond, Block *body, const Location &loc = Location{}) {
        return new (arena_) WhileLoop(loc.begin_line, loc.end_line, cond, body);
    }
    
    ForIterateLoop *NewForIterateLoop(ForIterateLoop::NameList *names, Expression *init,
                                      Block *body, const Location &loc = Location{}) {
        return new (arena_) ForIterateLoop(loc.begin_line, loc.end_line, names, init, body);
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

    Call *NewCall(Expression *callee, Call::ArgumentList *args, const Location &loc = Location{}) {
        return new (arena_) Call(loc.begin_line, callee, args);
    }
    
    SelfCall *NewSelfCall(Expression *callee, const String *method, Call::ArgumentList *args,
                          const Location &loc = Location{}) {
        return new (arena_) SelfCall(loc.begin_line, callee, method, args);
    }
    
    New *NewNew(Expression *callee, Call::ArgumentList *args, const Location &loc = Location{}) {
        return new (arena_) New(loc.begin_line, callee, args);
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

    Multiple *NewUnary(Operator::ID op, Expression *operand, const Location &loc = Location{}) {
        return new (arena_) Multiple(loc.begin_line, op, operand);
    }

    Multiple *NewBinary(Operator::ID op, Expression *lhs, Expression *rhs,
                        const Location &loc = Location{}) {
        return new (arena_) Multiple(loc.begin_line, op, lhs, rhs);
    }
    
    Multiple *NewEntry(Expression *key, Expression *value, const Location &loc = Location{}) {
        return new (arena_) Multiple(loc.begin_line, Operator::kEntry, key, value);
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
