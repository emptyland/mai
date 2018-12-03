#include "db/config.h"
#include "glog/logging.h"
#include <stdlib.h>
#include <limits>


namespace mai {
    
namespace db {

/*static*/ size_t Config::ComputeNumSlots(int level, size_t old_num_slots,
                                          float conflict_factor,
                                          size_t limit_min_num_slots) {
    DCHECK_GE(conflict_factor, 0.0);
    if (conflict_factor <= std::numeric_limits<float>::epsilon()) {
        return old_num_slots;
    }

    const float adjust_factor = 1.12f - (level * 0.12f);
    size_t result = old_num_slots * conflict_factor;
    if (result < limit_min_num_slots) {
        result = limit_min_num_slots;
    } else {
        result *= adjust_factor;
    }
    
    while (result % 2 == 0 || result % 3 == 0 || result % 5 == 0 ||
           result % 7 == 0 || result % 11 == 0) {
        result++;
    }
    
    return result;
}
    
/*static*/ size_t Config::ComputeNumSlots(int level, size_t n_entries,
                                          size_t limit_min_num_slots) {
    if (n_entries == 0) {
        return 0;
    }
    DCHECK_GT(limit_min_num_slots, 0);
    
    const float adjust_factor = 0.978f - (level * 0.212f);
    size_t result = n_entries * adjust_factor;
    if (result < limit_min_num_slots) {
        result = limit_min_num_slots;
    }
    while (result % 2 == 0 || result % 3 == 0 || result % 5 == 0 ||
           result % 7 == 0 || result % 11 == 0) {
        result++;
    }

    return result;
}

} // namespace db
    
} // namespace mai
