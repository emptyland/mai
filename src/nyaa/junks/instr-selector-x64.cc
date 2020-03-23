#include "nyaa/low-level-ir.h"
#include "nyaa/high-level-ir.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/object-factory.h"
#include "base/arena.h"
#include "glog/logging.h"
#include <unordered_map>
#include <numeric>

namespace mai {

namespace nyaa {
    
namespace {

class ConstantPoolBuiler {
public:
    ConstantPoolBuiler(NyaaCore *N) : N_(DCHECK_NOTNULL(N)) {}
    ~ConstantPoolBuiler() {}
    
    int32_t GetOrNewI64(int64_t val) {
        if (NySmi::Contain(val)) {
            return GetOrNew(NySmi::New(val));
        } else {
            return GetOrNew(NyInt::NewI64(val, N_->factory()));
        }
    }

    int32_t GetOrNewInt(NyInt *val) { return GetOrNew(val); }

    int32_t GetOrNewF64(f64_t val) { return GetOrNew(N_->factory()->NewFloat64(val)); }
    
    int32_t GetOrNewStr(NyString *val) { return GetOrNew(val); }
    
    int32_t GetOrNewMap(NyMap *val) { return GetOrNew(val); }

    int32_t GetOrNew(Object *val) {
        auto iter = keys_.find(val);
        if (iter != keys_.end()) {
            return Insert(val);
        } else {
            return iter->second;
        }
    }
    
    NyArray *Build() {
        if (pool_.empty()) {
            return nullptr;
        }
        NyArray *pool = N_->factory()->NewArray(pool_.size());
        for (auto val : pool_) {
            pool = pool->Add(val, N_);
        }
        return pool;
    }
private:
    int32_t Insert(Object *val) {
        int32_t pos = static_cast<int32_t>(pool_.size());
        pool_.push_back(val);
        keys_[val] = pos;
        return pos;
    }
    
    struct EqualTo : public std::binary_function<Object *, Object *, bool> {
        bool operator () (const Object *lhs, const Object *rhs) const;
    }; // struct EqualTo
    struct Hash : public std::unary_function<Object *, size_t> {
        size_t operator () (const Object *key) const;
    }; // struct Hash
    
    NyaaCore *const N_;
    std::unordered_map<Object *, int32_t, Hash, EqualTo> keys_;
    std::vector<Object *> pool_;
}; // class ConstPoolBuiler
    
bool ConstantPoolBuiler::EqualTo::operator () (const Object *lhs, const Object *rhs) const {
    if (lhs->GetType() != rhs->GetType()) {
        return false;
    }
    switch (lhs->GetType()) {
        case kTypeSmi:
            return lhs->ToSmi() == rhs->ToSmi();
        case kTypeInt:
            return NyInt::Compare(NyInt::Cast(lhs), NyInt::Cast(rhs)) == 0;
        case kTypeFloat64:
            return NyFloat64::Cast(lhs)->value() == NyFloat64::Cast(rhs)->value();
        case kTypeString:
            return NyString::Cast(lhs)->Compare(NyString::Cast(rhs)) == 0;
        case kTypeMap:
            return lhs == rhs;
        default:
            break;
    }
    NOREACHED();
    return false;
}
    
size_t ConstantPoolBuiler::Hash::operator () (const Object *key) const {
    switch (key->GetType()) {
        case kTypeSmi:
            return key->ToSmi();
        case kTypeFloat64:
            return NyFloat64::Cast(key)->HashVal();
        case kTypeInt:
            return NyInt::Cast(key)->HashVal();
        case kTypeString:
            return NyString::Cast(key)->hash_val();
        case kTypeMap:
            return reinterpret_cast<size_t>(key);
        default:
            break;
    }
    NOREACHED();
    return 0;
}
    
} // namespace

class X64InstrSelector final {
public:
    X64InstrSelector(NyaaCore *N, hir::Function *func, base::Arena *arena)
        : N_(DCHECK_NOTNULL(N))
        , kpool_(N)
        , arena_(DCHECK_NOTNULL(arena))
        , func_(DCHECK_NOTNULL(func))
        , target_(lir::Function::New(arena)) {
    }
    
    DEF_PTR_GETTER(lir::Function, target);
    NyArray *BuildConstantPool() { return kpool_.Build(); }
    
    void Run() {
        for (size_t i = 0; i < func_->parameters().size(); ++i) {
            const lir::Operand *param =
                lir::UnallocatedOperand::New(static_cast<int>(i),
                                             lir::UnallocatedOperand::kMustHasSlot, arena_);
            slots_[func_->parameters()[i]] = param;
            target_->AddParameter(param);
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
        const lir::Operand *lhs = GetOrNew(block, bin->lhs()), *rhs = GetOrNew(block, bin->rhs());
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
    
    const lir::Operand *GetOrNew(lir::Block *block, hir::Value *val);
    
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
    
    lir::Instruction *MakeBranch(hir::Branch *br, int subcode);
    
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

    NyaaCore *const N_;
    base::Arena *arena_;
    hir::Function *func_;
    lir::Function *target_;
    ConstantPoolBuiler kpool_;
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
                const lir::Operand *slot = GetOrNew(block, cond);
                const lir::Operand *zero = new (arena_) lir::ImmediateOperand(0, 32);
                instr = lir::Instruction::New(lir::kCmp, nullptr, slot, zero, cond->line(), arena_);
                block->Add(instr);
                instr = MakeBranch(br, lir::kEqual);
                block->Add(instr);
            }
        } break;
            
