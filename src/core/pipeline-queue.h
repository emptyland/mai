#ifndef MAI_CORE_PIPELINE_QUEUE_H_
#define MAI_CORE_PIPELINE_QUEUE_H_

#include "base/base.h"
#include <mutex>
#include <deque>
#include <vector>
#include <algorithm>

namespace mai {
    
namespace core {

template<class T>
class PipelineQueue {
public:
    PipelineQueue() {}
    
    void Add(T value) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push_back(value);
    }
    
    bool Take(T *result) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        *result = queue_.front();
        queue_.pop_front();
        return true;
    }
    
    void PeekAll(std::vector<T> *result) const {
        std::unique_lock<std::mutex> lock(mutex_);
        size_t old_size = result->size();
        result->resize(old_size + queue_.size());
        std::copy(queue_.begin(), queue_.end(), result->begin() + old_size);
    }
    
    bool InProgress() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return !queue_.empty();
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(PipelineQueue);
private:
    std::deque<T> queue_;
    mutable std::mutex mutex_;
}; // template<class T> class PipelineQueue
    
} // namespace core
    
} // namespace mai


#endif // MAI_CORE_PIPELINE_QUEUE_H_
