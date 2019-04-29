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
    const ast::String *, IVal,
    base::ArenaHash<const ast::String *>,
    base::ArenaEqualTo<const ast::String *>>;
    
using FieldTable = std::unordered_map<
    const ast::String *, int,
    base::ArenaHash<const ast::String *>,
    base::ArenaEqualTo<const ast::String *>>;

class FunctionScope {
public:
    inline FunctionScope(CodeGeneratorVisitor *owns);
    inline ~FunctionScope();
    
    ConstPoolBuilder *kpool() { return &kpool_builder_; }
    BytecodeArrayBuilder *builder() { return &builder_; }
    DEF_VAL_GETTER(int32_t, max_stack);
    DEF_VAL_GETTER(int, free_reg);
    DEF_VAL_GETTER(int, active_vars);
    
    IVal NewLocal() {
        IVal val = IVal::Local(free_reg_++);
        if (free_reg_ > max_stack_) {
            max_stack_ = free_reg_;
        }
        //printf("[alloc] free-reg:%d\n", free_reg_);
        return val;
    }
    
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
        IVal base = IVal::Local(free_reg_);
        free_reg_ += n;
        if (free_reg_ > max_stack_) {
            max_stack_ = free_reg_;
        }
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
    
    void FreeVar(IVal val) {
        if (val.kind == IVal::kLocal) {
            if (val.index >= active_vars_) {
                //printf("[free] index:%d, free-reg:%d\n", val.index, free_reg_);
                free_reg_--;
                DCHECK_EQ(val.index, free_reg_);
            }
        }
    }
    
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
    ConstPoolBuilder kpool_builder_;
    BytecodeArrayBuilder builder_;
    std::vector<Handle<NyFunction>> protos_;
    VariableTable upvals_;
    std::vector<UpvalDesc> upval_desc_;
    int active_vars_ = 0;
    int free_reg_ = 0;
    int32_t max_stack_ = 0;
}; // class FunctionScope
    
    
class BlockScope {
public:
    inline BlockScope(FunctionScope *owns);
    inline ~BlockScope();
    
    DEF_PTR_GETTER(BlockScope, prev);
    DEF_PTR_GETTER(FunctionScope, owns);
    DEF_PTR_PROP_RW(BytecodeLable, loop_in);
    DEF_PTR_PROP_RW(BytecodeLable, loop_out);
    
    IVal GetVariable(const ast::String *name) {
        auto iter = vars_.find(name);
        return iter == vars_.end() ? IVal::Void() : iter->second;
    }

    IVal PutVariable(const ast::String *name, const IVal *val) {
        IVal rv = IVal::Void();
        if (ast::IsPlaceholder(name)) {
            return rv; // ignore!
        }
        if (val) {
            rv = *val;
        } else {
            rv = owns_->NewLocal();
        }
        DCHECK(rv.kind != IVal::kLocal || rv.index < owns_->max_stack());
        vars_.insert({name, rv});
        //DCHECK_EQ(rv.index, active_vars_);
        active_vars_++;
        owns_->active_vars_++;
        return rv;
    }
    
    bool Protected(IVal val) {
        if (val.kind == IVal::kLocal) {
            DCHECK_GE(val.index, 0);
            DCHECK_LT(val.index, owns_->max_stack());
            return val.index < active_vars_;
        }
        return false;
    }

private:
    BlockScope *prev_;
    FunctionScope *const owns_;
    int active_vars_;
    VariableTable vars_;
    
    BytecodeLable *loop_in_ = nullptr;
    BytecodeLable *loop_out_ = nullptr;
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

