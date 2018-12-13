#ifndef MAI_CORE_BW_TREE_H_
#define MAI_CORE_BW_TREE_H_

#include "base/arena.h"
#include "base/base.h"
#include "glog/logging.h"
#include <atomic>
#include <map>
#include <deque>

namespace mai {
    
namespace core {
    
namespace bw {

using Pid = uint64_t;
    
static const Pid kInvalidPid = -1;
    
#define DEFINE_BW_NODE_KINDS(V) \
    V(BaseLine) \
    V(DeltaKey) \
    V(DeltaIndex) \
    V(SplitNode) \
    V(MergeNode) \
    V(RemoveNode)

struct Node {
    enum Kind : int {
#define DECL_ENUM(name) k##name,
        DEFINE_BW_NODE_KINDS(DECL_ENUM)
#undef DECL_ENUM
    };
    
    Node(Kind akind) : kind(akind) {}
    
#define DECL_KIND_CHECK(name) bool Is##name () const { return kind == k##name; }
    DEFINE_BW_NODE_KINDS(DECL_KIND_CHECK)
#undef  DECL_KIND_CHECK

    Kind  kind;
    Pid   pid = kInvalidPid;
    Node *base = nullptr;
}; // struct Node
    
template<class T>
struct DeltaNode : public Node {
    DeltaNode(Kind akind) : Node(akind) {}
    
    size_t size = 0; // virtual size
    int    depth = 0;
    Pid    sibling = kInvalidPid; // Right sibling node
    
    T smallest_key;
    T largest_key;
}; // struct KeyNode

template<class T>
struct SplitNode final : public DeltaNode<T> {
    SplitNode() : DeltaNode<T>(Node::kSplitNode) {}
    
    T separator;
    Pid splited = kInvalidPid;
}; // struct SplitNode


template<class T>
struct MergeNode final : public DeltaNode<T> {
    MergeNode() : DeltaNode<T>(Node::kMergeNode) {}
    
    Node *merged = nullptr;
}; // struct MergeNode


template<class T>
struct RemoveNode final : public DeltaNode<T> {
    RemoveNode() : DeltaNode<T>(Node::kRemoveNode) {}
}; // struct RemoveNode
    
template<class T>
struct Pair {
    T   key;
    Pid value;
}; // struct Pair

template<class T>
struct DeltaKey : public DeltaNode<T> {
    DeltaKey() : DeltaNode<T>(Node::kDeltaKey) {}
    DeltaKey(Node::Kind akind) : DeltaNode<T>(akind) {}
    
    T key;
}; // struct DeltaKeyNode
    
template<class T>
struct DeltaIndex final : public DeltaKey<T> {
    DeltaIndex() : DeltaKey<T>(Node::kDeltaIndex) {}
    
    Pid lhs = 0;
    Pid rhs = 0;
}; // template<class T> struct DeltaIndexEntry

template<class T>
struct BaseLine final : public DeltaNode<T> {
    BaseLine(size_t n)
        : DeltaNode<T>(Node::kBaseLine) { DeltaNode<T>::size = n; }
    
    Pid overflow = 0;
    Pair<T> entries[1];
}; // struct BaseLineNode
    

} // namespace bw
    
    
template<class T, class Comparator>
class BwTreeBase {
public:
    using Pid  = bw::Pid;
    using Node = bw::Node;
    using Pair = bw::Pair<T>;
    using DeltaNode = bw::DeltaNode<T>;
    using BaseLine = bw::BaseLine<T>;
    using DeltaKey = bw::DeltaKey<T>;
    using DeltaIndex = bw::DeltaIndex<T>;
    using SplitNode = bw::SplitNode<T>;
    
    BwTreeBase(Comparator cmp, base::Arena *arena, size_t max_pages)
        : cmp_(cmp)
        , arena_(DCHECK_NOTNULL(arena))
        , max_pages_(max_pages)
        , next_pid_(1) {
        pages_ = static_cast<PageSlot *>(arena_->Allocate(max_pages_ * sizeof(PageSlot)));
        ::memset(pages_, 0, max_pages_ * sizeof(PageSlot));
    }

    BaseLine *NewBaseLine(Pid pid, size_t n_entries, Pid sibling) {
        size_t size = sizeof(BaseLine) + n_entries * sizeof(Pair);
        void *chunk = ::malloc(size);
        BaseLine *n = new (chunk) BaseLine(n_entries);
        n->pid = pid;
        n->base    = nullptr;
        n->sibling = sibling;
        n->size    = n_entries;
        n->depth   = 0;
        if (pid) {
            UpdatePid(pid, n);
        }
        return n;
    }
    
