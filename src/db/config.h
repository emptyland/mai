#ifndef MAI_DB_CONFIG_H_
#define MAI_DB_CONFIG_H_

#include "base/base.h"

namespace mai {
    
namespace db {
    
struct Config final {
    static const int kMaxLevel = 4;
    static const int kMaxNumberLevel0File = 10; // the max of level0 file num
    static const int kMaxSizeLevel0File   = 80 * base::kMB;
    
    DISALLOW_ALL_CONSTRUCTORS(Config);
}; // struct Config
    
} // namespace db
    
} // namespace mai



#endif // MAI_DB_CONFIG_H_
