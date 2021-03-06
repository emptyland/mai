#ifndef MAI_LANG_ISOLATE_H_
#define MAI_LANG_ISOLATE_H_

#include "mai/type-defs.h"
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

union Span64;
class Heap;
class MetadataSpace;
class Factory;
class Scheduler;
class Machine;
class Class;
class AbstractValue;
class RootVisitor;
class GarbageCollector;
class Profiler;
class TracingHook;
struct NumberValueSlot;
struct TLSStorage;
struct GlobalHandleNode;
template<class T> class Handle;

enum GCOption {
    kNormalGC,
    kDebugMinorGCEveryTime,
    kDebugFullGCEveryTime,
};

struct Options {
    Env *env = Env::Default(); // The base api env pointer
    int concurrency = 0; // How many concrrent running
    size_t new_space_initial_size = 50L * 1024L * 1024L; // New space initial size: 20MB
    size_t old_space_limit_size = 2L * 1024L * 1024L * 1024L; // Old space limit size: 2GB
    // Minor GC available threshold rate
    float new_space_gc_threshold_rate = 0.25;
    // Major GC available threshold rate
    float old_space_gc_threshold_rate = 0.25;
    // GC Debug mode
    GCOption gc_option = kNormalGC;
    std::string base_pkg_dir = "src/lang/pkg"; // Language base pkg path
    std::set<std::string> search_pkg_dir;
    // Enable Just-in-time compiler
    bool enable_jit = true;
    // JIT compilation hot spot threshold
    int hot_spot_threshold = 10000;
    // Max number of threads for background JIT compiler worker
    int max_jit_compiling_workers = 4;
};

// The virtual machine isolate object:
class Isolate final {
public:
    static const int32_t kOffsetBytecodeHandlerEntries;
    static const int32_t kOffsetTrampolineSuspendPoint;
    static const int32_t kOffsetHotCountSlots;
    
    static Isolate *New(const Options &);
    void Dispose();

    // Initialize Virtual-Machine state
    Error Initialize();
    
    // Load base native functions
    Error LoadBaseLibraries();
    
    // Add a native function for external linker
    Error AddExternalLinkingFunction(const std::string &name, const Handle<Closure> &fun);
    
    // Compile project
    Error Compile(const std::string &dir);
    
    // Run Virtual-Machine
    void Run();

    // Get base api env object
    Env *env() const { return env_; }
    
    // Get new generation memory size in heap
    size_t new_space_initial_size() const { return new_space_initial_size_; }
    
    // Get old generation limit size in heap
    size_t old_space_limit_size() const { return old_space_limit_size_; }
    
    // Get gc option
    GCOption gc_option() const { return gc_option_; }

    void set_gc_option(GCOption option) { gc_option_ = option; }
    
    bool enable_jit() const { return enable_jit_; }
    
    // Get system base pkg dir
    const std::string &base_pkg_dir() const { return base_pkg_dir_; }
    
    // Get uncaught exception count
    int GetUncaughtCount() const;

    // Internal functions:
    inline Heap *heap() const;
    inline MetadataSpace *metadata_space() const;
    inline Scheduler *scheduler() const;
    inline const Class *builtin_type(BuiltinType type) const;
    inline uint8_t **bytecode_handler_entries() const;
    inline uint8_t **tracing_handler_entries() const;
    inline std::atomic<AbstractValue *> *cached_number_value(int slot, int64_t index);
    inline Factory *factory() const;
    inline GarbageCollector *gc() const;
    inline Closure *FindExternalLinkageOrNull(const std::string &name) const;
    inline Closure *TakeExternalLinkageOrNull(const std::string &name);
    inline Span64 *global_space() const;
    inline size_t global_space_length() const;
    inline void SetGlobalSpace(Span64 *spans, uint32_t *bitmap, size_t capacity, size_t length);
    template<class T>
    inline T* global_offset(int location) const;
    inline Profiler *profiler() const;
    inline void set_tracing_hook(TracingHook *hook);
    inline TracingHook *tracing_hook() const;

    void VisitRoot(RootVisitor *visitor);

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
    const size_t old_space_limit_size_;
    const std::string base_pkg_dir_;
    const bool enable_jit_;
    GCOption gc_option_;
    
    // The hook of tracing
    TracingHook *tracing_hook_ = nullptr;

    // External linking native functions
    std::map<std::string, Closure*> external_linkers_;

    Env *env_; // The base api env object
    Heap *heap_; // Heap allocator
    MetadataSpace *metadata_space_; // Metadata memory space
    Factory *factory_; // Some Heap object collection
    Scheduler *scheduler_; // Scheduler
    GarbageCollector *gc_; // Garbage Collector
    Profiler *profiler_; // Profiler for hot point profiiling
    
    Span64 *global_space_ = nullptr; // [nested strong ref] Global space
    uint32_t *global_space_bitmap_ = nullptr; // Bitmap of global space
    size_t global_space_capacity_ = 0; // Capacity of global space
    size_t global_space_length_ = 0; // Length of global space
    
    // [nested strong ref] Global handle double-linked list dummy
    GlobalHandleNode *persistent_dummy_;
    int n_global_handles_ = 0; // Number of global handles
    mutable std::mutex persistent_mutex_; // Mutex for persistent_dummy_
    
    uint8_t **bytecode_handler_entries_; // Entry address of all bytecode handlers
    uint8_t **tracing_handler_entries_; // Entry address of all tracing handlers
    uint8_t *trampoline_suspend_point_; // Entry address of suspend

    int *hot_count_slots_ = nullptr; // Hot count profiling slots
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
