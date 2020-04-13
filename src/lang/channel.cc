#include "lang/channel.h"
#include "lang/isolate-inl.h"
#include "lang/scheduler.h"
#include "lang/machine.h"
#include "lang/coroutine.h"
#include "asm/utils.h"

namespace mai {

namespace lang {

#define MEMBER_OFFSET_OF(field) \
    arch::ObjectTemplate<Channel, int32_t>::OffsetOf(&Channel :: field)

int32_t Channel::kOffsetCapacity = MEMBER_OFFSET_OF(capacity_);
int32_t Channel::kOffsetLength = MEMBER_OFFSET_OF(length_);
int32_t Channel::kOffsetClose = MEMBER_OFFSET_OF(close_);
int32_t Channel::kOffsetBuffer = MEMBER_OFFSET_OF(buffer_);

Address Channel::SendNoBarrier(const void *data, size_t n) {
    DCHECK_LE(data_type_->reference_size(), n);
    DCHECK(!has_close());
    const size_t size = std::min(n, static_cast<size_t>(data_type_->reference_size()));
    
    Coroutine *co = Coroutine::This();
    //std::unique_lock<std::mutex> lock(mutex_);
    base::SpinLock lock(&mutex_);
    if (is_not_buffered()) { // No buffered send
        if (QUEUE_EMPTY(recv_queue())) {
            AddSendQueue(data, size, co);
        } else {
            WakeupRecvQueue(data, size);
        }
        return nullptr;
    }

    if (is_buffer_full()) {
        if (QUEUE_EMPTY(recv_queue())) {
            AddSendQueue(data, size, co);
            return nullptr;
        } else {
            WakeupRecvQueue(TakeBufferHead(), size);
            return AddBufferTail(data, size);
        }
    }

    // On buffered send: just add data to buffer tail
    return AddBufferTail(data, size);
}

void Channel::Recv(void *data, size_t n) {
    DCHECK_LE(data_type_->reference_size(), n);
    DCHECK(!has_close());
    const size_t size = std::min(n, static_cast<size_t>(data_type_->reference_size()));
    
    Coroutine *co = Coroutine::This();
    //std::unique_lock<std::mutex> lock(mutex_);
    base::SpinLock lock(&mutex_);
    if (is_not_buffered()) { // No buffered send
        if (QUEUE_EMPTY(send_queue())) {
            AddRecvQueue(data, size, co);
        } else {
            WakeupSendQueue(data, size);
        }
        return;
    }
    
    if (is_buffer_full()) {
        if (QUEUE_EMPTY(send_queue())) {
            AddRecvQueue(data, size, co);
        } else {
            TakeBufferHead(data, size);
            WakeupSendQueue(AddBufferTail(), size);
        }
        return;
    }
    
    // On buffered recv: just take data from buffer head
    TakeBufferHead(data, size);
}

void Channel::CloseLockless() {
    DCHECK(!has_close());

    while (!QUEUE_EMPTY(send_queue())) {
        Request *req = send_queue()->next_;
        QUEUE_REMOVE(req);
        
        Machine *owner = req->co->owner();
        owner->TakeWaittingCoroutine(req->co);
        req->co->SwitchState(Coroutine::kWaitting, Coroutine::kRunnable);
        owner->PostRunnable(req->co);
        delete req;
    }
    delete send_queue_;
    
    while (!QUEUE_EMPTY(recv_queue())) {
        Request *req = recv_queue()->next_;
        QUEUE_REMOVE(req);
        
        Machine *owner = req->co->owner();
        owner->TakeWaittingCoroutine(req->co);
        req->co->SwitchState(Coroutine::kWaitting, Coroutine::kRunnable);
        if (data_type_->IsFloating()) {
            req->co->set_facc0(0);
        } else {
            req->co->set_acc0(0);
        }
        owner->PostRunnable(req->co);
        delete req;
    }
    delete recv_queue_;
    OfAtmoic(&close_)->store(1, std::memory_order_release);
}

void Channel::AddSendQueue(const void *data, size_t n, Coroutine *co) {
    Request *req = new Request;
    req->co = co;
    ::memcpy(&req->data, data, n);
    QUEUE_INSERT_TAIL(send_queue(), req);
  
    co->Yield(req);
}

void Channel::AddRecvQueue(void *data, size_t n, Coroutine *co) {
    Request *req = new Request;
    req->co = co;
    req->received = static_cast<Address>(data);
    QUEUE_INSERT_TAIL(recv_queue(), req);
  
    co->Yield(req);
}

void Channel::WakeupRecvQueue(const void *data, size_t n) {
    Request *req = recv_queue()->next_;
    QUEUE_REMOVE(req);
    if (data_type_->IsFloating()) {
        req->co->SetFACC0(data, n);
    } else {
        req->co->SetACC0(data, n);
    }

    Wakeup(req);
}

void Channel::WakeupSendQueue(void *data, size_t n) {
    Request *req = send_queue()->next_;
    QUEUE_REMOVE(req);
    ::memcpy(data, &req->data, n);

    Wakeup(req);
}

void Channel::Wakeup(Request *req) {
    //mutex_.unlock();
    while (!req->co->AcquireState(Coroutine::kWaitting, Coroutine::kRunnable)) {
        std::this_thread::yield();
    }

    Machine *owner = req->co->owner();
    owner->Wakeup(req->co, true/*now*/);
    delete req;
    
    //mutex_.lock();
}

} // namespace lang

} // namespace mai
