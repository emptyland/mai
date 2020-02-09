#pragma once
#ifndef MAI_LANG_ISOLATE_INL_H_
#define MAI_LANG_ISOLATE_INL_H_

#include "lang/isolate.h"
#include "lang/factory.h"
#include "lang/type-defs.h"
#include "lang/metadata-space.h"
#include "base/queue-macros.h"
#include "glog/logging.h"

namespace mai {

namespace lang {

class Machine;
class Coroutine;

struct TLSStorage {
    TLSStorage(Machine *m) : machine(m) {}
    
    static void Dtor(void *pv) { delete static_cast<TLSStorage *>(pv); }

    int mid = -1;
    Machine *machine;
    Coroutine *coroutine = nullptr;
}; // struct TLSStorage

struct GlobalHandleNode {
    static const ptrdiff_t kOffsetHandle;

    GlobalHandleNode() : prev_(this), next_(this) {}

    QUEUE_HEADER(GlobalHandleNode);
    Any *handle = nullptr; // Handle address
    int mid = 0; // Onwer thread id(machine-id)
}; // struct PersistentNode

static_assert(static_cast<int>(kType_bool) == static_cast<int>(NumberValueSlot::kIndex_bool), "");

extern Isolate *__isolate;

#define STATE (DCHECK_NOTNULL(__isolate))

extern thread_local TLSStorage *__tls_storage;

#define TLS_STORAGE (DCHECK_NOTNULL(__tls_storage))

inline Heap *Isolate::heap() const { return DCHECK_NOTNULL(heap_); }

inline MetadataSpace *Isolate::metadata_space() const { return DCHECK_NOTNULL(metadata_space_); }

inline Factory *Isolate::factory() const { return DCHECK_NOTNULL(factory_); }

inline Scheduler *Isolate::scheduler() const { return DCHECK_NOTNULL(scheduler_); }

inline const Class *Isolate::builtin_type(BuiltinType type) const {
    return metadata_space()->builtin_type(type);
}

inline uint8_t **Isolate::bytecode_handler_entries() const {
    return DCHECK_NOTNULL(bytecode_handler_entries_);
}

inline NumberValueSlot *Isolate::cached_number_slot(int index) {
    return factory_->cached_number_slot(index);
}

inline std::atomic<AbstractValue *> *Isolate::cached_number_value(int slot, int64_t index) {
    return factory_->cached_number_value(slot, index);
}

inline GlobalHandleNode *Isolate::NewGlobalHandle(const void *pointer) {
    GlobalHandleNode *node = new GlobalHandleNode();
    node->handle = static_cast<Any *>(const_cast<void *>(pointer));
    node->mid = TLS_STORAGE->mid;
    
    std::lock_guard<std::mutex> lock(persistent_mutex_);
    QUEUE_INSERT_TAIL(persistent_dummy_, node);
    n_global_handles_++;
    return node;
}

inline void Isolate::DeleteGlobalHandle(GlobalHandleNode *node) {
    std::lock_guard<std::mutex> lock(persistent_mutex_);
    QUEUE_REMOVE(node);
    n_global_handles_--;
}

inline int Isolate::GetNumberOfGlobalHandles() const {
    std::lock_guard<std::mutex> lock(persistent_mutex_);
    return n_global_handles_;
}

} // namespace lang

} // namespace mai

#endif // MAI_LANG_ISOLATE_INL_H_
