#include "lang/hir.h"


namespace mai {

namespace lang {

/*static*/ const char *HOperator::kNames[HMaxOpcode] = {
#define DEFINE_CODE(name, ...) #name,
    DECLARE_HIR_OPCODES(DEFINE_CODE)
#undef DEFINE_CODE
};

HNode::HNode(base::Arena *arena, int vid, int hint, HType type, const HOperator *op,
             int inputs_capacity, int inputs_size, HNode **inputs)
    : HValue(vid, hint, type)
    , op_(op)
    , inputs_capacity_(inputs_capacity)
    , inputs_size_(inputs_size) {
    for (int i = 0; i < inputs_size_; i++) {
        inputs_[i] = inputs[i];
        BeUsed(arena, inputs_[i]);
    }
}

Used *HNode::BeUsed(base::Arena *arena, HNode *use) {
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
    // TODO:
}

} // namespace lang

} // namespace mai
