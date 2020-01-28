#ifndef MAI_LANG_ISOLATE_INL_H_
#define MAI_LANG_ISOLATE_INL_H_

#include "lang/isolate.h"
#include "lang/metadata-space.h"
#include "glog/logging.h"

namespace mai {

namespace lang {

class Machine;
class Corouotine;

struct TLSStorage {
    TLSStorage(Machine *m) : machine(m) {}
    
    static void Dtor(void *pv) { delete static_cast<TLSStorage *>(pv); }

    Machine *machine;
    Corouotine *coroutine = nullptr;
};

extern Isolate *__isolate;

inline Heap *Isolate::heap() const { return DCHECK_NOTNULL(heap_); }

inline MetadataSpace *Isolate::metadata_space() const { return DCHECK_NOTNULL(metadata_space_); }

inline TLSStorage *Isolate::tls_storage() const {
    return static_cast<TLSStorage *>(DCHECK_NOTNULL(tls_->Get()));
}

inline Machine *Isolate::tls_machine() const { return tls_storage()->machine; }

inline Scheduler *Isolate::scheduler() const { return DCHECK_NOTNULL(scheduler_); }

inline const Class *Isolate::builtin_type(BuiltinType type) const {
    return metadata_space()->builtin_type(type);
}

} // namespace lang

} // namespace mai

#endif // MAI_LANG_ISOLATE_INL_H_
