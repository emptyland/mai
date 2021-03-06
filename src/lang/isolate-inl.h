#pragma once
#ifndef MAI_LANG_ISOLATE_INL_H_
#define MAI_LANG_ISOLATE_INL_H_

#include "lang/factory.h"
#include "lang/metadata-space.h"
#include "base/queue-macros.h"
#include "mai/isolate.h"
#include "mai/type-defs.h"
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
#define FACTORY (STATE->factory())

extern thread_local TLSStorage *__tls_storage;

#define TLS_STORAGE (DCHECK_NOTNULL(__tls_storage))

inline Heap *Isolate::heap() const { return DCHECK_NOTNULL(heap_); }

inline MetadataSpace *Isolate::metadata_space() const { return DCHECK_NOTNULL(metadata_space_); }

inline Factory *Isolate::factory() const { return DCHECK_NOTNULL(factory_); }

inline Scheduler *Isolate::scheduler() const { return DCHECK_NOTNULL(scheduler_); }

inline GarbageCollector *Isolate::gc() const { return DCHECK_NOTNULL(gc_); }

inline const Class *Isolate::builtin_type(BuiltinType type) const {
    return metadata_space()->builtin_type(type);
}

inline uint8_t **Isolate::bytecode_handler_entries() const {
    return DCHECK_NOTNULL(bytecode_handler_entries_);
}

inline uint8_t **Isolate::tracing_handler_entries() const {
    return DCHECK_NOTNULL(tracing_handler_entries_);
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
    delete node;
    n_global_handles_--;
}

inline int Isolate::GetNumberOfGlobalHandles() const {
    std::lock_guard<std::mutex> lock(persistent_mutex_);
    return n_global_handles_;
}

inline Closure *Isolate::FindExternalLinkageOrNull(const std::string &name) const {
    auto iter = external_linkers_.find(name);
    return iter == external_linkers_.end() ? nullptr : iter->second;
}

inline Closure *Isolate::TakeExternalLinkageOrNull(const std::string &name) {
    auto iter = external_linkers_.find(name);
    if (iter == external_linkers_.end()) {
        return nullptr;
    }
    Closure *fun = iter->second;
    external_linkers_.erase(iter);
    return fun;
}

Span64 *Isolate::global_space() const { return global_space_; }

size_t Isolate::global_space_length() const { return global_space_length_; }

template<class T>
inline T* Isolate::global_offset(int location) const {
    return reinterpret_cast<T *>(reinterpret_cast<Address>(global_space()) + location);
}

inline void Isolate::SetGlobalSpace(Span64 *spans, uint32_t *bitmap, size_t capacity,
                                    size_t length) {
    if (spans != global_space_) {
        delete[] global_space_;
        global_space_ = spans;
    }
    if (bitmap != global_space_bitmap_) {
        delete[] global_space_bitmap_;
        global_space_bitmap_ = bitmap;
    }
    global_space_capacity_ = capacity;
    global_space_length_ = length;
}

inline Profiler *Isolate::profiler() const { return profiler_; }

inline TracingHook *Isolate::tracing_hook() const { return tracing_hook_; }

inline void Isolate::set_tracing_hook(TracingHook *hook) { tracing_hook_ = hook; }

} // namespace lang

} // namespace mai

#endif // MAI_LANG_ISOLATE_INL_H_
