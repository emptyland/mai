#include "lang/hir.h"


namespace mai {

namespace lang {

/*static*/ const char *HOperator::kNames[HMaxOpcode] = {
#define DEFINE_CODE(name, ...) #name,
    DECLARE_HIR_OPCODES(DEFINE_CODE)
#undef DEFINE_CODE
};

bool HNode::UseIterator::UpdateTo(HNode *new_to) {
    DCHECK(Valid());
    HNode *old_to = to();
    if (old_to != new_to) {
        Use *next = use_->next_;
        if (old_to) {
            old_to->RemoveUse(use_);
        }
        set_to(new_to);
        if (new_to) {
            new_to->AppendUse(use_);
        }
        use_ = next;
    }
    return old_to != new_to;
}

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

void HNode::ReplaceInput(base::Arena *arena, int i, HNode *node) {
    HNode *old = input(i);
    if (old == node) {
        return;
    }
    Use *use = DCHECK_NOTNULL(old->FindUse(this));
    old->RemoveUse(use);
    inputs()[i] = node;
    DCHECK_EQ(i, use->input_index);
    DCHECK_EQ(this, use->user);
    node->AppendUse(use);
}

void HNode::ReplaceUses(HNode *that) {
//    DCHECK(this->use_.next_ == &this->use_ || this->use_.next_->prev_ == &this->use_);
//    DCHECK(that->use_.next_ == &that->use_ || that->use_.next_->prev_ == &that->use_);

    Use *last_use = nullptr;
    UseIterator iter(this);
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        iter.set_to(that);
        last_use = *iter;
    }
    
    if (last_use) {
        while (use_.next_ != &use_) {
            Use *use = use_.next_;
            QUEUE_REMOVE(use);
            QUEUE_INSERT_TAIL(&that->use_, use);
        }
    }
    
    // Clear uses list
    use_.next_ = &use_;
    use_.prev_ = &use_;
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
    cache_[H##name] = new (arena_) HOperator(H##name, 0, 2, 0, 0, 1, 0, 0);
    DECLARE_HIR_BINARY_ARITHMETIC(DEFINE_BINARY_OP)
    DECLARE_HIR_COMPAROR(DEFINE_BINARY_OP);
#undef  DEFINE_BINARY_OPS
}

/*static*/ bool NodeOps::IsConstant(const HNode *node) {
    switch (node->opcode()) {
        case HWord32Constant:
        case HWord64Constant:
        case HFloat32Constant:
        case HFloat64Constant:
        case HStringConstant:
            return true;
        default:
            break;
    }
    return false;
}

/*static*/
void NodeOps::CollectControlProjections(HNode *node, HNode **projections, size_t count) {
    DCHECK_LE(count, node->UseCount());
#if defined(DEBUG) || defined(_DEBUG)
    ::memset(projections, 0, sizeof(*projections) * count);
#endif
    HNode::UseIterator iter(node);
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        if (!IsControlEdge(iter)) {
            continue;
        }
        size_t index = 0;
        HNode *user = iter.user();
        switch (user->opcode()) {
            case HIfTrue:
                DCHECK_EQ(HBranch, node->opcode());
                index = 0;
                break;
            case HIfFalse:
                DCHECK_EQ(HBranch, node->opcode());
                index = 1;
                break;
            // TODO:
            default:
                continue;
        }
        DCHECK_LT(index, count);
        DCHECK(projections[index] == nullptr);
        projections[index] = user;
    }
#if defined(DEBUG) || defined(_DEBUG)
    for (size_t i = 0; i < count; i++) {
        DCHECK(projections[i] != nullptr);
    }
#endif
}

} // namespace lang

} // namespace mai
