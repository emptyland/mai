#ifndef MAI_CORE_BW_TREE_MEMORY_TABLE_H_
#define MAI_CORE_BW_TREE_MEMORY_TABLE_H_

#include "core/memory-table.h"
#include "core/bw-tree.h"
#include "core/internal-key-comparator.h"
#include "base/standalone-arena.h"

namespace mai {
class Env;
namespace core {
    
class BwTreeMemoryTable final : public MemoryTable {
public:
    BwTreeMemoryTable(const InternalKeyComparator *ikcmp, Env *env,
                      Allocator *ll_allocator);
    virtual ~BwTreeMemoryTable();
    
    virtual void Put(std::string_view key, std::string_view value,
                     SequenceNumber version, uint8_t flag) override;
    virtual Error Get(std::string_view key, SequenceNumber version, Tag *tag,
                      std::string *value) const override;
    virtual Iterator *NewIterator() override;
    virtual size_t NumEntries() const override;
    virtual size_t ApproximateMemoryUsage() const override;
    virtual float ApproximateConflictFactor() const override;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(BwTreeMemoryTable);
private:
    struct KeyComparator final {
        const InternalKeyComparator *ikcmp;
        int operator () (const KeyBoundle *lhs, const KeyBoundle *rhs) const {
            return ikcmp->Compare(lhs->key(), rhs->key());
        }
    }; // struct KeyComparator
    using Table = BwTree<const KeyBoundle *, KeyComparator>;
    
    class IteratorImpl;

    std::atomic<size_t> n_entries_;

    base::StandaloneArena arena_;
    mutable Table table_;
}; // class BwTreeMemoryTable
    
} // namespace core
    
} // namespace mai



#endif // MAI_CORE_BW_TREE_MEMORY_TABLE_H_
