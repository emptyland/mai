#ifndef MAI_CORE_UNORDERED_MEMORY_TABLE_H_
#define MAI_CORE_UNORDERED_MEMORY_TABLE_H_

#include "core/memory-table.h"
#include "core/hash-map-v2.h"
#include "core/internal-key-comparator.h"
#include "base/hash.h"
#include "glog/logging.h"
#include <atomic>

namespace mai {
    
namespace core {
    
class InternalKeyComparator;
    
class UnorderedMemoryTable final : public MemoryTable {
public:
    UnorderedMemoryTable(const InternalKeyComparator *ikcmp,
                         int initial_slot,
                         Allocator *low_level_alloc);
    virtual ~UnorderedMemoryTable();
    
    virtual void Put(std::string_view key, std::string_view value,
                     SequenceNumber version, uint8_t flag) override;
    virtual Error Get(std::string_view key, SequenceNumber version, Tag *tag,
                      std::string *value) const override;
    virtual Iterator *NewIterator() override;
    virtual size_t NumEntries() const override;
    virtual size_t ApproximateMemoryUsage() const override;
    virtual float ApproximateConflictFactor() const override;
    
private:
    struct KeyComparator {
        const InternalKeyComparator *ikcmp_;
        int operator ()(const KeyBoundle *lhs, const KeyBoundle *rhs) const {
            return ikcmp_->Compare(lhs->key(), rhs->key());
        }
        uint32_t operator ()(const KeyBoundle *key) const {
            return ikcmp_->Hash(key->key());
        }
    };
    class IteratorImpl;
    
    using Table = HashMap<const KeyBoundle *, KeyComparator>;

    const InternalKeyComparator *const ikcmp_;
    std::atomic<size_t> mem_usage_;
    base::Arena arena_;
    Table table_;
};
    
} // namespace core
    
} // namespace mai

#endif // MAI_CORE_UNORDERED_MEMORY_TABLE_H_
