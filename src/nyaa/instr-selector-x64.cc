#include "nyaa/low-level-ir.h"
#include "nyaa/high-level-ir.h"
#include "base/arena.h"
#include "glog/logging.h"
#include <map>
#include <numeric>

namespace mai {

namespace nyaa {

class X64InstrSelector final {
public:
    X64InstrSelector(hir::Function *func, base::Arena *arena)
        : arena_(DCHECK_NOTNULL(arena))
        , func_(DCHECK_NOTNULL(func))
        , target_(lir::Function::New(arena)) {
    }
    
    void Run() {
        //const int kVMBP = lir::Architecture::kVMBPCode;
        for (size_t i = 0; i < func_->parameters().size(); ++i) {
            slots_[func_->parameters()[i]] =
                lir::UnallocatedOperand::New(static_cast<int>(i),
                                             lir::UnallocatedOperand::kMustHasSlot,
                                             arena_);
        }
        for (auto bb : func_->basic_blocks()) {
            GenerateBasicBlock(bb);
        }
    }
    
    struct TwoAddress {
        const lir::Operand *rv;
        const lir::Operand *rhs;
    };
private:
    TwoAddress ToTwoAddress(lir::Block *block, hir::BinaryInst *bin) {
        const lir::Operand *lhs = GetOrNew(bin->lhs()), *rhs = GetOrNew(bin->rhs());
        const lir::Operand *rv = lir::UnallocatedOperand::New(bin->index(),
                                                              lir::UnallocatedOperand::kNone,
                                                              arena_);
        lir::Instruction *instr = lir::Instruction::New(lir::kMove, rv, lhs, bin->line(), arena_);
        block->Add(instr);
        return {rv, rhs};
    }
    
    void GenerateBasicBlock(hir::BasicBlock *bb) {
        lir::Block *block = nullptr;
        auto iter = blocks_.find(bb);
        if (iter == blocks_.end()) {
            block = target_->NewBlock(bb->label());
            blocks_[bb] = block;
        } else {
            block = iter->second;
            target_->AddBlock(block);
        }
        
        for (hir::Value *instr = bb->instrs_begin(); instr != bb->instrs_end();
             instr = instr->next()) {
            GenerateInstr(block, instr);
        }
    }
    
    void GenerateInstr(lir::Block *block, hir::Value *instr);
    
    const lir::Operand *GetOrNew(hir::Value *val) {
        auto iter = slots_.find(val);
        if (iter != slots_.end()) {
            return iter->second;
        }
        const lir::Operand *operand = nullptr;
        auto konst = hir::Constant::Cast(val);
        switch (konst->type()) {
            case hir::Type::kInt:
                if (konst->i62_val() >= std::numeric_limits<int32_t>::min() &&
                    konst->i62_val() <= std::numeric_limits<int32_t>::max()) {
                    operand = new (arena_) lir::ImmediateOperand(konst->i62_val(), 32);
                    slots_[val] = operand;
                }
                break;
            case hir::Type::kFloat:
                // TODO:
                break;
            default:
                NOREACHED();
                break;
        }
        return operand;
    }
    
    const lir::Block *GetOrNew(hir::BasicBlock *basic_block) {
        auto iter = blocks_.find(basic_block);
        lir::Block *target = nullptr;
        if (iter == blocks_.end()) {
            target = lir::Block::New(basic_block->label(), arena_);
            blocks_[basic_block] = target;
        } else {
            target = iter->second;
        }
        return target;
    }
    
    lir::Instruction *MakeBranch(hir::Branch *br, int subcode) {
        lir::Instruction *instr = lir::Instruction::New(lir::kBranch, subcode, nullptr, 2,
                                                        br->line(), arena_);
        switch (br->pref()) {
            case hir::Branch::kNone:
                instr->set_input(1, GetOrNew(br->if_false()));
                instr->set_input(0, GetOrNew(br->if_true()));
                instr->set_hint(lir::kNotSpecial);
                break;
            case hir::Branch::kLikelyTrue:
                instr->set_input(0, GetOrNew(br->if_true()));
                instr->set_input(1, GetOrNew(br->if_false()));
                instr->set_hint(lir::kLikely0);
                break;
            case hir::Branch::kLikelyFalse:
                instr->set_input(1, GetOrNew(br->if_false()));
                instr->set_input(0, GetOrNew(br->if_true()));
                instr->set_hint(lir::kLikely1);
                break;
            default:
                NOREACHED();
                break;
        }
        return instr;
    }
    
