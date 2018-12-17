#ifndef MAI_CORE_BW_TREE_H_
#define MAI_CORE_BW_TREE_H_

#include "base/base.h"
#include "base/ebr.h"
#include "mai/env.h"
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
    
#define DEFINE_CASTS(name) \
    static name *Cast(Node *x) { \
        DCHECK_EQ(x->kind, Node::k##name); \
        return static_cast<name *>(x); \
    } \
    static const name *Cast(const Node *x) { \
        DCHECK_EQ(x->kind, Node::k##name); \
        return static_cast<const name *>(x); \
    }

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
    
    DEFINE_CASTS(SplitNode);
    
    T separator;
    Pid splited = kInvalidPid;
}; // struct SplitNode


template<class T>
struct MergeNode final : public DeltaNode<T> {
    MergeNode() : DeltaNode<T>(Node::kMergeNode) {}
    
    DEFINE_CASTS(MergeNode);
    
    Node *merged = nullptr;
}; // struct MergeNode


template<class T>
struct RemoveNode final : public DeltaNode<T> {
    RemoveNode() : DeltaNode<T>(Node::kRemoveNode) {}
    
    DEFINE_CASTS(RemoveNode);
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
    
    DEFINE_CASTS(DeltaKey);
    
    T key;
}; // struct DeltaKeyNode
    
template<class T>
struct DeltaIndex final : public DeltaKey<T> {
    DeltaIndex() : DeltaKey<T>(Node::kDeltaIndex) {}
    
    DEFINE_CASTS(DeltaIndex);
    
    Pid lhs = 0;
    Pid rhs = 0;
}; // template<class T> struct DeltaIndexEntry

template<class T>
struct BaseLine final : public DeltaNode<T> {
    BaseLine(size_t n)
        : DeltaNode<T>(Node::kBaseLine) { DeltaNode<T>::size = n; }
    
    Pair<T> entry(size_t i) const {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, DeltaNode<T>::size);
        return entries[i];
    }
    
    void set_entry(size_t i, const Pair<T> &pair) {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, DeltaNode<T>::size);
        entries[i] = pair;
    }
    
    void UpdateBound() {
        DeltaNode<T>::smallest_key = entries[0].key;
        DeltaNode<T>::largest_key  = entries[DeltaNode<T>::size - 1].key;
    }
    
    DEFINE_CASTS(BaseLine);
    DISALLOW_IMPLICIT_CONSTRUCTORS(BaseLine);
    
    Pid overflow = 0;
    Pair<T> entries[1];
}; // struct BaseLineNode
    
    
#undef DEFINE_CASTS

} // namespace bw
    
    
template<class T, class Comparator>
class BwTreeBase {
public:
    using Pid        = bw::Pid;
    using Node       = bw::Node;
    using Pair       = bw::Pair<T>;
    using DeltaNode  = bw::DeltaNode<T>;
    using BaseLine   = bw::BaseLine<T>;
    using DeltaKey   = bw::DeltaKey<T>;
    using DeltaIndex = bw::DeltaIndex<T>;
    using SplitNode  = bw::SplitNode<T>;
    
    BwTreeBase(Comparator cmp, Env *env, size_t max_pages)
        : cmp_(cmp)
        , gc_(&Deleter, this)
        , max_pages_(max_pages)
        , next_pid_(1)
        , pages_(new PageSlot[max_pages_]) {
        ::memset(pages_, 0, max_pages_ * sizeof(PageSlot));
        Error rs = gc_.Init(env);
        DCHECK(rs.ok()) << rs.ToString();
    }
    
    ~BwTreeBase() {
        gc_.Cycle();
        for (Pid i = 1; i < next_pid_.load(); ++i) {
            auto x = pages_[i].address.load();
            while (x) {
                auto prev = x;
                x = x->base;
                Deleter(prev, this);
            }
        }
        delete[] pages_;
    }

