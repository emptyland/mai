#include "nyaa/code-gen-utils.h"
#include "nyaa/high-level-ir.h"
#include "nyaa/ast.h"
#include "nyaa/profiling.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/builtin.h"
#include "nyaa/function.h"

namespace mai {
    
namespace nyaa {
    
using UpValTable = std::unordered_map<const ast::String *, hir::UpvalDesc,
    base::ArenaHash<const ast::String *>,
    base::ArenaEqualTo<const ast::String *>>;
    
class HIRGeneratorVisitor;
    
namespace {
    
class HIRBlockScope;

struct ValueBundle {
    hir::Value *value;
    HIRBlockScope *out_scope;
}; // struct ValueBundle
    
static bool IsPlaceholder(const ast::String *name) {
    return name->size() == 1 && name->data()[0] == '_';
}

static bool IsNotPlaceholder(const ast::String *name) { return !IsPlaceholder(name); }
    
class HIRBlockScope final {
public:
    using ValueTable = std::unordered_map<const ast::String *, ValueBundle,
        base::ArenaHash<const ast::String *>,
        base::ArenaEqualTo<const ast::String *>>;
    
    inline HIRBlockScope(HIRBlockScope *prev, bool is_br_edge = false)
        : prev_(prev) {
        DCHECK_NE(this, prev);
    }
    inline ~HIRBlockScope() {}
    
    DEF_VAL_PROP_RMW(ValueTable, values);
    DEF_PTR_GETTER(HIRBlockScope, prev);
    DEF_PTR_PROP_RW(hir::BasicBlock, loop_entry);
    DEF_PTR_PROP_RW(hir::BasicBlock, loop_exit);
    
    void PutValue(const ast::String *name, hir::Value *val) {
        DCHECK(!val->IsVoid());
        ValueBundle bundle;
        bundle.value = val;
        bundle.out_scope = nullptr;
        values_[name] = bundle;
    }
    
    inline std::tuple<hir::Value*, HIRBlockScope*> GetValueOrNullNested(const ast::String *name);
    
    inline hir::Value *GetValueOrNull(const ast::String *name) const {
        auto iter = values_.find(name);
        return iter == values_.end() ? nullptr : iter->second.value;
    }
    
private:
    HIRBlockScope *prev_;
    ValueTable values_;
    hir::BasicBlock *loop_entry_ = nullptr;
    hir::BasicBlock *loop_exit_ = nullptr;
}; // class BlockScope
    
class HIRGeneratorContext : public ast::VisitorContext {
public:
    explicit HIRGeneratorContext(ast::VisitorContext *x = nullptr) {
        if (HIRGeneratorContext *prev = Cast(x)) {
            scope_ = DCHECK_NOTNULL(prev->scope_);
            //bb_    = prev->bb_;
        }
    }
    ~HIRGeneratorContext() {}
    
    DEF_VAL_PROP_RW(int, n_result);
    DEF_VAL_PROP_RW(bool, lval);
    DEF_VAL_PROP_RW(int, lval_offset);
    DEF_VAL_PROP_RW(IVal, rval);
    DEF_PTR_PROP_RW_NOTNULL2(HIRBlockScope, scope);
    //DEF_PTR_PROP_RW(hir::BasicBlock, bb);
    
    static HIRGeneratorContext *Cast(ast::VisitorContext *ctx) {
        return !ctx ? nullptr : down_cast<HIRGeneratorContext>(ctx);
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(HIRGeneratorContext);
private:
    int n_result_ = 0;
    bool lval_ = false;
    int  lval_offset_ = 0;
    IVal rval_ = IVal::Void();
    HIRBlockScope *scope_ = nullptr;
    //hir::BasicBlock *bb_ = nullptr;
}; // CodeGeneratorContext
    
    
inline std::tuple<hir::Value*, HIRBlockScope*>
HIRBlockScope::GetValueOrNullNested(const ast::String *name) {
    HIRBlockScope *p = this;
    while (p) {
        auto iter = p->values_.find(name);
        if (iter != p->values_.end()) {
            if (iter->second.out_scope) {
                p = iter->second.out_scope;
            }
            return {iter->second.value, p};
        }
        p = p->prev_;
    }
    return {nullptr, nullptr};
}

inline static hir::Type::ID ConvType(BuiltinType ty) {
    switch (ty) {
        case kTypeSmi:
        case kTypeInt:
            return hir::Type::kInt;
        case kTypeMap:
            return hir::Type::kMap;
        case kTypeString:
            return hir::Type::kString;
        default:
            return hir::Type::kObject;
    }
    return hir::Type::kVoid;
}
    
}; // namespace

class HIRGeneratorVisitor final : public ast::Visitor {
public:
    HIRGeneratorVisitor(std::vector<hir::Type::ID> &&args, UpValTable &&upvals,
                        base::Arena *arena, Profiler *profiler)
        : args_(args)
        , upvals_(upvals)
        , arena_(arena)
        , profiler_(profiler) {
    }

