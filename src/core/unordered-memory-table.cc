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
        const KeyBoundle *ikey = KeyBoundle::New(user_key, tag.version(),
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
        DLOG(FATAL) << "Noreached!";
    }

    Table::Iterator iter_;
    Error error_;
}; // class UnorderedMemoryTable::IteratorImpl
    

UnorderedMemoryTable::UnorderedMemoryTable(const InternalKeyComparator *ikcmp,
                                           int initial_slot)
    : ikcmp_(DCHECK_NOTNULL(ikcmp))
    , table_(initial_slot, KeyComparator{ikcmp}) {
}

/*virtual*/ UnorderedMemoryTable::~UnorderedMemoryTable() {
    // TODO: user class Area
    Table::Iterator iter(&table_);
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        base::MallocAllocator{}.Free(const_cast<KeyBoundle *>(iter.key()));
    }
}

/*virtual*/ void UnorderedMemoryTable::Put(std::string_view key,
                                           std::string_view value,
                                           Version version, uint8_t flag) {
    const KeyBoundle *ikey = KeyBoundle::New(key, value, version, flag);
    DCHECK_NOTNULL(ikey);
    table_.Put(ikey);
}
    
/*virtual*/ Error UnorderedMemoryTable::Get(std::string_view key,
                                            Version version, Tag *tag,
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
    switch (iter.key()->tag().flags()) {
        case Tag::kFlagValue:
            value->assign(iter.key()->value());
            if (tag) {
                *tag = iter.key()->tag();
            }
            break;
        case Tag::kFlagDeletion:
            return MAI_NOT_FOUND("Deleted key.");
        default:
            DLOG(FATAL) << "Incorrect tag type: " << iter.key()->tag().flags();
            break;
    }
    return Error::OK();
}
    
/*virtual*/ Iterator *UnorderedMemoryTable::NewIterator() {
    Iterator *iter = new IteratorImpl(this);
    AddRef();
    iter->RegisterCleanup(&IteratorImpl::Cleanup, this);
    return iter;
}
    
/*virtual*/ size_t UnorderedMemoryTable::ApproximateMemoryUsage() const {
    return 0;
}
    
} // namespace core
    
} // namespace mai
