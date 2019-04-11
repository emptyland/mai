#include "nyaa/code-gen.h"
#include "nyaa/bytecode-builder.h"
#include "nyaa/ast.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/object-factory.h"
#include "base/hash.h"
#include "glog/logging.h"
#include <unordered_map>
#include <set>

namespace mai {
    
namespace nyaa {

class FunctionScope;
class BlockScope;
class CodeGeneratorVisitor;

class FunctionScope {
public:
    inline FunctionScope(CodeGeneratorVisitor *owns);
    inline ~FunctionScope();
    
    ConstPoolBuilder *kpool() { return &kpool_builder_; }
    BytecodeArrayBuilder *builder() { return &builder_; }
    DEF_VAL_GETTER(int32_t, max_stack);
    
    IVal NewLocal() { return IVal::Local(max_stack_ ++); }
    
    IVal Reserve(int n) {
        IVal base = IVal::Local(max_stack_);
        max_stack_ += n;
        return base;
    }
    
    friend class CodeGeneratorVisitor;
    friend class BlockScope;
private:
    FunctionScope *prev_;
    BlockScope *top_;
    CodeGeneratorVisitor *const owns_;
    int32_t max_stack_ = 0; // min is 2;
    ConstPoolBuilder kpool_builder_;
    BytecodeArrayBuilder builder_;
}; // class FunctionScope
    
    
class BlockScope {
public:
    inline BlockScope(FunctionScope *owns);
    inline ~BlockScope();
    
    IVal GetOrNewLocal(const ast::String *name) {
        auto iter = locals_.find(name);
        if (iter == locals_.end()) {
            IVal val = owns_->NewLocal();
            locals_.insert({name, val});
            prot_regs_.insert(val.index);
            return val;
        }
        return iter->second;
    }
    
    IVal GetVariable(const ast::String *name) {
        BlockScope *scope = this;
        while (scope && scope->owns_ == owns_) {
            auto iter = scope->locals_.find(name);
            if (iter != scope->locals_.end()) {
                return iter->second;
            }
            scope = scope->prev_;
        }
        return IVal::None();
    }

    IVal PutLocal(const ast::String *name, const IVal *val) {
        IVal rv = IVal::None();
        if (name->size() == 1 && name->data()[0] == '_') {
            return rv; // ignore!
        }
        if (val) {
            rv = *val;
        } else {
            rv = owns_->NewLocal();
        }
        DCHECK_LT(rv.index, owns_->max_stack());
        auto iter = locals_.find(name);
        if (iter != locals_.end()) {
            prot_regs_.erase(iter->second.index);
        }
        locals_.insert({name, rv});
        prot_regs_.insert(rv.index);
        return rv;
    }
    
    bool Protected(IVal val) {
        if (IVal::kLocal != val.kind) {
            return false;
        }
        return prot_regs_.find(val.index) != prot_regs_.end();
    }

private:
    using VariableTable = std::unordered_map<const ast::String *,
        IVal,
        base::ArenaHash<const ast::String *>,
        base::ArenaEqualTo<const ast::String *>>;
    
    BlockScope *prev_;
    FunctionScope *const owns_;
    VariableTable locals_;
    std::set<int32_t> prot_regs_;
}; // class BlockScope
    
class CodeGeneratorContext : public ast::VisitorContext {
public:
    CodeGeneratorContext() {}
    ~CodeGeneratorContext() {}
    
    DEF_VAL_PROP_RW(int, n_result);
    DEF_VAL_PROP_RW(bool, localize);
    DEF_VAL_PROP_RW(bool, keep_const);
    DEF_VAL_PROP_RW(bool, lval);
    DEF_VAL_PROP_RW(IVal, rval);

    static CodeGeneratorContext *Cast(ast::VisitorContext *ctx) {
        return !ctx ? nullptr : down_cast<CodeGeneratorContext>(ctx);
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(CodeGeneratorContext);
private:
    int n_result_ = 0;
    bool localize_ = true;
    bool keep_const_ = false;
    bool lval_ = false;
    IVal rval_;
}; // CodeGeneratorContext

class CodeGeneratorVisitor : public ast::Visitor {
public:
    using Context = CodeGeneratorContext;

