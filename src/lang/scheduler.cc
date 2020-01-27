#include "lang/scheduler.h"

namespace mai {

namespace lang {

Scheduler::Scheduler(int concurrency)
    : concurrency_(concurrency)
    , all_machines_(new Machine*[concurrency]){
    DCHECK_GT(concurrency, 0);
    
    for (int i = 0; i < concurrency_; i++) {
        all_machines_[i] = new Machine(i, this);
    }
    machine0_ = all_machines_[0];
    DCHECK_EQ(0, machine0_->id());
}

Scheduler::~Scheduler() {
    
}

} // namespace lang

} // namespace mai