    virtual ~HIRGeneratorVisitor() override {}
    
    DEF_VAL_GETTER(Error, error);
    DEF_PTR_GETTER(hir::Function, target);
    
#define ACCEPT(subx, ctx) (subx)->Accept(this, (ctx)); if (error_.fail()) return IVal::Void()
    
    virtual IVal VisitLambdaLiteral(ast::LambdaLiteral *node, ast::VisitorContext *x) override {
        target_ = hir::Function::New(arena_);
        
        HIRBlockScope trunk_scope(nullptr);
        HIRGeneratorContext top_ctx;
        top_ctx.set_scope(&trunk_scope);
        
        insert_ = target_->NewBB(nullptr);
        target_->set_entry(insert_);
        
        const size_t n_params = !node->params() ? 0 : node->params()->size();
        for (int i = 0; i < n_params; ++i) {
            hir::Value *val = target_->Alloca(args_[i], node->line());
            if (i < args_.size()) {
                // ignore
            } else {
                target_->Store(insert_, val, target_->Nil(node->line()), node->line());
            }
            target_->AddParameter(val);
            trunk_scope.PutValue(node->params()->at(i), val);
        }
        if (node->vargs()) {
            int64_t nvargs = args_.size() - n_params;
            for (int64_t i = 0; i < nvargs; ++i) {
                vargs_.push_back(args_[i]);
            }
        }
        ACCEPT(node->value(), &top_ctx);
        return IVal::Void();
    }
    
    virtual IVal VisitBlock(ast::Block *node, ast::VisitorContext *x) override {
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);
        
        HIRBlockScope scope(ctx->scope());
        HIRGeneratorContext ix;
        ix.set_scope(&scope);
        if (node->stmts()) {
            for (auto stmt : *node->stmts()) {
                ACCEPT(stmt, &ix);
            }
        }
        return IVal::Void();
    }
    
    virtual IVal VisitVarDeclaration(ast::VarDeclaration *node, ast::VisitorContext *x) override {
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);
        
        if (node->inits()) {
            HIRGeneratorContext rix(x);
            if (node->names()->size() > 1 && node->GetNWanted() < 0) {
                rix.set_n_result(static_cast<int>(node->names()->size()));
                IVal rval = ACCEPT(node->inits()->at(0), &rix);
                for (size_t i = 0; i < node->names()->size(); ++i) {
                    hir::Value *val = rval.node;
                    if (i > 0) {
                        val = target_->BaseOfStack(insert_, rval.node, static_cast<int>(i),
                                                   node->line());
                    }
                    
                    ctx->scope()->PutValue(node->names()->at(i), val);
                }
            } else {
                rix.set_n_result(1);
                
                for (size_t i = 0; i < node->names()->size(); ++i) {
                    if (i < node->inits()->size()) {
                        IVal rval = ACCEPT(node->inits()->at(i), &rix);
                        hir::Value *lval = target_->Alloca(rval.node->type(), node->line());
                        target_->Store(insert_, lval, rval.node, node->line());
                        ctx->scope()->PutValue(node->names()->at(i), lval);
                    } else {
                        hir::Value *lval = target_->Alloca(hir::Type::kObject, node->line());
                        target_->Store(insert_, lval, target_->Nil(node->line()), node->line());
                        ctx->scope()->PutValue(node->names()->at(i), lval);
                    }
                }
            }
        } else {
            for (auto name : *node->names()) {
                if (IsNotPlaceholder(name)) {
                    hir::Value *lval = target_->Alloca(hir::Type::kObject, node->line());
                    target_->Store(insert_, lval, target_->Nil(node->line()), node->line());
                    ctx->scope()->PutValue(name, lval);
                }
            }
        }
        return IVal::Void();
    }
    
