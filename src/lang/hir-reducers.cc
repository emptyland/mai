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
    V(StoreWord64Field)

#define DECLARE_LOAD_STORE_ARRAY_AT(V) \
    V(LoadArrayAt) \
    V(LoadWord8ArrayAt) \
    V(LoadWord16ArrayAt) \
    V(LoadWord32ArrayAt) \
    V(LoadWord64ArrayAt) \
    V(StoreArrayAt) \
    V(StoreWord8ArrayAt) \
    V(StoreWord16ArrayAt) \
    V(StoreWord32ArrayAt) \
    V(StoreWord64ArrayAt)

Reduction NilUncheckedLowering::Reduce(HNode *node) /*final*/ {
    if (auto iter = checked_record_.find(node->vid()); iter != checked_record_.end()) {
        DCHECK_EQ(iter->second, node);
        
        switch (node->opcode()) {
                
        #define DEFINE_NIL_CHECKED_CASE(name) \
            case H##name: { \
                const HOperator *op = ops_->Unchecked##name(node->op()->control_in(), \
                                                            node->op()->value_in(), \
                                                            HOperatorWith<int32_t>::Data(node)); \
                return Replace(graph_->NewNodeWithInputs(op, node->type(), node->inputs_size(), \
                                                         node->inputs())); \
            } break;
                
            DECLARE_LOAD_STORE_FIELD(DEFINE_NIL_CHECKED_CASE)

        #undef DEFINE_NIL_CHECKED_CASE
        
        #define DEFINE_NIL_CHECKED_CASE(name) \
            case H##name: { \
                const HOperator *op = ops_->Unchecked##name(node->op()->control_in(), \
                                                            node->op()->value_in()); \
                return Replace(graph_->NewNodeWithInputs(op, node->type(), node->inputs_size(), \
                                                         node->inputs())); \
            } break;
                
            DECLARE_LOAD_STORE_ARRAY_AT(DEFINE_NIL_CHECKED_CASE)

        #undef DEFINE_NIL_CHECKED_CASE
                
            default:
                NOREACHED();
                break;
        }
        return NoChange();
    }
    
    switch (node->opcode()) {

        case HNewObject: {
            HNode::UseIterator iter(node);
            for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
               if (IsNilCheckedOperator(iter.user()->op())) {
                   checked_record_[iter.user()->vid()] = iter.user();
               }
            }
        } break;
            
    #define DEFINE_NIL_CHECKED_CASE(name) case H##name:
        DECLARE_LOAD_STORE_FIELD(DEFINE_NIL_CHECKED_CASE)
        DECLARE_LOAD_STORE_ARRAY_AT(DEFINE_NIL_CHECKED_CASE) {
            HNode *first = NodeOps::GetValueInput(node, 0);
            HNode::UseIterator iter(first);
            for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
                if (iter.user() != node && IsNilCheckedOperator(iter.user()->op())) {
                    checked_record_[iter.user()->vid()] = iter.user();
                }
            }
        } break;
    #undef DEFINE_NIL_CHECKED_CASE

        default:
            break;
    }
    return NoChange();
}

bool NilUncheckedLowering::IsNilCheckedOperator(const HOperator *op) const {
    switch (op->value()) {
    #define DEFINE_NIL_CHECKED_CASE(name) case H##name:
        DECLARE_LOAD_STORE_FIELD(DEFINE_NIL_CHECKED_CASE)
        DECLARE_LOAD_STORE_ARRAY_AT(DEFINE_NIL_CHECKED_CASE)
    #undef DEFINE_NIL_CHECKED_CASE
            return true;
        default:
            break;
    }
    return false;
}

void NilUncheckedLowering::Finalize() /*final*/ {
}

} // namespace lang

} // namespace mai
