#ifndef MAI_LANG_COMPILATION_WORK_H_
#define MAI_LANG_COMPILATION_WORK_H_

#include "base/base.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <deque>
#include <memory>

namespace mai {

namespace lang {

class CompilationJob {
public:
    CompilationJob() = default;
    virtual ~CompilationJob() = default;
    virtual void Run() = 0;
    virtual void Dispose() {}

    DISALLOW_IMPLICIT_CONSTRUCTORS(CompilationJob);
}; // class CompilationJob

class CompilationWorker final {
public:
    CompilationWorker(int max_workers);
    ~CompilationWorker();
    
    int shutting_down() const { return shutting_down_.load(std::memory_order_acquire); }
    int running_workers() const { return running_workers_.load(std::memory_order_acquire); }
    DEF_VAL_GETTER(int, max_workers);
    
    void Commit(CompilationJob *job);
    
    size_t Shutdown();
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(CompilationWorker);
private:
    class Worker;
    
    void Work(Worker *worker, CompilationJob *job);
    
    CompilationJob *TakeJob() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return nullptr;
        }
        CompilationJob *job = queue_.front();
        queue_.pop_front();
        return job;
    }
    
    const int max_workers_;
    std::atomic<int> running_workers_ = {0};
    std::atomic<int> shutting_down_ = {0};
    std::deque<CompilationJob *> queue_;
    std::unique_ptr<Worker[]> workers_;
    std::mutex mutex_;
}; // class CompilationWorker



} // namespace mai

} // namespace mai

#endif // MAI_LANG_COMPILATION_WORK_H_
