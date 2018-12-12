#ifndef MAI_BW_BW_NODES_H_
#define MAI_BW_BW_NODES_H_

#include "core/key-boundle.h"
#include <atomic>

namespace mai {
    
namespace bw {
    
using Pid = uint64_t;
    
struct DeltaNode;
struct SplitNode;
struct MergeNode;
struct RemoveNode;
struct DeltaKeyNode;
struct BaseLineNode;

extern const uint64_t kInvalidOffset;
extern const Pid      kInvalidPid;

struct Node {
    enum Kind : int {
        BASE_LINE,
        DELTA_KEY,
        SPLIT_NODE,
        MERGE_NODE,
        REMOVE_NODE,
    };
    
    Node(Kind akind) : kind(akind) {}
    
    bool is_base_line() const { return kind == BASE_LINE; }
    bool is_delta_key() const { return kind == DELTA_KEY; }
    bool is_split_node() const { return kind == SPLIT_NODE; }
    bool is_merge_node() const { return kind == MERGE_NODE; }
    bool is_remove_node() const { return kind == REMOVE_NODE; }
    
    Kind  kind;
    Pid   pid = kInvalidPid;
    Node *base = nullptr;
};
    
struct DeltaNode : public Node {
    DeltaNode(Kind akind) : Node(akind) {}
    
    size_t size = 0; // virtual size
    int    depth = 0;
    Pid    sibling = kInvalidPid; // Right sibling node
    
    core::KeyBoundle *smallest_key = nullptr;
    core::KeyBoundle *largest_key  = nullptr;
}; // struct KeyNode
    
struct SplitNode final : public DeltaNode {
    SplitNode() : DeltaNode(SPLIT_NODE) {}

    Pid splited = kInvalidPid;
}; // struct SplitNode
    

struct MergeNode final : public DeltaNode {
    MergeNode() : DeltaNode(MERGE_NODE) {}
    
    Node *merged = nullptr;
}; // struct MergeNode
    

struct RemoveNode final : public DeltaNode {
    RemoveNode() : DeltaNode(REMOVE_NODE) {}
}; // struct RemoveNode

struct DeltaKeyNode final : public DeltaNode {
    DeltaKeyNode() : DeltaNode(DELTA_KEY) {}
    
    core::KeyBoundle *key = nullptr;
}; // struct DeltaKeyNode

struct BaseLineNode final : public DeltaNode {
    BaseLineNode() : DeltaNode(BASE_LINE) {}
    
    size_t n_entries = 0;
    core::KeyBoundle *entries[1];
}; // struct BaseLineNode

} // namespace bw

} // namespace mai


#endif // MAI_BW_BW_NODES_H_
