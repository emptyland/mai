#ifndef MAI_LANG_HIR_REDUCER_H_
#define MAI_LANG_HIR_REDUCER_H_

#include "lang/hir.h"
#include "base/arena-utils.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {

namespace lang {

class HNode;
class HGraph;

class Reduction final {
public:
    explicit Reduction(HNode *replacement = nullptr): replacement_(replacement) {}

    DEF_PTR_GETTER(HNode, replacement);
    bool Changed() const { return replacement_ != nullptr; }
    Reduction FollowedBy(Reduction next) const { return next.Changed() ? next : *this; }
private:
    HNode *replacement_;
}; // class Reduction


class Reducer {
public:
    Reducer() = default;
    virtual ~Reducer() = default;
    
    virtual const char *GetReducerName() const = 0;
    
    virtual Reduction Reduce(HNode *node) = 0;
    
    virtual void Finalize() = 0;
    
    static Reduction NoChange() { return Reduction(); }
    static Reduction Replace(HNode *node) { return Reduction(node); }
    static Reduction Change(HNode *node) { return Reduction(node); }

    DISALLOW_IMPLICIT_CONSTRUCTORS(Reducer);
}; // class Reducer


class AdvancedReducer : public Reducer {
public:
    class Editor {
    public:
        Editor() = default;
        virtual ~Editor() = default;
        
        virtual void Replace(HNode *node, HNode *replacement) = 0;
        virtual void Revisit(HNode *node) = 0;
        virtual void ReplaceWithValue(HNode *node, HNode *value, HNode *effect, HNode *control) = 0;
    }; // class Editor
    
    explicit AdvancedReducer(Editor *editor): editor_(DCHECK_NOTNULL(editor)) {}

    DISALLOW_IMPLICIT_CONSTRUCTORS(AdvancedReducer);
protected:
    static Reduction Replace(HNode *node) { return Reducer::Replace(node); }

    void Replace(HNode *node, HNode *replacement) { editor_->Replace(node, replacement); }
    void Revisit(HNode *node) { editor_->Revisit(node); }
    void ReplaceWithValue(HNode *node, HNode *value, HNode *effect = nullptr,
                          HNode *control = nullptr) {
        editor_->ReplaceWithValue(node, value, effect, control);
    }
    void RelaxEffectsAndControls(HNode *node) {
        editor_->ReplaceWithValue(node, node, nullptr, nullptr);
    }
    void RelaxControls(HNode *node) {
        editor_->ReplaceWithValue(node, node, node, nullptr);
    }
private:
    Editor *editor_;
}; // class AdvancedReducer



class GraphReducer : public AdvancedReducer::Editor {
public:
    GraphReducer(base::Arena *arena, HGraph *graph, HNode *dead = nullptr);
    ~GraphReducer() override;
    
    DEF_PTR_GETTER(HGraph, graph);
    DEF_PTR_GETTER(HNode, dead);

    void AddReducer(Reducer *reducer) { reducers_.push_back(reducer); }
    
    void ReduceNode(HNode *node);
    void ReduceGraph() { ReduceNode(graph_->end()); }

    void Replace(HNode *node, HNode *replacement) final;
    void Revisit(HNode *node) final;
    void ReplaceWithValue(HNode *node, HNode *value, HNode *effect, HNode *control) final;
    
private:
    enum class State: uint8_t {
        kUnvisited,
        kRevisit,
        kOnStack,
        kVisited,
    };

    struct NodeState {
        HNode *node;
        int input_index;
    }; // struct NodeState
    
    Reduction Reduce(HNode *node);
    void ReduceTop();

    void Pop() {
        HNode *node = stack_.back().node;
        state_.Set(node, State::kVisited);
        stack_.pop_back();
    }

    void Push(HNode *node) {
        DCHECK_NE(static_cast<uint32_t>(State::kOnStack), state_.GetU32(node));
        state_.Set(node, State::kOnStack);
        stack_.push_back({node, 0});
    }
    
    bool Recurse(HNode *node) {
        if (state_.Get(node) > State::kRevisit) {
            return false;
        }
        Push(node);
        return true;
    }

    void Replace(HNode *node, HNode *replacement, int max_node_id);
    
    HGraph *const graph_;
    HNode *const dead_;
    HNodeState<State, 0, 4> state_;
    base::ArenaVector<Reducer *> reducers_;
    base::ArenaDeque<HNode *> revisit_;
    base::ArenaDeque<NodeState> stack_;
    
    static_assert(HNodeState<State, 0, 4>::kMask == 0xf, "incorrect mask");
}; // class GraphReducer

} // namespace lang

} // namespace mai

#endif // MAI_LANG_HIR_REDUCER_H_
