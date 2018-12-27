#ifndef MAI_DB_WRITE_BATCH_WITH_INDEX_H_
#define MAI_DB_WRITE_BATCH_WITH_INDEX_H_

#include "core/skip-list.h"
#include "base/arena.h"
#include "base/base.h"
#include "mai/write-batch.h"
#include <unordered_map>

namespace mai {
class Allocator;
class Comparator;
namespace core {
class InternalKeyComparator;
} // namespace core
namespace db {
    
class WriteBatchWithIndex final : public WriteBatch {
public:
    WriteBatchWithIndex(Allocator *ll_allocator);
    ~WriteBatchWithIndex();
    
    void Clear();
    
    void AddOrUpdate(ColumnFamily *cf, uint8_t flag,
                     std::string_view key, std::string_view value);
    
    Error Get(ColumnFamily *cf, std::string_view key, std::string *value) const;
    
    Error RawGet(ColumnFamily *cf, std::string_view key, uint8_t *flag,
                 std::string *value) const;
    
    struct SavePoint {
        size_t redo_size;
        size_t n_entries;
    };

    DISALLOW_IMPLICIT_CONSTRUCTORS(WriteBatchWithIndex);
private:
    struct WriteBatchEntry;
    
    struct KeyComparator final {
        const Comparator  *cmp;
        const std::string *buf;
        int operator () (const WriteBatchEntry *lhs,
                         const WriteBatchEntry *rhs) const;
    }; // struct KeyComparator
    
    using Table = core::SkipList<WriteBatchEntry *, KeyComparator>;
    
    base::Arena arena_;
    std::unordered_map<uint32_t, std::shared_ptr<Table>> tables_;
}; // class WriteBatchWithIndex
    
} // namespace db
    
} // namespace mai

#endif // MAI_DB_WRITE_BATCH_WITH_INDEX_H_
