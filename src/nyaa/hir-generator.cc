#include "nyaa/code-gen-utils.h"
#include "nyaa/hir-rewrite.h"
#include "nyaa/high-level-ir.h"
#include "nyaa/ast.h"
#include "nyaa/profiling.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/builtin.h"
#include "nyaa/function.h"
#include <set>

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

    using IncomingPathMap = std::unordered_map<
        const ast::String *,
        std::vector<hir::Phi::Path>,
        base::ArenaHash<const ast::String *>,
        base::ArenaEqualTo<const ast::String *>>;
    
    inline HIRBlockScope(HIRBlockScope *prev, bool is_br_edge = false)
        : is_br_edge_(is_br_edge)
        , prev_(prev) {
        DCHECK_NE(this, prev);
    }
    inline ~HIRBlockScope() {}
    
    DEF_PTR_GETTER(HIRBlockScope, prev);
    DEF_VAL_GETTER(bool, is_br_edge);
    DEF_VAL_PROP_RMW(ValueTable, values);
    DEF_PTR_PROP_RW(hir::BasicBlock, loop_retry);
    DEF_PTR_PROP_RW(hir::BasicBlock, loop_exit);
    
    void PutValueNested(const ast::String *name, hir::Value *val, HIRBlockScope *scope);
    
    void PutValue(const ast::String *name, hir::Value *val) {
        DCHECK(!val->IsVoidTy());
        ValueBundle bundle;
        bundle.value = val;
        bundle.out_scope = nullptr;
        values_[name] = bundle;
    }
    
    HIRBlockScope *GetBranchEdge() {
        for (HIRBlockScope *p = this; p != nullptr; p = p->prev_) {
            if (p->is_br_edge_) {
                return p;
            }
        }
        return nullptr;
    }
    
    inline std::tuple<hir::Value*, HIRBlockScope*> GetValueOrNullNested(const ast::String *name);
    
    inline hir::Value *GetValueOrNull(const ast::String *name) const {
        auto iter = values_.find(name);
        return iter == values_.end() ? nullptr : iter->second.value;
    }
    
    inline std::map<hir::Value *, hir::Value *> MergeBranch(HIRBlockScope *br);
    
    static IncomingPathMap
    MergeBranchs(HIRBlockScope *brs[], hir::BasicBlock *bbs[], size_t n_brs);
    
private:
    bool is_br_edge_;
    HIRBlockScope *prev_;
    ValueTable values_;
    hir::BasicBlock *loop_retry_ = nullptr;
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


void HIRBlockScope::PutValueNested(const ast::String *name, hir::Value *val, HIRBlockScope *scope) {
    //DCHECK(!val->IsVoid());
    ValueBundle bundle;
    bundle.value = val;
    bundle.out_scope = (scope == this) ? nullptr : scope;
    if (bundle.out_scope) {
        for (HIRBlockScope *p = this; p; p = p->prev_) {
            if (p->is_br_edge_) {
                p->values_[name] = bundle;
                return;
            }
        }
        bundle.out_scope = nullptr;
        scope->values_[name] = bundle;
    } else {
        values_[name] = bundle;
    }
}
    
    
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
    
