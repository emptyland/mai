#include "nyaa/hir-passes.h"
#include "nyaa/hir-rewrite.h"
#include "nyaa/high-level-ir.h"
#include "nyaa/low-level-ir.h"
#include "nyaa/runtime.h"
#include <unordered_map>

namespace mai {
    
namespace nyaa {

namespace hir {

PassesManagement::~PassesManagement() {
    for (const auto &pair : function_passes_) {
        delete pair.second;
    }
    for (const auto &pair: basic_block_passes_) {
        delete pair.second;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Phi Node Elimination Pass
////////////////////////////////////////////////////////////////////////////////////////////////////
/*virtual*/ int PhiEliminationPass::RunOnFunction(Function *input) {
    modified_ = 0;
    for (auto bb : input->basic_blocks()) {
        for (Value *instr = bb->insts_begin(); instr != bb->insts_end(); instr = instr->next()) {
            if (instr->IsVoidTy()) {
                continue;
            }
            if (instr->IsPhi()) {
                Value *next = instr->next();
                EliminatePhiNode(input, Phi::Cast(instr));
                modified_++;
                instr = next;
            }
        }
    }
    return modified_;
}

void PhiEliminationPass::EliminatePhiNode(Function *target, Phi *phi) {
    Value *repl = target->Alloca(phi->type(), phi->line());
    for (auto path : phi->incoming()) {
        BasicBlock *source_block = path.incoming_bb;
        Copy *move = target->Copy(nullptr, repl, path.incoming_value, phi->line());
        if (Value *val = source_block->insts_last(); val != source_block->insts_end()) {
            while (val->IsTerminator()) {
                val = val->prev();
            }
            source_block->InsertValueAfter(val, move);
        } else {
            source_block->InsertTail(move);
        }
    }
    target->ReplaceAllUses(phi, repl);
    phi->RemoveFromOwns();
}
    
const char PhiEliminationPass::ID = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Virtual register live intervals analysis pass
////////////////////////////////////////////////////////////////////////////////////////////////////

/*virtual*/ int LiveIntervalAnalysisPass::RunOnFunction(Function *input) {
    int number = 0;
    for (auto param : input->parameters()) {
        owns_->virtual_live_orders_[param] = number;
    }
    number += 16;
    
    for (auto bb : input->basic_blocks()) {
        for (Value *instr = bb->insts_begin(); instr != bb->insts_end(); instr = instr->next()) {
            if (instr->IsVoidTy()) {
                continue;
            }
            auto iter = owns_->virtual_live_orders_.find(instr);
            if (iter == owns_->virtual_live_orders_.end()) {
                owns_->virtual_live_orders_[instr] = number;
            }
            number += 16;
        }
    }
    
    for (auto param : input->parameters()) {
        SetLiveInterval(param);
    }
    for (auto bb : input->basic_blocks()) {
        for (Value *instr = bb->insts_begin(); instr != bb->insts_end(); instr = instr->next()) {
            SetLiveInterval(instr);
        }
    }
    return 0;
}

void LiveIntervalAnalysisPass::SetLiveInterval(Value *instr) {
    if (instr->IsVoidTy()) {
        return;
    }
    auto iter = owns_->virtual_live_orders_.find(instr);
    DCHECK(iter != owns_->virtual_live_orders_.end());
    
    int begin = iter->second;
    int end = begin;
    for (Use *use = instr->uses_begin(); use != instr->uses_end(); use = use->next) {
        if (use->val->IsVoidTy()) {
            continue;
        }
        iter = owns_->virtual_live_orders_.find(use->val);
        DCHECK(iter != owns_->virtual_live_orders_.end());
        if (iter->second > end) {
            end = iter->second;
        }
    }
    owns_->virtual_live_intervals_[instr] = {begin, end};
}
    
const char LiveIntervalAnalysisPass::ID = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Linear Scan Register Allocation Pass
////////////////////////////////////////////////////////////////////////////////////////////////////

/*virtual*/ int LSRAPass::RunOnFunction(Function *input) {
//    for (auto param : input->parameters()) {
//        const lir::MemoryOperand *slot = AllocateStackSlot();
//        virtual_to_physical_[param] = slot;
//    }
    
    for (auto bb : input->basic_blocks()) {
        for (Value *instr = bb->insts_begin(); instr != bb->insts_end(); instr = instr->next()) {
            if (instr->IsVoidTy()) {
                continue;
            }
            
            // TODO:
        }
    }
    return 0;
}
    
const char LSRAPass::ID = 0;
    
} // namespace nyaa

} // namespace nyaa
    
} // namespace mai
