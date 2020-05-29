#include "core/merging.h"
#include "core/iterator-warpper.h"
#include "core/unordered-memory-table.h"
#include "mai/iterator.h"
#include "mai/comparator.h"
#include "glog/logging.h"

namespace mai {
    
namespace core {
    
namespace {

class MergingIterator final : public Iterator {
public:
    MergingIterator(const Comparator *cmp, Iterator **children, size_t n)
        : cmp_(DCHECK_NOTNULL(cmp))
        , children_(new IteratorWarpper[n])
        , n_children_(n) {
        for (size_t i = 0; i < n; ++i) {
            children_[i].set_delegated(children[i]);
        }
    }

    virtual ~MergingIterator() {}
    
    virtual bool Valid() const override {
        return current_ != nullptr && DCHECK_NOTNULL(current_)->Valid();
    }

    virtual void SeekToFirst() override {
        for (size_t i = 0; i < n_children_; ++i) {
            Iterator *child = &children_[i];
            child->SeekToFirst();
        }
        FindSmallest();
        direction_ = kForward;
    }
    
    virtual void SeekToLast() override {
        for (size_t i = 0; i < n_children_; ++i) {
            Iterator *child = &children_[i];
            child->SeekToLast();
        }
        FindLargest();
        direction_ = kReserve;
    }
    
    virtual void Seek(std::string_view target) override {
        for (size_t i = 0; i < n_children_; ++i) {
            Iterator *child = &children_[i];
            child->Seek(target);
        }
        FindSmallest();
        direction_ = kForward;
    }
    
    virtual void Next() override {
        DCHECK(Valid());
        if (direction_ != kForward) {
            for (size_t i = 0; i < n_children_; ++i) {
                Iterator *child = &children_[i];
                if (child != current_) {
                    child->Seek(key());
                    if (child->Valid() &&
                        cmp_->Compare(key(), child->key()) == 0) {
                        child->Next();
                    }
                }
            }
            direction_ = kForward;
        }
        current_->Next();
        FindSmallest();
    }
    
    virtual void Prev() override {
        DCHECK(Valid());
        if (direction_ != kReserve) {
            for (size_t i = 0; i < n_children_; ++i) {
                Iterator *child = &children_[i];
                if (child != current_) {
                    child->Seek(key());
                    if (child->Valid()) {
                        child->Prev();
                    } else {
                        child->SeekToLast();
                    }
                }
            }
            direction_ = kReserve;
        }
        current_->Prev();
        FindLargest();
    }
    
    virtual std::string_view key() const override {
        DCHECK(Valid());
        return current_->key();
    }
    
    virtual std::string_view value() const override {
        DCHECK(Valid());
        return current_->value();
    }
    
    virtual Error error() const override {
        for (size_t i = 0; i < n_children_; ++i) {
            Iterator *child = &children_[i];
            Error rs = child->error();
            if (!rs.ok()) {
                return rs;
            }
        }
        return Error::OK();
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(MergingIterator);
    
private:
    void FindSmallest() {
        IteratorWarpper *smallest = nullptr;
        
        for (size_t i = 0; i < n_children_; ++i) {
            IteratorWarpper *child = &children_[i];
            
            if (child->Valid()) {
                if (smallest == nullptr) {
                    smallest = child;
                } else if (cmp_->Compare(child->key(), smallest->key()) < 0) {
                    smallest = child;
                }
//                std::string k(KeyBoundle::ExtractUserKey(child->key()));
//                printf("child key: %s\n", k.c_str());
            }
        }
        current_ = smallest;
    }
    
    void FindLargest() {
        IteratorWarpper *largest = nullptr;
        
        for (int64_t i = n_children_ - 1; i >= 0; --i) {
            IteratorWarpper *child = &children_[i];
            
            if (child->Valid()) {
                if (largest == nullptr) {
                    largest = child;
                } else if (cmp_->Compare(child->key(), largest->key()) > 0) {
                    largest = child;
                }
            }
        }
        current_ = largest;
    }
    
    const Comparator *const cmp_;
    std::unique_ptr<IteratorWarpper[]> children_;
    const size_t n_children_;
    
    IteratorWarpper *current_ = nullptr;
    Direction direction_ = kForward;
}; // class MergingIterator

} // namespace
    
/*static*/ Iterator *
Merging::NewUnorderedMergingIterator(const InternalKeyComparator *ikcmp,
                                     int tmp_initial_slots,
                                     Iterator **children, size_t n) {
    base::intrusive_ptr<MemoryTable>
    tmp_table(new UnorderedMemoryTable(ikcmp, tmp_initial_slots));
    return NewTempTableMergingIteraotr(tmp_table.get(), children, n);
}
    
/*static*/ Iterator *
Merging::NewTempTableMergingIteraotr(MemoryTable *tmp_table,
                                     Iterator **children, size_t n) {
    switch (n) {
        case 0:
            return Iterator::AsError(Error::OK());
        case 1:
            return DCHECK_NOTNULL(children[0]);
        default:
            break;
    }
    
    // TODO: concurrent merging.
    ParsedTaggedKey ikey;
    for (int i = 0; i < n; ++i) {
        Iterator *iter = children[i];
        
        for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
            KeyBoundle::ParseTaggedKey(iter->key(), &ikey);
            
            tmp_table->Put(ikey.user_key, iter->value(), ikey.tag.sequence_number(),
                           ikey.tag.flag());
        }
    }
    return tmp_table->NewIterator();
}
    
/*static*/ Iterator *
Merging::NewMergingIterator(const Comparator *cmp, Iterator **children,
                            size_t n) {
    switch (n) {
        case 0:
            return Iterator::AsError(Error::OK());
        case 1:
            return DCHECK_NOTNULL(children[0]);
        default:
            break;
    }
    
    return new MergingIterator(cmp, children, n);
}
    
} // namespace core
    
} // namespace mai
