#include "lang/hir-reducer.h"
#include "lang/hir.h"

namespace mai {

namespace lang {

GraphReducer::GraphReducer(base::Arena *arena, HGraph *graph, HNode *dead)
    : graph_(graph)
    , dead_(dead)
    , reducers_(arena)
    , revisit_(arena)
    , stack_(arena) {
}

GraphReducer::~GraphReducer() = default;

void GraphReducer::ReduceNode(HNode *node) {
    DCHECK(stack_.empty());
    DCHECK(revisit_.empty());
    
    Push(node);
    for (;;) {
        if (!stack_.empty()) {
            ReduceTop();
        } else if (!revisit_.empty()) {
            HNode *first = revisit_.front();
            revisit_.pop_front();
            
            if (state_.Get(node) == State::kRevisit) {
                Push(first);
            }
        } else {
            for (Reducer *reducer : reducers_) { reducer->Finalize(); }
            if (revisit_.empty()) {
                break;
            }
        }
    }
    DCHECK(revisit_.empty());
    DCHECK(stack_.empty());
}

void GraphReducer::Replace(HNode *node, HNode *replacement) /*final*/ {
    Replace(node, replacement, std::numeric_limits<int>::max());
}

void GraphReducer::ReplaceWithValue(HNode *node, HNode *value, HNode *effect, HNode *control)
/*final*/ {
    // TODO:
}

Reduction GraphReducer::Reduce(HNode *node) {
    auto skip = reducers_.end();
    for (auto i = reducers_.begin(); i != reducers_.end();) {
        if (i != skip) {
            Reduction reduction = (*i)->Reduce(node);
            if (!reduction.Changed()) {
                // ignore
            } else if (reduction.replacement() == node) {
                skip = i;
                i = reducers_.begin();
            } else {
                return reduction;
            }
        }
        i++;
    }
    if (skip == reducers_.end()) {
        return Reducer::NoChange();
    }
    return Reducer::Change(node);
}

void GraphReducer::ReduceTop() {
    NodeState *entry = &stack_.back();
    HNode *node = entry->node;
    DCHECK_EQ(static_cast<int>(State::kOnStack), state_.GetU32(node));
    
    if (node->IsDead()) {
        Pop();
        return;
    }
    
    View<HNode *> inputs = node->inputs_view();
    int start = entry->input_index < inputs.n ? entry->input_index : 0;
    for (int i = start; i < inputs.n; i++) {
        HNode *input = inputs.z[i];
        if (input != node && Recurse(input)) {
            entry->input_index = i + 1;
            return;
        }
    }
    
    for (int i = 0; i < start; i++) {
        HNode *input = inputs.z[i];
        if (input != node && Recurse(input)) {
            entry->input_index = i + 1;
            return;
        }
    }
    
    int max_id = graph_->node_count() - 1;
    Reduction reduction = Reduce(node);
    if (!reduction.Changed()) {
        Pop();
        return;
    }
    
    HNode *replacement = reduction.replacement();
    if (replacement == node) {
        HNode::UseIterator iter(node);
        for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
            DCHECK(iter.user() == node || state_.Get(node) != State::kVisited);
            Revisit(iter.user());
        }
        
        View<HNode *> inputs = node->inputs_view();
        for (int i = 0; i < inputs.n; i++) {
            HNode *input = inputs.z[i];
            if (input != node && Recurse(input)) {
                entry->input_index = i + 1;
                return;
            }
        }
    }
    
    Pop();
    
    if (replacement != node) {
        Replace(node, replacement, max_id);
    }
}

void GraphReducer::Revisit(HNode *node) /*final*/ {
    if (state_.Get(node) == State::kVisited) {
        state_.Set(node, State::kRevisit);
        revisit_.push_back(node);
    }
}

void GraphReducer::Replace(HNode *node, HNode *replacement, int max_node_id) {
    if (node == graph_->start()) {
        graph_->set_start(replacement);
    }
    if (node == graph_->end()) {
        graph_->set_end(replacement);
    }
    
    if (replacement->vid() <= max_node_id) {
        HNode::UseIterator iter(node);
        iter.SeekToFirst();
        while (iter.Valid()) {
            HNode *user = iter.user();
            if (!iter.UpdateTo(replacement)) { iter.Next(); }
            if (user != node) { Revisit(user); }
        }
        node->Kill();
    } else {
        HNode::UseIterator iter(node);
        iter.SeekToFirst();
        while (iter.Valid()) {
            HNode *user = iter.user();
            if (user->vid() <= max_node_id) {
                if (!iter.UpdateTo(replacement)) { iter.Next(); }
                if (user != node) { Revisit(user); }
            }
        }
        if (node->UseCount() == 0) { node->Kill(); }

        Recurse(replacement);
    }
}

} // namespace lang

} // namespace mai
