#include "nyaa/thread.h"
#include "nyaa/nyaa-core.h"
#include "mai-lang/isolate.h"
#include "mai/env.h"

namespace mai {
    
namespace nyaa {

NyThread::NyThread(NyaaCore *owns)
    : NyUDO(&UDOFinalizeDtor<NyThread>)
    , owns_(DCHECK_NOTNULL(owns)) {
}

NyThread::~NyThread() {
    delete[] stack_;
}
    
Error NyThread::Init() {
    stack_size_ = owns_->isolate()->init_thread_stack_size();
    stack_ = new Object *[stack_size_];
    stack_last_ = stack_ + stack_size_;
    return Error::OK();
}
    
} // namespace nyaa
    
} // namespace mai