        case hir::Value::kRet: {
            hir::Ret *ret = hir::Ret::Cast(val);
            lir::Operand *base = lir::UnallocatedOperand::New(ret->index(),
                                                              static_cast<int>(ret->ret_vals_size()),
                                                              lir::UnallocatedOperand::kMustHasSlot,
                                                              arena_);
            instr = lir::Instruction::New(lir::kMoveBaseOfStack, 0/*subcode*/, base/*output*/,
                                          static_cast<int>(ret->ret_vals_size()),
                                          ret->line(), arena_);
            for (size_t i = 0; i < ret->ret_vals().size(); ++i) {
                instr->set_input(static_cast<int>(i), GetOrNew(block, ret->ret_val(i)));
            }
            block->Add(instr);
            
            instr = lir::Instruction::New(lir::kSaveCallerRegisters, nullptr, ret->line(), arena_);
            block->Add(instr);
            
            instr = lir::Instruction::New(lir::kRestoreCallerRegisters, nullptr, ret->line(), arena_);
            block->Add(instr);
        } break;

        case hir::Value::kIAdd: {
            TwoAddress two = ToTwoAddress(block, static_cast<hir::BinaryInst *>(val));
            instr = lir::Instruction::New(lir::kSmiAdd, two.rv, two.rhs, val->line(), arena_);
            block->Add(instr);
            slots_[val] = two.rv;
        } break;
            
        case hir::Value::kISub: {
            TwoAddress two = ToTwoAddress(block, static_cast<hir::BinaryInst *>(val));
            instr = lir::Instruction::New(lir::kSmiSub, two.rv, two.rhs, val->line(), arena_);
            block->Add(instr);
            slots_[val] = two.rv;
        } break;

        case hir::Value::kIMul: {
            TwoAddress two = ToTwoAddress(block, static_cast<hir::BinaryInst *>(val));
            instr = lir::Instruction::New(lir::kSmiMul, two.rv, two.rhs, val->line(), arena_);
            block->Add(instr);
            slots_[val] = two.rv;
        } break;

        case hir::Value::kIMod:
        case hir::Value::kIDiv: {
            hir::IDiv *bin = hir::IDiv::Cast(val);
            const lir::Operand *lhs = GetOrNew(block, bin->lhs()), *rhs = GetOrNew(block, bin->rhs());
            const lir::Operand *rv =
                lir::UnallocatedOperand::New(bin->index(), lir::UnallocatedOperand::kMustHasRegister,
                                             arena_);
            const lir::Operand *temp =
                lir::UnallocatedOperand::New(bin->index(), lir::UnallocatedOperand::kMustHasRegister,
                                             arena_);
            instr = lir::Instruction::New(lir::kMove, rv, lhs, bin->line(), arena_);
            block->Add(instr);
            if (val->IsIMod()) {
                instr = lir::Instruction::New(lir::kSmiMod, rv, rhs, bin->line(), arena_);
            } else {
                instr = lir::Instruction::New(lir::kSmiDiv, rv, rhs, bin->line(), arena_);
            }
            instr->set_temp(temp);
            block->Add(instr);
            slots_[val] = rv;
        } break;
            
        case hir::Value::kICmp: {
            hir::ICmp *bin = hir::ICmp::Cast(val);
            const lir::Operand *lhs = GetOrNew(block, bin->lhs()), *rhs = GetOrNew(block, bin->rhs());
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
    
const lir::Operand *X64InstrSelector::GetOrNew(lir::Block *block, hir::Value *val) {
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
            } else {
                int off = kpool_.GetOrNewI64(konst->i62_val());
                operand = lir::UnallocatedOperand::New(0, lir::UnallocatedOperand::kNone, arena_);
                lir::Instruction *instr = lir::Instruction::New(lir::kConstant, off, operand, 0,
                                                                val->line(), arena_);
                block->Add(instr);
            }
            break;
        case hir::Type::kFloat: {
            int off = kpool_.GetOrNewF64(konst->f64_val());
            operand = lir::UnallocatedOperand::New(0, lir::UnallocatedOperand::kNone, arena_);
            lir::Instruction *instr = lir::Instruction::New(lir::kConstant, off, operand, 0,
                                                            val->line(), arena_);
            block->Add(instr);
        } break;
        case hir::Type::kString:
        case hir::Type::kLong:
        case hir::Type::kMap:
        case hir::Type::kArray:
        case hir::Type::kObject: {
            int off = kpool_.GetOrNew(konst->obj_val());
            operand = lir::UnallocatedOperand::New(0, lir::UnallocatedOperand::kNone, arena_);
            lir::Instruction *instr = lir::Instruction::New(lir::kConstant, off, operand, 0,
                                                            val->line(), arena_);
            block->Add(instr);
        } break;
        default:
            NOREACHED();
            break;
    }
    slots_[val] = operand;
    return operand;
}

lir::Instruction *X64InstrSelector::MakeBranch(hir::Branch *br, int subcode) {
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
    
Error Arch_GenerateLIR(hir::Function *func, Handle<NyArray> *kpool, lir::Function **rv,
                       base::Arena *arena, NyaaCore *core) {
    X64InstrSelector selector(core, func, arena);
    selector.Run();
    *rv = selector.target();
    *kpool = selector.BuildConstantPool();
    return Error::OK();
}

} // namespace nyaa

} // namespace mai
