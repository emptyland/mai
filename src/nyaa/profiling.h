#ifndef MAI_NYAA_PROFILING_H_
#define MAI_NYAA_PROFILING_H_

#include "base/base.h"
#include <unordered_map>
#include <vector>

namespace mai {
    
namespace nyaa {
    
class NyaaCore;
class Object;
    
struct ProfileEntry {
    enum Kind {
        kBranch,
        kCalling,
    };
    Kind kind;
    int line; // line of source
    int32_t nrets; // numbers of nrets
    uint64_t hit; // total count
    union {
        uint64_t true_guard; // branch true guard count
        int32_t ret; // if nrets <= 1
        int32_t *rets; // if nrets > 1
    };
}; // struct ProfilingSlot


class Profiler final {
public:
    Profiler(NyaaCore *owns) : owns_(owns) {}
    ~Profiler();
    
    const ProfileEntry *FindOrNull(int trace_id) const {
        auto iter = entries_.find(trace_id);
        return iter == entries_.end() ? nullptr : &iter->second;
    }
    
    std::vector<int> GetAllTraces() const {
        std::vector<int> all;
        for (const auto &pair : entries_) {
            all.push_back(pair.first);
        }
        return all;
    }
    
    int TraceBranchGuard(int trace_id, int value, int line);
    void TraceCalling(int trace_id, Object **rets, size_t n, int line);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Profiler);
private:
    void PurgeEntriesIfNeeded();

    NyaaCore *owns_;
    std::unordered_map<int, ProfileEntry> entries_; // profiling info
    uint64_t profiling_level_ = 0; // latest profiling ticks
    uint64_t profiling_ticks_ = 0; // profiling ticks
}; // class Profiler
    
} // namespace nyaa

} // namespace mai


#endif // MAI_NYAA_PROFILING_H_
