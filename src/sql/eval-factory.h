#ifndef MAI_SQL_EVAL_FACTORY_H_
#define MAI_SQL_EVAL_FACTORY_H_

#include "sql/evaluation.h"

namespace mai {
    
namespace sql {
    
namespace eval {
    
class Factory final {
public:    
    Factory(base::Arena *arena) : arena_(arena) {}
    
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
    
    Constant *NewConstNull() { return new (arena_) Constant(); }
    
    Variable *NewVariable(const VirtualSchema *schema, size_t entry_idx) {
        return new (arena_) Variable(schema, entry_idx);
    }
    
    Operation *NewUnary(SQLOperator op, Expression *operand) {
        return new (arena_) Operation(op, operand);
    }
    
    Operation *NewBinary(SQLOperator op, Expression *lhs, Expression *rhs) {
        return new (arena_) Operation(op, lhs, rhs, arena_);
    }
    
    Operation *NewMulti(SQLOperator op, Expression *lhs,
                        const std::vector<Expression *> &rhs) {
        return new (arena_) Operation(op, lhs, rhs, arena_);
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Factory);
private:
    base::Arena *arena_;
}; // class EvalFactory
    
} // namespace eval
    
} // namespace sql
    
} // namespace mai


#endif // MAI_SQL_EVAL_FACTORY_H_
