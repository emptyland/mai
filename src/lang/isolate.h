#ifndef MAI_LANG_ISOLATE_H_
#define MAI_LANG_ISOLATE_H_

#include "lang/type-defs.h"
#include "mai/env.h"
#include "mai/error.h"
#include <memory>

namespace mai {
namespace test {
class IsolateInitializer;
} // namespace test
namespace lang {

class Heap;
class MetadataSpace;
class Scheduler;
class Machine;
class Class;
struct TLSStorage;

struct Options {
    Env *env = Env::Default(); // The base api env pointer
    int concurrency = 0; // How many concrrent running
    size_t new_space_initial_size = 100 * 1024 * 1024; // New space initial size: 100MB
};


// The virtual machine isolate object:
class Isolate final {
public:
    static Isolate *New(const Options &);
    void Dispose();

    Error Initialize();
    
    size_t new_space_initial_size() const { return new_space_initial_size_; }
    
    inline Heap *heap() const;
    inline MetadataSpace *metadata_space() const;
    inline TLSStorage *tls_storage() const;
    inline Machine *tls_machine() const;
    inline Scheduler *scheduler() const;
    
    inline const Class *builtin_type(BuiltinType type) const;
    
    friend class Machine;
    friend class IsolateScope;
    friend class test::IsolateInitializer;
    Isolate(const Isolate&) = delete;
    void operator = (const Isolate &) = delete;
private:
    Isolate(const Options &opts);
    ~Isolate();

    void Enter();
    void Exit();
    
    ThreadLocalSlot *tls() const { return tls_.get(); }
    
    const size_t new_space_initial_size_;
    
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
        owns_->Exit();
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
