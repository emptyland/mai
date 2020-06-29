#include "lang/hir-reducers.h"

namespace mai {

namespace lang {

SimplifiedElimination::~SimplifiedElimination() = default;

Reduction SimplifiedElimination::Reduce(HNode *node) /*final*/ {
    switch (node->opcode()) {
        case HAdd8:
        case HSub8:
        case HMul8:
        case HDiv8:
        case HMod8:
            if (NodeOps::IsConstant(node->input(0)) && NodeOps::IsConstant(node->input(1))) {
                DCHECK_EQ(node->input(0)->opcode(), node->input(1)->opcode());
                DCHECK_EQ(HConstant32, node->input(0)->opcode());
                
                auto lhs = static_cast<uint8_t>(HOperatorWith<uint32_t>::Data(node->input(0)));
                auto rhs = static_cast<uint8_t>(HOperatorWith<uint32_t>::Data(node->input(1)));

                if (node->opcode() == HAdd8) {
                    return Replace(graph_->NewNode(ops_->Constant32(lhs + rhs), HTypes::Word8));
                } else if (node->opcode() == HSub8) {
                    return Replace(graph_->NewNode(ops_->Constant32(lhs - rhs), HTypes::Word8));
                } else if (node->opcode() == HMul8) {
                    return Replace(graph_->NewNode(ops_->Constant32(lhs * rhs), HTypes::Word8));
                } else if (node->opcode() == HDiv8) {
                    if (rhs != 0) {
                        return Replace(graph_->NewNode(ops_->Constant32(lhs / rhs), HTypes::Word8));
                    }
                } else {
                    if (rhs != 0) {
                        return Replace(graph_->NewNode(ops_->Constant32(lhs % rhs), HTypes::Word8));
                    }
                }
            }
            break;
            
        case HIMul8:
        case HIDiv8:
            if (NodeOps::IsConstant(node->input(0)) && NodeOps::IsConstant(node->input(1))) {
                DCHECK_EQ(node->input(0)->opcode(), node->input(1)->opcode());
                DCHECK_EQ(HConstant32, node->input(0)->opcode());
                
                auto lhs = static_cast<int8_t>(HOperatorWith<int32_t>::Data(node->input(0)));
                auto rhs = static_cast<int8_t>(HOperatorWith<int32_t>::Data(node->input(1)));

                if (node->opcode() == HIMul8) {
                    return Replace(graph_->NewNode(ops_->Constant32(lhs * rhs), HTypes::Word8));
                } else if (node->opcode() == HIDiv8) {
                    if (rhs != 0) {
                        return Replace(graph_->NewNode(ops_->Constant32(lhs / rhs), HTypes::Word8));
                    }
                }
            }
            break;
            
        case HAdd32:
        case HSub32:
        case HMul32:
        case HDiv32:
        case HMod32:
            if (NodeOps::IsConstant(node->input(0)) && NodeOps::IsConstant(node->input(1))) {
                DCHECK_EQ(node->input(0)->opcode(), node->input(1)->opcode());
                DCHECK_EQ(HConstant32, node->input(0)->opcode());
                
                auto lhs = HOperatorWith<uint32_t>::Data(node->input(0));
                auto rhs = HOperatorWith<uint32_t>::Data(node->input(1));

                if (node->opcode() == HAdd32) {
                    return Replace(graph_->NewNode(ops_->Constant32(lhs + rhs), HTypes::Word32));
                } else if (node->opcode() == HSub32) {
                    return Replace(graph_->NewNode(ops_->Constant32(lhs - rhs), HTypes::Word32));
                } else if (node->opcode() == HMul32) {
                    return Replace(graph_->NewNode(ops_->Constant32(lhs * rhs), HTypes::Word32));
                } else if (node->opcode() == HDiv32) {
                    if (rhs != 0) {
                        return Replace(graph_->NewNode(ops_->Constant32(lhs / rhs), HTypes::Word32));
                    }
                } else {
                    if (rhs != 0) {
                        return Replace(graph_->NewNode(ops_->Constant32(lhs % rhs), HTypes::Word32));
                    }
                }
            }
            break;
            
        case HIMul32:
        case HIDiv32:
            if (NodeOps::IsConstant(node->input(0)) && NodeOps::IsConstant(node->input(1))) {
                DCHECK_EQ(node->input(0)->opcode(), node->input(1)->opcode());
                DCHECK_EQ(HConstant32, node->input(0)->opcode());
                
                auto lhs = HOperatorWith<int32_t>::Data(node->input(0));
                auto rhs = HOperatorWith<int32_t>::Data(node->input(1));

                if (node->opcode() == HIMul32) {
                    return Replace(graph_->NewNode(ops_->Constant32(lhs * rhs), HTypes::Word32));
                } else if (node->opcode() == HIDiv32) {
                    if (rhs != 0) {
                        return Replace(graph_->NewNode(ops_->Constant32(lhs / rhs), HTypes::Word32));
                    }
                }
            }
            break;
        default:
            break;
    }
    return NoChange();
}

void SimplifiedElimination::Finalize() /*final*/ {
    
}

} // namespace lang

} // namespace mai
