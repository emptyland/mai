#ifndef MAI_LANG_ISOLATE_H_
#define MAI_LANG_ISOLATE_H_

#include "lang/type-defs.h"
#include "mai/env.h"
#include "mai/error.h"
#include <memory>
#include <mutex>
#include <set>
#include <map>

namespace mai {
namespace test {
class IsolateInitializer;
} // namespace test
namespace lang {

union Span32;
class Heap;
class MetadataSpace;
class Factory;
class Scheduler;
class Machine;
class Class;
class AbstractValue;
struct NumberValueSlot;
struct TLSStorage;
struct GlobalHandleNode;
template<class T> class Handle;

struct Options {
    Env *env = Env::Default(); // The base api env pointer
    int concurrency = 2; // How many concrrent running
    size_t new_space_initial_size = 100 * 1024 * 1024; // New space initial size: 100MB
    std::string base_pkg_dir = "src/lang/pkg"; // Language base pkg path
    std::set<std::string> search_pkg_dir;
};


// The virtual machine isolate object:
class Isolate final {
public:
    static const int32_t kOffsetBytecodeHandlerEntries;
    static const int32_t kOffsetTrampolineSuspendPoint;
    
    static Isolate *New(const Options &);
    void Dispose();

    // Initialize Virtual-Machine state
    Error Initialize();
    
    // Add a native function for external linker
    Error AddExternalLinkingFunction(const Handle<Closure> &fun);
    
    // Compile project
    Error Compile(const std::string &dir);
    
    // Run Virtual-Machine
    void Run();

    // Get base api env object
    Env *env() const { return env_; }
    
    // Get new generation memory size in heap
    size_t new_space_initial_size() const { return new_space_initial_size_; }
    
    // Get system base pkg dir
    const std::string &base_pkg_dir() const { return base_pkg_dir_; }

    // Internal functions:
    inline Heap *heap() const;
    inline MetadataSpace *metadata_space() const;
    inline Scheduler *scheduler() const;
    inline const Class *builtin_type(BuiltinType type) const;
    inline uint8_t **bytecode_handler_entries() const;
    inline std::atomic<AbstractValue *> *cached_number_value(int slot, int64_t index);
    inline Factory *factory() const;
    inline Closure *FindExternalLinkageOrNull(std::string_view name) const;
    inline Closure *TakeExternalLinkageOrNull(std::string_view name);

    friend class Machine;
    friend class IsolateScope;
    friend class GlobalHandles;
    friend class test::IsolateInitializer;
    Isolate(const Isolate&) = delete;
    void operator = (const Isolate &) = delete;
private:
    Isolate(const Options &opts);
    ~Isolate();

    void Enter();
    void Exit();
    
    inline GlobalHandleNode *NewGlobalHandle(const void *pointer);
    inline void DeleteGlobalHandle(GlobalHandleNode *node);
    inline int GetNumberOfGlobalHandles() const;
    
    inline NumberValueSlot *cached_number_slot(int index);

    // New space initial bytes size
    const size_t new_space_initial_size_;
    const std::string base_pkg_dir_;

    // External linking native functions
    std::map<std::string_view, Closure*> external_linkers_;

    Env *env_; // The base api env object
    Heap *heap_; // Heap allocator
    MetadataSpace *metadata_space_; // Metadata memory space
    Factory *factory_; // Some Heap object collection
    Scheduler *scheduler_; // Scheduler
    
    GlobalHandleNode *persistent_dummy_; // Global handle double-linked list dummy
    int n_global_handles_ = 0; // Number of global handles
    mutable std::mutex persistent_mutex_; // Mutex for persistent_dummy_
    
    uint8_t **bytecode_handler_entries_; // Entry address of all bytecode handlers
    uint8_t *trampoline_suspend_point_; // Entry address of suspend

    bool initialized_ = false;
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
