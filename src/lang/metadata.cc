#include "lang/metadata.h"
#include "lang/metadata-space.h"
#include "lang/isolate-inl.h"
#include "asm/utils.h"
#include "base/arenas.h"

namespace mai {

namespace lang {

#define MEMBER_OFFSET_OF(owns, field) \
    arch::ObjectTemplate<owns, int32_t>::OffsetOf(&owns :: field)

const int32_t Code::kOffsetEntry = MEMBER_OFFSET_OF(Code, instructions_);

const int32_t BytecodeArray::kOffsetEntry = MEMBER_OFFSET_OF(BytecodeArray, instructions_);

const int32_t Function::kOffsetStackSize = MEMBER_OFFSET_OF(Function, stack_size_);
const int32_t Function::kOffsetConstPool = MEMBER_OFFSET_OF(Function, const_pool_);
const int32_t Function::kOffsetBytecode = MEMBER_OFFSET_OF(Function, bytecode_);
const int32_t Function::kOffsetExceptionTableSize = MEMBER_OFFSET_OF(Function, exception_table_size_);

const int32_t Class::kOffsetNMethods = MEMBER_OFFSET_OF(Class, n_methods_);
const int32_t Class::kOffsetMethods = MEMBER_OFFSET_OF(Class, methods_);

const int32_t Method::kOffsetFunction = MEMBER_OFFSET_OF(Method, fn_);

bool Type::IsUnsignedIntegral() const {
    switch (static_cast<BuiltinType>(id_)) {
        case kType_u8:
        case kType_u16:
        case kType_u32:
        case kType_uint:
        case kType_u64:
            return true;
        default:
            return false;
    }
}

bool Type::IsSignedIntegral() const {
    switch (static_cast<BuiltinType>(id_)) {
        case kType_i8:
        case kType_i16:
        case kType_i32:
        case kType_int:
        case kType_i64:
            return true;
        default:
            return false;
    }
}

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

// TODO
int32_t Function::DispatchException(Any *exception, int32_t pc) {
    TODO();
    return -1;
}

void Function::Print(base::AbstractPrinter *out) const {
    out->Append("+-------------------------------\n");
    out->Printf("| %s%s\n", name(), prototype()->ToString().c_str());
    out->Append("+-------------------------------\n");
    out->Printf("stack-size: %u\n", stack_size_);
    out->Printf("const-pool-bytes: %u\n", const_pool_size_);
    out->Printf("const-pool-bitmap:");
    for (size_t i = 0; i < (const_pool_spans_size() + 31)/32; i++) {
        out->Printf(" %08x", const_pool_bitmap_[i]);
    }
    out->Append("\n");
//    if (const_pool_size_ > 0) {
//        out->Printf("const-pool: \n");
//    }
//    for (size_t i = 0; i < const_pool_spans_size(); i++) {
//        Span32 *span = const_pool_ + i;
//        out->Printf("    %08x %08x %08x %08x\n", span->v32[0].u32, span->v32[1].u32, span->v32[2].u32,
//                    span->v32[3].u32);
//    }
    out->Printf("captured-var-size: %u\n", captured_var_size_);
    if (captured_var_size_ > 0) {
        out->Printf("captured-var: \n");
    }
    for (size_t i = 0; i < captured_var_size_; i++) {
        if (captured_vars_[i].kind == IN_STACK) {
            out->Printf("    IN_STACK ");
        } else {
            out->Printf("    IN_CAPTURED ");
        }
        out->Printf("[%d] ", captured_vars_[i].index);
        out->Printf("%s: %s\n", captured_vars_[i].name, captured_vars_[i].type->name());
    }
    out->Printf("exception-table-size: %u\n", exception_table_size_);
    if (exception_table_size_ > 0) {
        out->Printf("exception-table: \n");
    }
    for (size_t i = 0; i < exception_table_size_; i++) {
        auto handler = exception_table_ + i;
        out->Printf("    %s %" PRIdPTR " %" PRIdPTR " %" PRIdPTR "\n",
                    !handler->expected_type? "nil" : handler->expected_type->name(),
                    handler->start_pc, handler->stop_pc, handler->handler_pc);
    }
    if (kind_ == BYTECODE) {
        out->Printf("bytecode-size: %u\n", bytecode_->size());
        out->Printf("bytecode: \n");
        base::ScopedArena arena;
        for (size_t i = 0; i < bytecode_->size(); i++) {
            BytecodeNode *node = BytecodeNode::From(&arena, bytecode_->entry()[i]);
            out->Printf("    %03zd ", i);
            node->Print(out);
            out->Append("\n");
        }
    }
}

} // namespace lang

} // namespace mai
