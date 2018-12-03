#ifndef MAI_CORE_ORDERED_MEMORY_TABLE_H_
#define MAI_CORE_ORDERED_MEMORY_TABLE_H_

#include "core/memory-table.h"
#include "core/skip-list.h"
#include "core/internal-key-comparator.h"
#include "base/arena.h"
#include "glog/logging.h"

namespace mai {
    
namespace core {
    
class InternalKeyComparator;

class OrderedMemoryTable final : public MemoryTable {
public:
    OrderedMemoryTable(const InternalKeyComparator *ikcmp,
                       Allocator *low_level_alloc);
    virtual ~OrderedMemoryTable();
    
    virtual void Put(std::string_view key, std::string_view value,
                     SequenceNumber version, uint8_t flag) override;
    virtual Error Get(std::string_view key, SequenceNumber version, Tag *tag,
                      std::string *value) const override;
    virtual Iterator *NewIterator() override;
    virtual size_t NumEntries() const override;
    virtual size_t ApproximateMemoryUsage() const override;
    virtual float ApproximateConflictFactor() const override;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(OrderedMemoryTable);
private:
    struct KeyComparator final {
        const InternalKeyComparator *ikcmp;
        int operator () (const KeyBoundle *lhs, const KeyBoundle *rhs) const {
            return ikcmp->Compare(lhs->key(), rhs->key());
        }
    }; // struct KeyComparator
    using Table = SkipList<const KeyBoundle *, KeyComparator>;
    
    class IteratorImpl;
    
    std::atomic<size_t> mem_usage_;
    base::Arena arena_;
    Table table_;
    std::atomic<size_t> n_entries_;
}; // class OrderedMemoryTable
    
} // namespace core
    
} // namespace mai

#endif // MAI_CORE_ORDERED_MEMORY_TABLE_H_
