#ifndef MAI_BASE_EBR_H_
#define MAI_BASE_EBR_H_

#include "base/base.h"
#include "mai/env.h"
#include "glog/logging.h"
#include <atomic>
#include <mutex>

namespace mai {
    
namespace base {
    
class Ebr final {
public:
    struct Tls {
        uint32_t local_epoch;
        Tls *next;
    }; // struct Tls
    
    static const uint32_t kActiveFlag = 0x80000000u;
    static const int kNumberEpochs = 3;

    Ebr() : tls_list_(nullptr), global_epoch_(0) {}

    Error Init(Env *env) {
        DCHECK(tls_list_.load() == nullptr);
        return env->NewThreadLocalSlot("ebr", ::free, &tls_slot_);
    }
    
    void Register() {
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
    
    void Enter() {
        Tls *n = DCHECK_NOTNULL(static_cast<Tls *>(tls_slot_->Get()));
        n->local_epoch = global_epoch_.load(std::memory_order_relaxed)
                       | kActiveFlag;
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }
    
    void Exit() {
        Tls *n = DCHECK_NOTNULL(static_cast<Tls *>(tls_slot_->Get()));
        std::atomic_thread_fence(std::memory_order_seq_cst);
        DCHECK(n->local_epoch & kActiveFlag);
        n->local_epoch = 0;
    }
    
    bool Sync(uint32_t *gc_epoch) {
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
    
    uint32_t GetGcEpoch() const {
        return (global_epoch_.load(std::memory_order_acquire) + 1) % kNumberEpochs;
    }
    
    uint32_t GetStagingEpoch() const {
        return global_epoch_.load(std::memory_order_acquire);
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Ebr);
private:
    std::atomic<Tls *> tls_list_;
    std::unique_ptr<ThreadLocalSlot> tls_slot_;
    std::atomic<uint32_t> global_epoch_;
}; // class Ebr


class EbrGC final {
public:
    struct Entry {
        void  *obj;
        Entry *next;
    }; // struct Entry
    
    using Deleter = void (*)(void *, void *);
    
    EbrGC(Deleter deleter, void *arg0)
        : deleter_(DCHECK_NOTNULL(deleter))
        , arg0_(arg0)
        , limbo_(nullptr) {
        ::memset(epoch_list_, 0, sizeof(Entry *) * Ebr::kNumberEpochs);
    }
    
    ~EbrGC() {
        for (int i = 0; i < Ebr::kNumberEpochs; ++i) {
            DCHECK(epoch_list_[i] == nullptr);
        }
        DCHECK(limbo_.load() == nullptr);
    }
    
    Error Init(Env *env) { return ebr_.Init(env); }
    
    void Register() { ebr_.Register(); }
    void Enter() { ebr_.Enter(); }
    void Exit() { ebr_.Exit(); }
    
    void Limbo(void *obj) {
        Entry *head;
        Entry *node = new Entry{obj, nullptr};
        do {
            head = limbo_.load(std::memory_order_relaxed);
            node->next = head;
        } while (!limbo_.compare_exchange_weak(head, node));
    }
    
    // WARNING: Not thread safe
    void CycleNoLock() {
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
    
    void Cycle() {
        std::lock_guard<std::mutex> lock(mutex_);
        CycleNoLock();
    }
    
    void Reclaim(Entry *entry) {
        while (entry) {
            auto obj = entry->obj;
            auto prev = entry;
            entry = entry->next;
            deleter_(obj, arg0_);
            delete prev;
        }
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(EbrGC);
private:
    Ebr ebr_;
    Deleter deleter_;
    void *arg0_;
    std::atomic<Entry *> limbo_;
    Entry *epoch_list_[Ebr::kNumberEpochs];

    std::mutex mutex_; // Only for Cycle()
}; // class EbrGC
    
} // namespace base
    
} // namespace mai

#endif // MAI_BASE_EBR_H_
