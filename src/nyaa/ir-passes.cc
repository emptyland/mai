#include "nyaa/ir-passes.h"
#include "nyaa/hir-rewrite.h"
#include "nyaa/high-level-ir.h"
#include "nyaa/low-level-ir.h"
#include "nyaa/runtime.h"
#include <unordered_map>

namespace mai {
    
namespace nyaa {

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
/*virtual*/ int PhiEliminationPass::RunOnFunction(hir::Function *input) {
    modified_ = 0;
    for (auto bb : input->basic_blocks()) {
        for (hir::Value *instr = bb->instrs_begin(); instr != bb->instrs_end(); instr = instr->next()) {
            if (instr->IsVoidTy()) {
                continue;
            }
            if (instr->IsPhi()) {
                hir::Value *next = instr->next();
                EliminatePhiNode(input, hir::Phi::Cast(instr));
                modified_++;
                instr = next;
            }
        }
    }
    return modified_;
}

void PhiEliminationPass::EliminatePhiNode(hir::Function *target, hir::Phi *phi) {
    hir::Value *repl = target->Alloca(phi->type(), phi->line());
    for (auto path : phi->incoming()) {
        hir::BasicBlock *source_block = path.incoming_bb;
        hir::Copy *move = target->Copy(nullptr, repl, path.incoming_value, phi->line());
        if (hir::Value *val = source_block->instrs_last(); val != source_block->instrs_end()) {
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

} // namespace nyaa
    
} // namespace mai
