#include "lang/compilation-worker.h"
#include "glog/logging.h"

namespace mai {

namespace lang {

class CompilationWorker::Worker {
public:
    enum State {
        kDead,
        kRunnable,
        kRunning,
        kWaitting,
    };
    
    Worker() = default;
    
    ~Worker() {
//        if (thread_.joinable()) {
//            thread_.join();
//        }
//        thread_.detach();
    }

    State state() const { return state_.load(std::memory_order_acquire); }

    bool TestAndSetState(State expect, State value) {
        return state_.compare_exchange_strong(expect, value);
    }

    void set_thread(std::thread &&thread) { thread_ = std::move(thread); }
    std::thread *mutable_thread() { return &thread_; }
    
    void Run(CompilationWorker *owns, CompilationJob *job) {
        if (thread_.joinable()) {
            thread_.join();
        }
        thread_ = std::thread([this] (CompilationWorker *owns, CompilationJob *job) {
            bool ok = TestAndSetState(Worker::kRunnable, Worker::kRunning);
            DCHECK(ok);

            owns->Work(this, job);
            
            ok = TestAndSetState(Worker::kRunning, Worker::kDead);
            DCHECK(ok); (void)ok;
        }, owns, job);
    }
    
private:
    std::thread thread_;
    std::atomic<State> state_ = {kDead};
}; // CompilationWorker::Worker

CompilationWorker::CompilationWorker(int max_workers)
    : max_workers_(max_workers)
    , workers_(new Worker[max_workers <= 0 ? 0 : max_workers]) {
    DCHECK_GT(max_workers_, 0);
}

CompilationWorker::~CompilationWorker() {
}

void CompilationWorker::Commit(CompilationJob *job) {
    for (int i = 0; i < max_workers_; i++) {
        if (shutting_down()) {
            return;
        }
        
        if (workers_[i].TestAndSetState(Worker::kDead, Worker::kRunnable)) {
            workers_[i].Run(this, job);
            return;
        }
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (shutting_down()) {
        return;
    }
    queue_.push_back(job);

    while (running_workers() == 0) {
        for (int i = 0; i < max_workers_; i++) {
            if (shutting_down()) {
                return;
            }
            
            if (workers_[i].TestAndSetState(Worker::kDead, Worker::kRunnable)) {
                workers_[i].Run(this, nullptr);
                return;
            }
        }
    }
}

size_t CompilationWorker::Shutdown() {
    Pause();

    std::lock_guard<std::mutex> lock(mutex_);
    size_t drops = queue_.size();
    for (auto job : queue_) {
        job->Dispose();
        delete job;
    }
    queue_.clear();
    return drops;
}

void CompilationWorker::Pause() {
    shutting_down_.fetch_add(1);

    do {
        for (int i = 0; i < max_workers_; i++) {
            if (workers_[i].state() > Worker::kDead) {
                workers_[i].mutable_thread()->join();
            } else if (workers_[i].mutable_thread()->joinable()) {
                workers_[i].mutable_thread()->join();
            }
        }
    } while (running_workers() > 0);
}

void CompilationWorker::Resume() {
    shutting_down_.store(0, std::memory_order_release);
    mutex_.lock();
    bool not_empty = !queue_.empty();
    mutex_.unlock();
    
    if (not_empty) {
        for (int i = 0; i < max_workers_; i++) {
            if (workers_[i].TestAndSetState(Worker::kDead, Worker::kRunnable)) {
                workers_[i].Run(this, nullptr);
                return;
            }
        }
    }
}

void CompilationWorker::Work(Worker *worker, CompilationJob *job) {
    running_workers_.fetch_add(1);
    
    if (job) {
        job->Run();
        job->Dispose();
        delete job;
    }
    
    job = nullptr;
    while (!shutting_down()) {
        if (job = TakeJob(); !job) {
            for (int n = 1; n < 2048; n <<= 1) {
                for (int i = 0; i < n; i++) {
                    // spin
                    __asm__ ("pause");
                }

                if (job = TakeJob(); job) {
                    break;
                }
            }
        }
        if (!job) {
            break;
        }
        
        DCHECK_NOTNULL(job)->Run();
        job->Dispose();
        delete job;
    }

    running_workers_.fetch_sub(1);
}

} // namespace lang

} // namespace mai
