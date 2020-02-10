#include "core/unordered-memory-table.h"
#include "base/reference-count.h"
#include "mai/iterator.h"

namespace mai {
    
namespace core {
    
class UnorderedMemoryTable::IteratorImpl final : public Iterator {
public:
    IteratorImpl(UnorderedMemoryTable *umt)
        : iter_(&umt->table_) {}
    
    virtual bool Valid() const override { return iter_.Valid(); }
    virtual void SeekToFirst() override { iter_.SeekToFirst(); }
    virtual void SeekToLast() override { Noreached(); }
    virtual void Seek(std::string_view target) override {
        std::string_view user_key = KeyBoundle::ExtractUserKey(target);
        Tag tag = KeyBoundle::ExtractTag(target);
        
        base::ScopedMemory scope;
        const KeyBoundle *ikey = KeyBoundle::New(user_key, tag.sequence_number(),
                                                 base::ScopedAllocator{&scope});
        iter_.Seek(ikey);
    }
    
    virtual void Next() override { iter_.Next(); }
    virtual void Prev() override { Noreached(); }
    
    virtual std::string_view key() const override {
        return iter_.key()->key();
    }
    virtual std::string_view value() const override {
        return iter_.key()->value();
    }
    virtual Error error() const override { return error_; }
    
    static void Cleanup(void *arg1, void */*arg2*/) {
        DCHECK_NOTNULL(static_cast<UnorderedMemoryTable *>(arg1))->ReleaseRef();
    }
private:
    void Noreached() {
        error_ = MAI_CORRUPTION("Noreached!");
        NOREACHED();
    }

    Table::Iterator iter_;
    Error error_;
}; // class UnorderedMemoryTable::IteratorImpl
    

UnorderedMemoryTable::UnorderedMemoryTable(const InternalKeyComparator *ikcmp,
                                           int initial_slot,
                                           Allocator *low_level_allocator)
    : ikcmp_(DCHECK_NOTNULL(ikcmp))
    , mem_usage_(sizeof(*this))
    , arena_(DCHECK_NOTNULL(low_level_allocator))
    , table_(initial_slot, KeyComparator{ikcmp}, &arena_) {
}

/*virtual*/ UnorderedMemoryTable::~UnorderedMemoryTable() {
    // arena_ can free all keys.
}

/*virtual*/ void UnorderedMemoryTable::Put(std::string_view key,
                                           std::string_view value,
                                           SequenceNumber version, uint8_t flag) {
    const KeyBoundle *ikey = KeyBoundle::New(key, value, version, flag,
                                             base::DelegatedAllocator{&arena_});
    table_.Put(DCHECK_NOTNULL(ikey));
    //mem_usage_.fetch_add(ikey->size(), std::memory_order_acq_rel);
}
    
/*virtual*/ Error UnorderedMemoryTable::Get(std::string_view key,
                                            SequenceNumber version, Tag *tag,
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
            break;
        case Tag::kFlagDeletion:
            break;
        default:
            DLOG(FATAL) << "Incorrect tag type: " << iter.key()->tag().flag();
            break;
    }
    if (tag) {
        *tag = iter.key()->tag();
    }
    return Error::OK();
}
    
/*virtual*/ Iterator *UnorderedMemoryTable::NewIterator() {
    Iterator *iter = new IteratorImpl(this);
    AddRef();
    iter->RegisterCleanup(&IteratorImpl::Cleanup, this);
    return iter;
}
    
/*virtual*/ size_t UnorderedMemoryTable::NumEntries() const {
    return table_.n_entries();
}
    
/*virtual*/ size_t UnorderedMemoryTable::ApproximateMemoryUsage() const {
    //return mem_usage_.load(std::memory_order_acquire);
    //return arena_.MemoryUsage();
    return arena_.memory_usage();
}
    
/*virtual*/ float UnorderedMemoryTable::ApproximateConflictFactor() const {
    DCHECK_GT(table_.n_slots(), 0);
    
    float n_slots = static_cast<float>(table_.n_slots());
    float n_entries = static_cast<float>(table_.n_entries());
    return n_entries / n_slots;
}
    
} // namespace core
    
} // namespace mai