    DeltaKey *NewDeltaKey(Pid pid, T key, DeltaNode *base) {
        void *chunk = ::malloc(sizeof(DeltaKey));
        DeltaKey *n = new (chunk) DeltaKey;
        n->pid     = pid;
        n->key     = key;
        n->smallest_key = key;
        n->largest_key  = key;
        n->size = base->size + 1;
        SetBase(n, base);
        if (pid) {
            UpdatePid(pid, n);
        }
        return n;
    }
    
    DeltaIndex *NewDeltaIndex(Pid pid, T key, Pid lhs, Pid rhs, DeltaNode *base) {
        void *chunk = ::malloc(sizeof(DeltaIndex));
        DeltaIndex *n = new (chunk) DeltaIndex;
        n->pid     = pid;
        n->key     = key;
        n->lhs     = lhs;
        n->rhs     = rhs;
        n->smallest_key = key;
        n->largest_key  = key;
        n->size = base->size + 1;
        SetBase(n, base);
        if (pid) {
            UpdatePid(pid, n);
        }
        return n;
    }
    
    SplitNode *NewSplitNode(Pid pid, T separator, Pid splited, DeltaNode *base) {
        void *chunk = ::malloc(sizeof(SplitNode));
        SplitNode *n = new (chunk) SplitNode;
        n->pid       = pid;
        n->separator = separator;
        n->splited   = splited;
        n->smallest_key = base->smallest_key;
        n->largest_key  = separator;
        n->size = base->size - base->size / 2;
        SetBase(n, base);
        if (pid) {
            UpdatePid(pid, n);
        }
        return n;
    }
    
    // Atomic update node
    void UpdatePid(Pid pid, Node *node) {
        node->pid = pid;
        auto slot = pages_ + pid;
        // TODO:
        Node *e;
        do {
            e = node->base;
        } while (!slot->address.compare_exchange_strong(e, node));
//        slot->address.store(node);
    }
    
    void ReplacePid(Pid pid, Node *old, Node *node) {
        node->pid = pid;
        auto slot = pages_ + pid;
        Node *e;
        do {
            e = old;
        } while (!slot->address.compare_exchange_strong(e, node));
    }
    
    Pid GeneratePid() { return next_pid_.fetch_add(1); }
    
    DeltaNode *GetNode(Pid pid) const {
        DCHECK_GE(pid, 0);
        DCHECK_LT(pid, next_pid_.load());
        return static_cast<DeltaNode *>(pages_[pid].address.load());
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(BwTreeBase);
protected:
    Comparator const cmp_;

private:
    void SetBase(DeltaNode *delta, DeltaNode *base) {
        delta->base    = base;
        delta->sibling = base->sibling;
        delta->depth   = base->depth + 1;
        
        if (base->size > 0) {
            if (cmp_(delta->smallest_key, base->smallest_key) > 0) {
                delta->smallest_key = base->smallest_key;
            }
            if (cmp_(delta->largest_key, base->largest_key) < 0) {
                delta->largest_key  = base->largest_key;
            }
        }
    }
    
    struct PageSlot {
        std::atomic<Node *> address;
    }; // struct PageSlot
    
    base::Arena *const arena_;
    size_t const max_pages_;
    PageSlot *pages_;
    std::atomic<Pid> next_pid_;
}; // template<class T> class BwTreeBase
    

template<class Key, class Comparator>
class BwTree final : public BwTreeBase<Key, Comparator> {
public:
    using Pid       = bw::Pid;
    using Node      = bw::Node;
    using BaseLine  = bw::BaseLine<Key>;
    using DeltaKey  = bw::DeltaKey<Key>;
    using DeltaIndex = bw::DeltaIndex<Key>;
    using DeltaNode = bw::DeltaNode<Key>;
    using SplitNode = bw::SplitNode<Key>;
    using Pair      = bw::Pair<Key>;
    using Base      = BwTreeBase<Key, Comparator>;
    using View      = std::map<Key, Pid>;
    
    class Iterator;

    BwTree(Comparator cmp, size_t consolidate_trigger,
           size_t split_trigger, base::Arena *arena)
        : BwTreeBase<Key, Comparator>(cmp, DCHECK_NOTNULL(arena), 1024)
        , consolidate_trigger_(consolidate_trigger)
        , split_trigger_(split_trigger)
        , level_(0) {
        Pid pid = Base::GeneratePid();
        Base::NewBaseLine(pid, 0, 0);
        root_.store(pid, std::memory_order_relaxed);
    }
    
    Pid GetRootId() const { return root_.load(); }
    
    void Put(Key key) {
        Pid parent_id;
        DeltaNode *leaf = FindGreaterOrEqual(key, &parent_id, true);
        DCHECK_NOTNULL(leaf);
        DeltaKey *p = Base::NewDeltaKey(leaf->pid, key, leaf);
        DCHECK_NOTNULL(p);

        if (NeedsSplit(p)) {
            SplitLeaf(p, parent_id);
        }
    }
    
