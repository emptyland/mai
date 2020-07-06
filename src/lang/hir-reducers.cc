#include "lang/hir-reducers.h"

namespace mai {

namespace lang {

SimplifiedElimination::~SimplifiedElimination() = default;

Reduction SimplifiedElimination::Reduce(HNode *node) /*final*/ {
    switch (node->opcode()) {
            
#define DEFINE_INTEGER_ARITH_OPS(bits, kbits) \
    case HWord##bits##Add: \
    case HWord##bits##Sub: \
    case HUInt##bits##Mul: \
    case HUInt##bits##Div: \
    case HWord##bits##Mod: \
        if (NodeOps::IsConstant(node->input(0)) && NodeOps::IsConstant(node->input(1))) { \
            DCHECK_EQ(node->input(0)->opcode(), node->input(1)->opcode()); \
            DCHECK_EQ(HWord##kbits##Constant, node->input(0)->opcode()); \
            auto lhs = static_cast<uint##bits##_t>(HOperatorWith<uint32_t>::Data(node->input(0))); \
            auto rhs = static_cast<uint##bits##_t>(HOperatorWith<uint32_t>::Data(node->input(1))); \
            if (node->opcode() == HWord##bits##Add) { \
                return Replace(graph_->NewNode(ops_->Word##kbits##Constant(lhs + rhs), \
                                               HTypes::Word8)); \
            } else if (node->opcode() == HWord8Sub) { \
                return Replace(graph_->NewNode(ops_->Word##kbits##Constant(lhs - rhs), \
                                               HTypes::Word8)); \
            } else if (node->opcode() == HUInt8Mul) { \
                return Replace(graph_->NewNode(ops_->Word##kbits##Constant(lhs * rhs), \
                                               HTypes::Word8)); \
            } else if (node->opcode() == HUInt8Div) { \
                if (rhs != 0) { \
                    return Replace(graph_->NewNode(ops_->Word##kbits##Constant(lhs / rhs), \
                                                   HTypes::Word8)); \
                } \
            } else { \
                if (rhs != 0) { \
                    return Replace(graph_->NewNode(ops_->Word##kbits##Constant(lhs % rhs), \
                                                   HTypes::Word8)); \
                } \
            } \
        } break; \
    case HInt##bits##Mul: \
    case HInt##bits##Div: \
        if (NodeOps::IsConstant(node->input(0)) && NodeOps::IsConstant(node->input(1))) { \
            DCHECK_EQ(node->input(0)->opcode(), node->input(1)->opcode()); \
            DCHECK_EQ(HWord##kbits##Constant, node->input(0)->opcode()); \
            auto lhs = static_cast<int##bits##_t>(HOperatorWith<int32_t>::Data(node->input(0))); \
            auto rhs = static_cast<int##bits##_t>(HOperatorWith<int32_t>::Data(node->input(1))); \
            if (node->opcode() == HInt8Mul) { \
                return Replace(graph_->NewNode(ops_->Word##kbits##Constant(lhs * rhs), \
                                               HTypes::Word8)); \
            } else if (node->opcode() == HInt8Div) { \
                if (rhs != 0) { \
                    return Replace(graph_->NewNode(ops_->Word##kbits##Constant(lhs / rhs), \
                                                   HTypes::Word8)); \
                } \
            } \
        } break

        DEFINE_INTEGER_ARITH_OPS(8, 32);
        DEFINE_INTEGER_ARITH_OPS(16, 32);
        DEFINE_INTEGER_ARITH_OPS(32, 32);
        DEFINE_INTEGER_ARITH_OPS(64, 64);
            
#undef DEFINE_INTEGER_ARITH_OPS
        default:
            break;
    }
    return NoChange();
}

void SimplifiedElimination::Finalize() /*final*/ {
    
}

NilUncheckedLowering::NilUncheckedLowering(Editor *editor, HGraph *graph, HOperatorFactory *ops)
    : AdvancedReducer(editor)
    , graph_(graph)
    , ops_(ops)
    , checked_record_(graph->arena()) {}

NilUncheckedLowering::~NilUncheckedLowering() /*override*/ = default;

#define DECLARE_LOAD_STORE_FIELD(V) \
    V(LoadField) \
    V(LoadWord8Field) \
    V(LoadWord16Field) \
    V(LoadWord32Field) \
    V(LoadWord64Field) \
    V(StoreField) \
    V(StoreWord8Field) \
    V(StoreWord16Field) \
    V(StoreWord32Field) \
    V(StoreWord64Field) \

Reduction NilUncheckedLowering::Reduce(HNode *node) /*final*/ {
    
    switch (node->opcode()) {

        case HNewObject:
            checked_record_[node->vid()] = 0;
            break;

#define DEFINE_UNCHECKED_CASE(name) \
    case H##name: \
        if (CanUncheckNil(node)) { \
            const HOperator *op = ops_->Unchecked##name(node->op()->control_in(), \
                                                        node->op()->value_in(), \
                                                        HOperatorWith<int32_t>::Data(node)); \
            return Replace(graph_->NewNodeWithInputs(op, node->type(), node->inputs_size(), \
                                                     node->inputs())); \
        } break;
        DECLARE_LOAD_STORE_FIELD(DEFINE_UNCHECKED_CASE)
#undef DEFINE_UNCHECKED_CASE

        default:
            break;
    }
    return NoChange();
}

void NilUncheckedLowering::Finalize() /*final*/ {
}

} // namespace lang

} // namespace mai