    CodeGeneratorVisitor(NyaaCore *core) : core_(DCHECK_NOTNULL(core)) {}
    virtual ~CodeGeneratorVisitor() override {};
    
    BytecodeArrayBuilder *builder() { return fun_scope_->builder(); }
    
    virtual IVal VisitBlock(ast::Block *node, ast::VisitorContext */*ctx*/) override {
        BlockScope scope(fun_scope_);
        CodeGeneratorContext ctx;
        IVal ret;
        for (auto stmt : *node->stmts()) {
            ret = stmt->Accept(this, &ctx);
        }
        return ret;
    }

    virtual IVal VisitVarDeclaration(ast::VarDeclaration *node, ast::VisitorContext *x) override {
        std::vector<IVal> vars;

        if (node->inits()) {
            CodeGeneratorContext rix;
            if (node->inits()->size() == 1 && node->inits()->at(0)->IsInvoke()) {
                rix.set_n_result(static_cast<int>(node->names()->size()));
                IVal rval = node->inits()->at(0)->Accept(this, &rix);
                fun_scope_->Reserve(rix.n_result() - 1);

                DCHECK_EQ(IVal::kLocal, rval.kind);
                for (size_t i = 0; i < node->names()->size(); ++i) {
                    blk_scope_->PutLocal(node->names()->at(i), &rval);
                    rval.index++;
                }
            } else {
                rix.set_n_result(1);
                
                for (size_t i = 0; i < node->names()->size(); ++i) {
                    if (i < node->inits()->size()) {
                        IVal rval = node->inits()->at(i)->Accept(this, &rix);
                        blk_scope_->PutLocal(node->names()->at(i), &rval);
                    } else {
                        IVal lval = blk_scope_->PutLocal(node->names()->at(i), nullptr);
                        builder()->LoadNil(lval, 1, node->line());
                    }
                }
            }
        } else {
            for (size_t i = 0; i < node->names()->size(); ++i) {
                const ast::String *name = node->names()->at(i);
                vars.push_back(blk_scope_->PutLocal(name, nullptr));
            }
            builder()->LoadNil(vars[0], static_cast<int32_t>(vars.size()), node->line());
        }
        return IVal::None();
    }
    
    virtual IVal VisitAssignment(ast::Assignment *node, ast::VisitorContext *x) override {
        CodeGeneratorContext rix;
        if (node->rvals()->size() == 1 && node->rvals()->at(0)->IsInvoke()) {
            rix.set_n_result(static_cast<int>(node->lvals()->size()));
            IVal val = node->rvals()->at(0)->Accept(this, &rix);
            for (size_t i = 0; i < node->lvals()->size(); ++i) {
                
                CodeGeneratorContext lix;
                lix.set_rval(val);
                lix.set_lval(true);
                node->lvals()->at(i)->Accept(this, &lix);
                val.index++;
            }
        } else {
            rix.set_n_result(1);
            size_t len = std::min(node->lvals()->size(), node->rvals()->size());
            for (size_t i = 0; i < len; ++i) {
                ast::Expression *expr = node->rvals()->at(i);
                IVal val = expr->Accept(this, &rix);
                
                CodeGeneratorContext lix;
                lix.set_rval(val);
                lix.set_lval(true);
                node->lvals()->at(i)->Accept(this, &lix);
            }
        }
        return IVal::None();
    }
    
    virtual IVal VisitStringLiteral(ast::StringLiteral *node, ast::VisitorContext *x) override {
        CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
        IVal val = IVal::Const(fun_scope_->kpool()->GerOrNewStr(node->value()));
        if (ctx->localize() && !ctx->keep_const()) {
            return Localize(val, node->line());
        }
        return val;
    }
    
    virtual IVal VisitApproxLiteral(ast::ApproxLiteral *node, ast::VisitorContext *x) override {
        CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
        IVal val = IVal::Const(fun_scope_->kpool()->GerOrNewF64(node->value()));
        if (ctx->localize() && !ctx->keep_const()) {
            return Localize(val, node->line());
        }
        return val;
    }
    
