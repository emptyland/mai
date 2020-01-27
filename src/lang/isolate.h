#ifndef MAI_LANG_ISOLATE_H_
#define MAI_LANG_ISOLATE_H_

#include "mai/env.h"
#include "mai/error.h"
#include <memory>

namespace mai {

namespace lang {

class Heap;
class MetadataSpace;
class Scheduler;
struct TLSStorage;

struct Options {
    Env *env = Env::Default(); // The base api env pointer
    int concurrency = 0; // How many concrrent running
};


// The virtual machine isolate object:
class Isolate final {
public:
    static Isolate *New(const Options &);
    void Dispose();

    Error Initialize();
    
    inline Heap *heap() const;
    inline MetadataSpace *metadata_space() const;
    inline TLSStorage *tls_storage() const;
    inline Scheduler *scheduler() const;
    
    friend class Machine;
    friend class IsolateScope;
    Isolate(const Isolate&) = delete;
    void operator = (const Isolate &) = delete;
private:
    Isolate(const Options &opts);
    ~Isolate();
    
    void Enter();
    void Leave();
    
    ThreadLocalSlot *tls() const { return tls_.get(); }
    
    Env *env_;
    Heap *heap_;
    MetadataSpace *metadata_space_;
    Scheduler *scheduler_;
    std::unique_ptr<ThreadLocalSlot> tls_;
}; // class Isolate


class IsolateScope final {
public:
    IsolateScope(Isolate *owns)
        : owns_(owns) { owns->Enter(); }

    ~IsolateScope() {
        owns_->Leave();
        owns_->Dispose();
    }
    
    IsolateScope(const IsolateScope &) = delete;
    void operator = (const IsolateScope &) = delete;
private:
    Isolate *owns_;
}; // class IsolateScope

} // namespace lang

} // namespace mai

#endif // MAI_LANG_ISOLATE_H_
