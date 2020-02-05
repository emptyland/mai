#include "lang/bytecode.h"
#include "base/slice.h"

namespace mai {

namespace lang {

const BytecodeDesc kBytecodeDesc[kMax_Bytecodes] = {
#define DEFINE_BYTECODE_DESC(name, kind, ...) \
    {BytecodeTraits<k##name>::kId, BytecodeTraits<k##name>::kType, BytecodeTraits<k##name>::kName, \
    {__VA_ARGS__}},
   DECLARE_ALL_BYTECODE(DEFINE_BYTECODE_DESC)
#undef  DEFINE_BYTECODE_DESC
};


BytecodeInstruction BytecodeNode::To() const {
    switch (kind_) {
        case BytecodeType::N:
            return static_cast<BytecodeInstruction>(id_) << 24;

        case BytecodeType::A:
            DCHECK_GE(params_[0], 0);
            DCHECK_LT(params_[0], 0x1000000);
            return (static_cast<BytecodeInstruction>(id_) << 24) |
                   (static_cast<BytecodeInstruction>(params_[0]) & 0xffffff);

        case BytecodeType::FA:
            DCHECK_GE(params_[0], 0);
            DCHECK_LT(params_[0], 0x100);
            
            DCHECK_GE(params_[1], 0);
            DCHECK_LT(params_[1], 0x10000);
            return (static_cast<BytecodeInstruction>(id_) << 24) |
                   ((static_cast<BytecodeInstruction>(params_[0]) & 0xff) << 16) |
                   (static_cast<BytecodeInstruction>(params_[1]) & 0xffff);

        case BytecodeType::AB:
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
    BytecodeID id = static_cast<BytecodeID>((instr & kIDMask) >> 24);
    DCHECK_GE(id, 0);
    DCHECK_LT(id, kMax_Bytecodes);
    
    int a = 0, b = 0;
    const BytecodeDesc &desc = kBytecodeDesc[id];
    switch (desc.kind) {
        case BytecodeType::N:
            break;
            
        case BytecodeType::A:
            a = static_cast<int>(instr & kAOfAMask);
            break;
            
        case BytecodeType::FA:
            a = static_cast<int>((instr & kFOfFAMask) >> 16);
            b = static_cast<int>(instr & kAOfFAMask);
            break;
            
        case BytecodeType::AB:
            a = static_cast<int>((instr & kAOfABMask) >> 12);
            b = static_cast<int>(instr & kBOfABMask);
            break;
            
        default:
            NOREACHED();
            return nullptr;
    }
    return new (arena) BytecodeNode(desc.id, desc.kind, a, b, 0, 0);
}

void BytecodeNode::Print(std::string *output) const {
    const BytecodeDesc desc = kBytecodeDesc[id_];
    
    output->append(desc.name);
    switch (kind_) {
        case BytecodeType::N:
            break;
        case BytecodeType::A:
            output->append(" ");
            PrintParam(output, desc.params[0], param(0));
            break;
        case BytecodeType::FA:
        case BytecodeType::AB:
            output->append(" ");
            PrintParam(output, desc.params[0], param(0));
            output->append(", ");
            PrintParam(output, desc.params[1], param(1));
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeNode::PrintSimple(std::string *output) const {
    const BytecodeDesc desc = kBytecodeDesc[id_];
    
    output->append(desc.name);
    switch (kind_) {
        case BytecodeType::N:
            break;
        case BytecodeType::A:
            output->append(base::Sprintf(" %d", param(0)));
            break;
        case BytecodeType::FA:
        case BytecodeType::AB:
            output->append(base::Sprintf(" %d, %d", param(0), param(1)));
            break;
        default:
            NOREACHED();
            break;
    }
}

void BytecodeNode::PrintParam(std::string *output, BytecodeParam::Kind kind, int param) const {
    switch (kind) {
        case BytecodeParam::kStackOffset:
            output->append(base::Sprintf("[BP-%d]", param));
            break;
        case BytecodeParam::kConstOffset:
            output->append(base::Sprintf("[KP+%d]", param));
            break;
        case BytecodeParam::kGlobalOffset:
            output->append(base::Sprintf("[GS+%d]", param));
            break;
        case BytecodeParam::kCapturedVarIndex:
            output->append(base::Sprintf("UP[%d]", param));
            break;
        case BytecodeParam::kAddressOffset:
            output->append(base::Sprintf("@%x", param));
            break;
        case BytecodeParam::kImmediate:
            output->append(base::Sprintf("#%d", param));
            break;
        case BytecodeParam::kIndex:
            output->append(base::Sprintf("[%d]", param));
            break;
        case BytecodeParam::kCode:
            output->append(base::Sprintf("%d", param));
            break;
        case BytecodeParam::kNone:
        default:
            NOREACHED();
            break;
    }
}

/*virtual*/ AbstractBytecodeEmitter::~AbstractBytecodeEmitter() {
}

} // namespace lang

} // namespace mai
