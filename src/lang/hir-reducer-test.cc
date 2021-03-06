#include "lang/hir-reducers.h"
#include "lang/hir-reducer.h"
#include "base/arenas.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

namespace {

class SanitizerReducer : public AdvancedReducer {
public:
    SanitizerReducer(Editor *editor): AdvancedReducer(editor) {}
    ~SanitizerReducer() override = default;
    
    const char *GetReducerName() const final { return "sanitizer"; }
    
    Reduction Reduce(HNode *node) final {
        sequence_.push_back(node);
        //printf("[%d] %s\n", node->vid(), node->op()->name());
        return NoChange();
    }

    void Finalize() final {}
        
    std::vector<HNode *> sequence_;
}; // class SanitizerReducer

} // internal

class HIRReducerTest : public ::testing::Test {
public:
    HIRReducerTest() : factory_(&arena_) {}
    
    std::vector<HNode *> ReduceSequence(HGraph *graph) {
        GraphReducer graph_reducer(&arena_, graph);
        SanitizerReducer sanitizer(&graph_reducer);
        
        graph_reducer.AddReducer(&sanitizer);
        graph_reducer.ReduceGraph();
        return std::move(sanitizer.sequence_);
    }
    
    base::StandaloneArena arena_;
    HOperatorFactory factory_;
};

TEST_F(HIRReducerTest, Sanity) {
    HGraph *graph = new (&arena_) HGraph(&arena_);
    HNode *begin = graph->NewNode(factory_.Begin(), HTypes::Void);
    HNode *lhs = graph->NewNode(factory_.Parameter(0), HTypes::Word32);
    HNode *rhs = graph->NewNode(factory_.Parameter(1), HTypes::Word32);
    HNode *add = graph->NewNode(factory_.Word32Add(), HTypes::Word32, lhs, rhs);
    HNode *end = graph->NewNode(factory_.End(1, 1), HTypes::Void, add, begin);
    
    graph->set_start(begin);
    graph->set_end(end);
    
    auto seq = ReduceSequence(graph);
    EXPECT_EQ(lhs,   seq[0]);
    EXPECT_EQ(rhs,   seq[1]);
    EXPECT_EQ(add,   seq[2]);
    EXPECT_EQ(begin, seq[3]);
    EXPECT_EQ(end,   seq[4]);
}

TEST_F(HIRReducerTest, BranchSequence) {
    HGraph *graph = new (&arena_) HGraph(&arena_);
   
    HNode *begin = graph->NewNode(factory_.Begin());
    HNode *x = graph->NewNode(factory_.Parameter(0), HTypes::Word8);
    HNode *branch = graph->NewNode(factory_.Branch(1, 1), HTypes::Void, x, begin);
    HNode *if_true = graph->NewNode(factory_.IfTrue(0), HTypes::Void, branch);
    HNode *if_false = graph->NewNode(factory_.IfFalse(0), HTypes::Void, branch);
    HNode *merge = graph->NewNode(factory_.Merge(2), HTypes::Void, if_true, if_false);
    HNode *k1 = graph->NewNode(factory_.Word32Constant(1), HTypes::Word32);
    HNode *k2 = graph->NewNode(factory_.Word32Constant(2), HTypes::Word32);
    HNode *phi = graph->NewNode(factory_.Phi(1, 2), HTypes::Word32, k1, k2, branch);
    HNode *end = graph->NewNode(factory_.End(1, 1), HTypes::Void, phi, merge);

    graph->set_start(begin);
    graph->set_end(end);
    
    auto seq = ReduceSequence(graph);
    EXPECT_EQ(k1,       seq[0]);
    EXPECT_EQ(k2,       seq[1]);
    EXPECT_EQ(x,        seq[2]);
    EXPECT_EQ(begin,    seq[3]);
    EXPECT_EQ(branch,   seq[4]);
    EXPECT_EQ(phi,      seq[5]);
    EXPECT_EQ(if_true,  seq[6]);
    EXPECT_EQ(if_false, seq[7]);
    EXPECT_EQ(merge,    seq[8]);
    EXPECT_EQ(end,      seq[9]);
}

