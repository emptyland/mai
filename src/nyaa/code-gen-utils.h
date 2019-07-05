#ifndef MAI_NYAA_CODE_GEN_UTILS_H_
#define MAI_NYAA_CODE_GEN_UTILS_H_

#include "nyaa/bytecode-builder.h"
#include "nyaa/object-factory.h"
#include "nyaa/ast.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/function.h"
#include "nyaa/nyaa-core.h"
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
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// class FunctionScope
////////////////////////////////////////////////////////////////////////////////////////////////////

class FunctionScope {
public:
    FunctionScope(CodeGeneratorVisitor *owns);
    ~FunctionScope();
    
    DEF_PTR_GETTER(FunctionScope, prev);
    ConstPoolBuilder *kpool() { return &kpool_builder_; }
    DEF_VAL_GETTER(int32_t, max_stack);
    DEF_VAL_PROP_RW(int, free_reg);
    DEF_VAL_GETTER(int, active_vars);
    Handle<NyFunction> proto(size_t i) const { return protos_[i]; }

    struct UpvalDesc {
        const ast::String *name;
        bool in_stack;
        int index;
    };

    const std::vector<UpvalDesc> &upval_desc() const { return upval_desc_; }
    size_t upval_desc_size() const { return upval_desc_.size(); }
    
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
    
    bool Protected(IVal val) {
        if (val.kind == IVal::kLocal) {
            DCHECK_GE(val.index, 0);
            DCHECK_LT(val.index, max_stack_);
            return val.index < active_vars_;
        }
        return false;
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
                free_reg_--;
                DCHECK_EQ(val.index, free_reg_);
            }
        }
    }
    
    //friend class CodeGeneratorVisitor;
    friend class BlockScope;
private:
    FunctionScope *prev_;
    CodeGeneratorVisitor *const owns_;
    int level_ = 0;
    BlockScope *top_ = nullptr;
    BlockScope *current_ = nullptr;
    ConstPoolBuilder kpool_builder_;
    std::vector<Handle<NyFunction>> protos_;
    VariableTable upvals_;
    std::vector<UpvalDesc> upval_desc_;
    int active_vars_ = 0;
    int free_reg_ = 0;
    int32_t max_stack_ = 0;
}; // class FunctionScope

////////////////////////////////////////////////////////////////////////////////////////////////////
/// class BlockScope
////////////////////////////////////////////////////////////////////////////////////////////////////

class BlockScope {
public:
    BlockScope(FunctionScope *owns);
    ~BlockScope();
    
    DEF_PTR_GETTER(BlockScope, prev);
    DEF_PTR_GETTER(FunctionScope, owns);
    DEF_PTR_PROP_RW(void, loop_in);
    DEF_PTR_PROP_RW(void, loop_out);
    
    IVal GetVariable(const ast::String *name) {
        auto iter = vars_.find(name);
        return iter == vars_.end() ? IVal::Void() : iter->second;
    }
    
    IVal PutVariable(const ast::String *name, const IVal *val);
    
//    bool Protected(IVal val) {
//        if (val.kind == IVal::kLocal) {
//            DCHECK_GE(val.index, 0);
//            DCHECK_LT(val.index, owns_->max_stack());
//            return val.index < active_vars_;
//        }
//        return false;
//    }
    
    friend class CodeGeneratorVisitor;
private:
    BlockScope *prev_;
    FunctionScope *const owns_;
    int active_vars_;
    VariableTable vars_;
    
    void *loop_in_ = nullptr;
    void *loop_out_ = nullptr;
}; // class BlockScope
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// class CodeGeneratorContext
////////////////////////////////////////////////////////////////////////////////////////////////////

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
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// class CodeGeneratorVisitor
////////////////////////////////////////////////////////////////////////////////////////////////////

class CodeGeneratorVisitor : public ast::Visitor {
public:
    using Context = CodeGeneratorContext;
    
    CodeGeneratorVisitor(NyaaCore *core, base::Arena *arena, Handle<NyString> file_name)
        : core_(DCHECK_NOTNULL(core))
        , arena_(DCHECK_NOTNULL(arena))
        , file_name_(file_name) {}
    virtual ~CodeGeneratorVisitor() override {}
    
    DEF_PTR_GETTER(NyaaCore, core);

