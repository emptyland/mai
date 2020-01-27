#ifndef MAI_LANG_MACHINE_H_
#define MAI_LANG_MACHINE_H_

#include "lang/isolate-inl.h"
#include "base/base.h"
#include "glog/logging.h"
#include <thread>
#include <atomic>

namespace mai {

namespace lang {

class Scheduler;

class Machine {
public:
    enum State: int {
        kDead,
        kIdle,
        kRunning,
    };
    
    Machine(int id, Scheduler *owner)
        : id_(id)
        , owner_(owner) {}
    
    DEF_VAL_GETTER(int, id);
    
    static Machine *Get() { return __isolate->tls_storage()->machine; }
    
    inline void Run();
    
    friend class MachineScope;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Machine);
private:
    void Entry() {
        // TODO:
        TODO();
    }
    
    void Enter() {
        DCHECK(__isolate->tls_storage() == nullptr);
        __isolate->tls()->Set(new TLSStorage(this));
    }

    void Leave() { DCHECK_EQ(this, __isolate->tls_storage()->machine); }
    
    const int id_; // Machine id
    Scheduler *const owner_; // Scheduler
    std::atomic<State> state_ = kDead; // Current machine state
    std::thread thread_; // Thread object
}; // class Machine


class MachineScope final {
public:
    MachineScope(Machine *owns) : owns_(owns) { owns_->Enter(); }
    ~MachineScope() { owns_->Leave(); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(MachineScope);
private:
    Machine *const owns_;
}; // class MachineScope


inline void Machine::Run() {
    thread_ = std::thread([this] () {
        MachineScope machine_scope(this);
        Enter();
    });
}

} // namespace lang

} // namespace mai


#endif // MAI_LANG_MACHINE_H_