    CodeGeneratorVisitor(NyaaCore *core, base::Arena *arena, Handle<NyString> file_name)
        : core_(DCHECK_NOTNULL(core))
        , arena_(DCHECK_NOTNULL(arena))
        , file_name_(file_name) {}
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
            fun_scope_->FreeVar(self);
            fun_scope_->FreeVar(index);
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
        fun_scope_->FreeVar(closure);
        return IVal::Void();
    }
    
    virtual IVal VisitBlock(ast::Block *node, ast::VisitorContext */*ctx*/) override {
        BlockScope scope(fun_scope_);
        CodeGeneratorContext ctx;
        IVal ret;
        if (node->stmts()) {
            for (auto stmt : *node->stmts()) {
                ret = stmt->Accept(this, &ctx);
            }
        }
        return ret;
    }

    virtual IVal VisitVarDeclaration(ast::VarDeclaration *node, ast::VisitorContext *x) override {
        std::vector<IVal> vars;

        if (node->inits()) {
            CodeGeneratorContext rix;
            if (node->names()->size() > 1 && node->GetNWanted() < 0) {
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
        if (node->rvals()->size() == 1 && node->GetNWanted() < 0) {
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
    
    virtual IVal VisitIfStatement(ast::IfStatement *node, ast::VisitorContext *x) override {
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        IVal cond = node->cond()->Accept(this, &ix);
        builder()->Test(cond, 0, 0, node->line());
        fun_scope_->FreeVar(cond);

        BytecodeLable else_lable;
        builder()->Jump(&else_lable, fun_scope_->kpool(), node->line());
        node->then_clause()->Accept(this, &ix);

        if (!node->else_clause()) {
            builder()->Bind(&else_lable, fun_scope_->kpool());
        } else {
            BytecodeLable out_lable;
            builder()->Jump(&out_lable, fun_scope_->kpool(), node->then_clause()->line());
            builder()->Bind(&else_lable, fun_scope_->kpool());
            node->else_clause()->Accept(this, &ix);
            builder()->Bind(&out_lable, fun_scope_->kpool());
        }
        return IVal::Void();
    }
    
    virtual IVal VisitWhileLoop(ast::WhileLoop *node, ast::VisitorContext *x) override {
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        BytecodeLable in_lable;
        builder()->Bind(&in_lable, fun_scope_->kpool());
        
        IVal cond = node->cond()->Accept(this, &ix);
        builder()->Test(cond, 0, 0, node->line());
        fun_scope_->FreeVar(cond);
        BytecodeLable out_lable;
        builder()->Jump(&out_lable, fun_scope_->kpool(), node->line());
        
        blk_scope_->set_loop_in(&in_lable);
        blk_scope_->set_loop_out(&out_lable);
        
        node->body()->Accept(this, x);
        
        blk_scope_->set_loop_in(nullptr);
        blk_scope_->set_loop_out(nullptr);
        
        builder()->Jump(&in_lable, fun_scope_->kpool(), node->end_line());
        builder()->Bind(&out_lable, fun_scope_->kpool());
        return IVal::Void();
    }
    
    // for v1, v2, ... = expr {
    //    stmts...
    // }
    virtual IVal VisitForIterateLoop(ast::ForIterateLoop *node, ast::VisitorContext *x) override {
        BlockScope scope(fun_scope_);
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        
        IVal generator = node->init()->Accept(this, &ix);
        blk_scope_->PutVariable(ast::String::New(arena_, "(generator)"), &generator);
        
        BytecodeLable in_label;
        builder()->Jump(&in_label, fun_scope_->kpool(), node->line());

        for (auto name : *DCHECK_NOTNULL(node->names())) {
            blk_scope_->PutVariable(name, nullptr);
        }
        
        BytecodeLable body_label, out_label;
        scope.set_loop_in(&in_label);
        scope.set_loop_out(&out_label);
        builder()->Bind(&body_label, fun_scope_->kpool());
        node->body()->Accept(this, x);
        scope.set_loop_in(nullptr);
        scope.set_loop_out(nullptr);
        
        // test
        builder()->Bind(&in_label, fun_scope_->kpool());
        //builder()->Bind(&test_lable, fun_scope_->kpool());
        IVal callee = fun_scope_->NewLocal();
        builder()->Move(callee, generator, node->end_line());
        
        int nrets = static_cast<int>(node->names()->size());
        fun_scope_->Reserve(nrets);
        builder()->Call(callee, 0, nrets, node->end_line());
        builder()->TestNil(callee, 1/*neg*/, 0, node->end_line());
        builder()->Jump(&out_label, fun_scope_->kpool(), node->end_line());
        
        IVal base = callee;
        for (auto name : *DCHECK_NOTNULL(node->names())) {
            builder()->Move(blk_scope_->GetVariable(name), base, node->end_line());
            base.index++;
        }
        builder()->Jump(&body_label, fun_scope_->kpool(), node->end_line());
        builder()->Bind(&out_label, fun_scope_->kpool());
        return IVal::Void();
    }
    
    virtual IVal VisitContinue(ast::Continue *node, ast::VisitorContext *x) override {
        auto blk = blk_scope_;
        while (blk && blk->owns() == fun_scope_) {
            if (blk->loop_in()) {
                break;
            }
            blk = blk->prev();
        }
        DCHECK_NOTNULL(blk);
        builder()->Jump(blk->loop_in(), fun_scope_->kpool(), node->line());
        return IVal::Void();
    }
    
    virtual IVal VisitBreak(ast::Break *node, ast::VisitorContext *x) override {
        auto blk = blk_scope_;
        while (blk && blk->owns() == fun_scope_) {
            if (blk->loop_out()) {
                break;
            }
            blk = blk->prev();
        }
        DCHECK_NOTNULL(blk);
        builder()->Jump(blk->loop_out(), fun_scope_->kpool(), node->line());
        return IVal::Void();
    }
    
    virtual IVal VisitObjectDefinition(ast::ObjectDefinition *node, ast::VisitorContext *x) override {
        IVal clazz = DefineClass(node, nullptr);
        builder()->New(clazz, clazz, 0, node->end_line());
        if (node->local()) {
            blk_scope_->PutVariable(node->name(), &clazz);
        } else {
            fun_scope_->FreeVar(clazz);
            IVal key = IVal::Global(fun_scope_->kpool()->GetOrNewStr(node->name()));
            builder()->StoreGlobal(clazz, key, node->end_line());
        }
        return IVal::Void();
    }
    
    virtual IVal VisitClassDefinition(ast::ClassDefinition *node, ast::VisitorContext *) override {
        IVal clazz = DefineClass(node, node->base());
        if (node->local()) {
            blk_scope_->PutVariable(node->name(), &clazz);
        } else {
            fun_scope_->FreeVar(clazz);
            IVal key = IVal::Global(fun_scope_->kpool()->GetOrNewStr(node->name()));
            builder()->StoreGlobal(clazz, key, node->end_line());
        }
        return IVal::Void();
    }
    
    virtual IVal VisitNilLiteral(ast::NilLiteral *node, ast::VisitorContext *) override {
        IVal tmp = fun_scope_->NewLocal();
        builder()->LoadNil(tmp, 1);
        return tmp;
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
    
    virtual IVal VisitIntLiteral(ast::IntLiteral *node, ast::VisitorContext *x) override {
        CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
        IVal val = IVal::Const(fun_scope_->kpool()->GetOrNewInt(node->value()));
        if (ctx->localize() && !ctx->keep_const()) {
            return Localize(val, node->line());
        }
        return val;
    }
    
    virtual IVal VisitMapInitializer(ast::MapInitializer *node, ast::VisitorContext *x) override {
        int index = 0;
        if (!node->value()) {
            IVal map = fun_scope_->NewLocal();
            builder()->NewMap(map, 0/*n*/, 0/*linear*/, node->line());
            return map;
        }
        bool linear = true;
        for (auto entry : *node->value()) {
            if (entry->key()) {
                linear = false;
                break;
            }
        }
        
        std::vector<IVal> kvs;
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        ix.set_localize(true);
        ix.set_keep_const(false);
        
        if (linear) {
            for (auto entry : *node->value()) {
                IVal value = entry->value()->Accept(this, &ix);
                if (blk_scope_->Protected(value)) {
                    IVal tmp = fun_scope_->NewLocal();
                    builder()->Move(tmp, value);
                    value = tmp;
                }
                kvs.push_back(value);
            }
        } else {
            for (auto entry : *node->value()) {
                IVal key;
                if (entry->key()) {
                    key = entry->key()->Accept(this, &ix);
                } else {
                    key = IVal::Const(fun_scope_->kpool()->GetOrNewSmi(index++));
                    key = Localize(key, entry->value()->line());
                }
                IVal value = entry->value()->Accept(this, &ix);
                if (blk_scope_->Protected(value)) {
                    IVal tmp = fun_scope_->NewLocal();
                    builder()->Move(tmp, value);
                    value = tmp;
                }
                kvs.push_back(key);
                kvs.push_back(value);
            }
        }
        
        IVal map = kvs.front();
        builder()->NewMap(map, static_cast<int>(kvs.size()), linear, node->line());
        for (int64_t i = kvs.size() - 1; i > 0; --i) {
            fun_scope_->FreeVar(kvs[i]);
        }
        return map;
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
            
            proto = core_->factory()->NewFunction(
                    nullptr/*name*/,
                    !node->params() ? 0 : node->params()->size()/*nparams*/,
                    node->vargs()/*vargs*/,
                    fun_scope.upval_desc_.size() /*n_upvals*/,
                    fun_scope.max_stack(),
                    file_name_.is_empty() ? nullptr : *file_name_,
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
        IVal val = ctx->lval() ? IVal::Void() : fun_scope_->NewLocal();
        
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        IVal self = node->self()->Accept(this, &ix);
        ix.set_keep_const(true);
        IVal index = node->index()->Accept(this, &ix);

        if (ctx->lval()) {
            builder()->SetField(self, index, ctx->rval(), node->line());
            fun_scope_->FreeVar(index);
            fun_scope_->FreeVar(self);
            return IVal::Void();
        } else {
            builder()->GetField(val, self, index, node->line());
            fun_scope_->FreeVar(index);
            fun_scope_->FreeVar(self);
            return val;
        }
    }
    
    virtual IVal VisitDotField(ast::DotField *node, ast::VisitorContext *x) override {
        CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
        IVal val = ctx->lval() ? IVal::Void() : fun_scope_->NewLocal();
        
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        IVal self = node->self()->Accept(this, &ix);
        IVal index = IVal::Const(fun_scope_->kpool()->GetOrNewStr(node->index()));
        
        if (ctx->lval()) {
            builder()->SetField(self, index, ctx->rval(), node->line());
            fun_scope_->FreeVar(index);
            //fun_scope_->FreeVar(self);
            return IVal::Void();
        } else {
            builder()->GetField(val, self, index, node->line());
            fun_scope_->FreeVar(index);
            fun_scope_->FreeVar(self);
            return val;
        }
    }
    
    virtual IVal VisitReturn(ast::Return *node, ast::VisitorContext *x) override {
        if (!node->rets()) {
            builder()->Ret(IVal::Local(0), 0, node->line());
            return IVal::Void();
        }
        
        int32_t n_rets = node->GetNRets();
        CodeGeneratorContext ix;
        ix.set_n_result(n_rets < 0 ? -1 : 1);
        
        std::vector<IVal> rets;
        IVal first = node->rets()->at(0)->Accept(this, &ix);
        rets.push_back(first);
        int32_t reg = first.index;

        for (size_t i = 1; i < node->rets()->size(); ++i) {
            ast::Expression *expr = node->rets()->at(i);
            rets.push_back(AdjustStackPosition(++reg, expr->Accept(this, &ix), expr->line()));
        }
        builder()->Ret(first, n_rets, node->line());

        for (int64_t i = rets.size() - 1; i >= 0; --i) {
            fun_scope_->FreeVar(rets[i]);
        }
        return IVal::Void();
    }
    
    virtual IVal VisitCall(ast::Call *node, ast::VisitorContext *x) override {
        int32_t n_args = node->GetNArgs();
        CodeGeneratorContext ix;
        ix.set_n_result(1);

        IVal callee = node->callee()->Accept(this, &ix);
        if (blk_scope_->Protected(callee)) {
            IVal tmp = fun_scope_->NewLocal();
            builder()->Move(tmp, callee);
            callee = tmp;
        }
        
        ix.set_n_result(n_args < 0 ? -1 : 1);
        int reg = callee.index;
        //std::vector<IVal> args;
        for (size_t i = 0; node->args() && i < node->args()->size(); ++i) {
            ast::Expression *expr = node->args()->at(i);
            AdjustStackPosition(++reg, expr->Accept(this, &ix), expr->line());
        }

        CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
        builder()->Call(callee, n_args, ctx->n_result(), node->line());

        fun_scope_->free_reg_ = callee.index + 1;
        return callee;
    }
    
    virtual IVal VisitNew(ast::New *node, ast::VisitorContext *x) override {
        int32_t n_args = node->GetNArgs();
        CodeGeneratorContext ix;
        
        ix.set_n_result(1);
        IVal val = fun_scope_->NewLocal();
        IVal clazz = node->callee()->Accept(this, &ix);

        ix.set_n_result(n_args < 0 ? -1 : 1);
        int reg = clazz.index;
        for (size_t i = 0; node->args() && i < node->args()->size(); ++i) {
            ast::Expression *expr = node->args()->at(i);
            AdjustStackPosition(++reg, expr->Accept(this, &ix), expr->line());
        }
        
        builder()->New(val, clazz, n_args, node->line());
        //fun_scope_->FreeVar(clazz);
        fun_scope_->free_reg_ = clazz.index;
        return val;
    }
    
    virtual IVal VisitSelfCall(ast::SelfCall *node, ast::VisitorContext *x) override {
        int32_t n_args = node->GetNArgs();
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        
        IVal base = fun_scope_->NewLocal();
        IVal self = fun_scope_->NewLocal();
        IVal callee = node->callee()->Accept(this, &ix);
        IVal method = IVal::Const(fun_scope_->kpool()->GetOrNewStr(node->method()));
        builder()->Self(base, callee, method, node->line());
        fun_scope_->FreeVar(callee);
        
        ix.set_n_result(n_args < 0 ? -1 : 1);
        int reg = base.index + 1;
        for (size_t i = 0; node->args() && i < node->args()->size(); ++i) {
            ast::Expression *expr = node->args()->at(i);
            AdjustStackPosition(++reg, expr->Accept(this, &ix), expr->line());
        }
        (void)self;
        //fun_scope_->FreeVar(self);
        
        CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
        builder()->Call(base, n_args, ctx->n_result(), node->line());
        
        fun_scope_->free_reg_ = base.index + 1;
        return base;
    }
    
    virtual
    IVal VisitVariableArguments(ast::VariableArguments *node, ast::VisitorContext *x) override {
        CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
        IVal vargs = fun_scope_->NewLocal();
        builder()->Vargs(vargs, ctx->n_result(), node->line());
        return vargs;
    }
    
    virtual IVal VisitMultiple(ast::Multiple *node, ast::VisitorContext *x) override {
        std::vector<IVal> operands;
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        ix.set_keep_const(true);
        
        IVal ret = fun_scope_->NewLocal();
        for (int i = 0; i < node->n_operands(); ++i) {
            operands.push_back(node->operand(i)->Accept(this, &ix));
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
            case Operator::kEQ:
                builder()->Equal(ret, operands[0], operands[1], node->line());
                break;
            case Operator::kNE:
                builder()->NotEqual(ret, operands[0], operands[1], node->line());
                break;
            case Operator::kLT:
                builder()->LessThan(ret, operands[0], operands[1], node->line());
                break;
            case Operator::kLE:
                builder()->LessEqual(ret, operands[0], operands[1], node->line());
                break;
            case Operator::kGT:
                builder()->GreaterThan(ret, operands[0], operands[1], node->line());
                break;
            case Operator::kGE:
                builder()->GreaterEqual(ret, operands[0], operands[1], node->line());
                break;
            default:
                DLOG(FATAL) << "TODO:";
                break;
        }
        for (int64_t i = operands.size() - 1; i >= 0; --i) {
            fun_scope_->FreeVar(operands[i]);
        }
        return ret;
    }
    
    virtual IVal VisitConcat(ast::Concat *node, ast::VisitorContext *x) override {
        CodeGeneratorContext ix;
        ix.set_n_result(1);

        IVal base = IVal::Local(fun_scope_->free_reg());
        int32_t reg = base.index;
        std::vector<IVal> ops;
        for (auto op : *node->operands()) {
            IVal val = AdjustStackPosition(reg++, op->Accept(this, &ix), op->line());
            ops.push_back(val);
        }

        for (int64_t i = ops.size() -1; i >= 1; --i) {
            fun_scope_->FreeVar(ops[i]);
        }
        builder()->Concat(base, base, static_cast<int32_t>(ops.size()), node->line());
        return base;
    }
    
    friend class FunctionScope;
    friend class BlockScope;
    DISALLOW_IMPLICIT_CONSTRUCTORS(CodeGeneratorVisitor);
private:
    IVal DefineClass(ast::ObjectDefinition *node, ast::Expression *base) {
        std::vector<IVal> kvs;
        size_t index = 0;
        if (node->members()) {
            FieldTable fields;
            for (auto stmt : *node->members()) {
                if (ast::PropertyDeclaration *decl = stmt->ToPropertyDeclaration()) {
                    index = DeclareClassProperies(decl, index, &fields, &kvs);
                } else if (ast::FunctionDefinition *func = stmt->ToFunctionDefinition()) {
                    DefineClassMethod(node->name(), func, &kvs);
                } else {
                    DLOG(FATAL) << "noreached!";
                }
            }
        }
        auto bkz = core_->bkz_pool();
        {
            IVal key = IVal::Const(fun_scope_->kpool()->GetOrNewStr(bkz->kInnerType));
            kvs.push_back(Localize(key, node->line()));
            IVal val = IVal::Const(fun_scope_->kpool()->GetOrNewStr(node->name()));
            kvs.push_back(Localize(val, node->line()));
        }
        if (base) {
            IVal key = IVal::Const(fun_scope_->kpool()->GetOrNewStr(bkz->kInnerBase));
            kvs.push_back(Localize(key, node->line()));
            CodeGeneratorContext ix;
            ix.set_n_result(1);
            IVal val = base->Accept(this, &ix);
            if (blk_scope_->Protected(val)) {
                IVal tmp = fun_scope_->NewLocal();
                builder()->Move(tmp, val);
                val = tmp;
            }
            kvs.push_back(Localize(val, node->line()));
        }
        
        IVal clazz = kvs.front();
        builder()->NewMap(clazz, static_cast<int>(kvs.size()), -1/*linear*/, node->end_line());
        for (int64_t i = kvs.size() - 1; i > 0; --i) {
            fun_scope_->FreeVar(kvs[i]);
        }
        return clazz;
    }

    size_t DeclareClassProperies(ast::PropertyDeclaration *decl, size_t offset, FieldTable *fields,
                                 std::vector<IVal> *kvs) {
        for (auto nm : *decl->names()) {
            int64_t tag = decl->readonly() ? 0x1 : 0x3;
            auto iter = fields->find(nm);
            if (iter == fields->end()) {
                tag |= (offset++ << 2);
            } else {
                tag |= (iter->second << 2);
            }

            IVal key = IVal::Const(fun_scope_->kpool()->GetOrNewStr(nm));
            kvs->push_back(Localize(key, decl->line()));
            IVal val = fun_scope_->NewLocal();
            DCHECK_LT(tag, INT32_MAX);
            builder()->LoadImm(val, static_cast<int32_t>(tag), decl->line());
            kvs->push_back(val);
        }
        return offset;
    }
    
    void DefineClassMethod(const ast::String *class_name, ast::FunctionDefinition *node,
                           std::vector<IVal> *kvs) {
        CodeGeneratorContext rix;
        rix.set_localize(false);
        rix.set_keep_const(true);
        IVal fn = node->literal()->Accept(this, &rix);
        DCHECK_EQ(IVal::kFunction, fn.kind);
        
        Handle<NyFunction> lambda = fun_scope_->protos_[fn.index];
        Handle<NyString> name = core_->factory()->Sprintf("%s::%s", class_name->data(),
                                                          node->name()->data());
        lambda->SetName(*name, core_);
        
        IVal key = IVal::Const(fun_scope_->kpool()->GetOrNewStr(node->name()));
        kvs->push_back(Localize(key, node->line()));
        IVal val = fun_scope_->NewLocal();
        builder()->Closure(val, fn, node->line());
        kvs->push_back(val);
    }
    
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
        //IVal dst{.kind = IVal::kLocal, .index = requried};
//        if (requried >= fun_scope_->max_stack_) {
//            dst = fun_scope_->NewLocal();
//            DCHECK_EQ(requried, dst.index);
//        }
        if (requried != val.index) {
            IVal dst = fun_scope_->NewLocal();
            DCHECK_EQ(requried, dst.index);
            builder()->Move(dst, val, line);
            return dst;
        }
        return val;
    }
    
    NyaaCore *const core_;
    Handle<NyString> file_name_;
    base::Arena *arena_;
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
    : owns_(DCHECK_NOTNULL(owns))
    , active_vars_(0) {

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

    owns_->active_vars_ -= active_vars_;
    owns_->free_reg_ = owns_->active_vars_;
    
//    printf("*****************\n");
//    for (const auto &var : vars_) {
//        printf("var: %s = %d\n", var.first->data(), var.second.index);
//    }
}
    
/*static*/ Handle<NyFunction> CodeGen::Generate(Handle<NyString> file_name, ast::Block *root,
                                                base::Arena *arena, NyaaCore *core) {
    HandleScope handle_scope(core->isolate());

    CodeGeneratorVisitor visitor(core, arena, file_name);
    FunctionScope scope(&visitor);
    root->Accept(&visitor, nullptr);
    
    Handle<NyByteArray> bcbuf;
    Handle<NyInt32Array> info;
    scope.builder()->Ret(IVal::Local(0), 0); // last return
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