    virtual IVal VisitSmiLiteral(ast::SmiLiteral *node, ast::VisitorContext *x) override {
        CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
        IVal val = IVal::Const(fun_scope_->kpool()->GerOrNewSmi(node->value()));
        if (ctx->localize() && !ctx->keep_const()) {
            return Localize(val, node->line());
        }
        return val;
    }

    virtual IVal VisitVariable(ast::Variable *node, ast::VisitorContext *x) override {
        CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
        
        IVal val = blk_scope_->GetVariable(node->name());
        if (ctx->lval()) {
            if (val.kind == IVal::kNone) {
                val = IVal::Global(fun_scope_->kpool()->GerOrNewStr(node->name()));
                builder()->StoreGlobal(val, ctx->rval());
            } else {
                DCHECK_EQ(IVal::kLocal, val.kind);
                builder()->Move(val, ctx->rval());
            }
            return IVal::None();
        } else {
            if (val.kind == IVal::kNone) {
                val = IVal::Global(fun_scope_->kpool()->GerOrNewStr(node->name()));
            }
            if (ctx->localize()) {
                return Localize(val, node->line());
            } else {
                return val;
            }
        }
    }
    
    virtual IVal VisitIndex(ast::Index *node, ast::VisitorContext *x) override {
        CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
        
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        IVal self = node->self()->Accept(this, &ix);
        ix.set_keep_const(true);
        IVal index = node->index()->Accept(this, &ix);
        
        if (ctx->lval()) {
            builder()->SetField(self, index, ctx->rval(), node->line());
            return IVal::None();
        } else {
            builder()->GetField(self, index, node->line());
            return self;
        }
    }
    
    virtual IVal VisitDotField(ast::DotField *node, ast::VisitorContext *x) override {
        CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
        
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        IVal self = node->self()->Accept(this, &ix);
        IVal index = IVal::Const(fun_scope_->kpool()->GerOrNewStr(node->index()));
        
        if (ctx->lval()) {
            builder()->SetField(self, index, ctx->rval(), node->line());
            return IVal::None();
        } else {
            builder()->GetField(self, index, node->line());
            return self;
        }
    }
    
    virtual IVal VisitReturn(ast::Return *node, ast::VisitorContext *x) override {
        if (!node->rets()) {
            builder()->Ret(IVal::Local(0), 0);
            return IVal::None();
        }
        
        int32_t n_rets = node->GetNRets();
        CodeGeneratorContext ix;
        ix.set_n_result(n_rets < 0 ? -1 : 1);
        
        IVal first = node->rets()->at(0)->Accept(this, &ix);
        int32_t reg = first.index;
        
        for (size_t i = 1; i < node->rets()->size(); ++i) {
            ast::Expression *expr = node->rets()->at(i);
            AdjustStackPosition(++reg, expr->Accept(this, &ix), expr->line());
        }
        
        builder()->Ret(first, n_rets, node->line());
        return IVal::None();
    }
    
    virtual IVal VisitCall(ast::Call *node, ast::VisitorContext *x) override {
        int32_t n_args = node->GetNArgs();
        CodeGeneratorContext ix;
        ix.set_n_result(n_args < 0 ? -1 : 1);

        IVal callee = node->callee()->Accept(this, &ix);
        int32_t reg = callee.index;
        for (size_t i = 0; node->args() && i < node->args()->size(); ++i) {
            ast::Expression *expr = node->args()->at(i);
            AdjustStackPosition(++reg, expr->Accept(this, &ix), expr->line());
        }
        CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
        builder()->Call(callee, n_args, ctx->n_result(), node->line());
        return callee;
    }
    
