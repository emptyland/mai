#ifndef MAI_CORE_UNORDERED_MEMORY_TABLE_H_
#define MAI_CORE_UNORDERED_MEMORY_TABLE_H_

#include "core/memory-table.h"
#include "core/hash-map.h"
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
                         int initial_slot);
    virtual ~UnorderedMemoryTable();
    
    virtual void Put(std::string_view key, std::string_view value,
                     Version version, uint8_t flag) override;
    virtual Error Get(std::string_view key, Version version, Tag *tag,
                      std::string *value) const override;
    virtual Iterator *NewIterator() override;
    virtual size_t ApproximateMemoryUsage() const override;
    
private:
    struct KeyComparator {
        const InternalKeyComparator *ikcmp_;
        int operator ()(const KeyBoundle *lhs, const KeyBoundle *rhs) const {
            return ikcmp_->Compare(lhs->key(), rhs->key());
        }
        int operator ()(const KeyBoundle *key) const {
            return base::Hash::Js(key->user_key().data(),
                                  key->user_key().size()) & 0x7fffffff;
        }
    };
    class IteratorImpl;
    
    typedef HashMap<const KeyBoundle *, KeyComparator> Table;

    const InternalKeyComparator *const ikcmp_;
    Table table_;
    std::atomic<size_t> mem_usage_;
};
    
} // namespace core
    
} // namespace mai

#endif // MAI_CORE_UNORDERED_MEMORY_TABLE_H_
