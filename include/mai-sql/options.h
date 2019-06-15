#ifndef MAI_SQL_OPTIONS_H_
#define MAI_SQL_OPTIONS_H_

#include "mai/env.h"
#include <string>

namespace mai {
    
struct MaiSQLOptions {

    Env *env = Env::Default();
    
    // Data directory
    std::string data_dir;
    
    // Metadata directory
    std::string meta_dir;
    
    // Bytes of zone cache
    size_t zone_max_cache_size = 10 * 1024 * 1024; // 10MB
};
    
} // namespace mai

#endif // MAI_SQL_OPTIONS_H_
