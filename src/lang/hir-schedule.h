#ifndef MAI_LANG_HIR_SCHEDULE_H_
#define MAI_LANG_HIR_SCHEDULE_H_

#include "base/arena-utils.h"
#include "base/arenas.h"
#include "base/queue-macros.h"
#include "base/base.h"
#include "glog/logging.h"
#include <type_traits>

namespace mai {

namespace lang {

class HNode;
class HGraph;

class HBasicBlock final : public base::ArenaObject {
public:
    enum Control {
        kNone,
        kGoto,
        kCall,
        kBranch,
        kSwitch,
        kDeoptimize,
        kTailCall,
        kReturn,
        kThrow,
    };
    class Id final {
    public:
        DEF_VAL_GETTER(size_t, index);
        int ToInt() const { return static_cast<int>(index_); }
        static Id FromInt(int value) { return Id(static_cast<size_t>(value)); }
        static Id FromSize(size_t value) { return Id(value); }
    private:
        explicit Id(size_t index): index_(index) {}
        size_t index_;
    };
    
    HBasicBlock(base::Arena *arena, Id id);

    DEF_VAL_GETTER(Id, id);
    DEF_ARENA_VECTOR_GETTER(HNode *, node);
    DEF_ARENA_VECTOR_GETTER(HBasicBlock *, successor);
    DEF_ARENA_VECTOR_GETTER(HBasicBlock *, predecessor);
    DEF_VAL_PROP_RW(int32_t, loop_number);
    DEF_VAL_PROP_RW(int32_t, rpo_number);
    DEF_VAL_PROP_RW(bool, deferred);
    DEF_VAL_PROP_RW(int32_t, dominator_depth);
    DEF_PTR_PROP_RW(HBasicBlock, dominator);
    DEF_PTR_PROP_RW(HBasicBlock, rpo_next);
    DEF_PTR_PROP_RW(HBasicBlock, loop_header);
    DEF_PTR_PROP_RW(HBasicBlock, loop_end);
    DEF_VAL_PROP_RW(int32_t, loop_depth);
    DEF_VAL_PROP_RW(Control, control);
    DEF_PTR_PROP_RW(HNode, control_input);

    void AddNode(HNode *node) { nodes_.push_back(node); }
    void RemoveNode(size_t pos) { nodes_.erase(nodes_.begin() + pos); }
    void AddSuccessor(HBasicBlock *block) { successors_.push_back(block); }
    void AddPredecessor(HBasicBlock *block) { predecessors_.push_back(block); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(HBasicBlock);
private:
    Id id_;
    base::ArenaVector<HNode *> nodes_;
    base::ArenaVector<HBasicBlock *> successors_;
    base::ArenaVector<HBasicBlock *> predecessors_;
    int32_t loop_number_ = 0;
    int32_t rpo_number_ = 0;
    bool deferred_ = false;
    int32_t dominator_depth_ = 0;
    HBasicBlock *dominator_ = nullptr;
    HBasicBlock *rpo_next_ = nullptr;
    HBasicBlock *loop_header_ = nullptr;
    HBasicBlock *loop_end_ = nullptr;
    int32_t loop_depth_ = 0;
    Control control_ = kNone;
    HNode *control_input_ = nullptr;
}; // class HBasicBlock


class HSchedule final : public base::ArenaObject {
public:
    explicit HSchedule(base::Arena *arena, int node_count_hint = 0);
    
    DEF_PTR_GETTER(base::Arena, arena);
    DEF_PTR_GETTER(HBasicBlock, start);
    DEF_PTR_GETTER(HBasicBlock, end);

    HBasicBlock *block(const HNode *node) const;

    size_t all_blocks_size() const { return all_blocks_.size(); }
    size_t rpo_order_size() const { return rpo_order_.size(); }
    
    bool IsScheduled(const HNode *node) const;
    
    HBasicBlock *GetBlockById(HBasicBlock::Id id) {
        DCHECK_LT(id.index(), all_blocks_.size());
        return all_blocks_[id.index()];
    }

    void ClearBlockById(HBasicBlock::Id id) {
        DCHECK_LT(id.index(), all_blocks_.size());
        all_blocks_[id.index()] = nullptr;
    }
    
    HBasicBlock *NewBlock() {
        HBasicBlock::Id id = HBasicBlock::Id::FromSize(all_blocks_.size());
        HBasicBlock *block = new (arena_) HBasicBlock(arena_, id);
        all_blocks_.push_back(block);
        return block;
    }
    
    bool SameBlock(const HNode *a, const HNode *b) {
        HBasicBlock *bb = block(a);
        return bb != nullptr && bb == block(b);
    }

    void PlanNode(HBasicBlock *bb, HNode *node) {
        DCHECK(block(node) != nullptr);
        SetBlockForNode(bb, node);
    }

    void AddNode(HBasicBlock *bb, HNode *node) {
        DCHECK(block(node) == nullptr || block(node) == bb);
        bb->AddNode(node);
        SetBlockForNode(bb, node);
    }
    
    void AddGoto(HBasicBlock *bb, HBasicBlock *succ) {
        DCHECK_EQ(bb->control(), HBasicBlock::kNone);
        bb->set_control(HBasicBlock::kGoto);
        AddSuccessor(bb, succ);
    }
    
    void AddCall(HBasicBlock *bb, HNode *call, HBasicBlock *succ) {
        DCHECK_EQ(bb->control(), HBasicBlock::kNone);
        bb->set_control(HBasicBlock::kCall);
        AddSuccessor(bb, succ);
        SetControlInput(bb, call);
    }
    
