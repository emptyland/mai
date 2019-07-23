#include "nyaa/hir-rewrite.h"
#include "nyaa/high-level-ir.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {

namespace hir {
    
class ReplacementRewriter {
public:
    ReplacementRewriter(Function *target, BasicBlock *begin, BasicBlock *end)
        : target_(DCHECK_NOTNULL(target)) {
        begin->GetAfterAll(&filter_);
    }
    
    void Run(Value *old_val, Value *new_val);
    
    void RunUse(Use *use, Value *old_val, Value *new_val);
    
private:
    
    Function *target_;
    std::set<BasicBlock *> filter_;
}; // class ReplacementRewriter
    
    
void ReplacementRewriter::Run(Value *old_val, Value *new_val) {
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
            Value::InstID kind = inst->kind(); (void)kind;
            Value *lhs = nullptr, *rhs = nullptr;
            CastPriority prio;
            if (inst->lhs() == old_val) {
                lhs = new_val;
                rhs = inst->rhs();
                prio = hir::GetCastPriority(new_val->type(), inst->rhs()->type());
            } else {
                DCHECK_EQ(inst->rhs(), old_val);
                lhs = inst->lhs();
                rhs = new_val;
                prio = hir::GetCastPriority(inst->lhs()->type(), new_val->type());
            }

            switch (prio.how) {
                case CastPriority::kLHS:
                    lhs = EmitCastIfNeed(target_, nullptr, prio.type, lhs, inst->line());
                    inst->owns()->InsertValueBefore(inst, lhs);
                    break;
                case CastPriority::kRHS:
                    rhs = EmitCastIfNeed(target_, nullptr, prio.type, rhs, inst->line());
                    inst->owns()->InsertValueBefore(inst, rhs);
                    break;
                case CastPriority::kBoth:
                    lhs = EmitCastIfNeed(target_, nullptr, prio.type, lhs, inst->line());
                    inst->owns()->InsertValueBefore(inst, lhs);
                    rhs = EmitCastIfNeed(target_, nullptr, prio.type, rhs, inst->line());
                    inst->owns()->InsertValueBefore(inst, rhs);
                    break;
                case CastPriority::kKeep:
                    inst->ReplaceUse(old_val, new_val);
                    use->RemoveFromList();
                    new_val->AddUse(new (target_->arena()) Use(inst));
                    return;
                case CastPriority::kNever:
                default:
                    DLOG(FATAL) << "Noreached!";
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
            
        DECL_HIR_CAST(DEFINE_CASE) {
            CastInst *inst = down_cast<hir::CastInst>(use->val);
            if (new_val->type() == inst->type()) {
                inst->RemoveFromOwns();
                use->RemoveFromList();
                Run(inst, new_val);
                return;
            }
            
            Value *repl = EmitCastIfNeed(target_, nullptr, new_val->type(), inst->from(),
                                         inst->line());
            inst->owns()->InsertValueBefore(inst, repl);
            inst->RemoveFromOwns();
            use->RemoveFromList();
            Run(inst, repl);
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
    
Value *EmitCastIfNeed(Function *target, BasicBlock *bb, Type::ID type, Value *from, int line) {
    if (type == from->type()) {
        return from;
    }
    Value::InstID inst = hir::GetCastAction(type, from->type());
    return EmitCast(target, bb, inst, from, line);
}
    
Value *EmitCast(Function *target, BasicBlock *bb, Value::InstID inst, Value *from, int line) {
    switch (inst) {
    #define DEFINE_CAST(name) \
        case hir::Value::k##name: \
            return target->name(bb, from, line);
        DECL_HIR_CAST(DEFINE_CAST)
    #undef DEFINE_CAST
        case hir::Value::kMaxInsts:
        default:
            DLOG(FATAL) << "Noreached!";
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
            DLOG(FATAL) << "Noreached!";
            break;
    }
    return nullptr;
}
    
void RewriteReplacement(Function *target, BasicBlock *begin, BasicBlock *end, Value *old_val,
                        Value *new_val) {
    ReplacementRewriter rewriter(target, begin, end);
    rewriter.Run(old_val, new_val);
}
    
} // namespace hir

} // namespace nyaa
    
} // namespace mai