TEST_F(HIRReducerTest, ForLoopSequence) {
    // for (int i = 0; i < n; i++) { ... }
    HGraph *graph = new (&arena_) HGraph(&arena_);
    HNode *begin = graph->NewNode(factory_.Begin());
    HNode *loop = graph->NewNode(factory_.Loop(), HTypes::Void, begin);
    HNode *n = graph->NewNode(factory_.Parameter(0), HTypes::Int32);
    HNode *zero = graph->NewNode(factory_.Word32Constant(0), HTypes::Int32);
    HNode *one = graph->NewNode(factory_.Word32Constant(1), HTypes::Int32);
    HNode *phi = graph->NewNodeReserved(factory_.Phi(1, 2), HTypes::Int32, 4);
    HNode *add = graph->NewNode(factory_.Word32Add(), HTypes::Int32, phi, one);
    phi->AppendInput(&arena_, zero);
    phi->AppendInput(&arena_, add);
    phi->AppendInput(&arena_, loop);
    HNode *less_than = graph->NewNode(factory_.Word32LessThan(), HTypes::Word8, phi, n);
    HNode *branch = graph->NewNode(factory_.Branch(1, 1), HTypes::Void, less_than, loop);
    HNode *if_true = graph->NewNode(factory_.IfTrue(0), HTypes::Void, branch);
    HNode *if_false = graph->NewNode(factory_.IfFalse(0), HTypes::Void, branch);
    HNode *loop_exit = graph->NewNode(factory_.LoopExit(2), HTypes::Void, if_true, loop);
    HNode *end = graph->NewNode(factory_.End(2, 0), HTypes::Void, if_false, loop_exit);

    graph->set_start(begin);
    graph->set_end(end);
    
    auto seq = ReduceSequence(graph);
    EXPECT_EQ(zero,      seq[0]);
    EXPECT_EQ(one,       seq[1]);
    EXPECT_EQ(add,       seq[2]);
    EXPECT_EQ(begin,     seq[3]);
    EXPECT_EQ(loop,      seq[4]);
    EXPECT_EQ(phi,       seq[5]);
    EXPECT_EQ(n,         seq[6]);
    EXPECT_EQ(less_than, seq[7]);
    EXPECT_EQ(branch,    seq[8]);
    EXPECT_EQ(if_false,  seq[9]);
    EXPECT_EQ(if_true,   seq[10]);
    EXPECT_EQ(loop_exit, seq[11]);
    EXPECT_EQ(end,       seq[12]);
}

TEST_F(HIRReducerTest, LoadStoreFieldAndFrameState) {
    HGraph *graph = new (&arena_) HGraph(&arena_);
    HNode *begin = graph->NewNode(factory_.Begin());
    HNode *k1 = graph->NewNode(factory_.Word32Constant(1));
    HNode *frame_state = graph->NewNode(factory_.FrameState(0, 1, 1/*bci*/, 64/*offset*/),
                                        HTypes::Void, k1);
    HNode *p1 = graph->NewNode(factory_.Parameter(0), HTypes::Any);
    HNode *store_field = graph->NewNode(factory_.StoreWord32Field(1, 3, 32), HTypes::Void,
                                        p1, k1, frame_state, begin);
    HNode *end = graph->NewNode(factory_.End(1, 0), HTypes::Void, store_field);
    
    graph->set_start(begin);
    graph->set_end(end);
    
    auto seq = ReduceSequence(graph);
    EXPECT_EQ(p1,          seq[0]);
    EXPECT_EQ(k1,          seq[1]);
    EXPECT_EQ(frame_state, seq[2]);
    EXPECT_EQ(begin,       seq[3]);
    EXPECT_EQ(store_field, seq[4]);
    EXPECT_EQ(end,         seq[5]);
}

