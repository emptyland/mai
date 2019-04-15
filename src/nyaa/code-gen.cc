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
    
using VariableTable = std::unordered_map<
    const ast::String *,
    IVal,
    base::ArenaHash<const ast::String *>,
    base::ArenaEqualTo<const ast::String *>>;

class FunctionScope {
public:
    inline FunctionScope(CodeGeneratorVisitor *owns);
    inline ~FunctionScope();
    
    ConstPoolBuilder *kpool() { return &kpool_builder_; }
    BytecodeArrayBuilder *builder() { return &builder_; }
    DEF_VAL_GETTER(int32_t, max_stack);
    
    IVal NewLocal() { return IVal::Local(max_stack_ ++); }
    
    IVal NewProto(Handle<NyFunction> proto) {
        int index = static_cast<int32_t>(protos_.size());
        protos_.push_back(proto);
        return IVal::Function(index);
    }
    
    IVal NewUpval(const ast::String *name, bool in_stack, int reg) {
        int32_t index = static_cast<int32_t>(upval_desc_.size());
        upval_desc_.push_back({name, in_stack, reg});
        IVal val = IVal::Upval(index);
        upvals_.insert({name, val});
        return val;
    }
    
    IVal Reserve(int n) {
        IVal base = IVal::Local(max_stack_);
        max_stack_ += n;
        return base;
    }
    
    Handle<NyArray> BuildProtos(NyaaCore *core) {
        if (protos_.empty()) {
            return Handle<NyArray>::Null();
        }
        Handle<NyArray> pool = core->factory()->NewArray(protos_.size());
        for (auto proto : protos_) {
            pool->Add(*proto, core);
        }
        return pool;
    }
    
    IVal GetVariable(const ast::String *name);
    
    IVal GetOrNewUpvalNested(const ast::String *name);
    
    struct UpvalDesc {
        const ast::String *name;
        bool in_stack;
        int index;
    };
    
    friend class CodeGeneratorVisitor;
    friend class BlockScope;
private:
    FunctionScope *prev_;
    CodeGeneratorVisitor *const owns_;
    int level_ = 0;
    BlockScope *top_ = nullptr;
    BlockScope *current_ = nullptr;
    int32_t max_stack_ = 0; // min is 2;
    ConstPoolBuilder kpool_builder_;
    BytecodeArrayBuilder builder_;
    std::vector<Handle<NyFunction>> protos_;
    VariableTable upvals_;
    std::vector<UpvalDesc> upval_desc_;
}; // class FunctionScope
    
    
class BlockScope {
public:
    inline BlockScope(FunctionScope *owns);
    inline ~BlockScope();
    
    DEF_PTR_GETTER(BlockScope, prev);
    DEF_PTR_GETTER(FunctionScope, owns);
    
    
    IVal GetVariable(const ast::String *name) {
        auto iter = vars_.find(name);
        return iter == vars_.end() ? IVal::Void() : iter->second;
    }

    IVal PutVariable(const ast::String *name, const IVal *val) {
        IVal rv = IVal::Void();
        if (name->size() == 1 && name->data()[0] == '_') {
            return rv; // ignore!
        }
        if (val) {
            rv = *val;
        } else {
            rv = owns_->NewLocal();
        }
        DCHECK(rv.kind != IVal::kLocal || rv.index < owns_->max_stack());
        //DCHECK_LT(rv.index, owns_->max_stack());
        auto iter = vars_.find(name);
        if (iter != vars_.end()) {
            prot_regs_.erase(iter->second.index);
        }
        vars_.insert({name, rv});
        if (rv.kind == IVal::kLocal) {
            prot_regs_.insert(rv.index);
        }
        return rv;
    }
    
    bool Protected(IVal val) {
        if (IVal::kLocal != val.kind) {
            return false;
        }
        return prot_regs_.find(val.index) != prot_regs_.end();
    }

private:
    BlockScope *prev_;
    FunctionScope *const owns_;
    VariableTable vars_;
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
    
