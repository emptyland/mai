#include "lang/isolate-inl.h"
#include "lang/garbage-collector.h"
#include "lang/scheduler.h"
#include "lang/machine.h"
#include "lang/bytecode.h"
#include "lang/heap.h"
#include "lang/metadata-space.h"
#include "lang/factory.h"
#include "lang/compiler.h"
#include "lang/runtime.h"
#include "lang/value-inl.h"
#include "lang/channel.h"
#include "lang/pgo.h"
#include "lang/compilation-worker.h"
#include "asm/utils.h"
#include "base/arenas.h"
#include "glog/logging.h"
#include <mutex>

namespace mai {

namespace lang {

static std::mutex init_mutex;

Isolate *__isolate = nullptr; // Global isolate object

thread_local TLSStorage *__tls_storage; // Global tls 

#define MEMBER_OFFSET_OF(field) \
    arch::ObjectTemplate<Isolate, int32_t>::OffsetOf(&Isolate :: field)

const ptrdiff_t GlobalHandleNode::kOffsetHandle = offsetof(GlobalHandleNode, handle);

const int32_t Isolate::kOffsetBytecodeHandlerEntries = MEMBER_OFFSET_OF(bytecode_handler_entries_);
const int32_t Isolate::kOffsetTrampolineSuspendPoint = MEMBER_OFFSET_OF(trampoline_suspend_point_);
const int32_t Isolate::kOffsetHotCountSlots = MEMBER_OFFSET_OF(hot_count_slots_);

class ErrorFeedback : public SyntaxFeedback {
public:
    void DidFeedback(const SourceLocation &location, const char *z, size_t n) override {
        ::printf("%s:%d:%d-%d:%d %s\n", file_name_.c_str(), location.begin_line, location.begin_row,
                 location.end_line, location.end_row, z);
    }
    
