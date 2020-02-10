#pragma once
#ifndef MAI_LANG_CHANNEL_H_
#define MAI_LANG_CHANNEL_H_

#include "lang/value-inl.h"
#include "lang/mm.h"
#include "base/queue-macros.h"
#include <mutex>

namespace mai {

namespace lang {

class Coroutine;

struct WaittingRequest {
    QUEUE_HEADER(WaittingRequest);
    Coroutine *co = nullptr; // Owned coroutine
    Address received; // Received Address
    union {
        intptr_t ixx;
        Any *any;
        float f32;
        double f64;
    } data;
    
    WaittingRequest(): next_(this), prev_(this) {}
};

class Channel : public Any {
public:
    using Request = WaittingRequest;
    
    static int32_t kOffsetCapacity;
    static int32_t kOffsetLength;
    static int32_t kOffsetClose;
    static int32_t kOffsetBuffer;

    int32_t has_close() { return OfAtmoic(&close_)->load(std::memory_order_acquire); }

    DEF_PTR_GETTER(const Class, data_type);
    DEF_VAL_GETTER(uint32_t, capacity);
    
    inline void SendAny(Any *any);
 
    Address SendNoBarrier(const void *data, size_t n);

    void Recv(void *data, size_t n);
    
    void Close();
    
    friend class Machine;
private:
    Channel(const Class *clazz, const Class *data_class, uint32_t capacity, uint32_t tags)
        : Any(clazz, tags)
        , data_type_(DCHECK_NOTNULL(data_class))
        , capacity_(capacity) {
    }
    
    Request *send_queue() { return &send_queue_; }
    Request *recv_queue() { return &recv_queue_; }
    
    static size_t RequiredSize(const Class *data_class, uint32_t capacity) {
        return sizeof(Channel) + data_class->reference_size() * capacity;
    }

    const Class *const data_type_; // The Type of data
    Request send_queue_; // Waitting-queue for send
    Request recv_queue_; // Waitting-queue for recv
    mutable std::mutex mutex_; // Mutex for read/write
    Any *hold_ = nullptr; // [strong ref] Hold heap object
    int32_t close_ = 0; // Has close?
    const uint32_t capacity_; // Capacity of ring-buffer
    uint32_t length_ = 0; // Length of valid data in ring-buffer
    uint32_t start_ = 0; // Start of ring-buffer
    uint32_t end_ = 0; // End of ring-buffer
    uint8_t buffer_[0]; // Buffered data
}; // class Channel

inline void Channel::SendAny(Any *any) {
    Address addr = SendNoBarrier(&any, sizeof(any));
    if (capacity_ == 0) {
        hold_ = any;
        if (any) {
            WriteBarrier(&hold_);
        }
    } else {
        if (addr) {
            WriteBarrier(reinterpret_cast<Any **>(addr));
        }
    }
}

} // namespace lang

} // namespace mai

#endif // MAI_LANG_CHANNEL_H_
