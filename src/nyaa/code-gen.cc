#include "nyaa/code-gen.h"
#include "nyaa/bytecode-builder.h"
#include "nyaa/ast.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/object-factory.h"
#include "base/hash.h"
#include "glog/logging.h"
#include <unordered_map>

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
    
    IVal NewLocal() { return IVal::Local(max_stack_++); }
    
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
        IVal rv;
        if (val) {
            rv = *val;
            locals_[name] = *val;
        } else {
            rv = owns_->NewLocal();
            locals_.insert({name, rv});
        }
        return rv;
    }

private:
    using VariableTable = std::unordered_map<const ast::String *,
        IVal,
        base::ArenaHash<const ast::String *>,
        base::ArenaEqualTo<const ast::String *>>;
    
    BlockScope *prev_;
    FunctionScope *const owns_;
    VariableTable locals_;
}; // class BlockScope
    
class CodeGeneratorContext : public ast::VisitorContext {
public:
    CodeGeneratorContext() {}
    ~CodeGeneratorContext() {}
    
    DEF_VAL_PROP_RW(int, n_result);
    DEF_VAL_PROP_RW(bool, localize);
    DEF_VAL_PROP_RW(bool, keep_const);

    static CodeGeneratorContext *Cast(ast::VisitorContext *ctx) {
        return !ctx ? nullptr : down_cast<CodeGeneratorContext>(ctx);
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(CodeGeneratorContext);
private:
    int n_result_ = 0;
    bool localize_ = true;
    bool keep_const_ = false;
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
            CodeGeneratorContext ix;
            if (node->names()->size() > 1 && node->inits()->size() == 1) {
                ix.set_n_result(static_cast<int>(node->names()->size()));
            } else {
                ix.set_n_result(1);
            }
            size_t len = std::min(node->names()->size(), node->inits()->size());
            size_t i = 0;
            for (i = 0; i < len; ++i) {
                ast::Expression *expr = node->inits()->at(i);
                IVal val = expr->Accept(this, &ix);
                blk_scope_->PutLocal(node->names()->at(i), &val);
            }
            if (node->inits()->size() > 1 || !node->inits()->at(0)->IsCall()) {
                for (; i < node->names()->size(); ++i) {
                    builder()->LoadNil(blk_scope_->GetOrNewLocal(node->names()->at(i)), 1,
                                       node->line());
                }
            }
        } else {
            for (size_t i = 0; i < node->names()->size(); ++i) {
                const ast::String *name = node->names()->at(i);
                vars.push_back(blk_scope_->GetOrNewLocal(name));
            }
            builder()->LoadNil(vars[0], static_cast<int32_t>(vars.size()), node->line());
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
    
    virtual IVal VisitVariable(ast::Variable *node, ast::VisitorContext *x) override {
        IVal val = blk_scope_->GetVariable(node->name());
        if (val.kind == IVal::kNone) {
            IVal g = {.kind = IVal::kGlobal};
            g.index = fun_scope_->kpool()->GerOrNewStr(node->name());
            val = g;
        }
        if (CodeGeneratorContext::Cast(x)->localize()) {
            return Localize(val, node->line());
        } else {
            return val;
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
        
        builder()->Ret(first, n_rets);
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
        builder()->Call(callee, n_args, ctx->n_result());
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
            if (op.kind == IVal::kLocal) {
                ret = op;
            }
        }
        if (ret.kind == IVal::kNone) {
            ret = fun_scope_->NewLocal();
        }
        switch (node->op()) {
            case Operator::kAdd:
                builder()->Add(ret, operands[0], operands[1]);
                break;
            case Operator::kSub:
                builder()->Sub(ret, operands[0], operands[1]);
                break;
            case Operator::kMul:
                builder()->Mul(ret, operands[0], operands[1]);
                break;
            case Operator::kDiv:
                builder()->Div(ret, operands[0], operands[1]);
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
    IVal Localize(IVal val, int line = 0) {
        switch (val.kind) {
            case IVal::kLocal:
                break;
            case IVal::kUpval:
            case IVal::kGlobal:
            case IVal::kConst: {
                IVal ret = fun_scope_->NewLocal();
                builder()->Load(ret, val);
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
    
/*static*/ Handle<NyScript> CodeGen::Generate(Handle<NyString> file_name, ast::Block *root,
                                              NyaaCore *core) {
    HandleScope handle_scope(core->isolate());

    CodeGeneratorVisitor visitor(core);
    FunctionScope scope(&visitor);
    root->Accept(&visitor, nullptr);
    
    Handle<NyByteArray> bcbuf;
    Handle<NyInt32Array> info;
    std::tie(bcbuf, info) = scope.builder()->Build(core);
    Handle<NyArray> kpool = scope.kpool()->Build(core);
    return core->factory()->NewScript(scope.max_stack(), *file_name, *info, *bcbuf, *kpool);
}
    
} // namespace nyaa
    
} // namespace mai