    virtual IVal VisitAssignment(ast::Assignment *node, ast::VisitorContext *x) override {
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);

        HIRGeneratorContext rix(ctx);
        if (node->rvals()->size() == 1 && node->GetNWanted() < 0) {
            rix.set_n_result(static_cast<int>(node->lvals()->size()));
            IVal base = ACCEPT(node->rvals()->front(), &rix);
            
            HIRGeneratorContext lix(ctx);
            lix.set_rval(base);
            lix.set_lval(true);
            for (size_t i = 0; i < node->lvals()->size(); ++i) {
                lix.set_lval_offset(static_cast<int>(i));
                ACCEPT(node->lvals()->at(i), &lix);
            }
        } else {
            rix.set_n_result(1);
            size_t len = std::min(node->lvals()->size(), node->rvals()->size());
            
            HIRGeneratorContext lix(ctx);
            for (size_t i = 0; i < len; ++i) {
                IVal val = ACCEPT(node->rvals()->at(i), &rix);
                lix.set_lval(true);
                lix.set_rval(val);
                ACCEPT(node->lvals()->at(i), &lix);
            }
        }
        return IVal::Void();
    }
    
    virtual IVal VisitVariable(ast::Variable *node, ast::VisitorContext *x) override {
        if (IsPlaceholder(node->name())) {
            return IVal::Void();
        }
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);
        hir::Value *val = nullptr;
        HIRBlockScope *scope = nullptr;
        std::tie(val, scope) = ctx->scope()->GetValueOrNullNested(node->name());
        if (ctx->lval()) {
            if (val) {
                if (val->IsFromAlloca()) {
                    target_->Store(insert_, val, ctx->rval().node, node->line());
                } else {
                    scope->PutValue(node->name(), ctx->rval().node);
                }
                return IVal::Void();
            }
            if (auto iter = upvals_.find(node->name()); iter != upvals_.end()) {
                target_->StoreUp(insert_, iter->second.slot, ctx->rval().node, node->line());
                return IVal::Void();
            }
            target_->StoreGlobal(insert_, node->name(), ctx->rval().node, node->line());
            return IVal::Void();
        } else {
            if (val) {
                if (val->IsFromAlloca()) {
                    val = target_->Load(insert_, val, node->line());
                }
                return IVal::HIR(val);
            }
            if (auto iter = upvals_.find(node->name()); iter != upvals_.end()) {
                val = target_->LoadUp(insert_, hir::Type::kObject, iter->second.slot, node->line());
                return IVal::HIR(val);
            }
            val = target_->LoadGlobal(insert_, hir::Type::kObject, node->name(), node->line());
            return IVal::HIR(val);
        }
    }
    
    virtual IVal VisitReturn(ast::Return *node, ast::VisitorContext *x) override {
        hir::Ret *ret = nullptr;
        if (!node->rets()) {
            ret = target_->Ret(insert_, node->line());
            ret->set_wanted(0);
            return IVal::Void();
        }

        int nrets = node->GetNRets();
        HIRGeneratorContext ix(x);
        ix.set_n_result(nrets < 0 ? -1 : 1);
        std::vector<hir::Value *> ret_vals;
        for (auto rv : *node->rets()) {
            IVal val = ACCEPT(rv, &ix);
            DCHECK_NE(IVal::kVoid, val.kind);
            ret_vals.push_back(val.node);
        }
        ret = target_->Ret(insert_, node->line());
        for (auto rv : ret_vals) {
            ret->AddRetVal(rv);
        }
        ret->set_wanted(nrets);
        return IVal::Void();
    }
    
    virtual IVal VisitMultiple(ast::Multiple *node, ast::VisitorContext *x) override {
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);
        HIRGeneratorContext ix(ctx);
        
        std::vector<hir::Value *> operands;
        for (int i = 0; i < node->n_operands(); ++i) {
            IVal operand = ACCEPT(node->operand(i), &ix);
            DCHECK_EQ(IVal::kHIR, operand.kind);
            operands.push_back(operand.node);
        }
        
        switch (node->op()) {
            case Operator::kAdd:
                // TODO:
                return EmitAdd(insert_, operands[0], operands[1], node->line());
                break;
                
            case Operator::kSub:
                // TODO:
                return EmitSub(insert_, operands[0], operands[1], node->line());
                break;
                
            case Operator::kMul:
                // TODO:
                return EmitMul(insert_, operands[0], operands[1], node->line());
                break;
                
            case Operator::kDiv:
                // TODO:
                return EmitDiv(insert_, operands[0], operands[1], node->line());
                break;
                
            case Operator::kMod:
                // TODO:
                return EmitMod(insert_, operands[0], operands[1], node->line());
                break;
                
            default:
                DLOG(FATAL) << "Noreached!";
                break;
        }
        return IVal::Void();
    }
    
    virtual IVal VisitCall(ast::Call *node, ast::VisitorContext *x) override {
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);
        
        int32_t nargs = node->GetNArgs();
        HIRGeneratorContext ix(ctx);
        ix.set_n_result(1);
        
        IVal callee = ACCEPT(node->callee(), &ix);
        ix.set_n_result(nargs < 0 ? -1 : 1);
        
        int wanted = ctx->n_result() == 0 ? 1 : ctx->n_result();
        hir::Type::ID ty;
        if (wanted == 0) {
            ty = hir::Type::kVoid;
        } else {
            ty = hir::Type::kObject;
        }
        std::vector<hir::Value *> args;
        for (int i = 0; node->args() && i < node->args()->size(); ++i) {
            IVal arg = ACCEPT(node->args()->at(i), &ix);
            DCHECK_EQ(IVal::kHIR, arg.kind);
            args.push_back(arg.node);
        }
        
        hir::Invoke *invoke = target_->Invoke(insert_, ty, callee.node, nargs, wanted,
                                              node->line());
        for (int i = 0; i < wanted - 1; ++i) {
            invoke->AddRetType(hir::Type::kObject);
        }
        for (int i = 0; i < args.size(); ++i) {
            invoke->SetArgument(i, args[i]);
        }
        return IVal::HIR(invoke);
    }
    
    virtual IVal VisitNilLiteral(ast::NilLiteral *node, ast::VisitorContext *) override {
        return IVal::HIR(target_->Nil(node->line()));
    }

    virtual IVal VisitStringLiteral(ast::StringLiteral *node, ast::VisitorContext *x) override {
        hir::Constant *val = target_->Constant(hir::Type::kString, node->line());
        val->set_string_val(node->value());
        return IVal::HIR(val);
    }

    virtual IVal VisitApproxLiteral(ast::ApproxLiteral *node, ast::VisitorContext *x) override {
        hir::Constant *val = target_->Constant(hir::Type::kFloat, node->line());
        val->set_float_val(node->value());
        return IVal::HIR(val);
    }

    virtual IVal VisitSmiLiteral(ast::SmiLiteral *node, ast::VisitorContext *x) override {
        hir::Constant *val = target_->Constant(hir::Type::kInt, node->line());
        val->set_int_val(node->value());
        return IVal::HIR(val);
    }

    virtual IVal VisitIntLiteral(ast::IntLiteral *node, ast::VisitorContext *x) override {
        // TODO:
        return IVal::Void();
    }

    virtual IVal VisitMapInitializer(ast::MapInitializer *node, ast::VisitorContext *x) override {
        // TODO:
        return IVal::Void();
    }
    
    virtual IVal VisitIfStatement(ast::IfStatement *node, ast::VisitorContext *x) override {
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);
        
        HIRGeneratorContext ix(ctx);
        ix.set_n_result(1);

        IVal cmp = ACCEPT(node->cond(), &ix);
        hir::Branch *br = target_->Branch(insert_, cmp.node, node->cond()->line());
        hir::BasicBlock *if_true = target_->NewBB(insert_);
        insert_ = if_true;
        br->set_if_true(if_true);
        
        ACCEPT(node->then_clause(), x);
        
        hir::BasicBlock *if_false = nullptr;
        HIRBlockScope if_false_scope(ctx->scope(), true/*is_br*/);
        if (node->else_clause()) {
            if_false = target_->NewBB(insert_);
            insert_ = if_false;
            ACCEPT(node->else_clause(), x);
            br->set_if_false(if_false);
        }

        hir::BasicBlock *out = target_->NewBB(if_true);
        insert_ = out;
        
        target_->NoCondBranch(if_true, out, node->line());
        if (if_false) {
            out->AddInEdge(if_false);
            target_->NoCondBranch(if_false, out, node->line());
        }
        return IVal::Void();
    }
    
    IVal VisitWhileLoop(ast::WhileLoop *node, ast::VisitorContext *x) override {
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);
        HIRGeneratorContext ix(ctx);
        ix.set_n_result(1);
        
        HIRBlockScope while_scope(ix.scope());
        hir::BasicBlock *while_bb = target_->NewBB(insert_);
        hir::BasicBlock *out = target_->NewBB(insert_, true/*dont_insert*/);
        while_scope.set_loop_entry(while_bb);
        while_scope.set_loop_exit(out);
        
        target_->NoCondBranch(insert_, while_bb, node->line());
        insert_ = while_bb;
        ix.set_scope(&while_scope);

        IVal cond = ACCEPT(node->cond(), &ix);

        DCHECK_EQ(IVal::kHIR, cond.kind);
        hir::Branch *br = target_->Branch(while_bb, cond.node, node->cond()->line());
        br->set_if_false(out);

        ACCEPT(node->body(), &ix);
        
        target_->NoCondBranch(insert_, while_bb, node->end_line());
    
        target_->AddBasicBlock(out);
        insert_ = out;
        return IVal::Void();
    }
    
    IVal VisitBreak(ast::Break *node, ast::VisitorContext *x) override {
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);
        HIRBlockScope *p = ctx->scope();
        while (p) {
            if (p->loop_exit()) {
                target_->NoCondBranch(insert_, p->loop_exit(), node->line());
                break;
            }
            p = p->prev();
        }
        if (!p) {
            error_ = MAI_CORRUPTION("break not found loop.");
        }
        return IVal::Void();
    }

    friend class HIRBlockScope;
