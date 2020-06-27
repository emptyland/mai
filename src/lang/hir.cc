#include "lang/hir.h"


namespace mai {

namespace lang {

/*static*/ const char *HOperator::kNames[HMaxOpcode] = {
#define DEFINE_CODE(name, ...) #name,
    DECLARE_HIR_OPCODES(DEFINE_CODE)
#undef DEFINE_CODE
};

HNode::HNode(base::Arena *arena, int vid, HType type, const HOperator *op,
             uint32_t inputs_capacity, uint32_t inputs_size, HNode **inputs)
    : HValue(vid, 0, type)
    , op_(op)
    , is_inline_inputs_(1)
    , inputs_capacity_(inputs_capacity)
    , inputs_size_(inputs_size) {
    for (int i = 0; i < inputs_size_; i++) {
        inline_inputs_[i] = inputs[i];
        BeUsed(arena, inline_inputs_[i]);
    }
}

void HNode::AppendInput(base::Arena *arena, HNode *node) {
    if (is_inline_inputs_) {
        if (inputs_size_ + 1 > inputs_capacity_) {
            is_inline_inputs_ = 0;
            OutOfLineInputs *ool = OutOfLineInputs::New(arena, this, inputs_capacity_ << 1,
                                                        inputs_size_, inline_inputs_);
            out_of_line_inputs_ = ool;
        }
    } else {
        if (out_of_line_inputs_->inputs_size_ + 1 > out_of_line_inputs_->inputs_capacity_) {
            OutOfLineInputs *ool = OutOfLineInputs::New(arena, this,
                                                        out_of_line_inputs_->inputs_capacity_ << 1,
                                                        out_of_line_inputs_->inputs_size_,
                                                        out_of_line_inputs_->inputs_);
            out_of_line_inputs_ = ool;
        }
    }
    
    if (is_inline_inputs_) {
        inline_inputs_[inputs_size_++] = node;
    } else {
        out_of_line_inputs_->inputs_[out_of_line_inputs_->inputs_size_++] = node;
    }
    BeUsed(arena, node);
}

HNode::Used *HNode::BeUsed(base::Arena *arena, HNode *use) {
    if (!use->used_) {
        use->used_ = new (arena) Used{};
        use->used_->next_ = use->used_;
        use->used_->prev_ = use->used_;
    }
    if (Used *u = use->FindUsed(this)) {
        return u;
    }

    Used *used = new (arena) Used{};
    used->user  = this;
    QUEUE_INSERT_TAIL(use->used_, used);
    return used;
}

void HOperatorFactory::Initialize() {
#define DEFINE_BINARY_OP(name) \
    cache_[H##name] = new (arena_) HOperator(H##name, 0, 0, 2, 0, 0, 1, 0);
    DECLARE_HIR_BINARY_ARITHMETIC(DEFINE_BINARY_OP)
    DECLARE_HIR_COMPAROR(DEFINE_BINARY_OP);
#undef  DEFINE_BINARY_OPS
}

} // namespace lang

} // namespace mai
