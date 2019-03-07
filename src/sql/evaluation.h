#ifndef MAI_SQL_EVALUATION_H_
#define MAI_SQL_EVALUATION_H_

#include "sql/types.h"
#include "base/arenas.h"
#include "base/arena-utils.h"
#include "mai/error.h"
#include "glog/logging.h"

namespace mai {
namespace sql {
using AstString = base::ArenaString;
class HeapTuple;
class ColumnDescriptor;
class VirtualSchema;
namespace ast {
class Expression;
} // namespace ast
namespace eval {
    
class Factory;
class EvalNode;
class Expression;
class Context;
    
struct Value {
    enum Kind : int {
        kNull,
        kString,
        kDecimal,
        kU64,
        kI64,
        kF64,
        kDate,
        kTime,
        kDateTime,
    };
    
    Kind kind;
    union {
        const AstString *str_val;
        const SQLDecimal *dec_val;
        double  f64_val;
        uint64_t u64_val;
        int64_t i64_val;
        SQLDateTime dt_val;
    };
    
    bool is_floating() const { return kind == kF64; }
    bool is_integral() const { return kind == kI64 || kind == kU64; }
    bool is_decimal() const { return kind == kDecimal; }
    bool is_number() const {
        return is_floating() || is_integral() || is_decimal();
    }
    bool is_null() const { return kind == kNull; }
    bool is_not_null() const { return !is_null(); }
    
    bool IsZero() const;

    Value ToNumeric(base::Arena *arena, Kind hint = kNull) const;
    Value ToIntegral(base::Arena *arena, Kind hint = kNull) const;
    Value ToFloating(base::Arena *arena, Kind hint = kNull) const;
    Value ToDecimal(base::Arena *arena, Kind hint = kNull) const;
    
    bool StrictEquals(const Value &rhs) const;
    int Compare(const Value &rhs, base::Arena *arena = nullptr) const;
    
    void Minus();
}; // struct RawData
    
class Context final {
public:
    Context(base::Arena *arena)
        : arena_(arena) {
        result_.kind = Value::kNull;
    }
    
    bool is_null() const { return result_.kind == Value::kNull; }
    bool is_not_null() const { return !is_null(); }
    
    void set_null() { result_.kind = Value::kNull; }
    
    void set_bool(bool value) {
        result_.kind    = Value::kI64;
        result_.i64_val = value;
    }
    
    DEF_VAL_PROP_RW(Value, result);
    DEF_VAL_PROP_RW(bool, done);
    DEF_VAL_PROP_RW(bool, init);
    DEF_PTR_GETTER_NOTNULL(base::Arena, arena);
    DEF_PTR_PROP_RW(const VirtualSchema, schema);
    DEF_PTR_PROP_RW(const HeapTuple, input);
    
private:
    base::Arena *const arena_; // arena for values
    Value result_;
    bool done_ = false;
    bool init_ = false;
    const VirtualSchema *schema_;
    const HeapTuple *input_;
}; // class Context
    
class EvalNode {
public:
    void *operator new (size_t n, base::Arena *arena) {
        return arena->Allocate(n);
    }
    void operator delete (void *) = delete;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(EvalNode);
protected:
    EvalNode() {}
    
}; // class EvalNode
    
class Expression : public EvalNode {
public:
    virtual Error Evaluate(Context *ctx) = 0;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Expression);
protected:
    Expression() {}
}; // class Expr

class Constant : public Expression {
public:
    using Kind = Value::Kind;
    using Data = Value;
    
    Kind kind() const { return data_.kind; }
    
    int64_t i64_val() const { return data_.i64_val; }
    double  f64_val() const { return data_.f64_val; }
    const SQLDecimal *dec_val() const { return data_.dec_val; }
    const AstString *str_val() const { return data_.str_val; }
    bool is_null() const { return kind() == Data::kNull; }
    bool is_not_null() const { return !is_null(); }
    
    virtual Error Evaluate(Context *ctx) override;
    
    friend class Factory;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Constant);
private:
    Constant(const AstString *str_val) {
        data_.kind = Data::kString;
        data_.str_val = str_val;
    }
    
    Constant(const SQLDecimal *dec_val) {
        data_.kind = Data::kDecimal;
        data_.dec_val = dec_val;
    }
    
    Constant(int64_t i64_val) {
        data_.kind = Data::kI64;
        data_.i64_val = i64_val;
    }
    
    Constant(double f64_val) {
        data_.kind = Data::kF64;
        data_.f64_val = f64_val;
    }
    
    Constant(const SQLDate &val) {
        data_.kind = Data::kDate;
        data_.dt_val.date = val;
    }
    
    Constant(const SQLTime &val) {
        data_.kind = Data::kTime;
        data_.dt_val.time = val;
    }
    
    Constant(const SQLDateTime &val) {
        data_.kind = Data::kDateTime;
        data_.dt_val = val;
    }
    
