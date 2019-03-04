#include "sql/evaluation.h"
#include "sql/eval-factory.h"
#include "sql/heap-tuple.h"
#include "sql/ast-visitor.h"
#include "sql/ast.h"
#include "core/decimal-v2.h"
#include "base/scoped-arena.h"
#include "base/slice.h"

namespace mai {
    
namespace sql {

namespace eval {
    
using sql::SQLDecimal;
using base::Slice;
    
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
    
////////////////////////////////////////////////////////////////////////////////
// [+]
////////////////////////////////////////////////////////////////////////////////
static inline uint64_t DoArithPlus(uint64_t lhs, uint64_t rhs, Context *) {
    return lhs + rhs;
}
static inline int64_t DoArithPlus(int64_t lhs, int64_t rhs, Context *) {
    return lhs + rhs;
}
static inline double DoArithPlus(double lhs, double rhs, Context *) {
    return lhs + rhs;
}
static inline SQLDecimal *DoArithPlus(const SQLDecimal *lhs,
                                      const SQLDecimal *rhs, Context *ctx) {
    return lhs->Add(rhs, ctx->arena());
}
    
////////////////////////////////////////////////////////////////////////////////
// [-]
////////////////////////////////////////////////////////////////////////////////
static inline uint64_t DoArithSub(uint64_t lhs, uint64_t rhs, Context *) {
    return lhs - rhs;
}
static inline int64_t DoArithSub(int64_t lhs, int64_t rhs, Context *) {
    return lhs - rhs;
}
static inline double DoArithSub(double lhs, double rhs, Context *) {
    return lhs - rhs;
}
static inline SQLDecimal *DoArithSub(const SQLDecimal *lhs,
                                     const SQLDecimal *rhs, Context *ctx) {
    return lhs->Sub(rhs, ctx->arena());
}

////////////////////////////////////////////////////////////////////////////////
// [*]
////////////////////////////////////////////////////////////////////////////////
static inline uint64_t DoArithMul(uint64_t lhs, uint64_t rhs, Context *) {
    return lhs / rhs;
}
static inline int64_t DoArithMul(int64_t lhs, int64_t rhs, Context *) {
    return lhs / rhs;
}
static inline double DoArithMul(double lhs, double rhs, Context *) {
    return lhs / rhs;
}
static inline SQLDecimal *DoArithMul(const SQLDecimal *lhs,
                                     const SQLDecimal *rhs, Context *ctx) {
    return lhs->Mul(rhs, ctx->arena());
}
    
////////////////////////////////////////////////////////////////////////////////
// [/]
////////////////////////////////////////////////////////////////////////////////
static inline uint64_t DoArithDiv(uint64_t lhs, uint64_t rhs, Context *) {
    return lhs / rhs;
}
static inline int64_t DoArithDiv(int64_t lhs, int64_t rhs, Context *) {
    return lhs / rhs;
}
static inline double DoArithDiv(double lhs, double rhs, Context *) {
    return lhs / rhs;
}
static inline SQLDecimal *DoArithDiv(const SQLDecimal *lhs,
                                     const SQLDecimal *rhs, Context *ctx) {
    SQLDecimal *rv;
    std::tie(rv, std::ignore) = lhs->Div(rhs, ctx->arena());
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
// [%]
////////////////////////////////////////////////////////////////////////////////
static inline uint64_t DoArithMod(uint64_t lhs, uint64_t rhs, Context *) {
    return lhs % rhs;
}
static inline int64_t DoArithMod(int64_t lhs, int64_t rhs, Context *) {
    return lhs % rhs;
}
static inline double DoArithMod(double lhs, double rhs, Context *) {
    return fmod(lhs, rhs);
}
static inline SQLDecimal *DoArithMod(const SQLDecimal *lhs,
                                     const SQLDecimal *rhs, Context *ctx) {
    SQLDecimal *re;
    std::tie(std::ignore, re) = lhs->Div(rhs, ctx->arena());
    return re;
}
    
static inline void AdjustPrecision(SQLDecimal const**lhs,
                                   SQLDecimal const**rhs, Context *ctx) {
    const SQLDecimal *l = *lhs;
    const SQLDecimal *r = *rhs;
    
    int max_exp = std::max(l->exp(), r->exp());
    *lhs = l->NewPrecision(max_exp, ctx->arena());
    *rhs = r->NewPrecision(max_exp, ctx->arena());
}

////////////////////////////////////////////////////////////////////////////////
// class Value
////////////////////////////////////////////////////////////////////////////////

bool Value::IsZero() const {
    if (!is_number()) {
        return false;
    }
    switch (kind) {
        case kI64:
            return i64_val == 0;
        case kU64:
            return u64_val == 0;
        case kF64:
            return f64_val == 0;
        case kDecimal:
            return dec_val->zero();
        default:
            DLOG(FATAL) << "Noreached";
            break;
    }
    return false;
}
    
static inline Value Str2U64(const AstString *str_val, base::Arena *arena) {
    Value v;
    int s = Slice::ParseU64(str_val->data(), str_val->size(), &v.u64_val);
    DCHECK_GE(s, 0);
    if (s > 0) {
        v.kind = Value::kDecimal;
        v.dec_val = SQLDecimal::NewDecLiteral(str_val->data(), str_val->size(),
                                              arena);
    } else {
        v.kind = Value::kU64;
    }
    return v;
}
    
static inline Value Str2H64(const AstString *str_val, bool sign, base::Arena *arena) {
    DCHECK_EQ('0', str_val->data()[0]);
    DCHECK_EQ('x', str_val->data()[1]);
    Value v;
    int s = Slice::ParseH64(str_val->data() + 2, str_val->size() - 2, &v.u64_val);
    DCHECK_GE(s, 0);
    if (s > 0) {
        v.kind = Value::kDecimal;
        v.dec_val = SQLDecimal::NewHexLiteral(str_val->data(), str_val->size(),
                                              arena);
    } else {
        v.kind = sign ? Value::kI64 : Value::kU64;
    }
    return v;
}
    
static inline Value Str2O64(const AstString *str_val, bool sign, base::Arena *arena) {
    DCHECK_EQ('0', str_val->data()[0]);
    Value v;
    int s = Slice::ParseO64(str_val->data() + 1, str_val->size() - 1, &v.u64_val);
    DCHECK_GE(s, 0);
    if (s > 0) {
        v.kind = Value::kDecimal;
        v.dec_val = SQLDecimal::NewOctLiteral(str_val->data(), str_val->size(),
                                              arena);
    } else {
        v.kind = sign ? Value::kI64 : Value::kU64;
    }
    return v;
}
    
static inline Value Str2I64(const AstString *str_val, base::Arena *arena) {
    Value v;
    int s = Slice::ParseI64(str_val->data(), str_val->size(), &v.i64_val);
    DCHECK_GE(s, 0);
    if (s > 0) {
        v.kind = Value::kDecimal;
        v.dec_val = SQLDecimal::NewDecLiteral(str_val->data(), str_val->size(),
                                              arena);
    } else {
        v.kind = Value::kI64;
    }
    return v;
}

Value Value::ToNumeric(base::Arena *arena, Kind hint) const {
    Value v;
    v.kind = kI64;
    v.i64_val = 0;
    
    int rv = 0;
    switch (kind) {
        case kNull:
        case kF64:
            return *this;
            
        case kI64:
            switch (hint) {
                case kU64:
                    v.kind = kU64;
                    v.u64_val = static_cast<uint64_t>(i64_val);
                    break;
                case kDecimal:
                    v.kind = kDecimal;
                    v.dec_val = SQLDecimal::NewI64(i64_val, arena);
                    break;
                default:
                    return *this;
            }
            break;
            
        case kU64:
            switch (hint) {
                case kI64:
                    v.kind = kI64;
                    v.i64_val = static_cast<int64_t>(u64_val);
                    break;
                case kDecimal:
                    v.kind = kDecimal;
                    v.dec_val = SQLDecimal::NewI64(u64_val, arena);
                    break;
                default:
                    return *this;
            }
            break;
            
        case kDecimal:
            if (hint == kF64) {
                v.kind = kF64;
                v.f64_val = dec_val->ToF64();
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
                        v = v.ToNumeric(arena, hint);
                        return v;
                    }
                    break;
                case kDecimal:
                    v.kind = kDecimal;
                    v.dec_val = SQLDecimal::NewParsed(str_val->data(),
                                                      str_val->size(), arena);
                    if (!v.dec_val) {
                        v.dec_val = SQLDecimal::NewU64(0, arena);
                    }
                    return v;
                default:
                    break;
            }
            rv = Slice::LikeNumber(str_val->data(), str_val->size());
            switch (rv) {
                case 'd':
                    if (hint == kI64) {
                        v = Str2I64(str_val, arena);
                    } else {
                        v = Str2U64(str_val, arena);
                    }
                    break;
                case 's':
                    if (hint == kU64) {
                        v = Str2U64(str_val, arena);
                    } else {
                        v = Str2I64(str_val, arena);
                    }
                    break;
                case 'o':
                    v = Str2O64(str_val, hint == kI64, arena);
                    break;
                case 'h':
                    v = Str2H64(str_val, hint == kI64, arena);
                    break;
                case 'f':
                    v.kind = kDecimal;
                    v.dec_val = SQLDecimal::NewRealLiteral(str_val->data(),
                                                           str_val->size(),
                                                           arena);
                    break;
                case 'e':
                    v.kind = kDecimal;
                    v.dec_val = SQLDecimal::NewExpLiteral(str_val->data(),
                                                          str_val->size(),
                                                          arena);
                    break;
                default:
                    v.kind = kU64;
                    v.u64_val = 0;
                    break;
            }
            break;

        case kDate:
            switch (hint) {
                case kDateTime:
                    v.kind = kU64;
                    v.u64_val = SQLTimeUtils::ConvertToDateTime(dt_val.date).ToU64();
                    break;
                case kTime:
                    v.kind = kU64;
                    v.u64_val = SQLTimeUtils::ConvertToTime(dt_val.date).ToU64();
                    break;
                case kDecimal:
                    v.kind = kDecimal;
                    v.dec_val = SQLDecimal::NewU64(dt_val.date.ToU64(), arena);
                    break;
                default:
                    v.kind = kU64;
                    v.u64_val = dt_val.date.ToU64();
                    break;
            }
            break;

        case kTime:
            switch (hint) {
                case kDateTime:
                    v.kind = kU64;
                    v.u64_val = SQLTimeUtils::ConvertToDateTime(dt_val.time).ToU64();
                    break;
                case kDate:
                    v.kind = kU64;
                    v.u64_val = SQLTimeUtils::ConvertToDate(dt_val.time).ToU64();
                    break;
                default:
                    v.kind = kDecimal;
                    v.dec_val = dt_val.time.ToDecimal(arena);
                    break;
            }
            break;

        case kDateTime:
            switch (hint) {
                case kTime:
                    v.kind = kU64;
                    v.u64_val = SQLTimeUtils::ConvertToTime(dt_val).ToU64();
                    break;
                case kDate:
                    v.kind = kU64;
                    v.u64_val = SQLTimeUtils::ConvertToDate(dt_val).ToU64();
                    break;
                default:
                    v.kind = kDecimal;
                    v.dec_val = dt_val.ToDecimal(arena);
                    break;
            }
            break;

        default:
            break;
    }
    return v;
}
    
Value Value::ToIntegral(base::Arena *arena, Kind hint) const {
    Value v = ToNumeric(arena, hint);
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
        case kDecimal:
            if (hint == kU64) {
                v.kind = kU64;
                v.u64_val = v.dec_val->ToU64();
            } else {
                v.kind = kI64;
                v.i64_val = v.dec_val->ToI64();
            }
            break;
        default:
            break;
    }
    return v;
}
    
Value Value::ToFloating(base::Arena *arena, Kind hint) const {
    Value v = ToNumeric(arena, hint);
    switch (v.kind) {
        case kNull:
            break;
        case kF64:
            break;
        case kI64:
            v.kind = kF64;
            v.f64_val = static_cast<double>(v.i64_val);
            break;
        case kU64:
            v.kind = kF64;
            v.f64_val = static_cast<double>(v.u64_val);
            break;
        case kDecimal:
            v.kind = kF64;
            v.f64_val = v.dec_val->ToF64();
            break;
        default:
            break;
    }
    return v;
}
    
bool Value::StrictEquals(const Value &v) const {
    
    switch (kind) {
        case kNull:
            return v.kind == kNull;
        case kF64:
            return v.kind == kind && v.f64_val == f64_val;
        case kI64:
            return v.kind == kind && v.i64_val == i64_val;
        case kString:
            return v.kind == kind && ::strncmp(v.str_val->data(),
                                               str_val->data(),
                                               str_val->size()) == 0;
        case kDecimal:
            return v.kind == kind && v.dec_val->Compare(dec_val) == 0;
        case kDate:
            return v.kind == kind && v.dt_val.date.ToU64() == dt_val.date.ToU64();
        case kTime:
            return v.kind == kind && ::memcmp(&dt_val.time, &v.dt_val.time,
                                              sizeof(SQLTime)) == 0;
        case kDateTime:
            return v.kind == kind && ::memcmp(&dt_val, &v.dt_val,
                                              sizeof(SQLDateTime)) == 0;
        default:
            break;
    }
    return false;
}
    
#define COMPARE_VAL(lhs, rhs) \
    ((lhs) == (rhs) ? 0 : ((lhs) < (rhs) ? -1 : 1))
    
int Value::Compare(const Value &rhs, base::Arena *arena) const {
    DCHECK_NE(kNull, kind);
    DCHECK_NE(kNull, rhs.kind);
    
    base::ScopedArena scoped_buf;
    if (arena == nullptr) {
        arena = &scoped_buf;
    }
    Value r;
    switch (kind) {
        case kF64:
            r = rhs.ToFloating(arena);
            return COMPARE_VAL(f64_val, r.f64_val);
        case kI64:
            r = rhs.ToIntegral(arena);
            if (r.kind == kI64) {
                return COMPARE_VAL(i64_val, r.i64_val);
            } else {
                if (i64_val < 0) {
                    return -1;
                } else {
                    return COMPARE_VAL(u64_val, r.u64_val);
                }
            }
        case kU64:
            r = rhs.ToIntegral(arena);
            if (r.kind == kU64) {
                return COMPARE_VAL(u64_val, r.u64_val);
            } else {
                if (r.i64_val < 0) {
                    return 1;
                } else {
                    return COMPARE_VAL(u64_val, r.u64_val);
                }
            }
        case kString:
            if (rhs.kind == kString) {
                return ::strncmp(str_val->data(), rhs.str_val->data(),
                                 str_val->size());
            }
            return -rhs.Compare(*this, arena);
        case kDecimal:
            r = rhs.ToNumeric(arena, kDecimal);
            if (r.is_floating()) {
                auto l = ToFloating(arena);
                return COMPARE_VAL(l.f64_val, r.f64_val);
            }
            DCHECK_EQ(kDecimal, r.kind);
            if (r.is_decimal()) {
                int max_exp = std::max(r.dec_val->exp(), dec_val->exp());
                auto lval =   dec_val->NewPrecision(max_exp, arena);
                auto rval = r.dec_val->NewPrecision(max_exp, arena);
                return lval->Compare(rval);
            }
            break;
        case kTime:
            r = rhs.ToIntegral(arena, kTime);
            return COMPARE_VAL(dt_val.time.ToU64(), r.u64_val);
        case kDate:
            r = rhs.ToIntegral(arena, kDate);
            return COMPARE_VAL(dt_val.date.ToU64(), r.u64_val);
        case kDateTime:
            r = rhs.ToIntegral(arena, kDateTime);
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
                    rv = rv.ToNumeric(ctx->arena());
                    if (rv.is_floating()) {
                        rv.f64_val = -rv.f64_val;
                    } else if (rv.is_integral()) {
                        rv.i64_val = -rv.i64_val;
                    } else {
                        DCHECK(rv.is_decimal());
                        rv.dec_val = rv.dec_val->Minus(ctx->arena());
                    }
                    ctx->set_result(rv);
                }
                break;
                
            case SQL_NOT:
                if (ctx->is_not_null()) {
                    rv = rv.ToNumeric(ctx->arena());
                    if (rv.is_floating()) {
                        rv.kind = Value::kI64;
                        rv.i64_val = !!rv.f64_val;
                    } else if (rv.is_decimal()) {
                        rv.kind = Value::kI64;
                        rv.i64_val = rv.dec_val->zero();
                    }
                    ctx->set_result(rv);
                }
                break;
                
            case SQL_BIT_INV:
                if (ctx->is_not_null()) {
                    rv = rv.ToIntegral(ctx->arena(), Value::kU64);
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
                lhs = lhs.ToNumeric(ctx->arena()); \
                rhs = ctx->result().ToNumeric(ctx->arena(), lhs.kind); \
                if (lhs.is_floating()) { \
                    rhs = rhs.ToFloating(ctx->arena()); \
                } else if (rhs.is_floating()) { \
                    lhs = lhs.ToFloating(ctx->arena()); \
                } \
                if (lhs.is_decimal()) { \
                    rhs = rhs.ToDecimal(ctx->arena()); \
                } else if (rhs.is_decimal()) { \
                    lhs = lhs.ToDecimal(ctx->arena()); \
                }

#define DO_REAL_NUMBER(op) \
                DCHECK_EQ(lhs.kind, rhs.kind); \
                DCHECK_EQ(lhs.is_number(), rhs.is_number()); \
                switch (lhs.kind) { \
                    case Value::kI64: \
                        rv.i64_val = op(lhs.i64_val, rhs.i64_val, ctx); \
                        break; \
                    case Value::kU64: \
                        rv.u64_val = op(lhs.u64_val, rhs.u64_val, ctx); \
                        break; \
                    case Value::kF64: \
                        rv.f64_val = op(lhs.f64_val, rhs.f64_val, ctx); \
                        break; \
                    case Value::kDecimal: \
                        AdjustPrecision(&lhs.dec_val, &rhs.dec_val, ctx); \
                        rv.dec_val = op(lhs.dec_val, rhs.dec_val, ctx); \
                        break; \
                    default: \
                        break; \
                } \
                rv.kind = lhs.kind; \
                ctx->set_result(rv)

#define DEF_REAL_ARITH(op) \
                TO_REAL_NUMBER \
                DO_REAL_NUMBER(op)
                
#define DO_PLUS(lval, rval, ctx) DoArithPlus(lval, rval, ctx)
#define DO_SUB(lval, rval, ctx)  DoArithSub(lval, rval, ctx)
#define DO_MUL(lval, rval, ctx)  DoArithMul(lval, rval, ctx)
#define DO_DIV(lval, rval, ctx)  DoArithDiv(lval, rval, ctx)
#define DO_MOD(lval, rval, ctx)  DoArithMod(lval, rval, ctx)

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
                if (rhs.IsZero()) {
                    return MAI_CORRUPTION("Division zero.");
                }
                DO_REAL_NUMBER(DO_DIV);
                break;
            case SQL_MOD:
                TO_REAL_NUMBER
                if (rhs.IsZero()) {
                    return MAI_CORRUPTION("Division zero.");
                }
                DO_REAL_NUMBER(DO_MOD);
                break;
                
#undef DEF_REAL_ARITH
#undef TO_REAL_NUMBER
#undef DO_REAL_NUMBER
            
#define DEF_INT_ARITH(op) \
                lhs = lhs.ToIntegral(ctx->arena(), Value::kU64); \
                rhs = ctx->result().ToIntegral(ctx->arena(), lhs.kind); \
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
    } else if (operands_count_ > 2) {
        // TODO:
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
    } else if (cd->CoverDecimal()) {
        v.kind = Value::kDecimal;
        v.dec_val = ctx->input()->GetDecimal(cd);
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
    
////////////////////////////////////////////////////////////////////////////////
/// Evaluation
////////////////////////////////////////////////////////////////////////////////

class EvalExpressionBuilder : public ast::VisitorWithStack<eval::Expression *> {
public:
    EvalExpressionBuilder(const VirtualSchema *env,
                          ast::ErrorBreakListener *listener,
                          eval::Factory *factory)
        : ast::VisitorWithStack<eval::Expression *>(DCHECK_NOTNULL(listener))
        , env_(env)
        , factory_(DCHECK_NOTNULL(factory)) {}
    
    virtual void VisitProjectionColumn(ast::ProjectionColumn *node) override {
        node->expr()->Accept(this);
    }
    
    virtual void VisitIdentifier(ast::Identifier *node) override {
        if (node->placeholder()) {
             Raise(MAI_CORRUPTION("Placeholder: ?"));
            return;
        }
        if (!env_) {
            Raise(MAI_CORRUPTION("Name: " + node->ToString() + " not in "
                                 "envirenment"));
            return;
        }
        auto col = env_->FindOrNull(node->ToString());
        if (!col) {
            Raise(MAI_CORRUPTION("Name: " + node->ToString() + " not found"));
            return;
        }
        Push(factory_->NewVariable(env_, col->index()));
    }
    
    virtual void VisitPlaceholder(ast::Placeholder *node) override {
        Raise(MAI_CORRUPTION("Placeholder: ?"));
    }
    
    virtual void VisitSubquery(ast::Subquery *node) override {
        Raise(MAI_CORRUPTION("Subquery"));
    }
    
    virtual void VisitLiteral(ast::Literal *node) override {
        switch (node->type()) {
            case ast::Literal::APPROX:
                Push(factory_->NewConstF64(node->approx_val()));
                break;
            case ast::Literal::INTEGER:
                Push(factory_->NewConstI64(node->integer_val()));
                break;
            case ast::Literal::STRING:
                Push(factory_->NewConstStr(node->string_val()));
                break;
            case ast::Literal::DECIMAL:
                Push(factory_->NewConstDec(node->decimal_val()));
                break;
            case ast::Literal::NULL_VAL:
                Push(factory_->NewConstNull());
                break;
            case ast::Literal::DEFAULT_PLACEHOLDER:
                Raise(MAI_CORRUPTION("Placeholder: DEFAULT"));
                break;
            default:
                DLOG(FATAL) << "noreached";
                break;
        }
    }
    
    virtual void VisitUnaryExpression(ast::UnaryExpression *node) override {
        node->operand(0)->Accept(this);
        auto operand = Pop();
        Push(factory_->NewUnary(node->op(), operand));
    }
    
    virtual void VisitBinaryExpression(ast::BinaryExpression *node) override {
        ProcessBinaryExpression(node);
    }
    
    virtual void VisitAssignment(ast::Assignment *node) override {
        // TODO:
        Raise(MAI_NOT_SUPPORTED("TODO:"));
    }
    
    virtual void VisitComparison(ast::Comparison *node) override {
        ProcessBinaryExpression(node);
    }
    
    virtual void VisitMultiExpression(ast::MultiExpression *node) override {
        std::vector<eval::Expression *> rhs;
        node->lhs()->Accept(this);
        auto lhs = Pop();
        
        for (auto expr : *node->rhs()) {
            expr->Accept(this);
            rhs.push_back(Pop());
        }
        Push(factory_->NewMulti(node->op(), lhs, rhs));
    }
    
private:
    void ProcessBinaryExpression(ast::FixedOperand<2> *node) {
        node->lhs()->Accept(this);
        auto lhs = Pop();
        
        node->rhs()->Accept(this);
        auto rhs = Pop();
        
        Push(factory_->NewBinary(node->op(), lhs, rhs));
    }
    
    const VirtualSchema *const env_;
    eval::Factory *factory_;
}; // class EvalExpressionBuilder
    
/*static*/ eval::Expression *
Evaluation::BuildExpression(const VirtualSchema *env, ast::Expression *ast,
                            base::Arena *arena) {
    eval::Expression *result = nullptr;
    auto rs = BuildExpression(env, ast, arena, &result);
    return !rs ? nullptr : result;
}
    
/*static*/ Error
Evaluation::BuildExpression(const VirtualSchema *env, ast::Expression *ast,
                            base::Arena *arena, eval::Expression **result) {
    
    eval::Factory factory(arena);
    ast::ErrorBreakVisitor box;
    EvalExpressionBuilder builder(env, &box, &factory);
    box.set_delegated(&builder);
    
    ast->Accept(&box);
    if (box.error().ok()) {
        *result = builder.Take(nullptr);
    }

    return box.error();
}

} // namespace sql
    
} // namespace mai
