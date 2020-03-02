#include "lang/stable-space-builder.h"

namespace mai {

namespace lang {

GlobalSpaceBuilder::GlobalSpaceBuilder()
    : spans_(new Span32[2])
    , capacity_(2)
    , length_(2)
    , bitmap_(new uint32_t[1]) {
    bitmap_[0] = 0x2;
    r_.index = 1;
    r_.used = 0;
    p_.index = 0;
    p_.used = 0;
}

int GlobalSpaceBuilder::Reserve(SpanState *state, size_t size, bool isref) {
    if (state->used + size > sizeof(Span32)) {
        CheckCapacity();
        state->index = std::max(p_.index, r_.index) + 1;
        if (isref) {
            bitmap_[state->index / 32] |= 1 << (state->index % 32);
        }
        length_++;
        state->used = 0;
    }
    int location = static_cast<int>(state->index * sizeof(Span32) + state->used);
    state->used += size;
    return location;
}

void GlobalSpaceBuilder::CheckCapacity() {
    if (length_ + 1 < capacity_) {
        return;
    }
    size_t old_bmp_size = (capacity_ + 31) / 32;
    capacity_ <<= 1;
    size_t bmp_size = (capacity_ + 31) / 32;
    Span32 *new_spans = new Span32[capacity_];
    ::memcpy(new_spans, spans_, length_ * sizeof(Span32));
    uint32_t *new_bmp = new uint32_t[bmp_size];
    ::memcpy(new_bmp, bitmap_, old_bmp_size * sizeof(uint32_t));
    delete [] spans_;
    spans_ = new_spans;
    delete [] bitmap_;
    bitmap_ = new_bmp;
}

} // namespace lang

} // namespace mai
