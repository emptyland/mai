#include "mai-lang/handles.h"
#include "mai-lang/nyaa.h"
#include "nyaa/nyaa-core.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {
    
HandleScope::HandleScope(Isolate *isolate)
    : isolate_(DCHECK_NOTNULL(isolate)) {
    //NyaaCore::Current()->EnterHandleScope(this);
    isolate_->GetNyaa()->core()->EnterHandleScope(this);
}

HandleScope::~HandleScope() {
    isolate_->GetNyaa()->core()->ExitHandleScope();
}

void **HandleScope::NewHandle(void *value) {
    auto core = isolate_->GetNyaa()->core();
    void **space = reinterpret_cast<void **>(core->AdvanceHandleSlots(1));
    *space = value;
    return space;
}

/*static*/ HandleScope *HandleScope::Current() {
    return NyaaCore::Current()->current_handle_scope();
}
    
} // namespace nyaa

} // namespace mai
