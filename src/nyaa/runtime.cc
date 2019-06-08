#include "nyaa/runtime.h"
#include "nyaa/thread.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/object-factory.h"
#include "nyaa/function.h"
#include "nyaa/builtin.h"

namespace mai {

namespace nyaa {

using ThreadTemplate = arch::ObjectTemplate<NyThread>;
using MapTemplate = arch::ObjectTemplate<NyMap>;
using ObjectTemplate = arch::ObjectTemplate<Object>;
using NyaaCoreTemplate = arch::ObjectTemplate<NyaaCore>;

/*static*/ Object *Runtime::Thread_GetUpVal(NyThread *thd, int slot) {
    return thd->frame_->upval(slot);
}

/*static*/ void Runtime::Thread_SetUpVal(NyThread *thd, Object *val, int up) {
    thd->frame_->SetUpval(up, val, thd->owns_);
}

/*static*/ Object *Runtime::Thread_GetProto(NyThread *thd, int slot) {
    return thd->frame_->proto()->proto_pool()->Get(slot);
}

/*static*/ Object *Runtime::Thread_Closure(NyThread *thd, int slot) {
    NyFunction *proto = NyFunction::Cast(thd->frame_->proto()->proto_pool()->Get(slot));
    DCHECK_NOTNULL(proto);
    NyClosure *closure = thd->owns_->factory()->NewClosure(proto);
    for (int i = 0; i < proto->n_upvals(); ++i) {
        thd->Bind(i, closure, proto->upval(i));
    }
    return closure;
}
    
/*static*/ Address Runtime::NyaaCore_GetSuspendPoint(NyaaCore *N) {
    return N->code_pool()->kEntryTrampoline->entry_address() + N->suspend_point_pc();
}
    
/*static*/ Address Runtime::kExternalLinks[kMaxLinks] = {
    ThreadTemplate::MethodAddress(&NyThread::Set),
    ThreadTemplate::MethodAddress(&NyThread::Get),
    reinterpret_cast<Address>(&Thread_GetUpVal),
    reinterpret_cast<Address>(&Thread_SetUpVal),
    reinterpret_cast<Address>(&Thread_GetProto),
    reinterpret_cast<Address>(&Thread_Closure),
    ThreadTemplate::MethodAddress(&NyThread::RuntimePrepareCall),
    ThreadTemplate::MethodAddress(&NyThread::FinializeCall),
    ThreadTemplate::MethodAddress(&NyThread::RuntimeRet),
    
    reinterpret_cast<Address>(&NyaaCore_GetSuspendPoint),
    
    ObjectTemplate::MethodAddress(&Object::IsFalse),
    
    MapTemplate::MethodAddress(&NyMap::RawGet),
    MapTemplate::MethodAddress(&NyMap::RawPut),
};
    
} // namespace nyaa
    
} // namespace mai
