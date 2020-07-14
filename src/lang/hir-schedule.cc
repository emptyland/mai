#include "lang/hir-schedule.h"
#include "lang/hir.h"

namespace mai {

namespace lang {

HBasicBlock::HBasicBlock(base::Arena *arena, Id id)
    : id_(id)
    , nodes_(arena)
    , successors_(arena)
    , predecessors_(arena) {
}

HSchedule::HSchedule(base::Arena *arena, int node_count_hint)
    : arena_(arena)
    , all_blocks_(arena)
    , nodeid_to_block_(arena)
    , rpo_order_(arena)
    , start_(NewBlock())
    , end_(NewBlock()) {
    nodeid_to_block_.reserve(node_count_hint);
}

HBasicBlock *HSchedule::block(const HNode *node) const {
    return node->vid() < nodeid_to_block_.size() ? nodeid_to_block_[node->vid()] : nullptr;
}

void HSchedule::InsertBranch(HBasicBlock *bb, HBasicBlock *end, HNode *branch, HBasicBlock *if_true,
                  HBasicBlock *if_false) {
    DCHECK_EQ(bb->control(), HBasicBlock::kNone);
    DCHECK_EQ(end->control(), HBasicBlock::kNone);
    DCHECK_EQ(branch->opcode(), HBranch);
    end->set_control(bb->control());
    bb->set_control(HBasicBlock::kBranch);
    MoveSuccessor(bb, end);
    AddSuccessor(bb, if_true);
    AddSuccessor(bb, if_false);
    if (bb->control_input() != nullptr) {
        SetControlInput(end, bb->control_input());
    }
    SetControlInput(bb, branch);
}

bool HSchedule::IsScheduled(const HNode *node) const {
    if (node->vid() >= nodeid_to_block_.size()) {
        return false;
    }
    return nodeid_to_block_[node->vid()] != nullptr;
}

void HSchedule::SetBlockForNode(HBasicBlock *block, HNode *node) {
    if (node->vid() >= nodeid_to_block_.size()) {
        nodeid_to_block_.resize(node->vid() + 1);
    }
    nodeid_to_block_[node->vid()] = block;
}

void HSchedule::EnsureCFGWellFormedness() {
    base::ArenaVector<HBasicBlock *> copied_all_blocks(all_blocks_);
    for (auto block : copied_all_blocks) {
        if (block->predecessors_size() > 1 && block != end_) {
            EnsureSplitEdgeForm(block);
        }
    }
    EliminateRedundantPhiNodes();
}

void HSchedule::EliminateRedundantPhiNodes() {
    // Ensure that useless phi nodes that only have a single input, identical
    // inputs, or are a self-referential loop phi,
    // -- which can happen with the automatically generated code in the CSA and
    // torque -- are pruned.
    // Since we have strucured control flow, this is enough to minimize the number
    // of phi nodes.
    bool reached_fixed_point = false;
    while (!reached_fixed_point) {
        reached_fixed_point = true;
        
        for (auto block : all_blocks_) {
            const size_t pred_count = block->predecessors_size();
            for (size_t i = 0; i < block->nodes_size(); i++) {
                HNode *node = block->node(i);
                if (node->opcode() == HPhi) {
                    HNode *first_input = node->input(0);
                    bool inputs_equal = true;
                    
                    for (int j = 1; j < pred_count; j++) {
                        HNode *input = node->input(j);
                        if (input != first_input && input != node) {
                            inputs_equal = false;
                            break;
                        }
                    }
                    if (!inputs_equal) {
                        continue;
                    }

                    node->ReplaceUses(first_input);
                    node->Kill();
                    block->RemoveNode(i);
                    --i;
                    reached_fixed_point = false;
                }
            }
        }
    }
}
    
void HSchedule::MovePhis(HBasicBlock *from, HBasicBlock *to) {
    for (size_t i = 0; i < from->nodes_size();) {
        HNode *node = from->node(i);
        if (node->opcode() == HPhi) {
            to->AddNode(node);
            from->RemoveNode(i);
            DCHECK_EQ(nodeid_to_block_[node->vid()], from);
            nodeid_to_block_[node->vid()] = to;
        } else {
            i++;
        }
    }
}
    
void HSchedule::PropagateDeferredMark() {
    // Push forward the deferred block marks through newly inserted blocks and
    // other improperly marked blocks until a fixed point is reached.
    // TODO(danno): optimize the propagation
    bool done = false;
    while (!done) {
        done = true;
        for (auto block : all_blocks_) {
            if (!block->deferred()) {
                bool deferred = block->predecessors_size() > 0;
                for (auto pred : block->predecessors()) {
                    if (!pred->deferred() && (pred->rpo_number() < block->rpo_number())) {
                        deferred = false;
                    }
                }
                if (deferred) {
                    block->set_deferred(true);
                    done = false;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
// class ControlEquivalence
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
// class HScheduler
//--------------------------------------------------------------------------------------------------

class HScheduler final {
public:
    HScheduler(base::Arena *arena, HGraph *graph, HSchedule *schedule, uint32_t flags,
               size_t node_count_hint);
    
    void BuildCFG();
private:
    // Placement of a node changes during scheduling. The placement state
    // transitions over time while the scheduler is choosing a position:
    //
    //                   +---------------------+-----+----> kFixed
    //                  /                     /     /
    //    kUnknown ----+------> kCoupled ----+     /
    //                  \                         /
    //                   +----> kSchedulable ----+--------> kScheduled
    //
    // 1) InitializePlacement(): kUnknown -> kCoupled|kSchedulable|kFixed
    // 2) UpdatePlacement(): kCoupled|kSchedulable -> kFixed|kScheduled
    //
    // We maintain the invariant that all nodes that are not reachable
    // from the end have kUnknown placement. After the PrepareUses phase runs,
    // also the opposite is true - all nodes with kUnknown placement are not
    // reachable from the end.
    enum Placement { kUnknown, kSchedulable, kFixed, kCoupled, kScheduled };
    
    struct Data {
        HBasicBlock *min_block;
        int unscheduled_count;
        Placement placement;
    };
    
    class CFGBuilder;
    class SpecialRPONumberer;
    
    Data DefaultData() const { return {schedule_->start(), 0, kUnknown}; }

    Data *NodeData(const HNode *node) {
        DCHECK_GE(node->vid(), 0);
        DCHECK_LT(node->vid(), node_data_.size());
        return &node_data_[node->vid()];
    }
    
    const Data &NodeData(const HNode *node) const {
        DCHECK_GE(node->vid(), 0);
        DCHECK_LT(node->vid(), node_data_.size());
        return node_data_[node->vid()];
    }
    
    void UpdatePlacement(HNode *node, Placement placement);
    
    Placement GetPlacement(const HNode *node) const { return NodeData(node).placement; }
    
    void DecrementUnscheduledUseCount(HNode *node, int index, HNode *from);
    void IncrementUnscheduledUseCount(HNode *node, int index, HNode *from);
    
    bool IsCoupledControlEdge(HNode *node, int index) {
        return GetPlacement(node) == kCoupled && NodeOps::GetControlIndex(node) == index;
    }
    
    bool IsLive(const HNode *node) const { return GetPlacement(node) != kUnknown; }
    
    base::Arena *const arena_;
    HGraph *const graph_;
    HSchedule *const schedule_;
    uint32_t flags_;
    base::ArenaVector<base::ArenaVector<HNode *> *> scheduled_nodes_;
    base::ArenaVector<HNode *> schedule_root_nodes_;
    base::ArenaVector<Data> node_data_;
    base::ArenaDeque<HNode *> schedule_queue_;
    CFGBuilder *control_flow_builder_ = nullptr;
    SpecialRPONumberer *special_rpo_ = nullptr;
    ControlEquivalence *equivalence_ = nullptr;
}; // class HScheduler


class HScheduler::CFGBuilder final : public base::ArenaObject {
public:
    CFGBuilder(base::Arena *arena, HScheduler *owns)
        : arena_(arena)
        , owns_(owns)
        , schedule_(owns->schedule_)
        , queue_(arena)
        , control_(arena) {}
    
    void Run();
    void Run(HBasicBlock *block, HNode *exit);

    DISALLOW_IMPLICIT_CONSTRUCTORS(CFGBuilder);
private:
    
    void FixNode(HBasicBlock *block, HNode *node) {
        schedule_->AddNode(block, node);
        owns_->UpdatePlacement(node, kFixed);
    }

    void Queue(HNode *node) {
        if (!queued_.Get(node)) {
            BuildBlocks(node);
            queue_.push_back(node);
            queued_.Set(node, true);
            control_.push_back(node);
        }
    }
    
    void ConnectBlocks(HNode *node);
    
    void ConnectMerge(HNode *merge);
    
    void ConnectBranch(HNode *branch);
    
    void ConnectDeoptimize(HNode *deopt) {
        HNode *control = NodeOps::GetControlInput(deopt, 0);
        HBasicBlock *block = schedule_->block(control);
        schedule_->AddDeoptimize(block, deopt);
    }
    
    void ConnectTailCall(HNode *call) {
        HNode *control = NodeOps::GetControlInput(call, 0);
        HBasicBlock *block = schedule_->block(control);
        schedule_->AddTailCall(block, call);
    }
    
    void ConnectReturn(HNode *ret) {
        HNode *control = NodeOps::GetControlInput(ret, 0);
        HBasicBlock *block = schedule_->block(control);
        schedule_->AddReturn(block, ret);
    }
    
    void ConnectThrow(HNode *node) {
        HNode *control = NodeOps::GetControlInput(node, 0);
        HBasicBlock *block = schedule_->block(control);
        schedule_->AddThrow(block, node);
    }

    void BuildBlocks(HNode *node);
    
    HBasicBlock *BuildBlockForNode(HNode *node) {
        HBasicBlock *block = schedule_->block(node);
        if (!block) {
            block = schedule_->NewBlock();
            FixNode(block, node);
        }
        return block;
    }
    
    HBasicBlock *FindPredecessorBlock(HNode *node) {
        HBasicBlock *block = nullptr;
        while (true) {
            block = schedule_->block(node);
            if (block) {
                break;
            }
            node = NodeOps::GetControlInput(node, 0);
        }
        return block;
    }
    
    bool IsSingleEntrySingleExitRegion(HNode *entry, HNode *exit) {
        size_t entry_class = owns_->equivalence_->ClassOf(entry);
        size_t exit_class = owns_->equivalence_->ClassOf(exit);
        return entry != exit && entry_class == exit_class;
    }
    
    bool IsFinalMerge(HNode *node) {
        return node->opcode() == HMerge && node == owns_->graph_->end()->input(0);
    }
    
    void Reset() {
        control_.clear();
        DCHECK(queue_.empty());
    }
    
    base::Arena *const arena_;
    HScheduler *const owns_;
    HSchedule *const schedule_;
    HNodeState<bool, 4, 8> queued_;
    base::ArenaDeque<HNode *> queue_;
    base::ArenaVector<HNode *> control_;
    HNode *component_entry_ = nullptr;
    HBasicBlock *component_start_ = nullptr;
    HBasicBlock *component_end_ = nullptr;
}; // class HScheduler::CFGBuilder

void HScheduler::CFGBuilder::Run() {
    Reset();
    Queue(owns_->graph_->end());
    while (!queue_.empty()) {
        HNode *node = queue_.front();
        queue_.pop_front();

        for (int i = 0; i < node->op()->control_in(); i++) {
            Queue(NodeOps::GetControlInput(node, i));
        }
    }

    for (auto node : control_) { ConnectBlocks(node); }
}

void HScheduler::CFGBuilder::Run(HBasicBlock *block, HNode *exit) {
    Reset();
    Queue(exit);
    
    component_entry_ = nullptr;
    component_start_ = block;
    component_end_   = schedule_->block(exit);
    owns_->equivalence_->Run(exit);
    
    while (!queue_.empty()) {
        HNode *node = queue_.front();
        queue_.pop_front();
        
        if (IsSingleEntrySingleExitRegion(node, exit)) {
            DCHECK(component_entry_ == nullptr);
            component_entry_ = node;
            continue;
        }
        
        for (int i = 0; i < node->op()->control_in(); i++) {
            Queue(NodeOps::GetControlInput(node, i));
        }
    }
    DCHECK(component_entry_ != nullptr);
    
    for (auto node : control_) { ConnectBlocks(node); }
}

void HScheduler::CFGBuilder::ConnectBlocks(HNode *node) {
    switch (node->opcode()) {
        case HLoop:
        case HMerge:
            ConnectMerge(node);
            break;
        case HBranch:
            owns_->UpdatePlacement(node, kFixed);
            ConnectBranch(node);
            break;
        case HDeoptimize:
            owns_->UpdatePlacement(node, kFixed);
            ConnectDeoptimize(node);
            break;
        case HTailCall:
            owns_->UpdatePlacement(node, kFixed);
            ConnectTailCall(node);
            break;
        case HReturn:
            owns_->UpdatePlacement(node, kFixed);
            ConnectReturn(node);
            break;
        case HThrow:
            owns_->UpdatePlacement(node, kFixed);
            ConnectThrow(node);
            break;
        case HCallVtab:
        case HCallInline:
        case HCallNative:
        case HCallBytecode:
            // TODO:
        default:
            break;
    }
}

void HScheduler::CFGBuilder::ConnectMerge(HNode *merge) {
    DCHECK_EQ(merge->opcode(), HMerge);
    if (IsFinalMerge(merge)) {
        return;
    }
    HBasicBlock *block = DCHECK_NOTNULL(schedule_->block(merge));
    for (int i = 0; i < merge->inputs_size(); i++) {
        HNode *input = merge->input(i);
        HBasicBlock *pred_block = FindPredecessorBlock(input);
        schedule_->AddGoto(pred_block, block);
    }
}

void HScheduler::CFGBuilder::ConnectBranch(HNode *branch) {
    DCHECK_EQ(branch->opcode(), HBranch);
    HNode *projections[2];
    NodeOps::CollectControlProjections(branch, projections, 2);
    HBasicBlock *blocks[2];
    blocks[0] = schedule_->block(projections[0]);
    blocks[1] = schedule_->block(projections[1]);
    
    switch (BranchHint::Of(branch->op())) {
        case BranchHint::kLikely:
            blocks[1]->set_deferred(true);
            break;
        case BranchHint::kUnlikely:
            blocks[0]->set_deferred(true);
            break;
        case BranchHint::kNone:
            break;
        default:
            NOREACHED();
            break;
    }
    
    if (branch == component_entry_) {
        schedule_->InsertBranch(component_start_, component_end_, branch, blocks[0], blocks[1]);
    } else {
        HNode *control = NodeOps::GetControlInput(branch, 0);
        HBasicBlock *block = schedule_->block(control);
        schedule_->AddBranch(block, branch, blocks[0], blocks[1]);
    }
}

void HScheduler::CFGBuilder::BuildBlocks(HNode *node) {
    switch (node->opcode()) {
        case HBegin:
            FixNode(schedule_->start(), node);
            break;
        case HEnd:
            FixNode(schedule_->end(), node);
            break;
        case HLoop:
        case HMerge:
            BuildBlockForNode(node);
            break;
        case HTerminate: {
            HNode *loop = NodeOps::GetControlInput(node, 0);
            DCHECK_EQ(loop->opcode(), HLoop);
            HBasicBlock *block = schedule_->block(loop);
            FixNode(block, node);
        } break;
        case HBranch: {
            HNode *successors[2];
            NodeOps::CollectControlProjections(node, successors, 2);
            BuildBlockForNode(successors[0]);
            BuildBlockForNode(successors[1]);
        } break;
            
        case HCallVtab:
        case HCallInline:
        case HCallNative:
        case HCallBytecode:
            // TODO:
        default:
            break;
    }
}


HScheduler::HScheduler(base::Arena *arena, HGraph *graph, HSchedule *schedule, uint32_t flags,
                       size_t node_count_hint)
    : arena_(arena)
    , graph_(graph)
    , schedule_(schedule)
    , flags_(flags)
    , scheduled_nodes_(arena)
    , schedule_root_nodes_(arena)
    , node_data_(arena)
    , schedule_queue_(arena) {
    node_data_.reserve(node_count_hint);
    node_data_.resize(node_count_hint, DefaultData());
}

void HScheduler::BuildCFG() {
    equivalence_ = new (arena_) ControlEquivalence(arena_, graph_);
    
    control_flow_builder_ = new (arena_) CFGBuilder(arena_, this);
    control_flow_builder_->Run();
    
    scheduled_nodes_.reserve(schedule_->all_blocks_size() * 1.1);
    scheduled_nodes_.reserve(schedule_->all_blocks_size());
}

void HScheduler::UpdatePlacement(HNode *node, Placement placement) {
    Data *data = NodeData(node);
    if (data->placement == kUnknown) {
        DCHECK_EQ(placement, kFixed);
        data->placement = placement;
        return;
    }
    
    switch (node->opcode()) {
        case HParameter:
            NOREACHED();
            break;
        
        case HPhi: {
            DCHECK_EQ(kCoupled, data->placement);
            DCHECK_EQ(kFixed, placement);
            HNode *control = NodeOps::GetControlInput(node, 0);
            HBasicBlock *block = schedule_->block(control);
            schedule_->AddNode(block, node);
        } break;

        #define DEFINE_CONTROL_CASE(name) case H##name:
            DECLARE_HIR_CONTROL(DEFINE_CONTROL_CASE)
        #undef DEFINE_CONTROL_CASE
        {
            HNode::UseIterator iter(node);
            for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
                if (GetPlacement(iter.user()) == kCoupled) {
                    DCHECK_EQ(node, NodeOps::GetControlInput(iter.user(), 0));
                    UpdatePlacement(iter.user(), placement);
                }
            }
        } break;
        
        default:
            DCHECK_EQ(kSchedulable, data->placement);
            DCHECK_EQ(kScheduled, placement);
            break;
    }
    
    HNode::UseIterator iter(node);
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        DecrementUnscheduledUseCount(iter.to(), iter.index(), iter.from());
    }
    data->placement = placement;
}

void HScheduler::IncrementUnscheduledUseCount(HNode *node, int index, HNode *from) {
    if (IsCoupledControlEdge(from, index)) {
        return;
    }
    
    if (GetPlacement(node) == kFixed) {
        return;
    }
    
    if (GetPlacement(node) == kCoupled) {
        HNode *control = NodeOps::GetControlInput(node, 0);
        IncrementUnscheduledUseCount(control, index, from);
        return;
    }
    
    Data *node_data = NodeData(node);
    ++(node_data->unscheduled_count);

}

void HScheduler::DecrementUnscheduledUseCount(HNode *node, int index, HNode *from) {
    if (IsCoupledControlEdge(from, index)) {
        return;
    }
    
    if (GetPlacement(node) == kFixed) {
        return;
    }
    
    if (GetPlacement(node) == kCoupled) {
        HNode *control = NodeOps::GetControlInput(node, 0);
        DecrementUnscheduledUseCount(control, index, from);
        return;
    }
    
    Data *node_data = NodeData(node);
    DCHECK_GT(node_data->unscheduled_count, 0);
    --(node_data->unscheduled_count);
    
    if (node_data->unscheduled_count == 0) {
        schedule_queue_.push_back(node);
    }
}


/*static*/
HSchedule* Scheduling::ComputeSchedule(base::Arena *arena, HGraph* graph, uint32_t flags) {
    return nullptr;
}

/*static*/
base::ArenaVector<HBasicBlock*> *Scheduling::ComputeSpecialRPO(base::Arena *arena,
                                                               HSchedule* schedule) {
    
    return nullptr;
}

// Computes the dominator tree on an existing schedule that has RPO computed.
/*static*/ void Scheduling::GenerateDominatorTree(HSchedule* schedule) {
    
}

} // namespace lang

} // namespace mai
