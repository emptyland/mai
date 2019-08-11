#include "nyaa/low-level-ir.h"
#include "nyaa/runtime.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/thread.h"
#include "nyaa/function.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/object-factory.h"
#include "asm/x64/asm-x64.h"
#include "asm/utils.h"
#include "base/base.h"
#include "mai-lang/nyaa.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {
    
using namespace x64;

#define __ masm_.
    
class X64OperandConversion {
public:
    X64OperandConversion(const lir::Instruction *instr) : instr_(DCHECK_NOTNULL(instr)) {}
    
    bool IsMemoryOutput() const {
        return instr_->output()->kind() == lir::Operand::kMemory;
    }
    bool IsRegisterOutput() const {
        return instr_->output()->kind() == lir::Operand::kRegister;
    }
    bool IsXMMRegisterOutput() const {
        return instr_->output()->kind() == lir::Operand::kFPRegister;
    }

    Operand MemoryOutput() const { return ToMemory(instr_->output()); }
    Register RegisterOutput() const { return ToRegister(instr_->output()); }
    XMMRegister XMMRegisterOutput() const { return ToXMMRegister(instr_->output()); }
    
    bool IsMemoryInput(int i) const {
        return instr_->input(i)->kind() == lir::Operand::kMemory;
    }
    bool IsRegisterInput(int i) const {
        return instr_->input(i)->kind() == lir::Operand::kRegister;
    }
    bool IsXMMRegisterInput(int i) const {
        return instr_->input(i)->kind() == lir::Operand::kFPRegister;
    }
    bool IsImmediateInput(int i) const {
        return instr_->input(i)->kind() == lir::Operand::kImmediate;
    }
    
    Operand MemoryInput(int i) const { return ToMemory(instr_->input(i)); }
    Register RegisterInput(int i) const { return ToRegister(instr_->input(i)); }
    XMMRegister XMMRegisterInput(int i) const { return ToXMMRegister(instr_->input(i)); }
    
    lir::IRCode Code() const { return instr_->op(); }
    
    int NumberOfInputs() const { return instr_->n_inputs(); }

    const lir::ImmediateOperand *ImmediateInput(int i) const {
        return ToImmediate(instr_->input(i));
    }

    static Operand ToMemory(const lir::Operand *operand) {
        const lir::MemoryOperand *mem = lir::MemoryOperand::Ensure(operand);
        Register base(mem->base());
        return Operand(base, mem->offset());
    }

    static Register ToRegister(const lir::Operand *reg) {
        return Register(lir::RegisterOperand::Ensure(reg)->code());
    }

    static XMMRegister ToXMMRegister(const lir::Operand *reg) {
        return XMMRegister(lir::FPRegisterOperand::Ensure(reg)->code());
    }
    
    static const lir::ImmediateOperand *ToImmediate(const lir::Operand *imm) {
        return lir::ImmediateOperand::Ensure(imm);
    }
    
    const lir::Instruction *ir() const { return instr_; }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(X64OperandConversion);
private:
    const lir::Instruction *const instr_;
}; // class X64OperandConversion

class LIRSourceLineScope final {
public:
    LIRSourceLineScope(int line) {}
}; // class LIRSourceLineScope
    
class LIRCodeGenerator final {
public:
    LIRCodeGenerator(const lir::InstructionBundle *instrs)
        : instrs_(DCHECK_NOTNULL(instrs)) {}
    
    void Run() {
        for (const auto &block : instrs_->blocks()) {
            for (size_t i = 0; i < block->instrs_size(); ++i) {
                X64OperandConversion instr(block->instr(i));
                AssembleInstr(instr);
            }
        }
    }
    
    void AssembleInstr(const X64OperandConversion &instr);
    
    //std::string code() const { return masm_.buf(); }
    
    std::string FullAlignmentCode() {
        size_t adjust_size = RoundUp(masm_.buf().size(), kPointerSize) - masm_.buf().size();
        for (size_t i = 0; i < adjust_size; ++i) {
            __ int3();
        }
        __ nop(4);
        std::string code(masm_.buf());
        DCHECK_EQ(0, code.size() % 4);
        return code;
    }

    const std::vector<int32_t> &source_lines() const { return source_lines_; }

private:
    size_t SaveCallerRegisters(const Register *gp_regs, size_t n_gp_regs,
                               const XMMRegister *fp_regs, size_t n_fp_regs) {
        size_t bytes = 0;
        for (int i = 0; i < n_gp_regs; ++i) {
            __ pushq(gp_regs[i]);
            bytes += kRegisterSize;
        }
        if (n_fp_regs > 0) {
            __ subq(rsp, static_cast<int>(n_fp_regs * sizeof(double)));
        }
        for (int i = 0; i < n_fp_regs; ++i) {
            __ movsd(Operand(rsp, i * sizeof(double)), fp_regs[i]);
            bytes += sizeof(double);
        }
        return bytes;
    }
    
