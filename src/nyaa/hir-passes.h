#ifndef MAI_NYAA_HIR_PASSES_H_
#define MAI_NYAA_HIR_PASSES_H_

#include "base/base.h"
#include "glog/logging.h"
#include <unordered_map>
#include <type_traits>
#include <vector>
#include <map>


namespace mai {
namespace base {
class Arena;
} // namespace base
namespace nyaa {
namespace lir {
class MemoryOperand;
class RegisterOperand;
class FPRegisterOperand;
class Operand;
} // namespace lir
namespace hir {

class PassesManagement;
class FunctionPass;
class Function;
class BasicBlock;
class Phi;
class Value;
    
class Pass {
public:
    explicit Pass(const char *unique_id) : unique_id_(unique_id) {}
    virtual ~Pass() {}

    DEF_PTR_GETTER(const char, unique_id);
    
    bool Equals(const Pass *other) const { return unique_id_ == other->unique_id_; }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Pass);
private:
    const char *const unique_id_;
}; // class Pass
    
class FunctionPass : public Pass {
public:
    explicit FunctionPass(const char *unique_id) : Pass(unique_id) {}
    virtual ~FunctionPass() {}

    virtual int RunOnFunction(Function *input) = 0;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(FunctionPass);
}; // class HIRPass
    
class BasicBlockPass : public Pass {
public:
    explicit BasicBlockPass(const char *unique_id) : Pass(unique_id) {}
    virtual ~BasicBlockPass() {}
    
    virtual int RunOnBasicBlock(BasicBlock *input) = 0;

    DISALLOW_IMPLICIT_CONSTRUCTORS(BasicBlockPass);
}; // class BasicBlockPass
    
class PassManagement final {
public:
    PassManagement() {}
    ~PassManagement();
    
    template<class T> inline T *AddPass() { return DoAddPass<T>([] () { return new T(); }); }

    template<class T> inline T *AddPass(base::Arena *arena) {
        return DoAddPass<T>([arena]() { return new T(arena); });
    }

    template<class T> inline T *GetPass() const {
        if (std::is_base_of<FunctionPass, T>::value) {
            auto iter = function_passes_.find(&T::ID);
            DCHECK(iter != function_passes_.end());
            return down_cast<T>(iter->second);
        } else if (std::is_base_of<BasicBlockPass, T>::value) {
            auto iter = basic_block_passes_.find(&T::ID);
            DCHECK(iter != basic_block_passes_.end());
            return down_cast<T>(iter->second);
        }
        NOREACHED();
        return nullptr;
    }

    int RunOnFunction(Function *target) {
        int modified = 0;
        for (Pass *pass : function_pass_seq_) {
            modified += static_cast<FunctionPass *>(pass)->RunOnFunction(target);
        }
        return modified;
    }

    int RunOnBasicBlock(BasicBlock *block) {
        int modified = 0;
        for (Pass *pass : basic_block_pass_seq_) {
            modified += static_cast<BasicBlockPass *>(pass)->RunOnBasicBlock(block);
        }
        return modified;
    }

private:
    template<class T, class Ctor> inline T *DoAddPass(Ctor callback) {
        if (std::is_base_of<FunctionPass, T>::value) {
            DCHECK(function_passes_.find(&T::ID) == function_passes_.end());
            T *pass = callback();
            function_pass_seq_.push_back(pass);
            function_passes_[&T::ID] = pass;
            return pass;
        } else if (std::is_base_of<BasicBlockPass, T>::value) {
            DCHECK(basic_block_passes_.find(&T::ID) == basic_block_passes_.end());
            T *pass = callback();
            basic_block_pass_seq_.push_back(pass);
            basic_block_passes_[&T::ID] = pass;
            return pass;
        }
        NOREACHED();
        return nullptr;
    }
    
    std::map<const char *, Pass *> function_passes_;
    std::vector<Pass *> function_pass_seq_;
    
    std::map<const char *, Pass *> basic_block_passes_;
    std::vector<Pass *> basic_block_pass_seq_;
}; // class PassManagement
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Phi Node Elimination Pass
// Remove all phi nodes and replacement by copy node
////////////////////////////////////////////////////////////////////////////////////////////////////
class PhiEliminationPass final : public FunctionPass {
public:
    PhiEliminationPass() : FunctionPass(&ID) {}
    virtual ~PhiEliminationPass() override {}

    virtual int RunOnFunction(Function *input) override;
    
    static const char ID;
private:
    void EliminatePhiNode(Function *target, Phi *phi);
    
    int modified_ = 0;
}; // class PhiEliminationPass
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Linear Scan Register Allocation Pass
// Papers:
//     [1] http://www2.seas.gwu.edu/~hchoi/teaching/cs160d/linearscan.pdf
//     [2] http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.1.9128&rep=rep1&type=pdf
////////////////////////////////////////////////////////////////////////////////////////////////////
class LSRAPass final : public FunctionPass {
public:
    struct LiveInterval {
        int begin;
        int end;
    };
    
    LSRAPass(base::Arena *arena)
        : FunctionPass(&ID)
        , arena_(DCHECK_NOTNULL(arena)) {}
    virtual ~LSRAPass() override {}
    virtual int RunOnFunction(Function *input) override;

private:
    class RegisterSet;
    
    void PreparePass(Function *input);
    const lir::RegisterOperand *AllocateRegister();
    const lir::FPRegisterOperand *AllocateFPRegister();
    const lir::MemoryOperand *AllocateStackSlot();
    
    base::Arena *arena_;
    std::unordered_map<Value *, int> virtual_live_orders_;
    std::unordered_map<Value *, LiveInterval> virtual_live_intervals_;
    std::unordered_map<Value *, const lir::Operand *> virtual_to_physical_;
    int stack_alive_ = 0;
    
    static const char ID;
}; // class LSRAPass
    
} // namespace nyaa
    
} // namespace nyaa

} // namespace mai


#endif // MAI_NYAA_HIR_PASSES_H_
