#ifndef MAI_CORE_MERGING_H_
#define MAI_CORE_MERGING_H_

#include "base/base.h"

namespace mai {
class Iterator;
class Comparator;
class Allocator;
namespace core {

class InternalKeyComparator;
class MemoryTable;
    
struct Merging final {
    
    static Iterator *NewUnorderedMergingIterator(const InternalKeyComparator *ikmp,
                                                 int tmp_initial_slots,
                                                 Iterator **children, size_t n,
                                                 Allocator *low_level_allocator);
    
    static Iterator *NewTempTableMergingIteraotr(MemoryTable *tmp_table,
                                                 Iterator **children, size_t n);
    
    static Iterator *NewMergingIterator(const Comparator *cmp,
                                        Iterator **children, size_t n);
                                                      

    DISALLOW_ALL_CONSTRUCTORS(Merging);
}; // struct Merging
    
} // namespace core
    
} // namespace mai

#endif // MAI_CORE_MERGING_H_
