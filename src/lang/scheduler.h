#ifndef MAI_LANG_SCHEDULER_H_
#define MAI_LANG_SCHEDULER_H_

#include "lang/machine.h"
#include "glog/logging.h"

namespace mai {

namespace lang {

class Scheduler final {
public:
    Scheduler(int concurrency);
    ~Scheduler();
    
    // TODO:
    void Schedule(Machine *m) { TODO(); } 
    
    DEF_VAL_GETTER(int, concurrency);
    DEF_PTR_GETTER(Machine, machine0);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Scheduler);
private:
    int concurrency_;
    std::unique_ptr<Machine *[]> all_machines_;
    Machine *machine0_ = nullptr;
    uint32_t next_coid_ = 0;
}; // class Scheduler

} // namespace lang

} // namespace mai

#endif // MAI_LANG_SCHEDULER_H_