inline std::map<hir::Value *, hir::Value *> HIRBlockScope::MergeBranch(HIRBlockScope *br) {
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
    
/*static*/ HIRBlockScope::IncomingPathMap
HIRBlockScope::MergeBranchs(HIRBlockScope *brs[], hir::BasicBlock *bbs[], size_t n) {
    if (n < 2) {
        return IncomingPathMap();
    }

    std::map<hir::Value *, hir::Value *> versions;
    IncomingPathMap paths;
    for (size_t i = 1; i < n; ++i) {
        HIRBlockScope *br = brs[i];

        for (const auto &pair : br->values_) {
            if (!pair.second.out_scope) {
                continue;
            }
            
            hir::Value *old_ver = DCHECK_NOTNULL(pair.second.out_scope->GetValueOrNull(pair.first));
            hir::Value *new_ver = pair.second.value;
            
            hir::Phi::Path path;
            path.incoming_bb = bbs[i];
            path.incoming_value = new_ver;
            paths[pair.first].push_back(path);

            versions.insert({new_ver, old_ver});
        }
    }

    for (auto &pair : paths) {
        DCHECK(!pair.second.empty());
        if (pair.second.size() == 1) {
            auto iter = versions.find(pair.second[0].incoming_value);
            DCHECK(iter != versions.end());
            
            hir::Phi::Path path;
            path.incoming_bb = bbs[0];
            path.incoming_value = iter->second;
            pair.second.push_back(path);
        }
    }
    return paths;
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

class HIRAnalyzeVisitor final : public ast::Visitor {
public:
    static hir::Value *const kPlaceholderVal;
    
    HIRAnalyzeVisitor(HIRBlockScope *scope)
        : br_edge_(scope, true/*is_br_edge*/){
    }

    DEF_VAL_MUTABLE_GETTER(HIRBlockScope, br_edge);
    
    void Run(ast::Block *node) {
        //DCHECK(scope->is_br_edge());
        HIRGeneratorContext ix;
        ix.set_scope(&br_edge_);
        node->Accept(this, &ix);
    }
    
private:
    // Entry
    virtual IVal VisitBlock(ast::Block *node, ast::VisitorContext *x) override {
        HIRGeneratorContext ix(x);
        if (node->stmts()) {
            for (auto stmt : *node->stmts()) {
                stmt->Accept(this, &ix);
            }
        }
        return IVal::Void();
    }
    
    virtual IVal VisitVarDeclaration(ast::VarDeclaration *node, ast::VisitorContext *x) override {
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);
        for (auto name : *node->names()) {
            if (IsNotPlaceholder(name)) {
                ctx->scope()->PutValue(name, kPlaceholderVal);
            }
        }
        return IVal::Void();
    }

    virtual IVal VisitAssignment(ast::Assignment *node, ast::VisitorContext *x) override {
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);
        for (auto lval : *node->lvals()) {
            if (lval->IsVariable()) {
                ast::Variable *symbol = ast::Variable::Cast(lval);
                
                hir::Value *val = nullptr;
                HIRBlockScope *owns = nullptr;
                std::tie(val, owns) = ctx->scope()->GetValueOrNullNested(symbol->name());
                if (val) {
                    ctx->scope()->PutValueNested(symbol->name(), kPlaceholderVal, owns);
                }
            }
        }
        return IVal::Void();
    }
    
    HIRBlockScope br_edge_;
}; // class HIRanalyzeVisitor
    
hir::Value *const HIRAnalyzeVisitor::kPlaceholderVal = reinterpret_cast<hir::Value *>(0x1);


