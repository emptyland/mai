#ifndef MAI_NYAA_IR_PASSES_H_
#define MAI_NYAA_IR_PASSES_H_

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
class Function;
class BasicBlock;
class Phi;
class Value;
} // namespace hir

class PassesManagement;
class FunctionPass;
    
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
    explicit FunctionPass(const char *unique_id, PassesManagement *owns)
        : Pass(unique_id)
        , owns_(DCHECK_NOTNULL(owns)) {}
    virtual ~FunctionPass() {}

    virtual int RunOnFunction(hir::Function *input) = 0;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(FunctionPass);
protected:
    PassesManagement *owns_;
}; // class HIRPass
    
class BasicBlockPass : public Pass {
public:
    explicit BasicBlockPass(const char *unique_id) : Pass(unique_id) {}
    virtual ~BasicBlockPass() {}
    
    virtual int RunOnBasicBlock(hir::BasicBlock *input) = 0;

    DISALLOW_IMPLICIT_CONSTRUCTORS(BasicBlockPass);
}; // class BasicBlockPass
    
struct LiveInterval {
    int begin;
    int end;
};
    
class PassesManagement final {
public:
    PassesManagement() {}
    ~PassesManagement();
    
    template<class T> inline T *AddPass() { return DoAddPass<T>([this] () { return new T(this); }); }

    template<class T> inline T *AddPass(base::Arena *arena) {
        return DoAddPass<T>([this, arena]() { return new T(this, arena); });
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

    int RunOnFunction(hir::Function *target) {
        int modified = 0;
        for (Pass *pass : function_pass_seq_) {
            modified += static_cast<FunctionPass *>(pass)->RunOnFunction(target);
        }
        return modified;
    }

    int RunOnBasicBlock(hir::BasicBlock *block) {
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
    PhiEliminationPass(PassesManagement *owns) : FunctionPass(&ID, owns) {}
    virtual ~PhiEliminationPass() override {}

    virtual int RunOnFunction(hir::Function *input) override;
    
    static const char ID;
private:
    void EliminatePhiNode(hir::Function *target, hir::Phi *phi);
    
    int modified_ = 0;
}; // class PhiEliminationPass
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Linear Scan Register Allocation Pass
// Papers:
//     [1] http://www2.seas.gwu.edu/~hchoi/teaching/cs160d/linearscan.pdf
//     [2] http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.1.9128&rep=rep1&type=pdf
////////////////////////////////////////////////////////////////////////////////////////////////////
    
} // namespace nyaa

} // namespace mai


#endif // MAI_NYAA_IR_PASSES_H_
