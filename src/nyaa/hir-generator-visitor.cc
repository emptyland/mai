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
    
class HIRBlockScope final {
public:
    using ValueTable = std::unordered_map<const ast::String *, ValueBundle,
        base::ArenaHash<const ast::String *>,
        base::ArenaEqualTo<const ast::String *>>;
    
    inline HIRBlockScope(HIRBlockScope *prev, bool is_br_edge = false)
        : is_br_edge_(is_br_edge)
        , prev_(prev) {
        DCHECK_NE(this, prev);
    }
    inline ~HIRBlockScope() {}
    
    void PutValueNested(const ast::String *name, hir::Value *val, HIRBlockScope *scope);
    
    void PutValue(const ast::String *name, hir::Value *val) {
        DCHECK(!val->IsVoid());
        ValueBundle bundle;
        bundle.value = val;
        bundle.out_scope = nullptr;
        values_.insert({name, bundle});
    }
    
    inline std::tuple<hir::Value*, HIRBlockScope*> GetValueOrNullNested(const ast::String *name);
    
    inline hir::Value *GetValueOrNull(const ast::String *name) const {
        auto iter = values_.find(name);
        return iter == values_.end() ? nullptr : iter->second.value;
    }
    
    std::map<hir::Value *, hir::Value *> MergeBranch(HIRBlockScope *br) {
        DCHECK(br->is_br_edge_);
        std::map<hir::Value *, hir::Value *> rv;
        for (const auto &pair : br->values_) {
            if (!pair.second.out_scope) {
                continue;
            }
            hir::Value *old_ver = DCHECK_NOTNULL(pair.second.out_scope->GetValueOrNull(pair.first));
            hir::Value *new_ver = pair.second.value;
            rv.insert({old_ver, new_ver});
        }
        return rv;
    }
    
private:
    bool is_br_edge_;
    HIRBlockScope *prev_;
    ValueTable values_;
}; // class BlockScope
    
class HIRGeneratorContext : public ast::VisitorContext {
public:
    explicit HIRGeneratorContext(ast::VisitorContext *x = nullptr) {
        if (HIRGeneratorContext *prev = Cast(x)) {
            scope_ = DCHECK_NOTNULL(prev->scope_);
            bb_    = prev->bb_;
        }
    }
    ~HIRGeneratorContext() {}
    
    DEF_VAL_PROP_RW(int, n_result);
    DEF_VAL_PROP_RW(bool, lval);
    DEF_VAL_PROP_RW(int, lval_offset);
    DEF_VAL_PROP_RW(IVal, rval);
    DEF_PTR_PROP_RW_NOTNULL2(HIRBlockScope, scope);
    DEF_PTR_PROP_RW(hir::BasicBlock, bb);
    
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
    hir::BasicBlock *bb_ = nullptr;
}; // CodeGeneratorContext


void HIRBlockScope::PutValueNested(const ast::String *name, hir::Value *val, HIRBlockScope *scope) {
    DCHECK(!val->IsVoid());
    ValueBundle bundle;
    bundle.value = val;
    bundle.out_scope = (scope == this) ? nullptr : scope;
    if (bundle.out_scope) {
        for (HIRBlockScope *p = this; p; p = p->prev_) {
            if (p->is_br_edge_) {
                p->values_.insert({name, bundle});
                return;
            }
        }
        bundle.out_scope = nullptr;
        scope->values_.insert({name, bundle});
    } else {
        values_.insert({name, bundle});
    }
}
    
    
inline std::tuple<hir::Value*, HIRBlockScope*>
HIRBlockScope::GetValueOrNullNested(const ast::String *name) {
    HIRBlockScope *p = this;
    while (p) {
        auto iter = p->values_.find(name);
        if (iter != p->values_.end()) {
            return {iter->second.value, p};
        }
        p = p->prev_;
    }
    return {nullptr, nullptr};
}
    