TEST_F(HIRReducerTest, ConstantFloding) {
    HGraph *graph = new (&arena_) HGraph(&arena_);
    
    HNode *begin = graph->NewNode(factory_.Begin());
    HNode *two = graph->NewNode(factory_.Word32Constant(2), HTypes::Int32);
    HNode *one = graph->NewNode(factory_.Word32Constant(1), HTypes::Int32);
    HNode *add = graph->NewNode(factory_.Word8Add(), HTypes::Int8, two, one);
    HNode *end = graph->NewNode(factory_.End(1, 1), HTypes::Void, begin, add);
    
    graph->set_start(begin);
    graph->set_end(end);
    
    GraphReducer graph_reducer(&arena_, graph);
    SimplifiedElimination reducer(&graph_reducer, graph, &factory_);

    graph_reducer.AddReducer(&reducer);
    graph_reducer.ReduceGraph();

    EXPECT_EQ(3, HOperatorWith<int32_t>::Data(end->input(1)));
    EXPECT_EQ(begin, end->input(0));
}

TEST_F(HIRReducerTest, ConstantFloding2) {
    HGraph *graph = new (&arena_) HGraph(&arena_);
    
    HNode *begin = graph->NewNode(factory_.Begin());
    HNode *two = graph->NewNode(factory_.Word32Constant(2), HTypes::Int32);
    HNode *one = graph->NewNode(factory_.Word32Constant(1), HTypes::Int32);
    HNode *tree = graph->NewNode(factory_.Word32Add(), HTypes::Int32, two, one);
    HNode *add = graph->NewNode(factory_.Word32Add(), HTypes::Int32, two, tree);
    HNode *end = graph->NewNode(factory_.End(1, 1), HTypes::Void, begin, add);
    
    graph->set_start(begin);
    graph->set_end(end);
    
    GraphReducer graph_reducer(&arena_, graph);
    SimplifiedElimination reducer(&graph_reducer, graph, &factory_);

    graph_reducer.AddReducer(&reducer);
    graph_reducer.ReduceGraph();
    
    EXPECT_EQ(HWord32Constant, end->input(1)->opcode());
    EXPECT_EQ(5, HOperatorWith<int32_t>::Data(end->input(1)));
    EXPECT_EQ(begin, end->input(0));
}

TEST_F(HIRReducerTest, UncheckedLoadField) {
    HGraph *graph = new (&arena_) HGraph(&arena_);
    
    HNode *begin = graph->NewNode(factory_.Begin());
    HNode *obj = graph->NewNode(factory_.NewObject(0, 0, nullptr));
    HNode *load_field = graph->NewNode(factory_.LoadField(0, 1, 64), HTypes::Any, obj);
    HNode *end = graph->NewNode(factory_.End(1, 1), HTypes::Void, load_field, begin);
    
    graph->set_start(begin);
    graph->set_end(end);
    
    GraphReducer graph_reducer(&arena_, graph);
    NilUncheckedLowering reducer(&graph_reducer, graph, &factory_);
    
    graph_reducer.AddReducer(&reducer);
    graph_reducer.ReduceGraph();

    EXPECT_EQ(HUncheckedLoadField, end->input(0)->opcode());
}

TEST_F(HIRReducerTest, UncheckedStoreField) {
    HGraph *graph = new (&arena_) HGraph(&arena_);
    
    HNode *begin = graph->NewNode(factory_.Begin());
    HNode *obj = graph->NewNode(factory_.Parameter(0), HTypes::Any);
    HNode *p1 = graph->NewNode(factory_.Parameter(1), HTypes::Any);
    HNode *p2 = graph->NewNode(factory_.Parameter(2), HTypes::Any);
    HNode *store_field1 = graph->NewNode(factory_.StoreField(1, 2, 64), HTypes::Void, obj, p1, begin);
    HNode *store_field2 = graph->NewNode(factory_.StoreField(1, 2, 64), HTypes::Void, obj, p2, store_field1);
    HNode *end = graph->NewNode(factory_.End(1, 0), HTypes::Void, store_field2);
    
    graph->set_start(begin);
    graph->set_end(end);
    
    GraphReducer graph_reducer(&arena_, graph);
    NilUncheckedLowering reducer(&graph_reducer, graph, &factory_);
    
    graph_reducer.AddReducer(&reducer);
    graph_reducer.ReduceGraph();
 
    store_field2 = end->input(0);
    EXPECT_EQ(HUncheckedStoreField, store_field2->opcode());
    store_field1 = NodeOps::GetControlInput(store_field2, 0);
    EXPECT_EQ(HStoreField, store_field1->opcode());
}

} // namespace lang

} // namespace mai
