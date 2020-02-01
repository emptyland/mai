#include "lang/value-inl.h"
#include "lang/isolate-inl.h"
#include "lang/metadata-space.h"
#include "lang/macro-assembler-x64.h"

namespace mai {

namespace lang {

#define __ macro_assembler_.

class StubBuilderX64 {
public:
    StubBuilderX64(Isolate *isolate)
        : isolate_(DCHECK_NOTNULL(isolate)) {}
    
    // TODO:
private:
    Isolate *isolate_;
    MacroAssembler macro_assembler_;
}; // class StubBuilderX64

/*static*/ Code *FunctionTemplate::MakeStub(const std::vector<uint32_t> &parameters, bool has_vargs,
                                            uint32_t return_type) {
    StubBuilderX64 builder(__isolate);
    // TODO:
    return nullptr;
}

} // namespace lang

} // namespace mai

