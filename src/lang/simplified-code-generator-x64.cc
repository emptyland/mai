#include "lang/macro-assembler-x64.h"
#include "lang/compiler.h"
#include "lang/bytecode.h"
#include "lang/metadata.h"
#include "lang/isolate-inl.h"
#include "base/arenas.h"
#include "base/arena-utils.h"
#include <stack>


namespace mai {

namespace lang {

// For Base-line JIT Compiler
// Generator simplified machhine code
class X64SimplifiedCodeGenerator final {
public:
    X64SimplifiedCodeGenerator(CompilationInfo *compilation_info, base::Arena *arena);
    
    void Initialize();
    
    void Generate();
    
    const BytecodeNode *bc() const { return bc_[position_]; }
    const Function *function() const { return fun_env_.top(); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(X64SimplifiedCodeGenerator);
private:
    void Select();
    
    CompilationInfo *const compilation_info_;
    base::Arena *const arena_;
    base::ArenaMap<const Function *, base::ArenaVector<uint32_t>> function_linear_pc_;
    BytecodeNode **bc_ = nullptr;
    size_t position_ = 0;
    std::stack<const Function *> fun_env_;
}; // class X64SimplifiedCodeGenerator

X64SimplifiedCodeGenerator::X64SimplifiedCodeGenerator(CompilationInfo *compilation_info,
                                                       base::Arena *arena)
    : compilation_info_(compilation_info)
    , arena_(arena)
    , function_linear_pc_(arena) {}


void X64SimplifiedCodeGenerator::Initialize() {
    function_linear_pc_.emplace(compilation_info_->start_fun(),
                                base::ArenaVector<uint32_t>(arena_));
    fun_env_.push(compilation_info_->start_fun());
    
    bc_ = arena_->NewArray<BytecodeNode *>(compilation_info_->linear_path().size());
    for (size_t i = 0; i < compilation_info_->linear_path().size(); i++) {
        bc_[i] = BytecodeNode::From(arena_, compilation_info_->linear_path()[i]);
        if (bc_[i]->id() == kCheckStack) {
            auto iter = compilation_info_->invoke_info().find(i);
            DCHECK(iter != compilation_info_->invoke_info().end());
            fun_env_.push(iter->second.fun);
        }
        
        if (bc_[i]->id() == kReturn) {
            fun_env_.pop();
        }
        
        DCHECK(!fun_env_.empty());
        
        if (auto iter = function_linear_pc_.find(fun_env_.top());
            iter == function_linear_pc_.end()) {
            base::ArenaVector<uint32_t> linear_pc(arena_);
            linear_pc.push_back(compilation_info_->associated_pc()[i]);
            function_linear_pc_.insert(std::make_pair(fun_env_.top(), std::move(linear_pc)));
        } else {
            iter->second.push_back(compilation_info_->associated_pc()[i]);
        }
    }

    //fun_env_.pop();
    DCHECK(!fun_env_.empty());
}

void X64SimplifiedCodeGenerator::Generate() {
    for (position_ = 0; position_ < compilation_info_->linear_path().size(); position_++) {
        if (bc()->id() == kCheckStack) {
            auto iter = compilation_info_->invoke_info().find(position_);
            DCHECK(iter != compilation_info_->invoke_info().end());
            fun_env_.push(iter->second.fun);
        }
        
        Select();

        if (bc()->id() == kReturn) {
            fun_env_.pop();
        }
    }
}

void X64SimplifiedCodeGenerator::Select() {
    switch (bc()->id()) {
        case kCheckStack:
            break;
            
        default:
            NOREACHED();
            break;
    }
}

} // namespace lang

} // namespace mai
