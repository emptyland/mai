#ifndef MAI_NYAA_CODE_GEN_BASE_H_
#define MAI_NYAA_CODE_GEN_BASE_H_

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
    BytecodeArrayBuilder *builder() { return &builder_; }
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
    
    //friend class CodeGeneratorVisitor;
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

////////////////////////////////////////////////////////////////////////////////////////////////////
/// class BlockScope
////////////////////////////////////////////////////////////////////////////////////////////////////

class BlockScope {
public:
    BlockScope(FunctionScope *owns);
    ~BlockScope();
    
    DEF_PTR_GETTER(BlockScope, prev);
    DEF_PTR_GETTER(FunctionScope, owns);
    DEF_PTR_PROP_RW(BytecodeLable, loop_in);
    DEF_PTR_PROP_RW(BytecodeLable, loop_out);
    
    IVal GetVariable(const ast::String *name) {
        auto iter = vars_.find(name);
        return iter == vars_.end() ? IVal::Void() : iter->second;
    }
    
    IVal PutVariable(const ast::String *name, const IVal *val);
    
    bool Protected(IVal val) {
        if (val.kind == IVal::kLocal) {
            DCHECK_GE(val.index, 0);
            DCHECK_LT(val.index, owns_->max_stack());
            return val.index < active_vars_;
        }
        return false;
    }
    
    friend class CodeGeneratorVisitor;
private:
    BlockScope *prev_;
    FunctionScope *const owns_;
    int active_vars_;
    VariableTable vars_;
    
    BytecodeLable *loop_in_ = nullptr;
    BytecodeLable *loop_out_ = nullptr;
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
    
    friend class FunctionScope;
    friend class BlockScope;
    DISALLOW_IMPLICIT_CONSTRUCTORS(CodeGeneratorVisitor);
protected:
    NyaaCore *const core_;
    Handle<NyString> file_name_;
    base::Arena *arena_;
    FunctionScope *fun_scope_ = nullptr;
    BlockScope *blk_scope_ = nullptr;
}; // class CodeGeneratorVisitor

} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_CODE_GEN_BASE_H_
