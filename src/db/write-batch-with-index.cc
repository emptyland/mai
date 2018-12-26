#include "db/write-batch-with-index.h"
#include "db/column-family.h"
#include "core/internal-key-comparator.h"
#include "base/slice.h"
#include "glog/logging.h"


namespace mai {
    
namespace db {
    
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
    
WriteBatchWithIndex::WriteBatchWithIndex(Allocator *ll_allocator)
    : arena_(DCHECK_NOTNULL(ll_allocator)) {
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
    
    auto pos = raw_buf().size();
    switch (flag) {
        case core::Tag::kFlagValue:
            Put(cf, key, value);
            break;
        case core::Tag::kFlagDeletion:
            Delete(cf, key);
            break;
        default:
            DLOG(FATAL) << "Noreached!";
            break;
    }
    WriteBatchEntry put;
    put.offset   = pos + Varint32::Sizeof(cf->id());
    put.key_size = key.size();
    put.size     = raw_buf().size() - put.offset;
    
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
    base::BufferReader rd(raw_buf().substr(entry->offset, entry->size));
    switch (rd.ReadByte()) {
        case core::Tag::kFlagValue:
            break;
        case core::Tag::kFlagDeletion:
            return MAI_NOT_FOUND("Not found!");
        default:
            DLOG(FATAL) << "Noreached!";
            break;
    }
    
    auto n = rd.ReadVarint64();
    DCHECK_EQ(entry->key_size, n);
    auto found_key = rd.ReadString(n);
    if (cf->comparator()->Compare(key, found_key) != 0) {
        return MAI_NOT_FOUND("Not found!");
    }

    *value = rd.ReadString();
    return Error::OK();
}
    
} // namespace db
    
} // namespace mai
