#include "sql/evaluation.h"
#include "sql/heap-tuple.h"
#include "base/slice.h"

namespace mai {
    
namespace sql {

namespace eval {
    
static inline bool LogicXor(uint64_t lhs, uint64_t rhs) {
    if ((lhs && rhs) || (!lhs && !rhs)) {
        return false;
    } else {
        return true;
    }
}
static inline bool LogicXor(int64_t lhs, int64_t rhs) {
    if ((lhs && rhs) || (!lhs && !rhs)) {
        return false;
    } else {
        return true;
    }
}
    
static inline uint64_t DoArithMod(uint64_t lhs, uint64_t rhs) {
    return lhs % rhs;
}
static inline int64_t DoArithMod(int64_t lhs, int64_t rhs) {
    return lhs % rhs;
}
static inline double DoArithMod(double lhs, double rhs) {
    return fmod(lhs, rhs);
}

Value Value::ToNumeric(Kind hint) const {
    Value v;
    v.kind    = Value::kI64;
    v.i64_val = 0;
    
    int rv = 0;
    switch (kind) {
        case kNull:
        case kF64:
            return *this;
            
        case kI64:
            if (hint == Value::kU64) {
                v.kind    = Value::kU64;
                v.u64_val = static_cast<uint64_t>(i64_val);
            } else {
                return *this;
            }
            break;
            
        case kU64:
            if (hint == Value::kI64) {
                v.kind    = Value::kI64;
                v.i64_val = static_cast<int64_t>(u64_val);
            } else {
                return *this;
            }
            break;
            
        case kString:
            switch (hint) {
                case kDate:
                case kTime:
                case kDateTime:
                    rv = SQLTimeUtils::Parse(str_val->data(), str_val->size(),
                                             &v.dt_val);
                    if (rv == 'd') {
                        v.kind = kDate;
                    } else if (rv == 't') {
                        v.kind = kTime;
                    } else if (rv == 'c') {
                        v.kind = kDateTime;
                    }
                    if (rv) {
                        v = v.ToNumeric(hint);
                    }
                    break;
                default:
                    break;
            }
            if (rv) {
                break;
            }
            rv = base::Slice::LikeNumber(str_val->data(), str_val->size());
            switch (rv) {
                case 'd':
                    if (hint == Value::kI64) {
                        v.kind = Value::kI64;
                        v.i64_val = atoll(str_val->data());
                    } else {
                        v.kind = Value::kU64;
                        v.u64_val = atoll(str_val->data());
                    }
                    break;
                case 's':
                    if (hint == Value::kU64) {
                        v.kind = Value::kU64;
                        v.u64_val = atoll(str_val->data());
                    } else {
                        v.kind = Value::kI64;
                        v.i64_val = atoll(str_val->data());
                    }
                    break;
                case 'o':
                    // TODO:
                    v.kind = Value::kU64;
                    v.u64_val = 0;
                    break;
                case 'h':
                    // TODO:
                    v.kind = Value::kU64;
                    v.u64_val = 0;
                    break;
                case 'f':
                    v.kind = Value::kF64;
                    v.f64_val = atof(str_val->data());
                    break;
                default:
                    break;
            }
            break;
        case kDate:
            v.kind = Value::kU64;
            switch (hint) {
                case kDateTime:
                    v.u64_val = SQLTimeUtils::ConvertToDateTime(dt_val.date).ToU64();
                    break;
                case kTime:
                    v.u64_val = SQLTimeUtils::ConvertToTime(dt_val.date).ToU64();
                    break;
                default:
                    v.u64_val = dt_val.date.ToU64();
                    break;
            }
            break;
        case kTime:
            
            switch (hint) {
                case kDateTime:
                    v.kind = Value::kU64;
                    v.u64_val = SQLTimeUtils::ConvertToDateTime(dt_val.time).ToU64();
                    break;
                case kDate:
                    v.kind = Value::kU64;
                    v.u64_val = SQLTimeUtils::ConvertToDate(dt_val.time).ToU64();
                    break;
                default:
                    if (dt_val.time.negative) {
                        v.kind = Value::kI64;
                        v.i64_val = dt_val.time.ToI64();
                    } else {
                        v.kind = Value::kU64;
                        v.u64_val = dt_val.time.ToU64();
                    }
                    break;
            }
            break;
        case kDateTime:
            v.kind = Value::kU64;
            switch (hint) {
                case kTime:
                    v.u64_val = SQLTimeUtils::ConvertToTime(dt_val).ToU64();
                    break;
                case kDate:
                    v.u64_val = SQLTimeUtils::ConvertToDate(dt_val).ToU64();
                    break;
                default:
                    v.u64_val = dt_val.ToU64();
                    break;
            }
            break;
        default:
            break;
    }
    return v;
}
    
Value Value::ToIntegral(Kind hint) const {
    Value v = ToNumeric(hint);
    switch (v.kind) {
        case kNull:
            break;
        case kF64:
            v.kind = Value::kI64;
            v.i64_val = static_cast<int64_t>(v.f64_val);
            break;
        case kI64:
            break;
        case kU64:
            break;
        default:
            break;
    }
    return v;
}
    
Value Value::ToFloating(Kind hint) const {
    Value v = ToNumeric(hint);
    switch (v.kind) {
        case kNull:
            break;
        case kF64:
            break;
        case kI64:
            v.kind = Value::kF64;
            v.f64_val = static_cast<double>(v.i64_val);
            break;
        case kU64:
            v.kind = Value::kF64;
            v.f64_val = static_cast<double>(v.u64_val);
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
        case Value::kDate:
            return v.kind == kind && v.dt_val.date.ToU64() == dt_val.date.ToU64();
        case Value::kTime:
            return v.kind == kind && v.dt_val.time.ToU64() == dt_val.time.ToU64();
        case Value::kDateTime:
            return v.kind == kind && v.dt_val.ToU64() == dt_val.ToU64();
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
            if (r.kind == Value::kI64) {
                return COMPARE_VAL(i64_val, r.i64_val);
            } else {
                if (i64_val < 0) {
                    return -1;
                } else {
                    COMPARE_VAL(u64_val, r.u64_val);
                }
            }
        case kU64:
            r = rhs.ToIntegral();
            if (r.kind == Value::kU64) {
                return COMPARE_VAL(u64_val, r.u64_val);
            } else {
                if (r.i64_val < 0) {
                    return 1;
                } else {
                    COMPARE_VAL(u64_val, r.u64_val);
                }
            }
        case kString:
            if (rhs.kind == kString) {
                return ::strncmp(str_val->data(), rhs.str_val->data(),
                                 str_val->size());
            }
            return -rhs.Compare(*this);
        case kTime:
            r = rhs.ToIntegral(Value::kTime);
            return COMPARE_VAL(dt_val.time.ToU64(), r.u64_val);
        case kDate:
            r = rhs.ToIntegral(Value::kDate);
            return COMPARE_VAL(dt_val.date.ToU64(), r.u64_val);
        case kDateTime:
            r = rhs.ToIntegral(Value::kDateTime);
            return COMPARE_VAL(dt_val.ToU64(), r.u64_val);
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
                    rv = rv.ToIntegral(Value::kU64);
                    rv.u64_val = ~rv.u64_val;
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
        Value lhs = ctx->result();
        rs = operand(1)->Evaluate(ctx);
        if (!rs) {
            return rs;
        }
        if (op_ == SQL_CMP_STRICT_EQ) {
            bool eq = lhs.StrictEquals(ctx->result());
            lhs.kind = Value::kI64;
            lhs.i64_val = eq;
            ctx->set_result(lhs);
            return rs;
        }
        if (lhs.kind == Value::kNull || ctx->result().kind == Value::kNull) {
            ctx->set_null();
            return rs;
        }
        
        Value rv, rhs;
        switch (op_) {
            // Comparion
            case SQL_CMP_EQ:
                ctx->set_bool(lhs.Compare(ctx->result()) == 0);
                break;
            case SQL_CMP_NE:
                ctx->set_bool(lhs.Compare(ctx->result()) != 0);
                break;
            case SQL_CMP_GT:
                ctx->set_bool(lhs.Compare(ctx->result()) > 0);
                break;
            case SQL_CMP_GE:
                ctx->set_bool(lhs.Compare(ctx->result()) >= 0);
                break;
            case SQL_CMP_LT:
                ctx->set_bool(lhs.Compare(ctx->result()) < 0);
                break;
            case SQL_CMP_LE:
                ctx->set_bool(lhs.Compare(ctx->result()) <= 0);
                break;
    
#define TO_REAL_NUMBER \
                lhs = lhs.ToNumeric(); \
                rhs = ctx->result().ToNumeric(lhs.kind); \
                if (lhs.is_floating()) { \
                    rhs = rhs.ToFloating(); \
                } else if (rhs.is_floating()) { \
                    lhs = lhs.ToFloating(); \
                } \

#define DO_REAL_NUMBER(op) \
                DCHECK_EQ(lhs.kind, rhs.kind); \
                DCHECK_EQ(lhs.is_number(), rhs.is_number()); \
                switch (lhs.kind) { \
                    case Value::kI64: \
                        rv.i64_val = op(lhs.i64_val, rhs.i64_val); \
                        break; \
                    case Value::kU64: \
                        rv.u64_val = op(lhs.u64_val, rhs.u64_val); \
                        break; \
                    case Value::kF64: \
                        rv.f64_val = op(lhs.f64_val, rhs.f64_val); \
                        break; \
                    default: \
                        break; \
                } \
                rv.kind = lhs.kind; \
                ctx->set_result(rv)

#define DEF_REAL_ARITH(op) \
                TO_REAL_NUMBER \
                DO_REAL_NUMBER(op)
                
#define DO_PLUS(lval, rval) ((lval) + (rval))
#define DO_SUB(lval, rval)  ((lval) - (rval))
#define DO_MUL(lval, rval)  ((lval) * (rval))
#define DO_DIV(lval, rval)  ((lval) / (rval))
#define DO_MOD(lval, rval)  DoArithMod(lval, rval)

            case SQL_PLUS:
                DEF_REAL_ARITH(DO_PLUS);
                break;
            case SQL_SUB:
                DEF_REAL_ARITH(DO_SUB);
                break;
            case SQL_MUL:
                DEF_REAL_ARITH(DO_MUL);
                break;
            case SQL_DIV:
                TO_REAL_NUMBER
                if (rhs.i64_val == 0 ||
                    rhs.u64_val == 0 ||
                    rhs.f64_val == 0) {
                    return MAI_CORRUPTION("Division zero.");
                }
                DO_REAL_NUMBER(DO_DIV);
                break;
            case SQL_MOD:
                TO_REAL_NUMBER
                if (rhs.i64_val == 0 ||
                    rhs.u64_val == 0 ||
                    rhs.f64_val == 0) {
                    return MAI_CORRUPTION("Division zero.");
                }
                DO_REAL_NUMBER(DO_MOD);
                break;
                
#undef DEF_REAL_ARITH
#undef TO_REAL_NUMBER
#undef DO_REAL_NUMBER
            
#define DEF_INT_ARITH(op) \
                lhs = lhs.ToIntegral(Value::kU64); \
                rhs = ctx->result().ToIntegral(lhs.kind); \
                switch (lhs.kind) { \
                    case Value::kI64: \
                        rv.i64_val = op(lhs.i64_val, rhs.i64_val); \
                        break; \
                    case Value::kU64: \
                        rv.u64_val = op(lhs.u64_val, rhs.u64_val); \
                        break; \
                    default: \
                        break; \
                } \
                rv.kind = lhs.kind; \
                ctx->set_result(rv)
                
#define DO_BIT_XOR(lval, rval) ((lval) ^ (rval))
#define DO_BIT_AND(lval, rval) ((lval) & (rval))
#define DO_BIT_OR(lval, rval)  ((lval) | (rval))
#define DO_LSHIFT(lval, rval)  ((lval) << (rval))
#define DO_RSHIFT(lval, rval)  ((lval) >> (rval))
#define DO_AND(lval, rval)     ((lval) && (rval))
#define DO_OR(lval, rval)      ((lval) || (rval))
#define DO_XOR(lval, rval)     LogicXor(lval, rval)
                
            case SQL_BIT_XOR:
                DEF_INT_ARITH(DO_BIT_XOR);
                break;
            case SQL_BIT_AND:
                DEF_INT_ARITH(DO_BIT_AND);
                break;
            case SQL_BIT_OR:
                DEF_INT_ARITH(DO_BIT_OR);
                break;
            case SQL_LSHIFT:
                DEF_INT_ARITH(DO_LSHIFT);
                break;
            case SQL_RSHIFT:
                DEF_INT_ARITH(DO_RSHIFT);
                break;
            case SQL_AND:
                DEF_INT_ARITH(DO_AND);
                break;
            case SQL_OR:
                DEF_INT_ARITH(DO_OR);
                break;
            case SQL_XOR:
                DEF_INT_ARITH(LogicXor);
                break;
                
#undef DEF_INT_ARITH
                
            // TODO:

            default:
                break;
        }
    }
    return rs;
}
    
/*virtual*/ Error Variable::Evaluate(Context *ctx) {
    DCHECK_NOTNULL(ctx->input());
    DCHECK_NOTNULL(ctx->schema());
    
    auto cd = ctx->schema()->column(entry_idx_);
    
    if (ctx->input()->IsNull(cd)) {
        ctx->set_null();
        return Error::OK();
    }
    
    Value v;
    if (cd->CoverU64()) {
        v.kind = Value::kU64;
        v.u64_val = ctx->input()->GetU64(cd);
    } else if (cd->CoverI64()) {
        v.kind = Value::kI64;
        v.i64_val = ctx->input()->GetI64(cd);
    } else if (cd->CoverF64()) {
        v.kind = Value::kF64;
        v.f64_val = ctx->input()->GetF64(cd);
    } else if (cd->CoverString()) {
        v.kind = Value::kString;
        v.str_val = ctx->input()->GetRawString(cd);
    } else if (cd->CoverDateTime()) {
        switch (cd->type()) {
            case SQL_TIME:
                v.kind = Value::kTime;
                break;
            case SQL_DATE:
                v.kind = Value::kDate;
                break;
            case SQL_DATETIME:
                v.kind = Value::kDateTime;
                break;
            default:
                DLOG(FATAL) << "noreached";
                break;
        }
        v.dt_val = ctx->input()->GetDateTime(cd);
    } else {
        DLOG(FATAL) << "noreached";
    }
    
    ctx->set_result(v);
    return Error::OK();
}

} // namespace eval

} // namespace sql
    
} // namespace mai