    //----------------------------------------------------------------------------------------------
    // Owns Interfaces
    //----------------------------------------------------------------------------------------------
    virtual IVal Localize(IVal val, int line) = 0;
    virtual void LoadNil(IVal val, int n, int line) = 0;
    virtual void LoadImm(IVal val, int32_t imm, int line) = 0;
    virtual void Call(IVal callee, int nargs, int wanted, int line) = 0;
    virtual void Ret(IVal base, int nrets, int line) = 0;
    virtual void Move(IVal dst, IVal src, int line) = 0;
    virtual void StoreUp(IVal val, IVal up, int line) = 0;
    virtual void StoreGlobal(IVal val, IVal name, int line) = 0;
    virtual void NewMap(IVal map, int n, int linear, int line) = 0;
    virtual void SetField(IVal self, IVal index, IVal value, int line) = 0;
    virtual void GetField(IVal value, IVal self, IVal index, int line) = 0;
    virtual void Closure(IVal closure, IVal func, int line) = 0;
    virtual void Self(IVal base, IVal callee, IVal method, int line) = 0;
    virtual void New(IVal val, IVal clazz, int nargs, int line) = 0;
    virtual void Vargs(IVal vargs, int wanted, int line) = 0;
    virtual void Concat(IVal val, IVal base, int n, int line) = 0;
    
    //----------------------------------------------------------------------------------------------
    // Implements from ast::Visitor
    //----------------------------------------------------------------------------------------------
    virtual IVal VisitObjectDefinition(ast::ObjectDefinition *node, ast::VisitorContext *x) override;
    virtual IVal VisitClassDefinition(ast::ClassDefinition *node, ast::VisitorContext *x) override;
    virtual IVal VisitFunctionDefinition(ast::FunctionDefinition *node, ast::VisitorContext *x) override;
    virtual IVal VisitBlock(ast::Block *node, ast::VisitorContext *x) override;
    virtual IVal VisitVarDeclaration(ast::VarDeclaration *node, ast::VisitorContext *x) override;
    virtual IVal VisitVariable(ast::Variable *node, ast::VisitorContext *x) override;
    virtual IVal VisitAssignment(ast::Assignment *node, ast::VisitorContext *x) override;
    virtual IVal VisitIndex(ast::Index *node, ast::VisitorContext *x) override;
    virtual IVal VisitDotField(ast::DotField *node, ast::VisitorContext *x) override;
    virtual IVal VisitSelfCall(ast::SelfCall *node, ast::VisitorContext *x) override;
    virtual IVal VisitNew(ast::New *node, ast::VisitorContext *x) override;
    virtual IVal VisitCall(ast::Call *node, ast::VisitorContext *x) override;
    virtual IVal VisitReturn(ast::Return *node, ast::VisitorContext *x) override;
    virtual IVal VisitNilLiteral(ast::NilLiteral *node, ast::VisitorContext *) override;
    virtual IVal VisitStringLiteral(ast::StringLiteral *node, ast::VisitorContext *x) override;
    virtual IVal VisitApproxLiteral(ast::ApproxLiteral *node, ast::VisitorContext *x) override;
    virtual IVal VisitSmiLiteral(ast::SmiLiteral *node, ast::VisitorContext *x) override;
    virtual IVal VisitIntLiteral(ast::IntLiteral *node, ast::VisitorContext *x) override;
    virtual IVal VisitMapInitializer(ast::MapInitializer *node, ast::VisitorContext *x) override;
    virtual IVal VisitConcat(ast::Concat *node, ast::VisitorContext *x) override;
    virtual IVal VisitVariableArguments(ast::VariableArguments *node, ast::VisitorContext *x) override;
    
    friend class FunctionScope;
    friend class BlockScope;
    DISALLOW_IMPLICIT_CONSTRUCTORS(CodeGeneratorVisitor);
protected:
    IVal DefineClass(ast::ObjectDefinition *node, ast::Expression *base);
    size_t DeclareClassProperies(ast::PropertyDeclaration *decl, size_t offset, FieldTable *fields,
                                 std::vector<IVal> *kvs);
    void DefineClassMethod(const ast::String *class_name, ast::FunctionDefinition *node,
                           std::vector<IVal> *kvs);
    IVal AdjustStackPosition(int requried, IVal val, int line);
    
    static bool IsPlaceholder(const ast::String *name) {
        return name->size() == 1 && name->data()[0] == '_';
    }
    
    static bool IsNotPlaceholder(const ast::String *name) { return !IsPlaceholder(name); }

    NyaaCore *const core_;
    Handle<NyString> file_name_;
    base::Arena *arena_;
    FunctionScope *fun_scope_ = nullptr;
    BlockScope *blk_scope_ = nullptr;
}; // class CodeGeneratorVisitor
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// AST Serialization
////////////////////////////////////////////////////////////////////////////////////////////////////

Error SerializeCompactedAST(ast::AstNode *node, std::string *buf);
    
Error LoadCompactedAST(std::string_view buf, ast::Factory *factory, ast::AstNode **rv);

} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_CODE_GEN_UTILS_H_
