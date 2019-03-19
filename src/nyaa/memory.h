#ifndef MAI_NYAA_MEMORY_H_
#define MAI_NYAA_MEMORY_H_

#include "base/base.h"

namespace mai {
    
namespace nyaa {
    
enum HeapArea : int {
    SEMI_SPACE,
    NEW_SPACE,
    OLD_SPACE,
    CODE_SPACE,
    LARGE_SPACE,
};
    
static const size_t kAllocateAlignmentSize = kPointerSize;
    
static const size_t kLargeStringLength = 1024;
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_MEMORY_H_
