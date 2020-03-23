#include "sql/evaluation.h"
#include "sql/eval-factory.h"
#include "sql/heap-tuple.h"
#include "sql/ast-visitor.h"
#include "sql/ast.h"
#include "core/decimal-v2.h"
#include "base/arenas.h"
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
    
/*virtual*/ Error Constant::Evaluate(Context *ctx) {
    ctx->set_result(data_);
    return Error::OK();
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
                NOREACHED();
                break;
        }
        v.dt_val = ctx->input()->GetDateTime(cd);
    } else {
        NOREACHED();
    }
    
    ctx->set_result(v);
    return Error::OK();
}
    
/*virtual*/ Error Invocation::Evaluate(Context *ctx) {
    Value v;
    v.kind = Value::kNull;
    switch (fnid_) {
        case SQL_F_ABS: {
            DCHECK_GE(parameters_count(), 1);
            Error rs = parameter(0)->Evaluate(ctx);
            if (!rs) {
                return rs;
            }
            v = ctx->result().ToNumeric(ctx->arena());
            switch (v.kind) {
                case Value::kI64:
                    v.i64_val = v.i64_val < 0 ? -v.i64_val : v.i64_val;
                    break;
                case Value::kF64:
                    v.f64_val = ::fabs(v.f64_val);
                    break;
                case Value::kDecimal:
                    if (v.dec_val->negative()) {
                        v.dec_val = v.dec_val->Abs(ctx->arena());
                    }
                    break;
                default:
                    break;
            }
        } break;

        case SQL_F_NOW:
            v.kind = Value::kDateTime;
            v.dt_val = SQLDateTime::Now();
            break;
            
        case SQL_F_DATE:
            v.kind = Value::kDate;
            v.dt_val.date = SQLDate::Now();
            break;

        default:
            return MAI_NOT_SUPPORTED("TODO:");
    }
    ctx->set_result(v);
    return Error::OK();
}
    
/*virtual*/ Error Aggregate::Evaluate(Context *ctx) {
    if (ctx->init()) {
        counter_ = 0;
    }
    
    Error rs;
    switch (fnid_) {            
        case SQL_F_SUM: {
            DCHECK_EQ(parameters_count(), 1);
            rs = parameter(0)->Evaluate(ctx);
            if (!rs) {
                return rs;
            }
            auto arg0 = ctx->result();
            rs = SUM(arg0, ctx);
            val_ = mid_;
            if (val_.kind == Value::kDecimal) {
                val_.dec_val = val_.dec_val->Clone(ctx->arena());
            }
        } break;
            
        case SQL_F_AVG: {
            DCHECK_EQ(parameters_count(), 1);
            rs = parameter(0)->Evaluate(ctx);
            if (!rs) {
                return rs;
            }
            auto arg0 = ctx->result();
            rs = AVG(arg0, ctx);
            val_ = mid_;
            if (val_.kind == Value::kDecimal) {
                val_.dec_val = val_.dec_val->Clone(ctx->arena());
            }
        } break;
        
        case SQL_F_COUNT:
        case SQL_F_MAX:
        case SQL_F_MIN:
            // TODO:
        default:
            return MAI_NOT_SUPPORTED("TODO:");
    }
    ctx->set_result(val_);
    return rs;
}
    
Error Aggregate::SUM(Value arg0, Context *ctx) {
    arg0 = arg0.ToNumeric(ctx->arena());
    switch (arg0.kind) {
        case Value::kNull:
            if (ctx->init()) {
                mid_.kind = Value::kNull;
            }
            break;
            
        case Value::kF64:
            if (ctx->init()) {
                mid_ = arg0;
            } else {
                if (!mid_.is_floating()) {
                    mid_ = mid_.ToFloating(nullptr);
                }
                mid_.f64_val += arg0.f64_val;
            }
            break;
            
        case Value::kI64:
            if (ctx->init()) {
                bool neg = arg0.i64_val < 0;
                uint64_t val = neg ? -arg0.i64_val : arg0.i64_val;
                sum_val_ = SQLDecimal::NewP64(val, neg, 0, SQLDecimal::kMaxCap, &owned_arena_);
                mid_.kind = Value::kDecimal;
                mid_.dec_val = DCHECK_NOTNULL(sum_val_);
            } else {
                if (mid_.is_floating()) {
                    mid_.f64_val += arg0.i64_val;
                } else {
                    DCHECK_EQ(mid_.kind, Value::kDecimal);
                    sum_val_->Add(arg0.i64_val);
                }
            }
            break;
            
        case Value::kU64:
            if (ctx->init()) {
                sum_val_ = SQLDecimal::NewP64(arg0.u64_val, false, 0, SQLDecimal::kMaxCap,
                                              &owned_arena_);
                mid_.kind = Value::kDecimal;
                mid_.dec_val = DCHECK_NOTNULL(sum_val_);
            } else {
                if (mid_.is_floating()) {
                    mid_.f64_val += arg0.u64_val;
                } else {
                    DCHECK_EQ(mid_.kind, Value::kDecimal);
                    sum_val_->Add(arg0.u64_val);
                }
            }
            break;
            
        case Value::kDecimal:
            if (ctx->init()) {
                sum_val_ = SQLDecimal::NewUninitialized(SQLDecimal::kMaxCap, &owned_arena_);
                DCHECK_NOTNULL(sum_val_)->CopyFrom(arg0.dec_val);
                mid_.kind = Value::kDecimal;
                mid_.dec_val = sum_val_;
            } else {
                if (mid_.is_floating()) {
                    mid_.f64_val += arg0.dec_val->ToF64();
                } else {
                    DCHECK_EQ(mid_.kind, Value::kDecimal);
                    SQLDecimal *add = arg0.dec_val->NewPrecision(sum_val_->exp(), ctx->arena());
                    sum_val_->Add(add);
                }
            }
            break;
            
        default:
            NOREACHED();
            break;
    }
    return Error::OK();
}
    