    void SplitLeaf(DeltaNode *p, Pid parent_id) {

        auto n_entries = p->size - p->size / 2;
        auto pid = Base::GeneratePid();
        BaseLine *q = Base::NewBaseLine(pid, n_entries, p->sibling);
        View view = GetView(p, nullptr);
        size_t n = 0, i = 0;
        for (auto pair : view) {
            if (i++ >= p->size / 2) {
                q->entries[n++] = {pair.first, pair.second};
            }
        }
        Key kp = q->entries[0].key;
        q->smallest_key = q->entries[0].key;
        q->largest_key  = q->entries[q->size - 1].key;
        SplitNode *split = Base::NewSplitNode(0, kp, q->pid, p);
        split->sibling = q->pid;
        Base::UpdatePid(p->pid, split);
        
        if (parent_id) {
            DeltaKey *term = Base::NewDeltaIndex(parent_id, kp, p->pid, q->pid,
                                                 Base::GetNode(parent_id));
            (void)term;
        } else {
            BaseLine *root = Base::NewBaseLine(0, 1, 0);
            root->entries[0] = {kp, p->pid};
            root->overflow   = q->pid;
            root->smallest_key = kp;
            root->largest_key  = kp;
            pid = Base::GeneratePid();
            Base::UpdatePid(pid, root);
            root_.store(pid);
            level_.fetch_add(1);
        }
    }
    
    DeltaNode *FindGreaterOrEqual(Key key, Pid *parent_id, bool trigger = false) {
        *parent_id = 0;
        DeltaNode *x = Base::GetNode(root_.load());
        DCHECK_NOTNULL(x);
        
        Pid overflow = 0;
        while (x) {
            if (trigger && NeedsConsolidate(x)) {
                x = Consolidate(x);
            }
            
            View view = GetView(x, &overflow);
            Pid pid = 0 ;
            auto iter = view.upper_bound(key);
            if (iter == view.end()) {
                pid = overflow;
            } else {
                pid = iter->second;
            }
            
            if (pid == 0) {
                return x;
            } else {
                *parent_id = x->pid;
                x = Base::GetNode(pid);
            }
        }
        return nullptr;
    }

    View TEST_GetView(const DeltaNode *node, Pid *result) {
        return GetView(node, result);
    }
    BaseLine *TEST_Consolidate(const DeltaNode *node) {
        return Consolidate(node);
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(BwTree);
private:
    
    Pid GetRightChildId(const DeltaNode *node) const {
        DCHECK_GT(node->size, 0);
        if (node->IsBaseLine()) {
            const BaseLine *n = static_cast<const BaseLine *>(node);
            return n->overflow;
        } else {
            Pid overflow = 0;
            View view = GetView(node, &overflow);
            return overflow;
        }
    }
    
    Pid GetLeftChildId(const DeltaNode *node) const {
        DCHECK_GT(node->size, 0);
        if (node->IsBaseLine()) {
            const BaseLine *n = static_cast<const BaseLine *>(node);
            return n->entries[0].value;
        } else {
            Pid overflow = 0;
            View view = GetView(node, &overflow);
            return view.begin()->second;
        }
    }

    bool NeedsSplit(const DeltaNode *node) const {
        return node->size > split_trigger_;
    }
    
    bool NeedsConsolidate(const DeltaNode *node) const {
        return node->depth > consolidate_trigger_;
    }
    
    BaseLine *Consolidate(const DeltaNode *node) {
        Pid overflow = 0;
        View view = GetView(node, &overflow);
        BaseLine *n = Base::NewBaseLine(0, view.size(), node->sibling);
        DCHECK_NOTNULL(n);
        
        size_t i = 0;
        for (auto pair : view) {
            n->entries[i++] = {pair.first, pair.second};
        }
        n->overflow = overflow;
        n->smallest_key = n->entries[0].key;
        n->smallest_key = n->entries[n->size - 1].key;
        
        //Base::UpdatePid(node->pid, n);
        Base::ReplacePid(node->pid,
                         const_cast<DeltaNode *>(node), n);
        return n;
    }
    
    View GetView(const DeltaNode *node, Pid *result) const {
        View view;
        std::deque<const DeltaNode *> records;
        
        auto x = node;
        while (x && !x->IsBaseLine()) {
            records.push_front(x);
            x = static_cast<DeltaNode *>(x->base);
        }

        Pid overflow = 0;
        if (x) {
            const BaseLine *base_line = static_cast<const BaseLine *>(x);
            for (size_t i = 0; i < base_line->size; ++i) {
                view[base_line->entries[i].key] = base_line->entries[i].value;
            }
            overflow = base_line->overflow;
        }
        
        for (auto x : records) {
            if (x->IsDeltaKey()) {
                auto d = static_cast<const DeltaKey *>(x);
                view[d->key] = 0;
            } else if (x->IsDeltaIndex()) {
                auto d = static_cast<const DeltaIndex *>(x);
                view[d->key] = d->lhs;
                // TODO: Adjust rhs
                auto iter = view.upper_bound(d->key);
                if (iter == view.end()) {
                    overflow = d->rhs;
                } else {
                    view[iter->first] = d->rhs;
                }
            } else if (x->IsSplitNode()) {
                auto d = static_cast<const SplitNode *>(x);
                view.erase(view.find(d->separator), view.end());
            }
        }
        if (result) {
            *result = overflow;
        }
        return view;
    }

    size_t split_trigger_;
    size_t consolidate_trigger_;

    std::atomic<Pid> root_;
    std::atomic<int> level_;
}; // template<class Key, class Comparator> class BwTree
    

template<class Key, class Comparator>
class BwTree<Key, Comparator>::Iterator {
public:
    using Owns = BwTree<Key, Comparator>;
    
