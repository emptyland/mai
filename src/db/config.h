#ifndef MAI_DB_CONFIG_H_
#define MAI_DB_CONFIG_H_

#include "base/base.h"

namespace mai {
    
namespace db {
    
struct Config final {
    // lv0 : ...
    // lv1 : 10GB
    // lv2 : 100GB
    // lv3 : no-limit
    static const int kMaxLevel = 4;
    static const int kMaxNumberLevel0File = 10; // the max of level0 file num
    static const int kMaxSizeLevel0File   = 400 * base::kMB;
    
    static const int kLimitMinNumberSlots = 17;
    
    static size_t ComputeNumSlots(int level, size_t old_num_slots,
                                  float conflict_factor,
                                  size_t limit_min_num_slots);
    
    static size_t ComputeNumSlots(int level, size_t n_entries,
                                  size_t limit_min_num_slots);
    
    DISALLOW_ALL_CONSTRUCTORS(Config);
}; // struct Config
    
} // namespace db
    
} // namespace mai



#endif // MAI_DB_CONFIG_H_