    size_t RestoreCallerRegisters(const Register *gp_regs, size_t n_gp_regs,
                                  const XMMRegister *fp_regs, size_t n_fp_regs) {
        size_t bytes = 0;
        for (int i = 0; i < n_fp_regs; ++i) {
            __ movsd(fp_regs[i], Operand(rsp, i * sizeof(double)));
            bytes += sizeof(double);
        }
        if (n_fp_regs > 0) {
            __ addq(rsp, static_cast<int>(n_fp_regs * sizeof(double)));
        }
        for (int i = static_cast<int>(n_gp_regs - 1); i >= 0; --i) {
            __ popq(gp_regs[i]);
            bytes += kRegisterSize;
        }
        return bytes;
    }

    const lir::InstructionBundle *instrs_;
    Assembler masm_;
    std::vector<int32_t> source_lines_;
}; // class LIRCodeGenerator
    
#define EMIT_MOVE() \
do { \
    if (instr.IsMemoryOutput()) { \
        if (instr.IsRegisterInput(0)) { \
            __ movq(instr.MemoryOutput(), instr.RegisterInput(0)); \
        } else if (instr.IsMemoryInput(0)) { \
            __ movq(Runtime::kScratch, instr.RegisterInput(0)); \
            __ movq(instr.MemoryOutput(), Runtime::kScratch); \
        } else if (instr.IsImmediateInput(0)) { \
            __ movq(Runtime::kScratch, instr.ImmediateInput(0)->i64_value()); \
            __ movq(instr.MemoryOutput(), Runtime::kScratch); \
        } else { \
            NOREACHED(); \
        } \
    } else if (instr.IsRegisterOutput()) { \
        if (instr.IsRegisterInput(0)) { \
            __ movq(instr.RegisterOutput(), instr.RegisterInput(0)); \
        } else if (instr.IsMemoryInput(0)) { \
            __ movq(instr.RegisterOutput(), instr.MemoryInput(0)); \
        } else if (instr.IsImmediateInput(0)) { \
            __ movq(instr.RegisterOutput(), instr.ImmediateInput(0)->i64_value()); \
        } else { \
            NOREACHED(); \
        } \
    } else { \
        NOREACHED(); \
    } \
} while (0)
    
#define EMIT_ARITH(asm_instr) \
do { \
    if (instr.IsMemoryOutput()) { \
        if (instr.IsRegisterInput(1)) { \
            __ asm_instr (instr.MemoryOutput(), instr.RegisterInput(1)); \
        } else if (instr.IsMemoryInput(1)) { \
            __ movq(Runtime::kScratch, instr.MemoryInput(1)); \
            __ asm_instr (instr.MemoryOutput(), Runtime::kScratch); \
        } else { \
            DCHECK(instr.IsImmediateInput(1)); \
            __ asm_instr (instr.MemoryOutput(), instr.ImmediateInput(1)->i32_value()); \
        } \
    } else { \
        DCHECK(instr.IsRegisterOutput()); \
        if (instr.IsRegisterInput(1)) { \
            __ asm_instr (instr.RegisterOutput(), instr.RegisterInput(1)); \
        } else if (instr.IsMemoryInput(1)) { \
            __ asm_instr (instr.RegisterOutput(), instr.RegisterInput(1)); \
        } else { \
            DCHECK(instr.IsImmediateInput(1)); \
            __ asm_instr (instr.RegisterOutput(), instr.ImmediateInput(1)->i32_value()); \
        } \
    } \
} while(0)

