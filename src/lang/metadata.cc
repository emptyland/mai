#include "lang/metadata.h"
#include "lang/metadata-space.h"
#include "lang/isolate-inl.h"
#include "asm/utils.h"

namespace mai {

namespace lang {

#define MEMBER_OFFSET_OF(owns, field) \
    arch::ObjectTemplate<owns, int32_t>::OffsetOf(&owns :: field)

const int32_t Code::kOffsetEntry = MEMBER_OFFSET_OF(Code, instructions_);

const int32_t BytecodeArray::kOffsetEntry = MEMBER_OFFSET_OF(BytecodeArray, instructions_);

std::string PrototypeDesc::ToString() const {
    return ToString(STATE->metadata_space());
}

std::string PrototypeDesc::ToString(const MetadataSpace *space) const {
    std::string buf;
    buf.append("(");
    uint32_t i = 0;
    for (i = 0; i < parameter_size_; i++) {
        if (i > 0) {
            buf.append(",");
        }
        const Class *type = space->type(parameter(i));
        buf.append(type->name());
    }
    if (has_vargs()) {
        if (i > 0) {
            buf.append(",");
        }
        buf.append("...");
    }
    buf.append("):");
    const Class *type = space->type(return_type_);
    buf.append(type->name());
    return buf;
}


} // namespace lang

} // namespace mai