    void AddBranch(HBasicBlock *bb, HNode *branch, HBasicBlock *if_true, HBasicBlock *if_false) {
        DCHECK_EQ(bb->control(), HBasicBlock::kNone);
        bb->set_control(HBasicBlock::kBranch);
        AddSuccessor(bb, if_true);
        AddSuccessor(bb, if_false);
        SetControlInput(bb, branch);
    }
    
    void AddTailCall(HBasicBlock *bb, HNode *node) {
        DCHECK_EQ(bb->control(), HBasicBlock::kNone);
        bb->set_control(HBasicBlock::kTailCall);
        SetControlInput(bb, node);
        if (bb != end_) { AddSuccessor(bb, end_); }
    }
    
    void AddReturn(HBasicBlock *bb, HNode *node) {
        DCHECK_EQ(bb->control(), HBasicBlock::kNone);
        bb->set_control(HBasicBlock::kReturn);
        SetControlInput(bb, node);
        if (bb != end_) { AddSuccessor(bb, end_); }
    }
    
    void AddDeoptimize(HBasicBlock *bb, HNode *node) {
        DCHECK_EQ(bb->control(), HBasicBlock::kNone);
        bb->set_control(HBasicBlock::kDeoptimize);
        SetControlInput(bb, node);
        if (bb != end_) { AddSuccessor(bb, end_); }
    }
    
    void AddThrow(HBasicBlock *bb, HNode *node) {
        DCHECK_EQ(bb->control(), HBasicBlock::kNone);
        bb->set_control(HBasicBlock::kThrow);
        SetControlInput(bb, node);
        if (bb != end_) { AddSuccessor(bb, end_); }
    }
    
    void InsertBranch(HBasicBlock *bb, HBasicBlock *end, HNode *node, HBasicBlock *if_true,
                      HBasicBlock *if_false);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(HSchedule);
private:
    void SetControlInput(HBasicBlock *bb, HNode *node) {
        bb->set_control_input(node);
        SetBlockForNode(bb, node);
    }
    
    void SetBlockForNode(HBasicBlock *block, HNode *node);
    
    void MoveSuccessor(HBasicBlock *from, HBasicBlock *to) {
        for (auto const succ : from->successors()) {
            to->AddSuccessor(succ);
            for (auto &pred : *succ->mutable_predecessors()) {
                if (pred == from) { pred = to; }
            }
        }
        from->mutable_successors()->clear();
    }
    
    void AddSuccessor(HBasicBlock *bb, HBasicBlock *succ) {
        bb->AddSuccessor(succ);
        succ->AddPredecessor(bb);
    }
    
    void EnsureCFGWellFormedness();
    
    void EliminateRedundantPhiNodes();
    
#if defined(DEBUG) || defined(_DEBUG)
    void EnsureSplitEdgeForm(HBasicBlock *block) {
        DCHECK_GT(block->predecessors_size(), 1);
        DCHECK_NE(block, end_);
        for (auto pred : block->predecessors()) {
            DCHECK_LE(pred->successors_size(), 1);
        }
    }
#else
    void EnsureSplitEdgeForm(HBasicBlock *block) {}
#endif
    
    void MovePhis(HBasicBlock *from, HBasicBlock *to);
    
    void PropagateDeferredMark();

    base::Arena *const arena_;
    base::ArenaVector<HBasicBlock *> all_blocks_;
    base::ArenaVector<HBasicBlock *> nodeid_to_block_;
    base::ArenaVector<HBasicBlock *> rpo_order_; // Reverse-post-order block list.
    HBasicBlock *start_;
    HBasicBlock *end_;
}; // class HSchedule


class ControlEquivalence final : public base::ArenaObject {
public:
    ControlEquivalence(base::Arena *arena, HGraph *graph)
        : arena_(arena)
        , graph_(graph) {}

    void Run(HNode *node) {}
    size_t ClassOf(HNode *node) { return 0; }
    
private:
    static constexpr size_t kInvalidClass = static_cast<size_t>(-1);
    enum DFSDirection { kInputDirection, kUseDirection };
    
    struct Bracket {
        DFSDirection direction;  // Direction in which this bracket was added.
        size_t recent_class;     // Cached class when bracket was topmost.
        size_t recent_size;      // Cached set-size when bracket was topmost.
        HNode* from;             // Node that this bracket originates from.
        HNode* to;               // Node that this bracket points to.
    };
    
    base::Arena *const arena_;
    HGraph *const graph_;
    int dfs_number_ = 0;
    int class_number_ = 1;
    
}; // class ControlEquivalence


struct Scheduling final {
    
    // Flags that control the mode of operation.
    enum Flag { kNoFlags = 0u, kSplitNodes = 1u << 1, kTempSchedule = 1u << 2 };

    // The complete scheduling algorithm. Creates a new schedule and places all
    // nodes from the graph into it.
    static HSchedule* ComputeSchedule(base::Arena *arena, HGraph* graph, uint32_t flags);

    // Compute the RPO of blocks in an existing schedule.
    static base::ArenaVector<HBasicBlock*> *ComputeSpecialRPO(base::Arena *arena,
                                                              HSchedule* schedule);
    // Computes the dominator tree on an existing schedule that has RPO computed.
    static void GenerateDominatorTree(HSchedule* schedule);
    
    DISALLOW_ALL_CONSTRUCTORS(Scheduling);
};

} // namespace lang

} // namespace mai

#endif // MAI_LANG_HIR_SCHEDULE_H_
