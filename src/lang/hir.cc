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
    : HValue(vid, type)
    , op_(op)
    , is_inline_inputs_(1)
    , inputs_capacity_(inputs_capacity)
    , inputs_size_(inputs_size) {
    use_.next_ = &use_;
    use_.prev_ = &use_;
    for (int i = 0; i < inputs_size_; i++) {
        inline_inputs_[i] = inputs[i];
        inputs[i]->AppendUser(arena, i, this);
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
        node->AppendUser(arena, inputs_size_, this);
        inline_inputs_[inputs_size_++] = node;
    } else {
        node->AppendUser(arena, out_of_line_inputs_->inputs_size_, this);
        out_of_line_inputs_->inputs_[out_of_line_inputs_->inputs_size_++] = node;
    }
    
}

HNode::Use *HNode::AppendUser(base::Arena *arena, int input_index, HNode *user) {
    if (Use *u = FindUse(user)) {
        return u;
    }

    Use *use = new (arena) Use{};
    use->user        = user;
    use->input_index = input_index;
    QUEUE_INSERT_TAIL(&use_, use);
    return use;
}

void HOperatorFactory::Initialize() {
#define DEFINE_BINARY_OP(name) \
    cache_[H##name] = new (arena_) HOperator(H##name, 0, 0, 2, 0, 0, 1, 0);
    DECLARE_HIR_BINARY_ARITHMETIC(DEFINE_BINARY_OP)
    DECLARE_HIR_COMPAROR(DEFINE_BINARY_OP);
#undef  DEFINE_BINARY_OPS
}

/*static*/ bool NodeOps::IsConstant(const HNode *node) {
    switch (node->opcode()) {
        case HConstant32:
        case HConstant64:
        case HFConstant32:
        case HFConstant64:
        case HConstantString:
            return true;
        default:
            break;
    }
    return false;
}

} // namespace lang

} // namespace mai
