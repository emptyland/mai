#include "lang/runtime.h"
#include "lang/isolate-inl.h"
#include "lang/value-inl.h"
#include "lang/machine.h"
#include "lang/coroutine.h"
#include "glog/logging.h"

namespace mai {

namespace lang {

/*static*/ void Runtime::DebugAbort(const char *message) {
    Machine *m = Machine::Get();
    Coroutine *co = m->running();
    uint64_t coid = !co ? -1 : co->coid();

    if (co) {
        co->set_state(Coroutine::kPanic);
    }
#if defined(DEBUG) || defined(_DEBUG)
    DLOG(FATAL) << "‼️☠️ Coroutine ABORT:\n    M:" << m->id() << ":C:" << coid << message;
#else
    fprintf(stdout, "‼️☠️ Coroutine ABORT:\n    M:%dC:%lld:%s", m->id(), coid, message);
    abort();
#endif
}

} // namespace lang

} // namespace mai