    Constant() { data_.kind = Data::kNull; }
    
    Data data_;
}; // class Constant


class Variable : public Expression {
public:
    DEF_PTR_GETTER_NOTNULL(const VirtualSchema, schema);
    DEF_VAL_GETTER(size_t, entry_idx);
    
    virtual Error Evaluate(Context *ctx) override;
    
    friend class Factory;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Variable);
private:
    Variable(const VirtualSchema *schema, size_t entry_idx)
        : schema_(DCHECK_NOTNULL(schema))
        , entry_idx_(entry_idx) {}
    
    const VirtualSchema *const schema_;
    size_t entry_idx_;
}; // class Identifier


class Operation : public Expression {
public:
    DEF_VAL_GETTER(size_t, operands_count);
    
    Expression *operand(size_t i) const {
        DCHECK_LT(i, operands_count_);
        return operands_count_ == 1
            ? reinterpret_cast<Expression *>(operands_)
            : operands_[i];
    }
    
    void set_operand(size_t i, Expression *p) {
        DCHECK_LT(i, operands_count_);
        if (operands_count_ == 1) {
            operands_ = reinterpret_cast<Expression **>(p);
        } else {
            operands_[i] = p;
        }
    }
    
    Expression *lhs() const { return operand(0); }
    
    Expression *rhs(size_t i) const {
        DCHECK_LT(i, rhs_count());
        return rhs()[i];
    }
    
    Expression **rhs() const {
        DCHECK_LT(operands_count_, 1);
        return operands_ + 1;
    }
    
    size_t rhs_count() const { return operands_count_ - 1; }
    
    virtual Error Evaluate(Context *ctx) override;
    
    friend class Factory;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Operation);
private:
    Operation(SQLOperator op, size_t operands_count, base::Arena *arena)
        : op_(op)
        , operands_count_(operands_count)
        , operands_(operands_count > 1 ?
                    arena->NewArray<Expression *>(operands_count) : nullptr) {
        if (operands_count > 1) {
            ::memset(operands_, 0, sizeof(*operands_) * operands_count);
        }
    }
    
    SQLOperator op_;
    size_t operands_count_;
    Expression **operands_;
}; // class Operation


class Invocation : public Expression {
public:
    DEF_VAL_GETTER(SQLFunction, fnid);
    DEF_VAL_GETTER(size_t, parameters_count);

    Expression *parameter(size_t i) {
        DCHECK_LT(i, parameters_count_);
        return parameters_count_ == 1
            ? reinterpret_cast<Expression *>(parameters_)
            : parameters_[i];
    }
    
    void set_parameter(size_t i, Expression *p) {
        DCHECK_LT(i, parameters_count_);
        if (parameters_count_ == 1) {
            parameters_ = reinterpret_cast<Expression **>(p);
        } else {
            parameters_[i] = p;
        }
    }

    virtual Error Evaluate(Context *ctx) override;
    
    friend class Factory;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Invocation);
protected:
    Invocation(SQLFunction fnid, size_t parameters_count, base::Arena *arena)
        : fnid_(fnid)
        , parameters_count_(parameters_count)
        , parameters_(parameters_count > 1 ?
                      arena->NewArray<Expression *>(parameters_count) : nullptr) {
        if (parameters_count > 1) {
            ::memset(parameters_, 0, sizeof(*parameters_) * parameters_count);
        }
    }
    
    const SQLFunction fnid_;
    const size_t parameters_count_;
    Expression **parameters_;
}; // class Call


class Aggregate : public Invocation {
public:
    DEF_VAL_GETTER(bool, distinct);
    
    virtual Error Evaluate(Context *ctx) override;
    
    friend class Factory;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Aggregate);
private:
    Aggregate(SQLFunction fnid, size_t parameters_count, bool distinct, base::Arena *arena)
        : Invocation(fnid, parameters_count, arena)
        , distinct_(distinct) {
        mid_.kind = Value::kNull;
        val_.kind = Value::kNull;
    }
    
    Error SUM(Value arg0, Context *ctx);
    Error AVG(Value arg0, Context *ctx);
    
    const bool distinct_;
    uint64_t counter_ = 0;
    SQLDecimal *sum_val_ = nullptr;
    SQLDecimal *fin_val_ = nullptr;
    Value mid_;
    Value val_;
    base::StaticArena<44 * 2> owned_arena_;
}; // class Aggregate
    
} // namespace eval
    
struct Evaluation final {
    
    static eval::Expression *BuildExpression(const VirtualSchema *env, ast::Expression *ast,
                                             base::Arena *arena);
    
    static Error BuildExpression(const VirtualSchema *env, ast::Expression *ast, base::Arena *arena,
                                 eval::Expression **result);
    
}; // struct Evaluation
    
} // namespace sql
    
} // namespace mai


#endif // MAI_SQL_EVALUATION_H_