private:
    
#define DEFINE_BINARY_OP(name) \
    IVal Emit##name(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs, int line) { \
        hir::Value *rv = nullptr; \
        hir::Type::ID ty = EmitCastIfNeed(bb, &lhs, &rhs, line); \
        switch (ty) { \
            case hir::Type::kInt: \
                rv = target_->I##name(bb, lhs, rhs, line); \
                break; \
            case hir::Type::kFloat: \
                rv = target_->F##name(bb, lhs, rhs, line); \
                break; \
            default: \
                DLOG(FATAL) << "Noreached!" << hir::Type::kNames[ty]; \
                break; \
        } \
        return IVal::HIR(rv); \
    }
    
    DEFINE_BINARY_OP(Add)
    DEFINE_BINARY_OP(Sub)
    DEFINE_BINARY_OP(Mul)
    DEFINE_BINARY_OP(Div)
    DEFINE_BINARY_OP(Mod)
    
    hir::Type::ID EmitCastIfNeed(std::vector<hir::Phi::Path> *paths, int line) {
        hir::Type::ID ty = paths->at(0).incoming_value->type();
        for (size_t i = 1; i < paths->size(); ++i) {
            hir::CastPriority prio = hir::GetCastPriority(ty, paths->at(i).incoming_value->type());
            switch (prio.how) {
                case hir::CastPriority::kKeep:
                case hir::CastPriority::kRHS:
                    break;
                case hir::CastPriority::kBoth:
                case hir::CastPriority::kLHS:
                    ty = prio.type;
                    break;
                case hir::CastPriority::kNever:
                default:
                    DLOG(FATAL) << "Noreached!";
                    break;
            }
        }
        
        for (size_t i = 0; i < paths->size(); ++i) {
            hir::Value *val = paths->at(i).incoming_value;
            hir::BasicBlock *bb = paths->at(i).incoming_bb;
            if (val->type() != ty) {
                hir::Value::Kind inst = hir::GetCastAction(ty, val->type());
                paths->at(i).incoming_value = EmitCast(bb, inst, val, line);
            }
        }
        return ty;
    }
    
    hir::Type::ID EmitCastIfNeed(hir::BasicBlock *bb, hir::Value **lhs, hir::Value **rhs, int line) {
        hir::Value::Kind inst;
        hir::CastPriority prio = hir::GetCastPriority((*lhs)->type(), (*rhs)->type());
        switch (prio.how) {
            case hir::CastPriority::kKeep:
                break;
            case hir::CastPriority::kRHS:
                inst = hir::GetCastAction(prio.type, (*rhs)->type());
                *rhs = EmitCast(bb, inst, *rhs, line);
                break;
            case hir::CastPriority::kLHS:
                inst = hir::GetCastAction(prio.type, (*lhs)->type());
                *lhs = EmitCast(bb, inst, *lhs, line);
                break;
            case hir::CastPriority::kBoth:
                inst = hir::GetCastAction(prio.type, (*lhs)->type());
                *lhs = EmitCast(bb, inst, *lhs, line);
                *rhs = EmitCast(bb, inst, *rhs, line);
                break;
            case hir::CastPriority::kNever:
            default:
                DLOG(FATAL) << "Noreached!";
                return hir::Type::kVoid;
        }
        return prio.type;
    }
    
    hir::Value *EmitCast(hir::BasicBlock *bb, hir::Value::Kind inst, hir::Value *from, int line) {
        switch (inst) {
            case hir::Value::kInbox:
                return target_->Inbox(bb, from, line);
            case hir::Value::kIntToLong:
                return target_->IntToLong(bb, from, line);
            case hir::Value::kIntToFloat:
                return target_->IntToFloat(bb, from, line);
            case hir::Value::kLongToInt:
                return target_->LongToInt(bb, from, line);
            case hir::Value::kLongToFloat:
                return target_->LongToFloat(bb, from, line);
            case hir::Value::kFloatToInt:
                return target_->FloatToInt(bb, from, line);
            case hir::Value::kFloatToLong:
                return target_->FloatToLong(bb, from, line);
            case hir::Value::kMaxInsts:
            default:
                DLOG(FATAL) << "Noreached!";
                break;
        }
        return nullptr;
    }
    
    std::vector<hir::Type::ID> args_;
    UpValTable upvals_;
    base::Arena *arena_;
    Profiler *profiler_;
    Error error_;
    std::vector<hir::Type::ID> vargs_;
    hir::Function *target_ = nullptr;
    hir::BasicBlock *insert_ = nullptr;
}; // class IRGeneratorVisitor

Error HIR_GenerateHIR(const BuiltinType *argv, size_t argc, const UpvalDesc *desc,
                      const BuiltinType *upvals, size_t n_upvals, ast::AstNode *ast,
                      hir::Function **rv, base::Arena *arena, NyaaCore *core) {
    std::vector<hir::Type::ID> args(argc);
    for (size_t i = 0; i < argc; ++i) {
        args[i] = ConvType(argv[i]);
    }
    UpValTable uvs;
    for (int i = 0; i < n_upvals; ++i) {
        const NyString *s = desc[i].name;
        const ast::String *name = ast::String::New(arena, s->bytes(), s->size());

        hir::UpvalDesc ud;
        ud.slot = i;
        ud.type_hint = ConvType(upvals[i]);
        uvs.insert({name, ud});
    }

    HIRGeneratorVisitor visitor(std::move(args), std::move(uvs), arena, core->profiler());
    HIRGeneratorContext ctx;
    DCHECK_NOTNULL(ast)->Accept(&visitor, &ctx);
    if (visitor.error().fail()) {
        return visitor.error();
    }
    *rv = visitor.target();
    return Error::OK();
}

} // namespace nyaa
    
} // namespace mai
