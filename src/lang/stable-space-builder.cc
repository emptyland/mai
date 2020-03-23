#include "lang/stable-space-builder.h"

namespace mai {

namespace lang {

StackSpaceAllocator::StackSpaceAllocator(StackFrame::Maker maker) {
    switch (maker) {
        case StackFrame::kBytecode:
            level_ = BytecodeStackFrame::kOffsetHeaderSize;
            max_   = level_;
            break;
        case StackFrame::kStub:
        case StackFrame::kTrampoline:
        default:
            NOREACHED();
            break;
    }
}

size_t ConstantPoolBuilder::KeyHash::operator () (Key key) const {
    size_t code = 0;
    switch (key.kind) {
        case Key::kP32:
            code = key.p32;
            break;
        case Key::kP64:
            code = key.p64;
            break;
        case Key::kF32:
            code = bit_cast<size_t>(key.f32);
            break;
        case Key::kF64:
            code = bit_cast<size_t>(key.f64);
            break;
        case Key::kString:
            code = base::Hash::Js(key.string.data(), key.string.size());
            break;
        case Key::kClosure:
            code = (bit_cast<size_t>(key.closure) >> 1) | 0x1;
            break;
        case Key::kMetadata:
            code = (bit_cast<size_t>(key.metadata) >> 1) | 0x1;
            break;
        default:
            NOREACHED();
            break;
    }
    return ((code * static_cast<size_t>(key.kind)) >> 32) ^ code;
}

bool ConstantPoolBuilder::KeyEqualTo::operator () (Key lhs, Key rhs) const {
    if (lhs.kind != rhs.kind) {
        return false;
    }
    switch (lhs.kind) {
        case Key::kP32:
            return lhs.p32 == rhs.p32;
        case Key::kP64:
            return lhs.p64 == rhs.p64;
        case Key::kF32:
            return lhs.f32 == rhs.f32;
        case Key::kF64:
            return lhs.f64 == rhs.f64;
        case Key::kString:
            return lhs.string.compare(rhs.string) == 0;
        case Key::kClosure:
            return lhs.closure == rhs.closure;
        case Key::kMetadata:
            return lhs.metadata == rhs.metadata;
        default:
            NOREACHED();
            return false;
    }
}

} // namespace lang

} // namespace mai