    BaseLine *NewBaseLine(Pid pid, size_t n_entries, Pid sibling) {
        size_t size = sizeof(BaseLine) + n_entries * sizeof(Pair);
        void *chunk = ::malloc(size);
        BaseLine *n = new (chunk) BaseLine(n_entries);
        n->pid = pid;
        n->base    = nullptr;
        n->sibling = sibling;
        //n->size    = n_entries;
        n->depth   = 0;
        if (pid) {
            UpdatePid(pid, n);
        }
        return n;
    }
    
    DeltaKey *NewDeltaKey(Pid pid, T key, DeltaNode *base) {
        void *chunk = ::malloc(sizeof(DeltaKey));
        DeltaKey *n = new (chunk) DeltaKey;
        n->pid          = pid;
        n->key          = key;
        n->smallest_key = key;
        n->largest_key  = key;
        n->size         = base->size + 1;
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
        n->size    = base->size + 1;
        n->smallest_key = key;
        n->largest_key  = key;
        SetBase(n, base);
        if (pid) {
            UpdatePid(pid, n);
        }
        return n;
    }
    
    SplitNode *NewSplitNode(Pid pid, size_t size, T separator, Pid splited,
                            DeltaNode *base) {
        void *chunk = ::malloc(sizeof(SplitNode));
        SplitNode *n = new (chunk) SplitNode;
        n->pid       = pid;
        n->separator = separator;
        n->splited   = splited;
        n->smallest_key = base->smallest_key;
        n->largest_key  = separator;
        n->size = size;
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
    }
    
    void ReplacePid(Pid pid, Node *old, Node *node) {
        node->pid = pid;
        auto slot = pages_ + pid;
        Node *e;
        do {
            e = old;
        } while (!slot->address.compare_exchange_strong(e, node));
        
        // Limbo all nodes
        auto x = old;
        while (x) {
            gc_.Limbo(x);
            x = x->base;
        }
        gc_.Cycle();
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
    base::EbrGC gc_;

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
    
    static void Deleter(void *chunk, void *arg0) {
        auto self = static_cast<BwTreeBase *>(arg0);
        DCHECK(self != nullptr);
        auto n = static_cast<Node *>(chunk);
        ::free(n);
    }
    
    struct PageSlot {
        std::atomic<Node *> address;
    }; // struct PageSlot

    size_t const max_pages_;
    std::atomic<Pid> next_pid_;
    PageSlot *pages_;
}; // template<class T> class BwTreeBase
    

template<class Key, class Comparator>
class BwTree final : public BwTreeBase<Key, Comparator> {
public:
    using Pid        = bw::Pid;
    using Node       = bw::Node;
    using BaseLine   = bw::BaseLine<Key>;
    using DeltaKey   = bw::DeltaKey<Key>;
    using DeltaIndex = bw::DeltaIndex<Key>;
    using DeltaNode  = bw::DeltaNode<Key>;
    using SplitNode  = bw::SplitNode<Key>;
    using Pair       = bw::Pair<Key>;
    using Base       = BwTreeBase<Key, Comparator>;
    
    struct ComparatorLess {
        Comparator cmp;
        bool operator () (Key lhs, Key rhs) const {
            return cmp(lhs, rhs) < 0;
        }
    };
    using View = std::map<Key, Pid, ComparatorLess>;
    
    class Iterator;

    BwTree(Comparator cmp, size_t consolidate_trigger,
           size_t split_trigger, Env *env)
        : BwTreeBase<Key, Comparator>(cmp, env, 1024)
        , consolidate_trigger_(consolidate_trigger)
        , split_trigger_(split_trigger)
        , level_(0) {
        Pid pid = Base::GeneratePid();
        Base::NewBaseLine(pid, 0, 0);
        root_.store(pid, std::memory_order_relaxed);
    }
    
    Pid GetRootId() const { return root_.load(); }
    
    int GetLevel() const { return level_.load(); }
    
