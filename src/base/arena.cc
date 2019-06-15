#include "base/arena.h"

namespace mai {
    
namespace base {
    
/*static*/ const uint32_t Arena::kInitZag = 0xcccccccc;
/*static*/ const uint32_t Arena::kFreeZag = 0xfeedfeed;
    
/*virtual*/ Arena::~Arena() {}
    
} // namespace base
    
} // namespace mai
