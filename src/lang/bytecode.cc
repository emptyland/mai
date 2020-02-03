#include "lang/bytecode.h"

namespace mai {

namespace lang {

struct BytecodeDesc {
    BytecodeID id;
    BytecodeKind kind;
    const char *name;
}; // struct BytecodeDesc

const BytecodeDesc kBytecodeDesc[kMax_Bytecodes] = {
#define DEFINE_BYTECODE_DESC(name, ...) \
    {BytecodeTraits<k##name>::kId, BytecodeTraits<k##name>::kType, BytecodeTraits<k##name>::kName},
   DECLARE_ALL_BYTECODE(DEFINE_BYTECODE_DESC)
#undef  DEFINE_BYTECODE_DESC
};


BytecodeInstruction BytecodeNode::To() const {
    switch (kind_) {
        case kBytecode_TypeN:
            return static_cast<BytecodeInstruction>(id_) << 24;

        case kBytecode_TypeA:
            DCHECK_GE(params_[0], 0);
            DCHECK_LT(params_[0], 0x1000000);
            return (static_cast<BytecodeInstruction>(id_) << 24) |
                   (static_cast<BytecodeInstruction>(params_[0]) & 0xffffff);

        case kBytecode_TypeFA:
            DCHECK_GE(params_[0], 0);
            DCHECK_LT(params_[0], 0x100);
            
            DCHECK_GE(params_[1], 0);
            DCHECK_LT(params_[1], 0x10000);
            return (static_cast<BytecodeInstruction>(id_) << 24) |
                   ((static_cast<BytecodeInstruction>(params_[0]) & 0xff) << 16) |
                   (static_cast<BytecodeInstruction>(params_[1]) & 0xffff);

        case kBytecode_TypeAB:
            DCHECK_GE(params_[0], 0);
            DCHECK_LT(params_[0], 0x1000);
            
            DCHECK_GE(params_[1], 0);
            DCHECK_LT(params_[1], 0x1000);
            return (static_cast<BytecodeInstruction>(id_) << 24) |
                   ((static_cast<BytecodeInstruction>(params_[0]) & 0xfff) << 12) |
                   (static_cast<BytecodeInstruction>(params_[1]) & 0xfff);

        default:
            NOREACHED();
            break;
    }
    return 0;
}

/*static*/ BytecodeNode *BytecodeNode::From(base::Arena *arena, BytecodeInstruction instr) {
    BytecodeID id = static_cast<BytecodeID>((instr & 0xff000000) >> 24);
    DCHECK_GE(id, 0);
    DCHECK_LT(id, kMax_Bytecodes);
    
    int a = 0, b = 0;
    const BytecodeDesc &desc = kBytecodeDesc[id];
    switch (desc.kind) {
        case kBytecode_TypeN:
            break;
            
        case kBytecode_TypeA:
            a = static_cast<int>(instr & 0x00ffffff);
            break;
            
        case kBytecode_TypeFA:
            a = static_cast<int>((instr & 0x00ff0000) >> 16);
            b = static_cast<int>(instr & 0x0000ffff);
            break;
            
        case kBytecode_TypeAB:
            a = static_cast<int>((instr & 0x00fff000) >> 12);
            b = static_cast<int>(instr & 0x00000fff);
            break;
            
        default:
            NOREACHED();
            return nullptr;
    }
    return new (arena) BytecodeNode(desc.id, desc.kind, a, b, 0, 0);
}

} // namespace lang

} // namespace mai