    void Put(Key key) {
        Pid parent_id;
        DeltaNode *leaf = FindRoomFor(key, &parent_id, true);
        DCHECK_NOTNULL(leaf);
        DeltaKey *p = Base::NewDeltaKey(leaf->pid, key, leaf);
        DCHECK_NOTNULL(p);

        if (NeedsSplit(p)) {
            SplitInner(p, parent_id);
        }
    }

    View TEST_MakeView(const DeltaNode *node, Pid *result) const {
        return MakeView(node, result);
    }
    BaseLine *TEST_Consolidate(const DeltaNode *node) {
        return Consolidate(node);
    }
    size_t TEST_FindGreaterOrEqual(const DeltaNode *node, Key key) const {
        bool found = false;
        return std::get<0>(FindGreaterOrEqual(node, key, &found));
    }
    SplitNode *TEST_SplitLeaf(DeltaNode *p, Pid parent_id) {
        return SplitLeaf(p, parent_id);
    }
    SplitNode *TEST_SpliInnter(DeltaNode *p, Pid parent_id) {
        return SplitInner(p, parent_id);
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(BwTree);
private:
    SplitNode *SplitLeaf(DeltaNode *p, Pid parent_id) {
        return SplitInner(p, parent_id);
    }
    
    SplitNode *SplitInner(DeltaNode *p, Pid parent_id) {
        const size_t page_size = p->size;
        DCHECK_GT(page_size, 2);
    
        size_t n_entries, sep;
        if (page_size % 2) {
            sep = page_size / 2;
            n_entries = page_size - page_size / 2 - 1;
        } else {
            sep = page_size / 2 - 1;
            n_entries = page_size - page_size / 2;
        }
        auto pid = Base::GeneratePid();
        BaseLine *q = Base::NewBaseLine(pid, n_entries, p->sibling);

        size_t n = 0, i = 0;
        Key kp{}, sp{};
        Pid overflow = 0;
        View view = MakeView(p, &overflow);
        for (auto pair : view) {
            if (i < sep) {
                sp = pair.first;
            } else if (i == sep) {
                kp = pair.first;
            } else if (i > sep) {
                q->set_entry(n++, {pair.first, pair.second});
                //DCHECK_NE(0, pair.second);
            }
            ++i;
        }
        DCHECK_EQ(n, n_entries);
        q->overflow = overflow;
        q->UpdateBound();

        SplitNode *split = Base::NewSplitNode(0, page_size - n_entries - 1, kp,
                                              q->pid, p);
        split->sibling = q->pid;
        split->largest_key = sp;
        Base::UpdatePid(p->pid, split);

#if 0
        {
            view = MakeView(split, nullptr);
            printf("<%llu>[%d]======P========\n", split->pid, kp);
            for (const auto &pair : view) {
                printf("%d, ", pair.first);
            }
            printf("\n");
            
            view = MakeView(q, nullptr);
            printf("<%llu>++++======Q========\n", q->pid);
            for (const auto &pair : view) {
                printf("%d, ", pair.first);
            }
            printf("\n");
        }
#endif
        if (parent_id) {
            DeltaNode *parent = Base::GetNode(parent_id);
            parent = Base::NewDeltaIndex(parent_id, kp, p->pid, q->pid, parent);
            if (NeedsSplit(parent)) {
                SplitInner(parent, FindParent(parent, kp));
            }
        } else {
            BaseLine *root = Base::NewBaseLine(0, 1, 0);
            root->set_entry(0, {kp, p->pid});
            root->overflow   = q->pid;
            root->UpdateBound();
            pid = Base::GeneratePid();
            Base::UpdatePid(pid, root);
            root_.store(pid);
            level_.fetch_add(1);
        }
        return split;
    }
    
    Pid FindParent(const DeltaNode *node, Key key) const {
        Pid parent_id = 0;
        const DeltaNode *x = Base::GetNode(root_.load());
        DCHECK_NOTNULL(x);

        while (x != node) {
            bool found = false;
            Pid pid = std::get<1>(FindGreaterOrEqual(x, key, &found));
            
            if (pid == 0) {
                break;
            }
            parent_id = x->pid;
            x = Base::GetNode(pid);
        }
        return parent_id;
    }
    
    DeltaNode *FindRoomFor(Key key, Pid *parent_id, bool trigger = false) {
        *parent_id = 0;
        DeltaNode *x = Base::GetNode(root_.load());
        DCHECK_NOTNULL(x);

        while (x) {
            if (trigger && NeedsConsolidate(x)) {
                x = Consolidate(x);
            }

            bool found = false;
            Pid pid = std::get<1>(FindGreaterOrEqual(x, key, &found));
            if (pid == 0) {
                return x;
            }
            *parent_id = x->pid;
            x = Base::GetNode(pid);
        }
        return nullptr;
    }
    
    std::tuple<DeltaNode *, size_t>
    FindGreaterOrEqual(Key key, bool trigger = false) {
        DeltaNode *x = Base::GetNode(root_.load());
        DCHECK_NOTNULL(x);
        while (x) {
            if (trigger && NeedsConsolidate(x)) {
                x = Consolidate(x);
            }
#if 0
            View view = MakeView(x, nullptr);
            printf("-------------\n");
            for (const auto &pair : view) {
                printf(" %d", pair.first);
            }
            printf("\n");
#endif
            bool found = false;
            size_t idx;
            Pid pid;
            std::tie(idx, pid) = FindGreaterOrEqual(x, key, &found);
            if (pid == 0) {
                return std::make_tuple(x, idx);
            }
            if (found) {
                auto largestKey = GetLargestKey(Base::GetNode(pid));
                if (Base::cmp_(key, largestKey) > 0) {
                    return std::make_tuple(x, idx);
                }
            }
            x = Base::GetNode(pid);
        }
        
        return std::make_tuple(nullptr, 0);
    }

    std::tuple<size_t, Pid>
    FindGreaterOrEqual(const DeltaNode *node, Key key, bool *found) const {
        *found = false;
        if (node->IsBaseLine()) {
            auto base_line = BaseLine::Cast(node);
            size_t count = base_line->size, first = 0;
            while (count > 0) {
                auto i = first;
                auto step = count / 2;
                i += step;
                if (Base::cmp_(base_line->entry(i).key, key) < 0) {
                    first = ++i;
                    count -= step + 1;
                } else {
                    count = step;
                }
            }
            *found = first < base_line->size;
            if (*found) {
                return std::make_tuple(first, base_line->entry(first).value);
            }
            return std::make_tuple(base_line->size, base_line->overflow);
        } else {
            Pid overflow;
            View view = MakeView(node, &overflow);
            auto iter = view.lower_bound(key);
            *found = iter != view.end();
            if (*found) {
                size_t idx = std::distance(view.begin(), iter);
                return std::make_tuple(idx, iter->second);
            }
            return std::make_tuple(view.size(), overflow);
        }
    }
    
    std::tuple<Key, Pid> NodeAt(const DeltaNode *node, size_t idx) const {
        DCHECK_GE(idx, 0);
        DCHECK_LT(idx, node->size);
        
        if (node->IsBaseLine()) {
            auto base_line = BaseLine::Cast(node);
            return std::make_tuple(base_line->entry(idx).key,
                                   base_line->entry(idx).value);
        } else {
            View view = MakeView(node, nullptr);
            DCHECK_EQ(node->size, view.size());
            auto iter = view.begin();
            std::advance(iter, idx);
            return *iter;
        }
    }
    
    Key GetLargestKey(const DeltaNode *x) const {
        DCHECK_NOTNULL(x);
        
        const DeltaNode *t;
        Key largest_key{};
        Pid child = 0;
        do {
            if (x->IsBaseLine()) {
                auto base_line = BaseLine::Cast(x);
                child = base_line->overflow;
                largest_key = base_line->entry(base_line->size - 1).key;
            } else {
                View view = MakeView(x, &child);
                for (const auto &pair : view) {
                    largest_key = pair.first;
                }
            }
            t = x;
            x = Base::GetNode(child);
        } while (child != 0);
        //DCHECK_EQ(Base::cmp_(t->largest_key, largest_key), 0);
        //return t->largest_key;
        return largest_key;
    }
    
    Pid GetRightChildId(const DeltaNode *node) const {
        DCHECK_GT(node->size, 0);
        if (node->IsBaseLine()) {
            const BaseLine *n = static_cast<const BaseLine *>(node);
            return n->overflow;
        } else {
            Pid overflow = 0;
            View view = MakeView(node, &overflow);
            return overflow;
        }
    }
    
    Pid GetLeftChildId(const DeltaNode *node) const {
        DCHECK_GT(node->size, 0);
        if (node->IsBaseLine()) {
            const BaseLine *n = static_cast<const BaseLine *>(node);
            return n->entry(0).value;
        } else {
            Pid overflow = 0;
            View view = MakeView(node, &overflow);
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
        View view = MakeView(node, &overflow);
        BaseLine *n = Base::NewBaseLine(0, view.size(), node->sibling);
        DCHECK_NOTNULL(n);
        
        size_t i = 0;
        for (auto pair : view) {
            n->set_entry(i++, {pair.first, pair.second});
        }
        n->overflow = overflow;
        n->UpdateBound();

        Base::ReplacePid(node->pid,
                         const_cast<DeltaNode *>(node), n);
        // Should GC replaced linked-nodes.
        return n;
    }
    
    View MakeView(const DeltaNode *node, Pid *result) const {
        View view(ComparatorLess{Base::cmp_});
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
                view.emplace(base_line->entry(i).key,
                             base_line->entry(i).value);
            }
            overflow = base_line->overflow;
        }
        
        for (auto x : records) {
            if (x->IsDeltaKey()) {
                auto d = DeltaKey::Cast(x);
                view.emplace(d->key, 0);
            } else if (x->IsDeltaIndex()) {
                auto d = DeltaIndex::Cast(x);
                view.emplace(d->key, d->lhs);

                auto iter = view.upper_bound(d->key);
                if (iter == view.end()) {
                    overflow = d->rhs;
                } else {
                    view[iter->first] = d->rhs;
                }
            } else if (x->IsSplitNode()) {
                auto d = SplitNode::Cast(x);
                auto iter = view.find(d->separator);
                DCHECK(iter != view.end());
                overflow = iter->second;
                view.erase(iter, view.end());
            }
        }
        if (result) {
            *result = overflow;
        }
        DCHECK_EQ(node->size, view.size());
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
    
    Iterator(Owns *owns)
        : owns_(DCHECK_NOTNULL(owns)) {
        owns_->gc_.Register();
        owns_->gc_.Enter();
    }
    
    ~Iterator() { owns_->gc_.Exit(); }
    
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

        DeltaNode *x;
        std::tie(x, current_) = owns_->FindGreaterOrEqual(key, true);
        page_id_ = (!x) ? 0 : x->pid;
    }
    
    bool Valid() const { return page_id_ != 0 && current_ >= 0; }

    void Next() {
        DCHECK(Valid());
        // TODO:
    }

    void Prev() {
        DCHECK(Valid());
        // TODO:
    }
    
    Key key() const {
        DCHECK(Valid());
        const DeltaNode *x = owns_->GetNode(page_id_);
        if (x->IsBaseLine()) {
            const BaseLine *n = BaseLine::Cast(x);
            return n->entries[current_].key;
        } else {
            View view = owns_->MakeView(x, nullptr);
            auto iter = view.begin();
            std::advance(iter, current_);
            DCHECK(iter != view.end());
            return iter->first;
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