    virtual IVal
    VisitFunctionDefinition(ast::FunctionDefinition *node, ast::VisitorContext *x) override {
        // TODO: for object or class scope.
        CodeGeneratorContext rix;
        rix.set_localize(false);
        rix.set_keep_const(true);
        IVal rval = node->literal()->Accept(this, &rix);
        DCHECK_EQ(IVal::kFunction, rval.kind);
        
        Handle<NyFunction> lambda = fun_scope_->protos_[rval.index];
        lambda->SetName(core_->factory()->NewString(node->name()->data(),
                                                    node->name()->size()), core_);
        IVal closure = fun_scope_->NewLocal();
        builder()->Closure(closure, rval, node->line());
        if (node->self()) {
            CodeGeneratorContext lix;
            lix.set_n_result(1);
            IVal self = node->self()->Accept(this, &lix);
            IVal index = IVal::Const(fun_scope_->kpool()->GetOrNewStr(node->name()));
            builder()->SetField(self, index, closure);
        } else {
            if (fun_scope_->prev_ == nullptr) {
                // as global variable
                IVal lval = IVal::Global(fun_scope_->kpool()->GetOrNewStr(node->name()));
                builder()->StoreGlobal(closure, lval, node->line());
            } else {
                // as local variable
                blk_scope_->PutVariable(node->name(), &closure);
            }
        }
        return IVal::Void();
    }
    
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
            if (node->names()->size() > 1 &&
                node->inits()->size() == 1 && node->inits()->at(0)->IsInvoke()) {
                rix.set_n_result(static_cast<int>(node->names()->size()));
                IVal rval = node->inits()->at(0)->Accept(this, &rix);
                fun_scope_->Reserve(rix.n_result() - 1);

                DCHECK_EQ(IVal::kLocal, rval.kind);
                for (size_t i = 0; i < node->names()->size(); ++i) {
                    blk_scope_->PutVariable(node->names()->at(i), &rval);
                    rval.index++;
                }
            } else {
                rix.set_n_result(1);
                
                for (size_t i = 0; i < node->names()->size(); ++i) {
                    if (i < node->inits()->size()) {
                        ast::Expression *init = node->inits()->at(i);
                        IVal rval = init->Accept(this, &rix);
                        blk_scope_->PutVariable(node->names()->at(i), &rval);
                    } else {
                        IVal lval = blk_scope_->PutVariable(node->names()->at(i), nullptr);
                        builder()->LoadNil(lval, 1, node->line());
                    }
                }
            }
        } else {
            for (size_t i = 0; i < node->names()->size(); ++i) {
                const ast::String *name = node->names()->at(i);
                vars.push_back(blk_scope_->PutVariable(name, nullptr));
            }
            builder()->LoadNil(vars[0], static_cast<int32_t>(vars.size()), node->line());
        }
        return IVal::Void();
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
        return IVal::Void();
    }
    
    virtual IVal VisitStringLiteral(ast::StringLiteral *node, ast::VisitorContext *x) override {
        CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
        IVal val = IVal::Const(fun_scope_->kpool()->GetOrNewStr(node->value()));
        if (ctx->localize() && !ctx->keep_const()) {
            return Localize(val, node->line());
        }
        return val;
    }
    
    virtual IVal VisitApproxLiteral(ast::ApproxLiteral *node, ast::VisitorContext *x) override {
        CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
        IVal val = IVal::Const(fun_scope_->kpool()->GetOrNewF64(node->value()));
        if (ctx->localize() && !ctx->keep_const()) {
            return Localize(val, node->line());
        }
        return val;
    }
    
    virtual IVal VisitSmiLiteral(ast::SmiLiteral *node, ast::VisitorContext *x) override {
        CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
        IVal val = IVal::Const(fun_scope_->kpool()->GetOrNewSmi(node->value()));
        if (ctx->localize() && !ctx->keep_const()) {
            return Localize(val, node->line());
        }
        return val;
    }
    
    virtual IVal VisitLambdaLiteral(ast::LambdaLiteral *node, ast::VisitorContext *x) override {
        //HandleScope handle_scope(core_->isolate());
        Handle<NyFunction> proto;
        {
            FunctionScope fun_scope(this);
            BlockScope blk_scope(&fun_scope);
            if (node->params()) {
                for (auto param : *node->params()) {
                     blk_scope.PutVariable(param, nullptr);
                }
            }
            CodeGeneratorContext bix;
            node->value()->Accept(this, &bix);
            builder()->Ret(IVal::Local(0), 0);

            Handle<NyByteArray> bcbuf;
            Handle<NyInt32Array> info;
            std::tie(bcbuf, info) = fun_scope.builder()->Build(core_);
            Handle<NyArray> kpool = fun_scope.kpool()->Build(core_);
            Handle<NyArray> fpool = fun_scope.BuildProtos(core_);
            
            proto = core_->factory()->NewFunction(nullptr/*name*/,
                                                  !node->params() ? 0 : node->params()->size()/*nparams*/,
                                                  node->vargs()/*vargs*/,
                                                  fun_scope.upval_desc_.size() /*n_upvals*/,
                                                  fun_scope.max_stack(),
                                                  nullptr/*file_name*/,
                                                  *info, *bcbuf, *fpool, *kpool);
            size_t i = 0;
            for (auto upval : fun_scope.upval_desc_) {
                NyString *name = core_->factory()->NewString(upval.name->data(), upval.name->size());
                proto->SetUpval(i++, name, upval.in_stack, upval.index, core_);
            }
        }
        IVal val = fun_scope_->NewProto(proto);
        CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
        if (ctx->localize() && !ctx->keep_const()) {
            return Localize(val, node->line());
        }
        return val;
    }

    virtual IVal VisitVariable(ast::Variable *node, ast::VisitorContext *x) override {
        CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
        
        IVal val = fun_scope_->GetOrNewUpvalNested(node->name());
        if (ctx->lval()) {
            switch (val.kind) {
                case IVal::kVoid:
                    val = IVal::Global(fun_scope_->kpool()->GetOrNewStr(node->name()));
                    builder()->StoreGlobal(ctx->rval(), val, node->line());
                    break;
                case IVal::kUpval:
                    builder()->StoreUp(ctx->rval(), val, node->line());
                    break;
                default:
                    DCHECK_EQ(IVal::kLocal, val.kind);
                    builder()->Move(val, ctx->rval(), node->line());
                    break;
            }
            return IVal::Void();
        } else {
            switch (val.kind) {
                case IVal::kVoid:
                    val = IVal::Global(fun_scope_->kpool()->GetOrNewStr(node->name()));
                    break;
                case IVal::kLocal:
                case IVal::kUpval:
                    break;
                default:
                    break;
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
            return IVal::Void();
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
        IVal index = IVal::Const(fun_scope_->kpool()->GetOrNewStr(node->index()));
        
        if (ctx->lval()) {
            builder()->SetField(self, index, ctx->rval(), node->line());
            return IVal::Void();
        } else {
            builder()->GetField(self, index, node->line());
            return self;
        }
    }
    
    virtual IVal VisitReturn(ast::Return *node, ast::VisitorContext *x) override {
        if (!node->rets()) {
            builder()->Ret(IVal::Local(0), 0);
            return IVal::Void();
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
        return IVal::Void();
    }
    
    virtual IVal VisitCall(ast::Call *node, ast::VisitorContext *x) override {
        int32_t n_args = node->GetNArgs();
        CodeGeneratorContext ix;
        ix.set_n_result(n_args < 0 ? -1 : 1);

        IVal callee = node->callee()->Accept(this, &ix);
        if (blk_scope_->Protected(callee)) {
            IVal tmp = fun_scope_->NewLocal();
            builder()->Move(tmp, callee);
            callee = tmp;
        }
        int reg = callee.index;
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
        
        IVal ret = IVal::Void();
        for (auto op : operands) {
            if (op.kind == IVal::kLocal && !blk_scope_->Protected(op)) {
                ret = op;
            }
        }
        if (ret.kind == IVal::kVoid) {
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
            case IVal::kFunction:
            case IVal::kGlobal:
            case IVal::kConst: {
                IVal ret = fun_scope_->NewLocal();
                builder()->Load(ret, val, line);
                return ret;
            } break;
                break;
            case IVal::kVoid:
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
        
    if (prev_) {
        level_ = prev_->level_ + 1;
    }
}

inline FunctionScope::~FunctionScope() {
    DCHECK_EQ(this, owns_->fun_scope_);
    owns_->fun_scope_ = prev_;
}
    
IVal FunctionScope::GetVariable(const ast::String *name) {
    BlockScope *blk = current_;
    while (blk && blk->owns() == this) {
        IVal val = blk->GetVariable(name);
        if (val.kind != IVal::kVoid) {
            return val;
        }
        blk = blk->prev();
    }
    auto iter = upvals_.find(name);
    if (iter != upvals_.end()) {
        return iter->second;
    }
    return IVal::Void();
}

IVal FunctionScope::GetOrNewUpvalNested(const ast::String *name) {
    IVal val = GetVariable(name);
    if (val.kind != IVal::kVoid) {
        return val;
    }
    if (prev_) {
        val = prev_->GetOrNewUpvalNested(name);
        if (val.kind == IVal::kLocal) {
            val = NewUpval(name, true, val.index);
        } else if (val.kind == IVal::kUpval) {
            val = NewUpval(name, false, val.index);
        }
        return val;
    }
    return IVal::Void();
}

inline BlockScope::BlockScope(FunctionScope *owns)
    : owns_(DCHECK_NOTNULL(owns)) {
        

    prev_ = owns_->owns_->blk_scope_;
    owns_->owns_->blk_scope_ = this;
    if (!owns_->top_) {
        owns_->top_ = this;
    }
    owns_->current_ = this;
}

inline BlockScope::~BlockScope() {
    DCHECK_EQ(this, owns_->owns_->blk_scope_);
    owns_->owns_->blk_scope_ = prev_;
    if (owns_->top_ == this) {
        owns_->top_ = nullptr;
    }
    owns_->current_ = prev_;
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
    Handle<NyArray> fpool = scope.BuildProtos(core);

    Handle<NyFunction> result = core->factory()->NewFunction(nullptr/*name*/,
            0/*nparams*/,
            false/*vargs*/,
            0/*n_upvals*/,
            scope.max_stack(),
            *file_name,
            *info,
            *bcbuf,
            *fpool/*proto_pool*/,
            *kpool);
    return handle_scope.CloseAndEscape(result);
    //return result;
}
    
} // namespace nyaa
    
} // namespace mai
