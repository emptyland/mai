#include "nyaa/hir-rewrite.h"
#include "nyaa/high-level-ir.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/object-factory.h"
#include "nyaa/nyaa-values.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {

namespace hir {
    
class ReplacementRewriter {
public:
    ReplacementRewriter(NyaaCore *core, Function *target, BasicBlock *begin, BasicBlock *end)
        : N_(DCHECK_NOTNULL(core))
        , target_(DCHECK_NOTNULL(target)) {
        begin->GetAfterAll(&filter_);
    }
    
    void Run(Value *old_val, Value *new_val);
private:
    
    void RunUse(Use *use, Value *old_val, Value *new_val);
    
    NyaaCore *N_;
    Function *target_;
    std::set<BasicBlock *> filter_;
}; // class ReplacementRewriter
    
    
void ReplacementRewriter::Run(Value *old_val, Value *new_val) {
    if (old_val->type() == new_val->type()) {
        for (BasicBlock *bb : filter_) {
            bb->ReplaceAllUses(old_val, new_val);
        }
        return;
    }
    
    std::vector<Use *> uses;
    for (Use *use = old_val->uses_begin(); use != old_val->uses_end(); use = use->next) {
        if (filter_.find(use->val->owns()) != filter_.end()) {
            uses.push_back(use);
        }
    }
    
    for (auto use : uses) {
        RunUse(use, old_val, new_val);
    }
}
    
void ReplacementRewriter::RunUse(Use *use, Value *old_val, Value *new_val) {
    switch (use->val->kind()) {

#define DEFINE_CASE(name) case Value::k##name:
        DECL_HIR_BINARY(DEFINE_CASE) {
            BinaryInst *inst = down_cast<BinaryInst>(use->val);
            //Value::InstID kind = inst->kind(); (void)kind;
            Value *lhs = nullptr, *rhs = nullptr;
            CastPriority prio;
            if (inst->lhs() == old_val) {
                lhs = new_val;
                rhs = inst->rhs();
                prio = hir::GetArithCastPriority(new_val->type(), inst->rhs()->type());
            } else {
                DCHECK_EQ(inst->rhs(), old_val);
                lhs = inst->lhs();
                rhs = new_val;
                prio = hir::GetArithCastPriority(inst->lhs()->type(), new_val->type());
            }

            switch (prio.how) {
                case CastPriority::kLHS:
                    lhs = EmitCastIfNeed(N_, target_, nullptr, prio.type, lhs, inst->line());
                    if (!lhs->IsConstant()) {
                        inst->owns()->InsertValueBefore(inst, lhs);
                    }
                    break;
                case CastPriority::kRHS:
                    rhs = EmitCastIfNeed(N_, target_, nullptr, prio.type, rhs, inst->line());
                    if (!rhs->IsConstant()) {
                        inst->owns()->InsertValueBefore(inst, rhs);
                    }
                    break;
                case CastPriority::kBoth:
                    lhs = EmitCastIfNeed(N_, target_, nullptr, prio.type, lhs, inst->line());
                    if (!lhs->IsConstant()) {
                        inst->owns()->InsertValueBefore(inst, lhs);
                    }
                    rhs = EmitCastIfNeed(N_, target_, nullptr, prio.type, rhs, inst->line());
                    if (!rhs->IsConstant()) {
                        inst->owns()->InsertValueBefore(inst, rhs);
                    }
                    break;
                case CastPriority::kKeep:
                    inst->ReplaceUse(old_val, new_val);
                    use->RemoveFromList();
                    new_val->AddUse(new (target_->arena()) Use(inst));
                    return;
                case CastPriority::kNever:
                default:
                    NOREACHED();
                    break;
            }
            Value::InstID iid = TransformBinaryInst(prio.type, inst->kind());
            DCHECK_LT(iid, Value::kMaxInsts);
            Value *repl = EmitBinary(target_, nullptr, iid, lhs, rhs, inst->line());
            inst->owns()->InsertValueBefore(inst, repl);
            inst->RemoveFromOwns();
            use->RemoveFromList();
            Run(inst, repl);
        } break;
            
        DECL_HIR_COMPARE(DEFINE_CASE) {
            Comparator *inst = down_cast<Comparator>(use->val);
            Value *lhs = nullptr, *rhs = nullptr;
            CastPriority prio;
            if (inst->lhs() == old_val) {
                lhs = new_val;
                rhs = inst->rhs();
                prio = hir::GetArithCastPriority(new_val->type(), inst->rhs()->type());
            } else {
                DCHECK_EQ(inst->rhs(), old_val);
                lhs = inst->lhs();
                rhs = new_val;
                prio = hir::GetArithCastPriority(inst->lhs()->type(), new_val->type());
            }
            
            switch (prio.how) {
                case CastPriority::kLHS:
                    lhs = EmitCastIfNeed(N_, target_, nullptr, prio.type, lhs, inst->line());
                    if (!lhs->IsConstant()) {
                        inst->owns()->InsertValueBefore(inst, lhs);
                    }
                    break;
                case CastPriority::kRHS:
                    rhs = EmitCastIfNeed(N_, target_, nullptr, prio.type, rhs, inst->line());
                    if (!rhs->IsConstant()) {
                        inst->owns()->InsertValueBefore(inst, rhs);
                    }
                    break;
                case CastPriority::kBoth:
                    lhs = EmitCastIfNeed(N_, target_, nullptr, prio.type, lhs, inst->line());
                    if (!lhs->IsConstant()) {
                        inst->owns()->InsertValueBefore(inst, lhs);
                    }
                    rhs = EmitCastIfNeed(N_, target_, nullptr, prio.type, rhs, inst->line());
                    if (!rhs->IsConstant()) {
                        inst->owns()->InsertValueBefore(inst, rhs);
                    }
                    break;
                case CastPriority::kKeep:
                    inst->ReplaceUse(old_val, new_val);
                    use->RemoveFromList();
                    new_val->AddUse(new (target_->arena()) Use(inst));
                    return;
                case CastPriority::kNever:
                default:
                    NOREACHED();
                    break;
            }
            Value::InstID iid = TransformComparatorInst(prio.type);
            DCHECK_LT(iid, Value::kMaxInsts);
            Value *repl = EmitComparator(target_, nullptr, iid, inst->op_code(), lhs, rhs,
                                         inst->line());
            inst->owns()->InsertValueBefore(inst, repl);
            inst->RemoveFromOwns();
            use->RemoveFromList();
            Run(inst, repl);
        } break;

        DECL_HIR_CAST(DEFINE_CASE) {
            CastInst *inst = down_cast<hir::CastInst>(use->val);
            if (new_val->type() == inst->type()) {
                inst->RemoveFromOwns();
                use->RemoveFromList();
                Run(inst, new_val);
                return;
            }
            
            Value *repl = EmitCastIfNeed(N_, target_, nullptr, new_val->type(), inst->from(),
                                         inst->line());
            if (!repl->IsConstant()) {
                inst->owns()->InsertValueBefore(inst, repl);
            }
            inst->RemoveFromOwns();
            use->RemoveFromList();
            Run(inst, repl);
        } break;
            
        case Value::kBranch: {
            Branch *inst = Branch::Cast(use->val);
            DCHECK_EQ(inst->cond(), old_val);
            if (new_val->IsIntTy()) {
                inst->set_cond(new_val);
                new_val->AddUse(new (target_->arena()) Use(inst));
                use->RemoveFromList();
                return;
            }

            switch (new_val->type()) {
                case Type::kFloat: {
                    Value *zero = target_->FloatVal(0, inst->line());
                    //zero->set_float_val(0);
                    
                    Value *cond = target_->FCmp(nullptr, Compare::kEQ, new_val, zero, inst->line());
                    inst->set_cond(cond);
                    cond->AddUse(new (target_->arena()) Use(inst));
                    inst->owns()->InsertValueBefore(inst, cond);
                    use->RemoveFromList();
                } break;
                    
                default: {
                    Value *inbox = nullptr;
                    if (!new_val->IsObjectTy()) {
                        inbox = target_->Inbox(nullptr, new_val, inst->line());
                        inst->owns()->InsertValueBefore(inst, inbox);
                    }
                    CallBuiltin *val = target_->CallBuiltin(nullptr, BuiltinFunction::kIsTrue, 1,
                                                            inst->line());
                    val->SetArgument(0, !inbox ? new_val : inbox);
                    inst->owns()->InsertValueBefore(inst, val);
                    val->AddUse(new (target_->arena()) Use(inst));
                    inst->set_cond(val);
                    use->RemoveFromList();
                } break;
            }
        } break;

        default: {
            bool ok = use->val->ReplaceUse(old_val, new_val);
            DCHECK(ok); (void)ok;
            use->RemoveFromList();
            new_val->AddUse(new (target_->arena()) Use(use->val));
        } break;
#undef DEFINE_CASE
    }
}
    
Value *EmitCastIfNeed(NyaaCore *N, Function *target, BasicBlock *bb, Type::ID type, Value *from,
                      int line) {
    if (type == from->type()) {
        return from;
    }
    Value::InstID inst = hir::GetCastAction(type, from->type());
    return EmitCast(N, target, bb, inst, from, line);
}
    
Value *EmitCast(NyaaCore *N, Function *target, BasicBlock *bb, Value::InstID inst, Value *from,
                int line) {
    if (from->IsConstant()) {
        Constant *src = Constant::Cast(from);
        switch (inst) {
            case Value::kInbox:
                return target->ObjectVal(src->AsObject(N), line);
                
            case Value::kIToF:
                return target->FloatVal(static_cast<double>(src->i62_val()), line);
                
            case Value::kIToL:
                return target->LongVal(NyInt::NewI64(src->i62_val(), N->factory()), line);
                
            case Value::kLToF:
                return target->FloatVal(src->long_val()->ToF64(), line);
                
            case Value::kLToI:
                return target->IntVal(src->long_val()->ToI64(), line);
                
            case Value::kFToI:
                return target->IntVal(static_cast<int64_t>(src->f64_val()), line);
                
            case Value::kFToL:
                return target->LongVal(NyInt::NewI64(static_cast<int64_t>(src->f64_val()),
                                                     N->factory()), line);
                
            case Value::kAToI: {
                bool ok;
                int64_t i64 = src->string_val()->TryI64(&ok);
                if (ok && (i64 >= NySmi::kMinValue && i64 <= NySmi::kMaxValue)) {
                    return target->IntVal(i64, line);
                }
            } break;
                
            case Value::kAToL: {
                bool ok;
                NyInt *lll = src->string_val()->TryInt(N, &ok);
                if (ok) {
                    return target->LongVal(lll, line);
                }
            } break;
                
            case Value::kAToF: {
                bool ok;
                f64_t f64 = src->string_val()->TryF64(&ok);
                if (ok) {
                    return target->FloatVal(f64, line);
                }
            } break;
            default:
                break;
        }
    }

    switch (inst) {
    #define DEFINE_CAST(name) \
        case hir::Value::k##name: \
            return target->name(bb, from, line);
        DECL_HIR_CAST(DEFINE_CAST)
    #undef DEFINE_CAST
        case hir::Value::kMaxInsts:
        default:
            NOREACHED();
            break;
    }
    return nullptr;
}
    
Value *EmitBinary(Function *target, BasicBlock *bb, Value::InstID inst, Value *lhs, Value *rhs,
                  int line) {
    switch (inst) {
    #define DEFINE_CAST(name) \
        case hir::Value::k##name: \
            return target->name(bb, lhs, rhs, line);
        DECL_HIR_BINARY(DEFINE_CAST)
    #undef DEFINE_CAST
        case hir::Value::kMaxInsts:
        default:
            NOREACHED();
            break;
    }
    return nullptr;
}
    
Value *EmitComparator(Function *target, BasicBlock *bb, Value::InstID inst, Compare::Op op,
                      Value *lhs, Value *rhs, int line) {
    switch (inst) {
    #define DEFINE_CAST(name) \
        case hir::Value::k##name: \
            return target->name(bb, op, lhs, rhs, line);
        DECL_HIR_COMPARE(DEFINE_CAST)
    #undef DEFINE_CAST
        case hir::Value::kMaxInsts:
        default:
            NOREACHED();
            break;
    }
    return nullptr;
}
    
void RewriteReplacement(NyaaCore *N, Function *target, BasicBlock *begin, BasicBlock *end,
                        Value *old_val, Value *new_val) {
    ReplacementRewriter rewriter(N, target, begin, end);
    rewriter.Run(old_val, new_val);
}
    
} // namespace hir

} // namespace nyaa
    
} // namespace mai
