#include "nyaa/code-gen-utils.h"
#include "nyaa/high-level-ir.h"
#include "nyaa/ast.h"
#include "nyaa/profiling.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/builtin.h"
#include "nyaa/function.h"

namespace mai {
    
namespace nyaa {
    
using ValueTable = std::unordered_map<const ast::String *, hir::Value *,
    base::ArenaHash<const ast::String *>,
    base::ArenaEqualTo<const ast::String *>>;
    
using UpValTable = std::unordered_map<const ast::String *, hir::UpvalDesc,
    base::ArenaHash<const ast::String *>,
    base::ArenaEqualTo<const ast::String *>>;
    
class HIRGeneratorVisitor;
    
namespace {

class HIRBlockScope final {
public:
    inline HIRBlockScope(HIRBlockScope *prev)
        : prev_(prev) {
        DCHECK_NE(this, prev);
    }
    inline ~HIRBlockScope() {}
    
    void PutValue(const ast::String *name, hir::Value *val) {
        DCHECK(!val->IsVoid());
        values_.insert({name, val});
    }
    
    inline std::tuple<hir::Value*, HIRBlockScope*> GetValueOrNullNested(const ast::String *name);
    
private:
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
    IVal rval_ = IVal::Void();
    HIRBlockScope *scope_ = nullptr;
    hir::BasicBlock *bb_ = nullptr;
}; // CodeGeneratorContext
    
    
inline std::tuple<hir::Value*, HIRBlockScope*>
HIRBlockScope::GetValueOrNullNested(const ast::String *name) {
    HIRBlockScope *p = this;
    while (p) {
        auto iter = p->values_.find(name);
        if (iter != p->values_.end()) {
            return {iter->second, p};
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
    
//
//                   +---------+
//                   | calling |
//                   +---------+
//                   /          \
// +----------------+           +------------+
// | fallback point | <-------- | trunk node |
// +----------------+           +------------+
//
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
    
#define ACCEPT(subx) (subx)->Accept(this, x); if (error_.fail()) return IVal::Void()
    
    virtual IVal VisitLambdaLiteral(ast::LambdaLiteral *node, ast::VisitorContext *x) override {
        target_ = hir::Function::New(arena_);
        
        HIRBlockScope trunk_scope(nullptr);
        HIRGeneratorContext top_ctx;
        top_ctx.set_scope(&trunk_scope);
        
        for (int i = 0; i < node->params()->size(); ++i) {
            hir::Value *val = nullptr;
            if (i < args_.size()) {
                val = target_->Parameter(args_[i], node->line());
            } else {
                val = target_->Nil(node->line());
            }
            trunk_scope.PutValue(node->params()->at(i), val);
            
        }
        if (node->vargs()) {
            int64_t nvargs = args_.size() - node->params()->size();
            for (int64_t i = 0; i < nvargs; ++i) {
                vargs_.push_back(args_[i]);
            }
        }

        top_ctx.set_bb(target_->NewBB(nullptr));
        ACCEPT(node->value());
        return IVal::Void();
    }
    
    virtual IVal VisitBlock(ast::Block *node, ast::VisitorContext *x) override {
        if (node->stmts()) {
            for (auto stmt : *node->stmts()) {
                ACCEPT(stmt);
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
                IVal rval = ACCEPT(node->inits()->at(0));
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
                        ast::Expression *init = node->inits()->at(i);
                        IVal rval = init->Accept(this, &rix);
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
        (void)ctx;
        // TODO:
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
                scope->PutValue(node->name(), val);
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

Error HIR_GenerateHIR(int /*n_params*/, bool /*vargs*/, BuiltinType *argv, size_t argc,
                      const UpvalDesc *desc, BuiltinType *upvals, size_t n_upvals, ast::AstNode *ast,
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
