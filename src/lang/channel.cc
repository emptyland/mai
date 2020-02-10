#include "lang/channel.h"
#include "lang/isolate-inl.h"
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

//void Channel::SendIxx(intptr_t value) {
//
//}
//
//void Channel::SendAny(Any *value) {
//
//}

Address Channel::SendNoBarrier(const void *data, size_t n) {
    Coroutine *co = Coroutine::This();
    
    std::lock_guard<std::mutex> lock(mutex_);
    if (capacity_ == 0) { // No buffered send
        if (QUEUE_EMPTY(recv_queue())) {
            Request *req = new Request;
            req->co = co;
            ::memcpy(&req->data, data, n);
            QUEUE_INSERT_TAIL(send_queue(), req);
            
            co->set_waitting(req);
            co->RequestYield();
        } else {
            Request *req = recv_queue()->next_;
            QUEUE_REMOVE(req);
            if (data_type_->id() == kType_f32 || data_type_->id() == kType_f64) {
                req->co->SetFACC0(data, n);
            } else {
                req->co->SetACC0(data, n);
            }
            Machine *owner = req->co->owner();
            owner->TakeWaittingCoroutine(req->co);
            req->co->SwitchState(Coroutine::kWaitting, Coroutine::kRunnable);
            owner->PostRunnable(req->co);
            delete req;
        }
        return nullptr;
    }
    // TODO:
    return nullptr;
}

void Channel::Recv(void *data, size_t n) {
    Coroutine *co = Coroutine::This();
    
    std::lock_guard<std::mutex> lock(mutex_);
    if (capacity_ == 0) { // No buffered send
        if (QUEUE_EMPTY(send_queue())) {
            Request *req = new Request;
            req->co = co;
            req->received = static_cast<Address>(data);
            QUEUE_INSERT_TAIL(recv_queue(), req);
            
            co->set_waitting(req);
            co->RequestYield();
        } else {
            Request *req = send_queue()->next_;
            QUEUE_REMOVE(req);
            ::memcpy(data, &req->data, n);

            Machine *owner = req->co->owner();
            owner->TakeWaittingCoroutine(req->co);
            req->co->SwitchState(Coroutine::kWaitting, Coroutine::kRunnable);
            owner->PostRunnable(req->co);
            delete req;
        }
        return;
    }
    // TODO:
}

void Channel::Close() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    while (!QUEUE_EMPTY(send_queue())) {
        Request *req = send_queue()->next_;
        QUEUE_REMOVE(req);
        
        Machine *owner = req->co->owner();
        owner->TakeWaittingCoroutine(req->co);
        req->co->SwitchState(Coroutine::kWaitting, Coroutine::kRunnable);
        owner->PostRunnable(req->co);
        delete req;
    }
    
    while (!QUEUE_EMPTY(recv_queue())) {
        Request *req = recv_queue()->next_;
        QUEUE_REMOVE(req);
        
        Machine *owner = req->co->owner();
        owner->TakeWaittingCoroutine(req->co);
        req->co->SwitchState(Coroutine::kWaitting, Coroutine::kRunnable);
        if (data_type_->id() == kType_f32 || data_type_->id() == kType_f64) {
            req->co->set_facc0(0);
        } else {
            req->co->set_acc0(0);
        }
        owner->PostRunnable(req->co);
        delete req;
    }
    OfAtmoic(&close_)->store(1, std::memory_order_release);
}

} // namespace lang

} // namespace mai
