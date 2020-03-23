#include "nyaa/profiling.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/nyaa-values.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {
    
Profiler::~Profiler() {
    for (auto pair : entries_) {
        ProfileEntry *slot = &pair.second;
        if (slot->kind == ProfileEntry::kCalling && slot->nrets > 1) {
            ::free(slot->rets);
        }
    }
}
    
int Profiler::TraceBranchGuard(int trace_id, int value, int line) {
    DCHECK_LT(trace_id, owns_->max_trace_id());
    auto iter = entries_.find(trace_id);
    if (iter == entries_.end()) {
        ProfileEntry slot;
        slot.kind = ProfileEntry::kBranch;
        slot.line = line;
        slot.hit = 1;
        slot.true_guard = value ? 1 : 0;
        entries_.insert({trace_id, slot});
    } else {
        DCHECK_EQ(ProfileEntry::kBranch, iter->second.kind);
        iter->second.hit++;
        iter->second.true_guard += (value ? 1 : 0);
    }
    PurgeEntriesIfNeeded();
    return value;
}

static inline void UpdateRets(ProfileEntry *slot, Object **rets, size_t n) {
    switch (n) {
        case 0:
            break;
        case 1:
            slot->ret = rets[0]->GetType();
            break;
        default:
            slot->rets = static_cast<int32_t *>(::malloc(n * sizeof(int32_t)));
            for (size_t i = 0; i < n; ++i) {
                slot->rets[i] = rets[i]->GetType();
            }
            break;
    }
}

void Profiler::TraceCalling(int trace_id, Object **rets, size_t n, int line) {
    DCHECK_LT(trace_id, owns_->max_trace_id());
    auto iter = entries_.find(trace_id);
    if (iter == entries_.end()) {
        ProfileEntry slot;
        slot.kind = ProfileEntry::kCalling;
        slot.line = line;
        slot.hit  = 1;
        slot.nrets    = static_cast<int32_t>(n);
        UpdateRets(&slot, rets, n);
        entries_.insert({trace_id, slot});
    } else {
        ProfileEntry *slot = &iter->second;
        DCHECK_EQ(ProfileEntry::kCalling, slot->kind);
        slot->hit++;
        if (slot->nrets > 1) {
            ::free(slot->rets);
        }
        UpdateRets(slot, rets, n);
    }
    PurgeEntriesIfNeeded();
}

void Profiler::PurgeEntriesIfNeeded() {
    if (profiling_ticks_++ < profiling_level_ + 10000) {
        return;
    }
    std::set<int> for_clean;
    for (auto pair : entries_) {
        ProfileEntry *slot = &pair.second;
        if (slot->hit <= profiling_level_) {
            for_clean.insert(pair.first);
            if (slot->kind == ProfileEntry::kCalling && slot->nrets > 1) {
                ::free(slot->rets);
            }
        }
    }
    for (auto trace_id : for_clean) {
        entries_.erase(trace_id);
    }
    profiling_level_ = profiling_ticks_;
}
    
} // namespace nyaa
    
} // namespace mai