class HIRGeneratorVisitor final : public ast::Visitor {
public:
    HIRGeneratorVisitor(std::vector<hir::Type::ID> &&args, UpValTable &&upvals,
                        base::Arena *arena, Profiler *profiler, NyaaCore *core)
        : args_(args)
        , upvals_(upvals)
        , arena_(arena)
        , profiler_(profiler)
        , N_(core) {
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

        insert_ = target_->NewBB(nullptr);
        target_->set_entry(insert_);
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
                target_->StoreUp(insert_, iter->second.slot, ctx->rval().node, node->line());
                return IVal::Void();
            }
            target_->StoreGlobal(insert_, node->name(), ctx->rval().node, node->line());
            return IVal::Void();
        } else {
            if (val) {
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
        hir::Ret *ret = target_->Ret(insert_, node->line());
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
    
//    hir::Value *DoEmitAdd(hir::Value *lhs, hir::Value *rhs, int line, int trace_id) {
//         EmitCastIfNeed(insert_, &lhs, &rhs, line);
//        return nullptr;
//    }
    
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
            case Operator::kAdd: {
                // TODO:
                //node->trace_id()
                return EmitAdd(insert_, operands[0], operands[1], node->line());
            } break;

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

            case Operator::kEQ:
                break;
            case Operator::kNE:
                break;
            case Operator::kLT:
                break;
            case Operator::kLE:
                break;
            case Operator::kGT:
                break;
            case Operator::kGE:
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
        NyString *s = N_->factory()->NewString(node->value()->data(), node->value()->size());
        val->set_string_val(s);
        return IVal::HIR(val);
    }

    virtual IVal VisitApproxLiteral(ast::ApproxLiteral *node, ast::VisitorContext *x) override {
        hir::Constant *val = target_->Constant(hir::Type::kFloat, node->line());
        val->set_float_val(node->value());
        return IVal::HIR(val);
    }

    virtual IVal VisitSmiLiteral(ast::SmiLiteral *node, ast::VisitorContext *x) override {
        hir::Constant *val = target_->Constant(hir::Type::kInt, node->line());
        val->set_smi_val(node->value());
        return IVal::HIR(val);
    }

    virtual IVal VisitIntLiteral(ast::IntLiteral *node, ast::VisitorContext *x) override {
        hir::Constant *val = target_->Constant(hir::Type::kLong, node->line());
        val->set_long_val(NyInt::Parse(node->value()->data(), node->value()->size(), N_->factory()));
        return IVal::HIR(val);
    }

    virtual IVal VisitMapInitializer(ast::MapInitializer *node, ast::VisitorContext *x) override {
        // TODO:
        return IVal::Void();
    }
    
    virtual IVal VisitIfStatement(ast::IfStatement *node, ast::VisitorContext *x) override {
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);
        
        HIRGeneratorContext ix(ctx);
        ix.set_n_result(1);
        hir::BasicBlock *origin = insert_;

        IVal cmp = ACCEPT(node->cond(), &ix);
        hir::Branch *br = target_->Branch(insert_, cmp.node, node->cond()->line());
        hir::BasicBlock *if_true = target_->NewBB(insert_);
        insert_ = if_true;
        br->set_if_true(if_true);
        
        HIRGeneratorContext if_true_ctx(ctx);
        HIRBlockScope if_true_scope(ctx->scope(), true/*is_br*/);
        if_true_ctx.set_scope(&if_true_scope);
        ACCEPT(node->then_clause(), &if_true_ctx);
        
        hir::BasicBlock *if_false = nullptr;
        HIRBlockScope if_false_scope(ctx->scope(), true/*is_br*/);
        if (node->else_clause()) {
            HIRGeneratorContext if_false_ctx(ctx);
            if_false = target_->NewBB(insert_);
            insert_ = if_false;
            if_false_ctx.set_scope(&if_false_scope);
            
            ACCEPT(node->else_clause(), &if_false_ctx);
            br->set_if_false(if_false);
        }

        // TODO: reuse out basic block.
        hir::BasicBlock *out = target_->NewBB(if_true);
        insert_ = out;
        if (!node->else_clause()) {
            br->set_if_false(out);
        }

        HIRBlockScope::IncomingPathMap paths;
        if (node->else_clause()) {
            HIRBlockScope *brs[] = {ctx->scope(), &if_true_scope, &if_false_scope};
            hir::BasicBlock *bbs[] = {origin, if_true, if_false};
            paths = HIRBlockScope::MergeBranchs(brs, bbs, 3);
        } else {
            HIRBlockScope *brs[] = {ctx->scope(), &if_true_scope};
            hir::BasicBlock *bbs[] = {origin, if_true};
            paths = HIRBlockScope::MergeBranchs(brs, bbs, 2);
    
        }
        for (auto &pair : paths) {
            hir::Type::ID ty = EmitCastIfNeed(&pair.second, node->line());
            hir::Phi *phi = target_->Phi(out, ty, node->line());
            for (auto path : pair.second) {
                phi->AddIncoming(path.incoming_bb, path.incoming_value);
            }
            
            HIRBlockScope *scope = nullptr;
            std::tie(std::ignore, scope) = ctx->scope()->GetValueOrNullNested(pair.first);
            if (HIRBlockScope *edge = ctx->scope()->GetBranchEdge()) {
                edge->PutValueNested(pair.first, phi, scope);
            } else {
                scope->PutValue(pair.first, phi);
            }
        }
        
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
        hir::BasicBlock *origin = insert_;
        
        HIRBlockScope while_scope(ix.scope(), true/*is_br_edge*/);
        hir::BasicBlock *retry = target_->NewBB(insert_);
        target_->NoCondBranch(insert_, retry, node->line());
        insert_ = retry;
        ix.set_scope(&while_scope);
        
        IVal cond = ACCEPT(node->cond(), &ix);

        DCHECK_EQ(IVal::kHIR, cond.kind);
        hir::Branch *br = target_->Branch(retry, cond.node, node->cond()->line());
        hir::BasicBlock *body = target_->NewBB(retry);
        br->set_if_true(body);
        
        hir::BasicBlock *out = target_->NewBB(nullptr, true/*dont_insert*/);
        while_scope.set_loop_exit(out);
        while_scope.set_loop_retry(retry);

        insert_ = body;
        ACCEPT(node->body(), &ix);

        target_->NoCondBranch(insert_, retry, node->end_line());

        std::vector<hir::Phi *> phi_nodes;
        HIRBlockScope *brs[] = {ctx->scope(), &while_scope};
        hir::BasicBlock *bbs[] = {origin, insert_};
        HIRBlockScope::IncomingPathMap paths = HIRBlockScope::MergeBranchs(brs, bbs, 2);
        for (auto &pair : paths) {
            hir::Type::ID ty = EmitCastIfNeed(&pair.second, node->line());
            hir::Phi *phi = target_->Phi(nullptr, ty, node->line());
            phi_nodes.push_back(phi);
            hir::Value *val = nullptr;
            HIRBlockScope *scope = nullptr;
            std::tie(val, scope) = ctx->scope()->GetValueOrNullNested(pair.first);
            hir::RewriteReplacement(N_, target_, retry, insert_, val, phi);

            for (auto path : pair.second) {
                phi->AddIncoming(path.incoming_bb, path.incoming_value);
            }
            if (HIRBlockScope *edge = ctx->scope()->GetBranchEdge()) {
                edge->PutValueNested(pair.first, phi, scope);
            } else {
                scope->PutValue(pair.first, phi);
            }
        }
        for (auto phi : phi_nodes) { retry->InsertHead(phi); }
    
        out->set_label(target_->NextBBId());
        out->AddInEdge(insert_);
        target_->AddBasicBlock(out);
        br->set_if_false(out);
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
            case hir::Type::kLong: \
                rv = target_->L##name(bb, lhs, rhs, line); \
                break; \
            case hir::Type::kObject: \
                rv = target_->O##name(bb, lhs, rhs, line); \
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
                hir::Value::InstID inst = hir::GetCastAction(ty, val->type());
                paths->at(i).incoming_value = hir::EmitCast(N_, target_, bb, inst, val, line);
            }
        }
        return ty;
    }
    
    hir::Type::ID EmitCastIfNeed(hir::BasicBlock *bb, hir::Value **lhs, hir::Value **rhs, int line) {
        hir::Value::InstID inst;
        hir::CastPriority prio = hir::GetCastPriority((*lhs)->type(), (*rhs)->type());
        switch (prio.how) {
            case hir::CastPriority::kKeep:
                break;
            case hir::CastPriority::kRHS:
                inst = hir::GetCastAction(prio.type, (*rhs)->type());
                *rhs = hir::EmitCast(N_, target_, bb, inst, *rhs, line);
                break;
            case hir::CastPriority::kLHS:
                inst = hir::GetCastAction(prio.type, (*lhs)->type());
                *lhs = hir::EmitCast(N_, target_, bb, inst, *lhs, line);
                break;
            case hir::CastPriority::kBoth:
                inst = hir::GetCastAction(prio.type, (*lhs)->type());
                *lhs = hir::EmitCast(N_, target_, bb, inst, *lhs, line);
                *rhs = hir::EmitCast(N_, target_, bb, inst, *rhs, line);
                break;
            case hir::CastPriority::kNever:
            default:
                DLOG(FATAL) << "Noreached!";
                return hir::Type::kVoid;
        }
        return prio.type;
    }
    
    std::vector<hir::Type::ID> args_;
    UpValTable upvals_;
    base::Arena *arena_;
    Profiler *profiler_;
    NyaaCore *N_;
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

    HIRGeneratorVisitor visitor(std::move(args), std::move(uvs), arena, core->profiler(), core);
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