    lir::IRComparator ToComparator(hir::Compare::Op cc) {
        switch (cc) {
            case hir::Compare::kEQ:
                return lir::kEqual;
            case hir::Compare::kNE:
                return lir::kNotEqual;
            case hir::Compare::kLT:
                return lir::kLessThan;
            case hir::Compare::kLE:
                return lir::kLessThanOrEqual;
            case hir::Compare::kGT:
                return lir::kGreaterThan;
            case hir::Compare::kGE:
                return lir::kGreaterThanOrEqual;
            default:
                break;
        }
        NOREACHED();
        return lir::kNotOverflow;
    }

    base::Arena *arena_;
    hir::Function *func_;
    lir::Function *target_;
    std::map<hir::BasicBlock *, lir::Block *> blocks_;
    std::map<hir::Value *, const lir::Operand *> slots_;
}; // class LIRCodeGenerator
    
void X64InstrSelector::GenerateInstr(lir::Block *block, hir::Value *val) {
    lir::Instruction *instr = nullptr;
    switch (val->kind()) {
        case hir::Value::kNoCondBranch: {
            auto br = hir::NoCondBranch::Cast(val);
            const lir::Block *target = GetOrNew(br->target());
            instr = lir::Instruction::New(lir::kJump, target, val->line(), arena_);
            block->Add(instr);
        } break;
            
        case hir::Value::kBranch: {
            hir::Branch *br = hir::Branch::Cast(val);
            hir::Value *cond = br->cond();
            if (cond->IsICmp()) {
                hir::ICmp *cmp = hir::ICmp::Cast(cond);
                instr = MakeBranch(br, ToComparator(cmp->op_code()));
                block->Add(instr);
            } else {
                DCHECK(cond->IsIntTy());
                const lir::Operand *slot = GetOrNew(cond);
                const lir::Operand *zero = new (arena_) lir::ImmediateOperand(0, 32);
                instr = lir::Instruction::New(lir::kCmp, nullptr, slot, zero, cond->line(), arena_);
                block->Add(instr);
                instr = MakeBranch(br, lir::kEqual);
                block->Add(instr);
            }
        } break;

        case hir::Value::kIAdd: {
            TwoAddress two = ToTwoAddress(block, static_cast<hir::BinaryInst *>(val));
            instr = lir::Instruction::New(lir::kAdd, two.rv, two.rhs, val->line(), arena_);
            block->Add(instr);
            slots_[val] = two.rv;
        } break;
            
        case hir::Value::kISub: {
            TwoAddress two = ToTwoAddress(block, static_cast<hir::BinaryInst *>(val));
            instr = lir::Instruction::New(lir::kSub, two.rv, two.rhs, val->line(), arena_);
            block->Add(instr);
            slots_[val] = two.rv;
        } break;
            
        case hir::Value::kIMul: {
            TwoAddress two = ToTwoAddress(block, static_cast<hir::BinaryInst *>(val));
            instr = lir::Instruction::New(lir::kMul, two.rv, two.rhs, val->line(), arena_);
            block->Add(instr);
            slots_[val] = two.rv;
        } break;
            
        case hir::Value::kIDiv:
            TODO();
            break;
            
        case hir::Value::kIMod:
            TODO();
            break;
            
        case hir::Value::kICmp: {
            hir::ICmp *bin = hir::ICmp::Cast(val);
            const lir::Operand *lhs = GetOrNew(bin->lhs()), *rhs = GetOrNew(bin->rhs());
            const lir::Operand *rv = lir::UnallocatedOperand::New(bin->index(),
                                                                  lir::UnallocatedOperand::kNone,
                                                                  arena_);
            instr = lir::Instruction::New(lir::kCmp, rv, lhs, rhs, val->line(), arena_);
            block->Add(instr);
            slots_[val] = rv;
        } break;

        case hir::Value::kConstant:
        default:
            NOREACHED();
            break;
    }
    
}

} // namespace nyaa

} // namespace mai
