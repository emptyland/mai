#include "core/merging.h"
#include "core/iterator-warpper.h"
#include "core/unordered-memory-table.h"
#include "mai/iterator.h"
#include "mai/comparator.h"
#include "glog/logging.h"

namespace mai {
    
namespace core {
    
/*static*/ Iterator *
Merging::NewUnorderedMergingIterator(const InternalKeyComparator *ikcmp,
                                     int tmp_initial_slots,
                                     Iterator **children, size_t n) {
    base::Handle<MemoryTable>
    tmp_table(new UnorderedMemoryTable(ikcmp, tmp_initial_slots));
    
    return NewTempTableMergingIteraotr(tmp_table.get(), children, n);
}
    
/*static*/ Iterator *
Merging::NewTempTableMergingIteraotr(MemoryTable *tmp_table,
                                     Iterator **children, size_t n) {
    switch (n) {
        case 0:
            return Iterator::AsError(Error::OK());
        case 1:
            return DCHECK_NOTNULL(children[0]);
        default:
            break;
    }
    
    // TODO: cocurrent merging.
    ParsedTaggedKey ikey;
    for (int i = 0; i < n; ++i) {
        Iterator *iter = children[i];
        
        for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
            KeyBoundle::ParseTaggedKey(iter->key(), &ikey);
            
            tmp_table->Put(ikey.user_key, iter->value(), ikey.tag.version(),
                           ikey.tag.flags());
        }
    }
    return tmp_table->NewIterator();
}
    
} // namespace core
    
} // namespace mai
