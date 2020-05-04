#include "lang/stable-space-builder.h"

namespace mai {

namespace lang {

int StackSpaceAllocator::Reserve(size_t size) {
//    if (level_.p == 0 && level_.r == 0) {
//        AdvanceSpan(false/*isref*/);
//    }
    if (bitmap_.empty()) {
        AdvanceSpan(false/*isref*/);
    }
    while (Test(level_.p) && level_.p < max_spans_ * kSpanSize) {
        DCHECK_EQ(0, level_.p % kSpanSize);
        level_.p += kSpanSize;
    }
    size_t request_size = RoundUp(size, kSizeGranularity);
    if (level_.p >= max_spans_ * kSpanSize || level_.p % kSpanSize + request_size > kSpanSize) {
        AdvanceSpan(false/*isref*/);
    }
    level_.p += request_size;
    return level_.p + kSlotBase;
}

int StackSpaceAllocator::ReserveRef() {
//    if (level_.p == 0 && level_.r == 0) {
//        AdvanceSpan(true/*isref*/);
//    }
    if (bitmap_.empty()) {
        AdvanceSpan(false/*isref*/);
    }
    while (!Test(level_.r) && level_.r < max_spans_ * kSpanSize) {
        DCHECK_EQ(0, level_.r % kSpanSize);
        level_.r += kSpanSize;
    }
    if (level_.r >= max_spans_ * kSpanSize || level_.r % kSpanSize + kPointerSize > kSpanSize) {
        AdvanceSpan(true/*isref*/);
    }
    level_.r += kPointerSize;
    return level_.r + kSlotBase;
}

void StackSpaceAllocator::AdvanceSpan(bool isref) {
    if (isref) {
        level_.r = static_cast<int>(max_spans_ * kSpanSize);
    } else {
        level_.p = static_cast<int>(max_spans_ * kSpanSize);
    }
    max_spans_++;
    if ((max_spans_ + 31) / 32 > bitmap_.size()) {
        bitmap_.push_back(0);
    }
    if (isref) {
        size_t index = level_.r / kSpanSize;
        bitmap_[index / 32] |= (1u << (index % 32));
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
