#include "base/ebr.h"
#include <thread>

namespace mai {
    
namespace base {
    
void Ebr::Register() {
    auto node = static_cast<Tls *>(tls_slot_->Get());
    if (!node) {
        node = static_cast<Tls *>(::malloc(sizeof(Tls)));
        ::memset(node, 0, sizeof(*node));
        tls_slot_->Set(node);
    }

    Tls *head;
    do {
        head = tls_list_.load(std::memory_order_relaxed);
        node->next = head;
    } while (!tls_list_.compare_exchange_weak(head, node));
}
    
bool Ebr::Sync(uint32_t *gc_epoch) {
    auto epoch = global_epoch_.load(std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    
    auto n = tls_list_.load();
    while (n) {
        const uint32_t local_epoch = n->local_epoch;
        const bool active = (local_epoch & kActiveFlag) != 0;
        
        if (active && (local_epoch != (epoch | kActiveFlag))) {
            *gc_epoch = GetGcEpoch();
            return false;
        }
        n = n->next;
    }
    global_epoch_.store((epoch + 1) % kNumberEpochs,
                        std::memory_order_release);
    *gc_epoch = GetGcEpoch();
    return true;
}
    
void EbrGC::CycleNoLock() {
    uint32_t gc_epoch = 0;
    Entry *gc_list = nullptr;
    
    for (int i = 0; i < Ebr::kNumberEpochs; ++i) {
        if (!ebr_.Sync(&gc_epoch)) {
            return;
        }
        uint32_t staging_epoch = ebr_.GetStagingEpoch();
        DCHECK(epoch_list_[staging_epoch] == nullptr);
        epoch_list_[staging_epoch] = limbo_.exchange(nullptr);
        
        gc_list = epoch_list_[gc_epoch];
        if (gc_list) {
            break;
        }
    }
    Reclaim(gc_list);
    epoch_list_[gc_epoch] = nullptr;
}
    
void EbrGC::Full(uint64_t ms_for_retry) {
    bool done = false;
    
    std::unique_lock<std::mutex> lock(mutex_);
    do {
        CycleNoLock();
        
        done = true;
        for (int i = 0; i < Ebr::kNumberEpochs; ++i) {
            if (epoch_list_[i]) {
                done = false;
                break;
            }
        }
        if (!done || limbo_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(ms_for_retry));
        }
    } while (!done || limbo_.load());
}

void EbrGC::Reclaim(Entry *entry) {
    while (entry) {
        auto obj = entry->obj;
        auto prev = entry;
        entry = entry->next;
        deleter_(obj, arg0_);
        delete prev;
    }
}
    
} // namespace base
    
} // namespace mai