    void DidFeedback(const char *z) override {
        ::puts(z);
    }
};

static void BadSuspendPointDummy() {
    NOREACHED() << "Bad suspend-point";
}

/*static*/ Isolate *Isolate::New(const Options &opts) {
    return new Isolate(opts);
}

void Isolate::Dispose() {
    delete this;
}

Error Isolate::Initialize() {
    if (new_space_initial_size_ < 10 * base::kMB) {
        return MAI_CORRUPTION("new_space_initial_size too small(least than 10MB)");
    }
    if (enable_jit_ && compilation_worker_->max_workers() <= 0) {
        return MAI_CORRUPTION("no wokers for JIT compiler");
    }

    if (auto err = heap_->Initialize(new_space_initial_size_); err.fail()) {
        return err;
    }

    if (auto err = metadata_space_->Initialize(false/*enable_debug*/, enable_jit_); err.fail()) {
        return err;
    }
    
    for (int i = 0; i < kMax_Bytecodes; i++) {
        bytecode_handler_entries_[i] =
            DCHECK_NOTNULL(metadata_space_->bytecode_handlers()[i])->entry();
        if (enable_jit_) {
            tracing_handler_entries_[i] =
                DCHECK_NOTNULL(metadata_space_->tracing_handlers()[i])->entry();
        }
    }
    trampoline_suspend_point_ = metadata_space_->trampoline_suspend_point();

    initialized_ = true;
    return Error::OK();
}

Error Isolate::LoadBaseLibraries() {
    HandleScope handle_scope(HandleScope::INITIALIZER);
#define ADD_EXTERNAL_RUNTIME_FUNCTION(name, symbol) \
    if (auto rs = AddExternalLinkingFunction(symbol, FunctionTemplate::New(Runtime::name)); !rs) { \
        return rs; \
    }
    DECLARE_RUNTIME_FUNCTIONS(ADD_EXTERNAL_RUNTIME_FUNCTION)
#undef ADD_EXTERNAL_RUNTIME_FUNCTION
    
    return Error::OK();
}

Error Isolate::AddExternalLinkingFunction(const std::string &name, const Handle<Closure> &fun) {
    if (!fun->is_cxx_function()) {
        return MAI_CORRUPTION("Invalid function type");
    }
    if (auto iter = external_linkers_.find(name); iter != external_linkers_.end()) {
        return MAI_CORRUPTION("Duplicated function name");
    }
    external_linkers_[name] = *fun;
    return Error::OK();
}

Error Isolate::Compile(const std::string &dir) {
    DCHECK(initialized_);
    if (!initialized_) {
        return MAI_CORRUPTION("Not initialize yet");
    }
    
    Function *init0_fun = nullptr;
    ErrorFeedback feedback;
    base::StandaloneArena arena;
    if (auto rs = Compiler::CompileInterpretion(this, dir, &feedback, &init0_fun, &arena); !rs) {
        return rs;
    }
    if (enable_jit_) {
        profiler_->Reset();
        hot_count_slots_ = profiler_->hot_count_slots();
    }
    
    Closure *init0 = scheduler_->machine0()->NewClosure(init0_fun, 0, Heap::kOld);
    Coroutine *co = scheduler_->NewCoroutine(init0, true/*co0*/);
    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
    scheduler_->machine0()->PostRunnable(co, true/*now*/);
    return Error::OK();
}

void Isolate::Run() {
    DCHECK(initialized_);
    scheduler_->Start(); // Start worker threads
    
    // Run main thread
    scheduler_->machine0()->Entry();

    // Shutdown background works first
    compilation_worker_->Shutdown();
    
    // Main thread finished
    // Should shutdown others worker threads
    scheduler_->Shutdown();
}

Isolate::Isolate(const Options &opts)
    : new_space_initial_size_(opts.new_space_initial_size)
    , old_space_limit_size_(opts.old_space_limit_size)
    , gc_option_(opts.gc_option)
    , base_pkg_dir_(opts.base_pkg_dir)
    , env_(opts.env)
    , heap_(new Heap(env_->GetLowLevelAllocator()))
    , metadata_space_(new MetadataSpace(env_->GetLowLevelAllocator()))
    , scheduler_(new Scheduler(opts.concurrency <= 0 ?
                               env_->GetNumberOfCPUCores() : opts.concurrency,
                               opts.env->GetLowLevelAllocator(), opts.enable_jit))
    , gc_(new GarbageCollector(this, opts.new_space_gc_threshold_rate,
                               opts.old_space_gc_threshold_rate))
    , enable_jit_(opts.enable_jit)
    , profiler_(new Profiler(opts.hot_spot_threshold))
    , compilation_worker_(new CompilationWorker(opts.max_jit_compiling_workers))
    , persistent_dummy_(new GlobalHandleNode{})
    , bytecode_handler_entries_(new Address[kMax_Bytecodes])
    , tracing_handler_entries_(opts.enable_jit ? new Address[kMax_Bytecodes] : nullptr)
    , trampoline_suspend_point_(reinterpret_cast<Address>(BadSuspendPointDummy))
    , factory_(new Factory()) {
}

Isolate::~Isolate() {
    delete [] global_space_;
    delete [] global_space_bitmap_;
    delete factory_;
    delete [] bytecode_handler_entries_;
    
    while (!QUEUE_EMPTY(persistent_dummy_)) {
        auto x = persistent_dummy_->next_;
        QUEUE_REMOVE(x);
        delete x;
    }
    delete persistent_dummy_;

    delete compilation_worker_;
    delete gc_;
    delete metadata_space_;
    delete scheduler_;
    delete heap_;
    delete profiler_;
}

int Isolate::GetUncaughtCount() const {
    int count = 0;
    for (int i = 0; i < scheduler_->concurrency(); i++) {
        count += scheduler_->machine(i)->uncaught_count();
    }
    return count;
}

void Isolate::Enter() {
    {
        std::lock_guard<std::mutex> lock(init_mutex);
        DCHECK(__isolate == nullptr) << "Reinitialize isolate";
        __isolate = this;
        DCHECK_EQ(this, __isolate);
    }

    // Init machine0
    scheduler_->machine0()->Enter();

    // Initialize factory
    factory_->Initialize();
}

void Isolate::Exit() {
    // Exit machine0
    scheduler_->machine0()->Exit();

    {
        std::lock_guard<std::mutex> lock(init_mutex);
        DCHECK_EQ(this, __isolate);
        __isolate = nullptr;
    }
}

template<class T>
inline size_t GetBitmapSize(size_t n_items) {
    return (n_items + (sizeof(T) * CHAR_BIT) - 1) / (sizeof(T) * CHAR_BIT);
}

void Isolate::VisitRoot(RootVisitor *visitor) {
    for (size_t i = 0; i < global_space_length_; i++) {
        if (global_space_bitmap_[i / 32] & (1u << (i % 32))) {
            Address base = reinterpret_cast<Address>(global_space_ + i);
            visitor->VisitRootPointers(reinterpret_cast<Any **>(base),
                                       reinterpret_cast<Any **>(base + sizeof(Span64)));
        }
    }
    for (GlobalHandleNode *node = persistent_dummy_->next_; node != persistent_dummy_;
         node = node->next_) {
        visitor->VisitRootPointer(&node->handle);
    }
    for (auto &pair : external_linkers_) {
        visitor->VisitRootPointer(reinterpret_cast<Any **>(&pair.second));
    }
    metadata_space_->VisitRoot(visitor);
    factory_->VisitRoot(visitor);
    scheduler_->VisitRoot(visitor);
}

} // namespace lang

} // namespace mai