    virtual IVal VisitMultiple(ast::Multiple *node, ast::VisitorContext *x) override {
        std::vector<IVal> operands;
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        ix.set_keep_const(true);
        for (int i = 0; i < node->n_operands(); ++i) {
            operands.push_back(node->operand(i)->Accept(this, &ix));
        }
        
        IVal ret = IVal::None();
        for (auto op : operands) {
            if (op.kind == IVal::kLocal && !blk_scope_->Protected(op)) {
                ret = op;
            }
        }
        if (ret.kind == IVal::kNone) {
            ret = fun_scope_->NewLocal();
        }
        switch (node->op()) {
            case Operator::kAdd:
                builder()->Add(ret, operands[0], operands[1], node->line());
                break;
            case Operator::kSub:
                builder()->Sub(ret, operands[0], operands[1], node->line());
                break;
            case Operator::kMul:
                builder()->Mul(ret, operands[0], operands[1], node->line());
                break;
            case Operator::kDiv:
                builder()->Div(ret, operands[0], operands[1], node->line());
                break;
                // TODO:
            default:
                DLOG(FATAL) << "TODO:";
                break;
        }
        return ret;
    }
    
    friend class FunctionScope;
    friend class BlockScope;
    DISALLOW_IMPLICIT_CONSTRUCTORS(CodeGeneratorVisitor);
private:
    IVal Localize(IVal val, int line) {
        switch (val.kind) {
            case IVal::kLocal:
                break;
            case IVal::kUpval:
            case IVal::kGlobal:
            case IVal::kConst: {
                IVal ret = fun_scope_->NewLocal();
                builder()->Load(ret, val, line);
                return ret;
            } break;
                break;
            case IVal::kNone:
            default:
                DLOG(FATAL) << "noreached.";
                break;
        }
        return val;
    }
    
    IVal AdjustStackPosition(int requried, IVal val, int line) {
        DCHECK_EQ(IVal::kLocal, val.kind);
        IVal dst{.kind = IVal::kLocal, .index = requried};
        if (requried >= fun_scope_->max_stack_) {
            dst = fun_scope_->NewLocal();
            DCHECK_EQ(requried, dst.index);
        }
        if (requried != val.index) {
            builder()->Move(dst, val, line);
            return dst;
        }
        return val;
    }
    
    NyaaCore *const core_;
    FunctionScope *fun_scope_ = nullptr;
    BlockScope *blk_scope_ = nullptr;
}; // class CodeGeneratorVisitor
    
inline FunctionScope::FunctionScope(CodeGeneratorVisitor *owns)
    : owns_(DCHECK_NOTNULL(owns))
    , kpool_builder_(owns->core_->factory()) {
    prev_ = owns_->fun_scope_;
    owns_->fun_scope_ = this;
}

inline FunctionScope::~FunctionScope() {
    DCHECK_EQ(this, owns_->fun_scope_);
    owns_->fun_scope_ = prev_;
}
    
inline BlockScope::BlockScope(FunctionScope *owns)
    : owns_(DCHECK_NOTNULL(owns)) {
    if (owns_->top_ == nullptr) {
        owns_->top_ = this;
    }
    prev_ = owns_->owns_->blk_scope_;
    owns_->owns_->blk_scope_ = this;
}

inline BlockScope::~BlockScope() {
    DCHECK_EQ(this, owns_->owns_->blk_scope_);
    owns_->owns_->blk_scope_ = prev_;
}
    
/*static*/ Handle<NyFunction> CodeGen::Generate(Handle<NyString> file_name, ast::Block *root,
                                                NyaaCore *core) {
    HandleScope handle_scope(core->isolate());

    CodeGeneratorVisitor visitor(core);
    FunctionScope scope(&visitor);
    root->Accept(&visitor, nullptr);
    
    Handle<NyByteArray> bcbuf;
    Handle<NyInt32Array> info;
    std::tie(bcbuf, info) = scope.builder()->Build(core);
    Handle<NyArray> kpool = scope.kpool()->Build(core);

    return core->factory()->NewFunction(nullptr/*name*/,
                                        0/*nparams*/,
                                        false/*vargs*/,
                                        0/*n_upvals*/,
                                        scope.max_stack(),
                                        *file_name,
                                        *info,
                                        *bcbuf,
                                        nullptr/*proto_pool*/,
                                        *kpool);
}
    
} // namespace nyaa
    
} // namespace mai
