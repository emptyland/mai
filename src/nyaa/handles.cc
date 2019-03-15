#include "mai-lang/handles.h"
#include "mai-lang/nyaa.h"
#include "nyaa/nyaa-core.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {
    
HandleScope::HandleScope(Nyaa *N)
    : nyaa_(DCHECK_NOTNULL(N)) {
    //nyaa_->core()->
}

HandleScope::~HandleScope() {
    
}

Value **HandleScope::NewHandle(Value *input) {
    return nullptr;
}

Object **HandleScope::NewHandle(Object *input) {
    return nullptr;
    
}

/*static*/ HandleScope *HandleScope::current() {
    return nullptr;
}
    
} // namespace nyaa

} // namespace mai
