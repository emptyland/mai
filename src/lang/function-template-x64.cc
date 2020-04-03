#include "lang/value-inl.h"
#include "lang/isolate-inl.h"
#include "lang/metadata-space.h"
#include "lang/macro-assembler-x64.h"
#include "lang/stack-frame.h"

namespace mai {

namespace lang {

struct StubPrototype {
    const std::vector<uint32_t> parameters;
    bool has_vargs;
    uint32_t return_type;
}; // struct StubPrototype

#define __ masm_.

class StubBuilder {
public:
    StubBuilder(Isolate *isolate)
        : isolate_(DCHECK_NOTNULL(isolate)) {}
    
    void Build(const StubPrototype &prototype, Address cxx_func_entry) {
        __ Reset();
        StackFrameScope frame_scope(&masm_, StubStackFrame::kSize);
        __ movl(Operand(rbp, StubStackFrame::kOffsetMaker), StubStackFrame::kMaker);

        SetupArguments(prototype);
        __ InlineSwitchSystemStackCall(cxx_func_entry);
    }

    const std::string &GetInstructionsBuf() const { return masm_.buf(); }
private:
    void SetupArguments(const StubPrototype &ctx) {
        MetadataSpace *space = isolate_->metadata_space();
        size_t args_size = 0;
        for (size_t i = 0; i < ctx.parameters.size(); i++) {
            const Class *param = space->type(ctx.parameters[i]);
            args_size += RoundUp(param->reference_size(), kStackSizeGranularity);
        }
        args_size = RoundUp(args_size, kStackAligmentSize);
        
        int argc = 0, fargc = 0;
        // arg0
        // args ...
        // padding
        // +8 return address
        // +8 saved bp
        int32_t offset = static_cast<int>(kPointerSize * 2 + args_size);
        for (size_t i = 0; i < ctx.parameters.size(); i++) {
            const Class *param = space->type(ctx.parameters[i]);
            if (param->id() == kType_f32 || param->id() == kType_f64) {
                // Floating
                switch (param->reference_size()) {
                    case 4: // 32 bits
                        offset -= 4;
                        __ movss(kXmmArgv[fargc++], Operand(rbp, offset));
                        break;
                    case 8: // 64 bits
                        offset -= 8;
                        __ movsd(kXmmArgv[fargc++], Operand(rbp, offset));
                        break;
                    default:
                        NOREACHED() << "incorrect type size: " << param->name();
                        break;
                }
                DCHECK_GE(offset, kPointerSize * 2) << param->name();
            } else {
                // Integral and references
                switch (param->reference_size()) {
                    case 1: // 8 bits
                    case 2: // 16 bits
                    case 4: // 32 bits
                        offset -= 4;
                        __ movl(kRegArgv[argc++], Operand(rbp, offset));
                        break;
                    case 8: // 64 bits
                        offset -= 8;
                        __ movq(kRegArgv[argc++], Operand(rbp, offset));
                        break;
                    default:
                        NOREACHED() << "incorrect type size: " << param->name();
                        break;
                }
                DCHECK_GE(offset, kPointerSize * 2) << param->name();
            }
        }
    }
    
    Isolate *isolate_;
    MacroAssembler masm_;
}; // class StubBuilderX64

/*static*/ Code *FunctionTemplate::MakeStub(const std::vector<uint32_t> &parameters, bool has_vargs,
                                            uint32_t return_type, Address cxx_func_entry) {

    StubPrototype context{parameters, has_vargs, return_type};
    StubBuilder builder(STATE);
    builder.Build(context, cxx_func_entry);
    PrototypeDesc *prototype = STATE->metadata_space()->NewPrototypeDesc(&parameters[0],
                                                                         parameters.size(),
                                                                         has_vargs, return_type);
    return STATE->metadata_space()->NewCode(Code::STUB, builder.GetInstructionsBuf(), prototype);
}

} // namespace lang

} // namespace mai

