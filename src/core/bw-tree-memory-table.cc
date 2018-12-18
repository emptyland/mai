#include "core/bw-tree-memory-table.h"
#include "mai/env.h"
#include "mai/iterator.h"


namespace mai {
    
namespace core {
    
class BwTreeMemoryTable::IteratorImpl final : public Iterator {
public:
    IteratorImpl(const BwTreeMemoryTable *bmt)
        : iter_(&bmt->table_) {}
    
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
        DCHECK_NOTNULL(static_cast<BwTreeMemoryTable *>(arg1))->ReleaseRef();
    }
private:
    Table::Iterator iter_;
    Error error_;
}; // class BwTreeMemoryTable::IteratorImpl
    
BwTreeMemoryTable::BwTreeMemoryTable(const InternalKeyComparator *ikcmp, Env *env,
                                     Allocator *ll_allocator)
    : n_entries_(0)
    , arena_(ll_allocator)
    , table_(KeyComparator{ikcmp}, 5, 127, env) {
}

/*virtual*/ BwTreeMemoryTable::~BwTreeMemoryTable() {
}

/*virtual*/
void BwTreeMemoryTable::Put(std::string_view key, std::string_view value,
                            SequenceNumber version, uint8_t flag) {
    const KeyBoundle *ikey = KeyBoundle::New(key, value, version, flag,
                                             base::DelegatedAllocator{&arena_});
    DCHECK_NOTNULL(ikey);
    table_.Put(ikey);
    n_entries_.fetch_add(1);
}

/*virtual*/
Error BwTreeMemoryTable::Get(std::string_view key, SequenceNumber version,
                             Tag *tag, std::string *value) const {
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

/*virtual*/ Iterator *BwTreeMemoryTable::NewIterator() {
    Iterator *iter = new IteratorImpl(this);
    AddRef();
    iter->RegisterCleanup(&IteratorImpl::Cleanup, this);
    return iter;
}

/*virtual*/ size_t BwTreeMemoryTable::NumEntries() const {
    return n_entries_.load(std::memory_order_acquire);
}

/*virtual*/ size_t BwTreeMemoryTable::ApproximateMemoryUsage() const {
    return arena_.memory_usage();
}

/*virtual*/ float BwTreeMemoryTable::ApproximateConflictFactor() const {
    return 0.0f;
}
    
} // namespace core
    
} // namespace mai
