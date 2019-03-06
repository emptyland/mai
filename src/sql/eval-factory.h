#ifndef MAI_SQL_EVAL_FACTORY_H_
#define MAI_SQL_EVAL_FACTORY_H_

#include "sql/evaluation.h"
#include "core/decimal-v2.h"
#include "glog/logging.h"

namespace mai {
    
namespace sql {
    
namespace eval {
    
class Factory final {
public:    
    Factory(base::Arena *arena) : arena_(DCHECK_NOTNULL(arena)) {}
    
    Constant *NewConstI64(int64_t value) {
        return new (arena_) Constant(value);
    }
    
    Constant *NewConstF64(double value) {
        return new (arena_) Constant(value);
    }
    
    Constant *NewConstStr(const AstString *value, bool clone = true) {
        if (clone && !value->empty()) {
            value = AstString::New(arena_, value->data(), value->size());
        }
        return new (arena_) Constant(value);
    }
    
    Constant *NewConstStr(const char *s, size_t n) {
        const AstString *value = (!s || n == 0) ? AstString::kEmpty :
            AstString::New(arena_, s, n);
        return new (arena_) Constant(value);
    }
    
    Constant *NewConstStr(const char *s) {
        return NewConstStr(s, !s ? 0 : strlen(s));
    }
    
    Constant *NewConstDec(const sql::SQLDecimal *val, bool clone = true) {
        if (clone) {
            val = sql::SQLDecimal::NewCopied(val, arena_);
        }
        return new (arena_) Constant(val);
    }
    
    Constant *NewConstNull() { return new (arena_) Constant(); }
    
    Variable *NewVariable(const VirtualSchema *schema, size_t entry_idx) {
        return new (arena_) Variable(schema, entry_idx);
    }
    
    Operation *NewUnary(SQLOperator op, Expression *operand) {
        auto opt = NewOperation(op, 1);
        opt->set_operand(0, operand);
        return opt;
    }
    
    Operation *NewBinary(SQLOperator op, Expression *lhs, Expression *rhs) {
        auto opt = NewOperation(op, 2);
        opt->set_operand(0, lhs);
        opt->set_operand(1, rhs);
        return opt;
    }
    
    Operation *NewOperation(SQLOperator op, size_t operands_count) {
        return new (arena_) Operation(op, operands_count, arena_);
    }
    
    Invocation *NewCall(SQLFunction fnid, size_t parameters_count) {
        return new (arena_) Invocation(fnid, parameters_count, arena_);
    }
    
    Aggregate *NewCall(SQLFunction fnid, size_t parameters_count, bool distinct) {
        return new (arena_) Aggregate(fnid, parameters_count, distinct, arena_);
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Factory);
private:
    base::Arena *arena_;
}; // class EvalFactory
    
} // namespace eval
    
} // namespace sql
    
} // namespace mai


#endif // MAI_SQL_EVAL_FACTORY_H_
