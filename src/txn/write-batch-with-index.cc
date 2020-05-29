#include "txn/write-batch-with-index.h"
#include "db/column-family.h"
#include "core/internal-key-comparator.h"
#include "core/delta-amend-iterator.h"
#include "base/slice.h"
#include "mai/iterator.h"
#include "glog/logging.h"

namespace mai {
    
namespace txn {
    
using ::mai::base::Slice;
using ::mai::base::Varint32;
using ::mai::base::Varint64;

struct WriteBatchWithIndex::WriteBatchEntry {
    size_t offset;
    size_t size;
    union {
        size_t key_size;
        const char *lookup_key;
    };
}; // struct WriteBatchWithIndex::KeyComparator::WriteBatchEntry
    
    
class WriteBatchWithIndex::IteratorImpl : public Iterator {
public:
    IteratorImpl(const Table *table, const std::string *batch)
        : iter_(table)
        , batch_(DCHECK_NOTNULL(batch)) {}
    virtual ~IteratorImpl() override {}
    
    virtual bool Valid() const override { return iter_.Valid(); }
    virtual void SeekToFirst() override {
        iter_.SeekToFirst();
        Update();
    }
    virtual void SeekToLast() override {
        iter_.SeekToLast();
        Update();
    }
    virtual void Seek(std::string_view target) override {
        WriteBatchEntry lookup;
        lookup.key_size = target.size();
        lookup.lookup_key = target.data();
        iter_.Seek(&lookup);
    }
    virtual void Next() override {
        iter_.Next();
        if (Valid()) {
            Update();
        }
    }
    virtual void Prev() override {
        iter_.Prev();
        if (Valid()) {
            Update();
        }
    }
    virtual std::string_view key() const override {
        DCHECK(Valid());
        return saved_key_;
    }
    virtual std::string_view value() const override {
        DCHECK(Valid());
        return saved_value_;
    }
    virtual Error error() const override { return error_; }
private:
    void Update();
    