inline static hir::Type::ID ConvType(BuiltinType ty) {
    switch (ty) {
        case kTypeNil:
            return hir::Type::kNil;
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
        
        const size_t n_params = !node->params() ? 0 : node->params()->size();
        for (int i = 0; i < n_params; ++i) {
            hir::Value *val = nullptr;
            if (i < args_.size()) {
                val = target_->Parameter(args_[i], node->line());
            } else {
                val = target_->Nil(node->line());
            }
            trunk_scope.PutValue(node->params()->at(i), val);
            
        }
        if (node->vargs()) {
            int64_t nvargs = args_.size() - n_params;
            for (int64_t i = 0; i < nvargs; ++i) {
                vargs_.push_back(args_[i]);
            }
        }

        top_ctx.set_bb(target_->NewBB(nullptr));
        ACCEPT(node->value(), &top_ctx);
        return IVal::Void();
    }
    
    virtual IVal VisitBlock(ast::Block *node, ast::VisitorContext *x) override {
        if (node->stmts()) {
            for (auto stmt : *node->stmts()) {
                ACCEPT(stmt, x);
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
                        val = target_->BaseOfStack(ctx->bb(), rval.node, static_cast<int>(i),
                                                   node->line());
                    }
                    ctx->scope()->PutValue(node->names()->at(i), val);
                }
            } else {
                rix.set_n_result(1);
                
                for (size_t i = 0; i < node->names()->size(); ++i) {
                    if (i < node->inits()->size()) {
                        IVal rval = ACCEPT(node->inits()->at(i), &rix);
                        ctx->scope()->PutValue(node->names()->at(i), rval.node);
                    } else {
                        ctx->scope()->PutValue(node->names()->at(i), target_->Nil(node->line()));
                    }
                }
            }
        } else {
            for (auto name : *node->names()) {
                if (IsNotPlaceholder(name)) {
                    ctx->scope()->PutValue(name, target_->Nil(node->line()));
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
                ctx->scope()->PutValueNested(node->name(), ctx->rval().node, scope);
                return IVal::Void();
            }
            if (auto iter = upvals_.find(node->name()); iter != upvals_.end()) {
                target_->StoreUp(ctx->bb(), iter->second.slot, ctx->rval().node, node->line());
                return IVal::Void();
            }
            target_->StoreGlobal(ctx->bb(), node->name(), ctx->rval().node, node->line());
            return IVal::Void();
        } else {
            if (val) {
                return IVal::HIR(val);
            }
            if (auto iter = upvals_.find(node->name()); iter != upvals_.end()) {
                val = target_->LoadUp(ctx->bb(), hir::Type::kObject, iter->second.slot, node->line());
                return IVal::HIR(val);
            }
            val = target_->LoadGlobal(ctx->bb(), hir::Type::kObject, node->name(), node->line());
            return IVal::HIR(val);
        }
    }
    
    virtual IVal VisitReturn(ast::Return *node, ast::VisitorContext *x) override {
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);

        hir::Ret *ret = target_->Ret(ctx->bb(), node->line());
        if (!node->rets()) {
            ret->set_wanted(0);
            return IVal::Void();
        }
        
        int nrets = node->GetNRets();
        HIRGeneratorContext ix(x);
        ix.set_n_result(nrets < 0 ? -1 : 1);
        
        IVal first = ACCEPT(node->rets()->at(0), &ix);
        DCHECK_NE(IVal::kVoid, first.kind);
        
        ret->AddRetVal(first.node);
        for (size_t i = 1; i < node->rets()->size(); ++i) {
            IVal ret_val = ACCEPT(node->rets()->at(i), &ix);
            ret->AddRetVal(ret_val.node);
        }
        ret->set_wanted(nrets);
        return IVal::Void();
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
        hir::Branch *br = target_->Branch(ctx->bb(), cmp.node, node->cond()->line());
        hir::BasicBlock *if_true = target_->NewBB(ctx->bb());
        br->set_if_true(if_true);
        
        HIRGeneratorContext if_true_ctx(ctx);
        HIRBlockScope if_true_scope(ctx->scope(), true/*is_br*/);
        if_true_ctx.set_bb(if_true);
        if_true_ctx.set_scope(&if_true_scope);
        ACCEPT(node->then_clause(), &if_true_ctx);
        
        hir::BasicBlock *if_false = nullptr;
        HIRBlockScope if_false_scope(ctx->scope(), true/*is_br*/);
        if (node->else_clause()) {
            HIRGeneratorContext if_false_ctx(ctx);
            if_false = target_->NewBB(ctx->bb());
            if_false_ctx.set_bb(if_false);
            if_false_ctx.set_scope(&if_false_scope);
            
            ACCEPT(node->else_clause(), &if_false_ctx);
            br->set_if_false(if_false);
        }

        hir::BasicBlock *out = target_->NewBB(if_true);

        if (node->else_clause()) {
            // TODO:
        } else {
            auto info = ctx->scope()->MergeBranch(&if_true_scope);
            for (const auto &pair : info) {
                // TODO: cast
                hir::Value *val = target_->Inbox(out, pair.second, node->line());
                hir::Phi *phi = target_->Phi(out, pair.first->type(), node->line());
                phi->AddIncoming(ctx->bb(), pair.first);
                phi->AddIncoming(if_true, val);
            }
        }
        
        target_->NoCondBranch(if_true, out, node->line());
        if (if_false) {
            out->AddInEdge(if_false);
            target_->NoCondBranch(if_false, out, node->line());
        }
        ctx->set_bb(out);

        return IVal::Void();
    }

    friend class HIRBlockScope;
private:
    static bool IsPlaceholder(const ast::String *name) {
        return name->size() == 1 && name->data()[0] == '_';
    }
    
    static bool IsNotPlaceholder(const ast::String *name) { return !IsPlaceholder(name); }
    
    std::vector<hir::Type::ID> args_;
    UpValTable upvals_;
    base::Arena *arena_;
    Profiler *profiler_;
    Error error_;
    std::vector<hir::Type::ID> vargs_;
    hir::Function *target_ = nullptr;
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
