#include "nyaa/nyaa-core.h"
#include "mai-lang/nyaa.h"
#include "mai-lang/isolate.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {

Nyaa::Nyaa(Isolate *is)
    : isolate_(DCHECK_NOTNULL(is)) {
}

Nyaa::~Nyaa() {
    
}
    
} // namespace nyaa
    
} // namespace mai
