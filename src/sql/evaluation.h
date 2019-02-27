#ifndef MAI_SQL_EVALUATION_H_
#define MAI_SQL_EVALUATION_H_

#include "sql/types.h"
#include "base/arena.h"
#include "base/arena-utils.h"
#include "mai/error.h"
#include "glog/logging.h"

namespace mai {
namespace core {
namespace v2 {
class Decimal;
} // namespace v2
} // namespace core
namespace sql {
using AstString = base::ArenaString;
using AstDecimal = core::v2::Decimal;
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
        const AstDecimal *dec_val;
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

    Value ToNumeric(Kind hint = kNull) const;
    Value ToIntegral(Kind hint = kNull) const;
    Value ToFloating(Kind hint = kNull) const;
    
    bool StrictEquals(const Value &rhs) const;
    int Compare(const Value &rhs) const;
    
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
    DEF_PTR_GETTER_NOTNULL(base::Arena, arena);
    DEF_PTR_PROP_RW(const VirtualSchema, schema);
    DEF_PTR_PROP_RW(const HeapTuple, input);
    
private:
    base::Arena *const arena_; // arena for values
    Value result_;
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
    
    Constant(int64_t i64_val) {
        data_.kind = Data::kI64;
        data_.i64_val = i64_val;
    }
    
    Constant(double f64_val) {
        data_.kind = Data::kF64;
        data_.f64_val = f64_val;
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
    
    Expression *operand(size_t i) {
        DCHECK_LT(i, operands_count_);
        return operands_count_ == 1
            ? reinterpret_cast<Expression *>(operands_)
            : operands_[i];
    }
    
    Expression *lhs() const { return operands_[0]; }
    
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
    Operation(SQLOperator op, Expression *operand)
        : op_(op)
        , operands_count_(1)
        , operands_(reinterpret_cast<Expression **>(operand)) {}
    
    Operation(SQLOperator op, Expression *lhs, Expression *rhs,
              base::Arena *arena)
        : op_(op)
        , operands_count_(2)
        , operands_(arena->NewArray<Expression *>(2)) {
        operands_[0] = lhs;
        operands_[1] = rhs;
    }
    
    Operation(SQLOperator op, Expression *lhs,
              const std::vector<Expression *> &rhs, base::Arena *arena);
    
    SQLOperator op_;
    size_t operands_count_;
    Expression **operands_;
}; // class Operation
    
} // namespace eval
    
struct Evaluation final {
    
    static eval::Expression *
    BuildExpression(const VirtualSchema *env, ast::Expression *ast,
                    base::Arena *arena);
    
    static Error BuildExpression(const VirtualSchema *env, ast::Expression *ast,
                                 base::Arena *arena, eval::Expression **result);
    
}; // struct Evaluation
    
} // namespace sql
    
} // namespace mai


#endif // MAI_SQL_EVALUATION_H_
