#include "core/ordered-memory-table.h"
#include "base/reference-count.h"
#include "mai/iterator.h"

namespace mai {
    
namespace core {
    
class OrderedMemoryTable::IteratorImpl final : public Iterator {
public:
    IteratorImpl(const OrderedMemoryTable *omt)
        : iter_(&omt->table_) {}
    
    virtual bool Valid() const override { return iter_.Valid(); }
    virtual void SeekToFirst() override { iter_.SeekToFirst(); }
    virtual void SeekToLast() override { iter_.SeekToLast(); }
    virtual void Seek(std::string_view target) override {
        std::string_view user_key = KeyBoundle::ExtractUserKey(target);
        Tag tag = KeyBoundle::ExtractTag(target);
        
        base::ScopedMemory scope;
        const KeyBoundle *ikey = KeyBoundle::New(user_key, tag.sequence_number(),
                                                 base::ScopedAllocator{&scope});
        iter_.Seek(ikey);
    }
    
    virtual void Next() override { iter_.Next(); }
    virtual void Prev() override { iter_.Prev(); }
    
    virtual std::string_view key() const override {
        return iter_.key()->key();
    }
    virtual std::string_view value() const override {
        return iter_.key()->value();
    }
    virtual Error error() const override { return error_; }
    
    static void Cleanup(void *arg1, void */*arg2*/) {
        DCHECK_NOTNULL(static_cast<OrderedMemoryTable *>(arg1))->ReleaseRef();
    }
private:
    void Noreached() {
        error_ = MAI_CORRUPTION("Noreached!");
        DLOG(FATAL) << "Noreached!";
    }
    
    Table::Iterator iter_;
    Error error_;
}; // class UnorderedMemoryTable::IteratorImpl
    
OrderedMemoryTable::OrderedMemoryTable(const InternalKeyComparator *ikcmp,
                                       Allocator *low_level_alloc)
    : mem_usage_(sizeof(*this))
    , arena_(low_level_alloc)
    , table_(KeyComparator{ikcmp}, &arena_)
    , n_entries_(0) {
}

/*virtual*/ OrderedMemoryTable::~OrderedMemoryTable() {
    // arena_ can free all keys.
}

/*virtual*/ void
OrderedMemoryTable::Put(std::string_view key, std::string_view value,
                        SequenceNumber version, uint8_t flag) {
    const KeyBoundle *ikey = KeyBoundle::New(key, value, version, flag,
                                             base::DelegatedAllocator{&arena_});
    //const KeyBoundle *ikey = KeyBoundle::New(key, value, version, flag);
    DCHECK_NOTNULL(ikey);
    table_.Put(ikey);
    //mem_usage_.fetch_add(ikey->size()); // TODO: use arean size
    n_entries_.fetch_add(1);
}
    
/*virtual*/ Error
OrderedMemoryTable::Get(std::string_view key, SequenceNumber version, Tag *tag,
                        std::string *value) const {
    base::ScopedMemory scope;
    const KeyBoundle *ikey = KeyBoundle::New(key, version,
                                             base::ScopedAllocator{&scope});
    Table::Iterator iter(&table_);
    iter.Seek(ikey);
    
    if (!iter.Valid()) {
        return MAI_NOT_FOUND("Can not seek key.");
    }
    if (iter.key()->user_key() != key) {
        return MAI_NOT_FOUND("Key not equals target.");
    }
    switch (iter.key()->tag().flag()) {
        case Tag::kFlagValue:
            value->assign(iter.key()->value());
            if (tag) {
                *tag = iter.key()->tag();
            }
            break;
        case Tag::kFlagDeletion:
            return MAI_NOT_FOUND("Deleted key.");
        default:
            DLOG(FATAL) << "Incorrect tag type: " << iter.key()->tag().flag();
            break;
    }
    return Error::OK();
}
    
/*virtual*/ Iterator *OrderedMemoryTable::NewIterator() {
    Iterator *iter = new IteratorImpl(this);
    AddRef();
    iter->RegisterCleanup(&IteratorImpl::Cleanup, this);
    return iter;
}
    
/*virtual*/ size_t OrderedMemoryTable::NumEntries() const {
    return n_entries_.load(std::memory_order_acquire);
}
    
/*virtual*/ size_t OrderedMemoryTable::ApproximateMemoryUsage() const {
    //return mem_usage_.load(std::memory_order_acquire);
    //return arena_.MemoryUsage();
    return arena_.memory_usage();
}

/*virtual*/ float OrderedMemoryTable::ApproximateConflictFactor() const {
    // Skip List has no Conflict Factor.
    // Equlas 0 forever.
    return 0;
}
    
} // namespace core
    
} // namespace mai