    Iterator(Owns *owns) : owns_(DCHECK_NOTNULL(owns)) {}
    
    void SeekToFirst() {
        page_id_ = 0;
        current_ = -1;
        const DeltaNode *x = owns_->GetNode(owns_->GetRootId());
        if (x->size == 0) {
            return;
        }
        while (true) {
            Pid pid = owns_->GetLeftChildId(x);
            if (pid == 0) {
                page_id_ = x->pid;
                current_ = 0;
                break;
            }
        }
    }

    void SeekToLast() {
        page_id_ = 0;
        current_ = -1;
        const DeltaNode *x = owns_->GetNode(owns_->GetRootId());
        if (x->size == 0) {
            return;
        }
        while (true) {
            Pid pid = owns_->GetRightChildId(x);
            if (pid == 0) {
                page_id_ = x->pid;
                current_ = x->size - 1;
                break;
            }
        }
    }
    
    void Seek(Key key) {
        page_id_ = 0;
        current_ = -1;
        Pid parent_id = 0;
        const DeltaNode *x = owns_->FindGreaterOrEqual(key, &parent_id);
        if (!x) {
            return;
        }
        if (x->IsBaseLine()) {
            const BaseLine *n = static_cast<const BaseLine *>(x);
            for (ssize_t i = n->size - 1; i >= 0; i--) {
                if (owns_->cmp_(key, n->entries[i].key) >= 0) {
                    page_id_ = x->pid;
                    current_ = i;
                    break;
                }
            }
        } else {
            View view = owns_->GetView(x, nullptr);
            auto iter = view.lower_bound(key);
            if (iter == view.end()){
                return;
            }
            page_id_ = x->pid;
            current_ = std::distance(view.begin(), iter);
        }
    }
    
    bool Valid() const { return page_id_ != 0 && current_ >= 0; }

    void Next() {
        DCHECK(Valid());
        const DeltaNode *x = owns_->GetNode(page_id_);
        if (current_ < x->size - 1) {
            ++current_;
        } else {
            page_id_ = x->sibling;
            current_ = 0;
        }
    }

    void Prev() {
        DCHECK(Valid());
        const DeltaNode *x = owns_->GetNode(page_id_);
        if (current_ > 0) {
            --current_;
        } else {
            const Pid saved_pid = page_id_;
            SeekToFirst();
            if (!Valid()) {
                return;
            }
            if (saved_pid == page_id_) {
                current_ = -1;
                page_id_ = 0;
                return;
            }

            x = owns_->GetNode(page_id_);
            while (x->sibling != saved_pid) {
                x = owns_->GetNode(x->sibling);
            }
            page_id_ = x->pid;
            current_ = x->size - 1;
        }
    }
    
    Key key() const {
        DCHECK(Valid());
        const DeltaNode *x = owns_->GetNode(page_id_);
        if (x->IsBaseLine()) {
            const BaseLine *n = static_cast<const BaseLine *>(x);
            return n->entries[current_].key;
        } else {
            View view = owns_->GetView(x, nullptr);
            int i = 0;
            for (auto pair : view) {
                if (i++ == current_) {
                    return pair.first;
                }
            }
            return Key();
        }
    }
    
private:
    Owns *const owns_;
    Pid page_id_ = 0;
    ssize_t current_ = -1;
}; // template<class Key, class Comparator> class BwTree::Iterator
    
} // namespace core
    
} // namespace mai


#endif // MAI_CORE_BW_TREE_H_