    Table::Iterator iter_;
    const std::string *batch_;
    std::string saved_key_;
    std::string saved_value_;
    Error error_;
}; // class WriteBatchWithIndex::IteratorImpl

void WriteBatchWithIndex::IteratorImpl::Update() {
    const auto entry = iter_.key();
    base::BufferReader rd(batch_->substr(entry->offset, entry->size));
    auto flag = rd.ReadByte();
    
    bool should_read_value = false;
    switch (flag) {
        case core::Tag::kFlagValue:
            should_read_value = true;
            break;
        case core::Tag::kFlagDeletion:
            break;
        default:
            NOREACHED();
            break;
    }

    auto n = rd.ReadVarint64();
    DCHECK_EQ(entry->key_size, n);
    saved_key_ = rd.ReadString(n);
    auto tag = core::Tag::Encode(core::Tag::kMaxSequenceNumber, flag);
    base::Slice::WriteFixed64(&saved_key_, tag);
    
    if (should_read_value) {
        saved_value_ = rd.ReadString();
    } else {
        saved_value_ = "";
    }
}

int WriteBatchWithIndex::KeyComparator::operator () (const WriteBatchEntry *lhs,
                                                     const WriteBatchEntry *rhs) const {
    size_t n = 0;
    std::string_view a;
    if (lhs->offset > 0) {
        auto p = buf->data() + lhs->offset + 1;
        auto s = Varint64::Decode(p, &n);
        DCHECK_EQ(s, lhs->key_size);
        a = std::string_view(p + n, lhs->key_size);
    } else {
        a = std::string_view(lhs->lookup_key, lhs->size);
    }
    
    std::string_view b;
    if (rhs->offset > 0) {
        auto p = buf->data() + rhs->offset + 1;
        auto s = Varint64::Decode(p, &n);
        DCHECK_EQ(s, rhs->key_size);
        b = std::string_view(p + n, rhs->key_size);
    } else {
        b = std::string_view(rhs->lookup_key, rhs->size);
    }
    return cmp->Compare(a, b);
}
    
WriteBatchWithIndex::WriteBatchWithIndex() {
}

WriteBatchWithIndex::~WriteBatchWithIndex() {
}
    
void WriteBatchWithIndex::Clear() {
    WriteBatch::Clear();
    tables_.clear();
    arena_.Purge(true);
}

void WriteBatchWithIndex::AddOrUpdate(ColumnFamily *cf,
                                      uint8_t flag,
                                      std::string_view key,
                                      std::string_view value) {
    auto iter = tables_.find(cf->id());
    if (iter == tables_.end()) {
        KeyComparator cmp{cf->comparator(), mutable_raw_buf()};
        std::tie(iter, std::ignore) = tables_.emplace(cf->id(),
                                                      new Table(cmp, &arena_));
    }
    
    WriteBatchEntry lookup;
    lookup.offset = 0;
    lookup.size   = key.size();
    lookup.lookup_key = key.data();
    
    auto pos = raw_buf()->size();
    switch (flag) {
        case core::Tag::kFlagValue:
            Put(cf, key, value);
            break;
        case core::Tag::kFlagDeletion:
            Delete(cf, key);
            break;
        default:
            NOREACHED();
            break;
    }
    WriteBatchEntry put;
    put.offset   = pos + Varint32::Sizeof(cf->id());
    put.key_size = key.size();
    put.size     = raw_buf()->size() - put.offset;
    
    Table::Iterator table_iter(iter->second.get());
    table_iter.Seek(&lookup);
    if (table_iter.Valid()) {
        ::memcpy(table_iter.key(), &put, sizeof(put));
    } else {
        WriteBatchEntry *wr =
            static_cast<WriteBatchEntry *>(arena_.Allocate(sizeof(WriteBatchEntry)));
        ::memcpy(wr, &put, sizeof(put));
        iter->second->Put(wr);
    }
}
    
Error WriteBatchWithIndex::Get(ColumnFamily *cf, std::string_view key,
                               std::string *value) const {
    uint8_t flag;
    Error rs = RawGet(cf, key, &flag, value);
    if (!rs) {
        return rs;
    }
    if (flag == core::Tag::kFlagDeletion) {
        return MAI_NOT_FOUND("Deleted.");
    }
    return rs;
}
    
Error WriteBatchWithIndex::RawGet(ColumnFamily *cf, std::string_view key,
                                  uint8_t *flag,
                                  std::string *value) const {
    auto iter = tables_.find(cf->id());
    if (iter == tables_.end()) {
        return MAI_NOT_FOUND("No any write.");
    }
    
    WriteBatchEntry lookup;
    lookup.offset = 0;
    lookup.size   = key.size();
    lookup.lookup_key = key.data();
    
    Table::Iterator table_iter(iter->second.get());
    table_iter.Seek(&lookup);
    if (!table_iter.Valid()) {
        return MAI_NOT_FOUND("Not found!");
    }
    
    const auto entry = table_iter.key();
    base::BufferReader rd(raw_buf()->substr(entry->offset, entry->size));
    *flag = rd.ReadByte();
    
    bool should_read_value = false;
    switch (*flag) {
        case core::Tag::kFlagValue:
            should_read_value = true;
            break;
        case core::Tag::kFlagDeletion:
            break;
        default:
            NOREACHED();
            break;
    }
    
    auto n = rd.ReadVarint64();
    DCHECK_EQ(entry->key_size, n);
    auto found_key = rd.ReadString(n);
    if (cf->comparator()->Compare(key, found_key) != 0) {
        return MAI_NOT_FOUND("Not found!");
    }

    if (should_read_value) {
        *value = rd.ReadString();
    }
    return Error::OK();
}
    
Iterator *WriteBatchWithIndex::NewIterator(ColumnFamily *cf) const {
    auto iter = tables_.find(cf->id());
    if (iter == tables_.cend()) {
        return Iterator::AsError(MAI_NOT_FOUND("No any write in this column "
                                               "family."));
    }
    return new IteratorImpl(iter->second.get(), raw_buf());
}

Iterator *WriteBatchWithIndex::NewIteratorWithBase(ColumnFamily *cf,
                                                   Iterator *base) const {
    auto delta = NewIterator(cf);
    if (delta->error().fail()) {
        return delta;
    }
    return new core::DeltaAmendIterator(cf->comparator(), base, delta);
}
    
} // namespace txn
    
} // namespace mai