void LIRCodeGenerator::AssembleInstr(const X64OperandConversion &instr) {
    LIRSourceLineScope scope(instr.ir()->line());

    switch (instr.Code()) {
        case lir::kInboxSmi:
            if (instr.IsMemoryOutput()) {
                if (instr.IsRegisterInput(0)) {
                    __ movq(Runtime::kScratch, instr.RegisterInput(0));
                    __ shlq(Runtime::kScratch, 2);
                    __ orq(Runtime::kScratch, 0x1);
                    __ movq(instr.MemoryOutput(), Runtime::kScratch);
                } else if (instr.IsMemoryInput(0)) {
                    __ movq(Runtime::kScratch, instr.MemoryInput(0));
                    __ shlq(Runtime::kScratch, 2);
                    __ orq(Runtime::kScratch, 0x1);
                    __ movq(instr.MemoryOutput(), Runtime::kScratch);
                } else {
                    DCHECK(instr.IsImmediateInput(0));
                    Object *smi = NySmi::New(instr.ImmediateInput(0)->i64_value());
                    __ movq(Runtime::kScratch, reinterpret_cast<Address>(smi));
                    __ movq(instr.MemoryOutput(), Runtime::kScratch);
                }
            } else {
                DCHECK(instr.IsRegisterOutput());
                // TODO:
            }
            break;

        case lir::kConstant: {
            __ movq(Runtime::kScratch, Operand(Runtime::kThread, NyThread::kOffsetFrame));
            __ movq(Runtime::kScratch, Operand(Runtime::kScratch, CallFrame::kOffsetConstPool));
            int index = instr.ImmediateInput(0)->i32_value();
            int32_t disp = NyArray::kOffsetElems + (index << kPointerShift);
            if (instr.IsMemoryOutput()) {
                __ movq(Runtime::kScratch, Operand(Runtime::kScratch, disp));
                __ movq(instr.MemoryOutput(), Runtime::kScratch);
            } else {
                DCHECK((instr.IsRegisterOutput()));
                __ movq(instr.RegisterOutput(), Operand(Runtime::kScratch, disp));
            }
        } break;
            
        case lir::kSaveCallerPartialRegisters: {
            std::vector<Register> gp_regs;
            std::vector<XMMRegister> fp_regs;
            for (int i = 0; i < instr.NumberOfInputs(); ++i) {
                if (instr.IsRegisterInput(i)) {
                    gp_regs.push_back(instr.RegisterInput(i));
                } else if (instr.IsXMMRegisterInput(i)) {
                    fp_regs.push_back(instr.XMMRegisterInput(i));
                }
            }
            SaveCallerRegisters(&gp_regs[0], gp_regs.size(), &fp_regs[0], fp_regs.size());
        } break;

        case lir::kRestoreCallerPartialRegisters: {
            std::vector<Register> gp_regs;
            std::vector<XMMRegister> fp_regs;
            for (int i = 0; i < instr.NumberOfInputs(); ++i) {
                if (instr.IsRegisterInput(i)) {
                    gp_regs.push_back(instr.RegisterInput(i));
                } else if (instr.IsXMMRegisterInput(i)) {
                    fp_regs.push_back(instr.XMMRegisterInput(i));
                }
            }
            RestoreCallerRegisters(&gp_regs[0], gp_regs.size(), &fp_regs[0], fp_regs.size());
        } break;

        case lir::kBreakpoint:
            __ Breakpoint();
            break;

        case lir::kMove:
            EMIT_MOVE();
            break;

        case lir::kAdd:
            if (instr.ir()->output() != instr.ir()->input(0)) {
                EMIT_MOVE();
            }
            EMIT_ARITH(addq);
            break;

        case lir::kAdd32:
            if (instr.ir()->output() != instr.ir()->input(0)) {
                EMIT_MOVE();
            }
            EMIT_ARITH(addl);
            break;
            
        case lir::kSub:
            if (instr.ir()->output() != instr.ir()->input(0)) {
                EMIT_MOVE();
            }
            EMIT_ARITH(subq);
            break;
            
        case lir::kSub32:
            if (instr.ir()->output() != instr.ir()->input(0)) {
                EMIT_MOVE();
            }
            EMIT_ARITH(subl);
            break;
            
        case lir::kRet:
            __ ret(0);
            break;
    
        default:
            break;
    }
}
    
Handle<NyCode> Code_CodeGenerateByLIR(const lir::InstructionBundle *ir_code, // low-level-ir code
                                      const BuiltinType *args, // type of arguments
                                      size_t argc, // number of arguments
                                      NyaaCore *core) {
    LIRCodeGenerator generator(ir_code);
    generator.Run();
    std::string code(generator.FullAlignmentCode());
    for (size_t i = 0; i < argc; ++i) {
        code.push_back(static_cast<char>(args[i]));
    }
    code.push_back(static_cast<char>(argc));
    
    std::vector<int32_t> lines(generator.source_lines());
    Handle<NyInt32Array> source_lines = core->factory()->NewInt32Array(generator.source_lines().size());
    source_lines = source_lines->Add(&lines[0], lines.size(), core);
    
    return core->factory()->NewCode(NyCode::kOptimizedFunction, *source_lines,
                                    reinterpret_cast<uint8_t *>(&code[0]), code.size());
}

} // namespace nyaa
    
} // namespace mai
