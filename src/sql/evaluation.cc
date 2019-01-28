#include "sql/evaluation.h"
#include "base/slice.h"

namespace mai {
    
namespace sql {

namespace eval {

Value Value::ToNumeric() const {
    Value v;
    v.kind    = Value::kI64;
    v.i64_val = 0;
    
    switch (kind) {
        case kNull:
        case kF64:
        case kI64:
            return *this;
        case kString:
            if (base::Slice::LikeFloating(str_val->data(), str_val->size())) {
                v.kind = Value::kF64;
                v.f64_val = atof(str_val->data());
            } else {
                v.kind = Value::kI64;
                v.i64_val = atoll(str_val->data());
            }
            break;
        default:
            break;
    }
    return v;
}
    
Value Value::ToIntegral() const {
    Value v = ToNumeric();
    switch (v.kind) {
        case kNull:
            break;
        case kF64:
            v.kind = Value::kI64;
            v.i64_val = static_cast<int64_t>(v.f64_val);
            break;
        case kI64:
            break;
        default:
            break;
    }
    return v;
}
    
Value Value::ToFloating() const {
    Value v = ToNumeric();
    switch (v.kind) {
        case kNull:
            break;
        case kF64:
            break;
        case kI64:
            v.kind = Value::kF64;
            v.f64_val = static_cast<double>(v.i64_val);
            break;
        default:
            break;
    }
    return v;
}
    
bool Value::StrictEquals(const Value &v) const {
    
    switch (kind) {
        case Value::kNull:
            return v.kind == Value::kNull;
        case Value::kF64:
            return v.kind == kind && v.f64_val == f64_val;
        case Value::kI64:
            return v.kind == kind && v.i64_val == i64_val;
        case Value::kString:
            return v.kind == kind && ::strncmp(v.str_val->data(),
                                               str_val->data(),
                                               str_val->size()) == 0;
        default:
            break;
    }
    return false;
}
    
#define COMPARE_VAL(lhs, rhs) \
    ((lhs) == (rhs) ? 0 : ((lhs) < (rhs) ? -1 : 1))
    
int Value::Compare(const Value &rhs) const {
    DCHECK_NE(kNull, kind);
    DCHECK_NE(kNull, rhs.kind);
    
    Value r;
    switch (kind) {
        case kF64:
            r = rhs.ToFloating();
            return COMPARE_VAL(f64_val, r.f64_val);
        case kI64:
            r = rhs.ToIntegral();
            return COMPARE_VAL(i64_val, r.i64_val);
        case kString:
            if (rhs.kind == kString) {
                return ::strncmp(str_val->data(), rhs.str_val->data(),
                                 str_val->size());
            }
            return -rhs.Compare(*this);
        default:
            DLOG(FATAL) << "Noreached";
            break;
    }
    return 0;
}
    
/*virtual*/ Error Constant::Evaluate(Context *ctx) {
    ctx->set_result(data_);
    return Error::OK();
}

Operation::Operation(SQLOperator op, Expression *lhs,
                     const std::vector<Expression *> &rhs,
                     base::Arena *arena)
    : op_(op)
    , operands_count_(1 + rhs.size()) {
    if (rhs.size() == 0) {
        operands_ = reinterpret_cast<Expression **>(lhs);
    } else {
        operands_ = arena->NewArray<Expression *>(1 + rhs.size());
        operands_[0] = lhs;
        for (size_t i = 0; i < rhs.size(); ++i) {
            operands_[i + 1] = rhs[i];
        }
    }
}

/*virtual*/ Error Operation::Evaluate(Context *ctx) {
    Error rs;
    if (operands_count_ == 1) {
        rs = operand(0)->Evaluate(ctx);
        if (!rs) {
            return rs;
        }
        Value rv = ctx->result();
        switch (op_) {
            case SQL_MINUS:
                if (ctx->is_not_null()) {
                    rv = rv.ToNumeric();
                    if (rv.kind == Value::kF64) {
                        rv.f64_val = -rv.f64_val;
                    } else {
                        rv.i64_val = -rv.i64_val;
                    }
                    ctx->set_result(rv);
                }
                break;
                
            case SQL_NOT:
                if (ctx->is_not_null()) {
                    rv = rv.ToNumeric();
                    if (rv.kind == Value::kF64) {
                        rv.kind = Value::kI64;
                        rv.i64_val = !!rv.f64_val;
                    }
                    ctx->set_result(rv);
                }
                break;
                
            case SQL_BIT_INV:
                if (ctx->is_not_null()) {
                    rv = rv.ToIntegral();
                    rv.i64_val = ~rv.i64_val;
                    ctx->set_result(rv);
                }
                break;
                
            case SQL_IS_NULL:
                rv.kind = Value::kI64;
                rv.i64_val = ctx->is_null();
                ctx->set_result(rv);
                break;
                
            case SQL_IS_NOT_NULL:
                rv.kind = Value::kI64;
                rv.i64_val = ctx->is_not_null();
                ctx->set_result(rv);
                break;
                
            default:
                break;
        }
    } else if (operands_count_ == 2) {
        rs = operand(0)->Evaluate(ctx);
        if (!rs) {
            return rs;
        }
        Value rv = ctx->result();
        rs = operand(1)->Evaluate(ctx);
        if (!rs) {
            return rs;
        }
        if (op_ == SQL_CMP_STRICT_EQ) {
            bool eq = rv.StrictEquals(ctx->result());
            rv.kind = Value::kI64;
            rv.i64_val = eq;
            ctx->set_result(rv);
            return rs;
        }
        if (rv.kind == Value::kNull || ctx->result().kind == Value::kNull) {
            ctx->set_null();
            return rs;
        }
        
        switch (op_) {
            case SQL_CMP_EQ:
                ctx->set_bool(rv.Compare(ctx->result()) == 0);
                break;
            case SQL_CMP_NE:
                ctx->set_bool(rv.Compare(ctx->result()) != 0);
                break;
            case SQL_CMP_GT:
                ctx->set_bool(rv.Compare(ctx->result()) > 0);
                break;
            case SQL_CMP_GE:
                ctx->set_bool(rv.Compare(ctx->result()) >= 0);
                break;
            case SQL_CMP_LT:
                ctx->set_bool(rv.Compare(ctx->result()) < 0);
                break;
            case SQL_CMP_LE:
                ctx->set_bool(rv.Compare(ctx->result()) <= 0);
                break;
                // TODO:
            default:
                break;
        }
    }
/*
 V(CMP_EQ, "=",  0) \
 V(CMP_STRICT_EQ, "<=>", 0) \
 V(CMP_NE, "<>", 0) \
 V(CMP_GT, ">",  0) \
 V(CMP_GE, ">=", 0) \
 V(CMP_LT, "<",  0) \
 V(CMP_LE, "<=", 0)
 */
    return rs;
}

} // namespace eval

} // namespace sql
    
} // namespace mai