Error Aggregate::AVG(Value arg0, Context *ctx) {
    Error rs = SUM(arg0, ctx);
    if (!rs) {
        return rs;
    }
    if (arg0.is_not_null()) {
        counter_++;
    }

    switch (mid_.kind) {
        case Value::kNull:
            val_.kind = Value::kNull;
            break;
            
        case Value::kF64:
            val_.kind = Value::kF64;
            val_.f64_val = mid_.f64_val / counter_;
            break;
            
        case Value::kI64:
            val_.kind = Value::kI64;
            val_.i64_val = mid_.i64_val / counter_;
            break;
            
        case Value::kU64:
            val_.kind = Value::kU64;
            val_.u64_val = mid_.u64_val / counter_;
            break;
            
        case Value::kDecimal:
            if (ctx->init()) {
                fin_val_ = SQLDecimal::NewUninitialized(SQLDecimal::kMaxCap, &owned_arena_);
                fin_val_->CopyFrom(mid_.dec_val);
                val_.kind = Value::kDecimal;
                val_.dec_val = fin_val_;
            } else {
                DCHECK_EQ(val_.kind, Value::kDecimal);
                fin_val_->CopyFrom(sum_val_);
                fin_val_->Div(counter_);
            }
            break;
            
        default:
            NOREACHED();
            break;
    }
    return rs;
}

} // namespace eval
    
////////////////////////////////////////////////////////////////////////////////
/// Evaluation
////////////////////////////////////////////////////////////////////////////////

class EvalExpressionBuilder : public ast::VisitorWithStack<eval::Expression *> {
public:
    EvalExpressionBuilder(const VirtualSchema *env, ast::ErrorBreakListener *listener,
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
        Raise(MAI_CORRUPTION("Placeholder: *"));
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
            case ast::Literal::BIND_PLACEHOLDER:
                Raise(MAI_CORRUPTION("Placeholder: Unbinded"));
                break;
            default:
                NOREACHED();
                break;
        }
    }
    
    virtual void VisitUnaryExpression(ast::UnaryExpression *node) override {
        eval::Operation *opt = factory_->NewOperation(node->op(), 1);
        node->operand(0)->Accept(this);
        opt->set_operand(0, Pop());
        Push(opt);
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
        eval::Operation *opt = factory_->NewOperation(node->op(), node->operands_count());
        for (size_t i = 0; i < node->operands_count(); ++i) {
            node->operand(static_cast<int>(i))->Accept(this);
            if (error().fail()) {
                return;
            }
            opt->set_operand(i, Pop());
        }
        Push(opt);
    }
    
    virtual void VisitCall(ast::Call *node) override {
        eval::Invocation *call = factory_->NewCall(node->fnid(), node->parameters_size());
        for (size_t i = 0; i < node->parameters_size(); ++i) {
            node->parameter(i)->Accept(this);
            if (error().fail()) {
                return;
            }
            call->set_parameter(i, Pop());
        }
        Push(call);
    }

    virtual void VisitAggregate(ast::Aggregate *node) override {
        size_t n_params = 0;
        eval::Aggregate *call = nullptr;
        if (ast::Placeholder *holder = ast::Placeholder::Cast(node->parameter(0))) {
            n_params = env_->columns_size();
            call = factory_->NewCall(node->fnid(), n_params, node->distinct());
            for (size_t i = 0; i < env_->columns_size(); ++i) {
                call->set_parameter(i, factory_->NewVariable(env_, env_->column(i)->index()));
            }
        } else {
            n_params = node->parameters_size();
            call = factory_->NewCall(node->fnid(), n_params, node->distinct());
            for (size_t i = 0; i < node->parameters_size(); ++i) {
                node->parameter(i)->Accept(this);
                call->set_parameter(i, Pop());
            }
        }
        Push(call);
    }
    
private:
    void ProcessBinaryExpression(ast::FixedOperand<2> *node) {
        eval::Operation *opt = factory_->NewOperation(node->op(), 2);
        node->lhs()->Accept(this);
        opt->set_operand(0, Pop());
        
        node->rhs()->Accept(this);
        opt->set_operand(1, Pop());
        
        Push(opt);
    }
    
    const VirtualSchema *const env_;
    eval::Factory *factory_;
}; // class EvalExpressionBuilder
    
/*static*/ eval::Expression *
Evaluation::BuildExpression(const VirtualSchema *env, ast::Expression *ast, base::Arena *arena) {
    eval::Expression *result = nullptr;
    auto rs = BuildExpression(env, ast, arena, &result);
    if (!rs) {
        DLOG(ERROR) << rs.ToString();
    }
    return !rs ? nullptr : result;
}
    
/*static*/ Error
Evaluation::BuildExpression(const VirtualSchema *env, ast::Expression *ast, base::Arena *arena,
                            eval::Expression **result) {
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
