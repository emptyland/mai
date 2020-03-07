#include "lang/isolate-inl.h"
#include "lang/scheduler.h"
#include "lang/machine.h"
#include "lang/bytecode.h"
#include "lang/heap.h"
#include "lang/metadata-space.h"
#include "lang/factory.h"
#include "lang/compiler.h"
#include "lang/syntax.h"
#include "lang/runtime.h"
#include "lang/value-inl.h"
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

class ErrorFeedback : public SyntaxFeedback {
public:
    void DidFeedback(const SourceLocation &location, const char *z, size_t n) override {
        ::printf("%s:%d:%d-%d:%d %s\n", file_name_.c_str(), location.begin_line, location.begin_row,
                 location.end_line, location.end_row, z);
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

    if (auto err = heap_->Initialize(new_space_initial_size_); err.fail()) {
        return err;
    }

    if (auto err = metadata_space_->Initialize(); err.fail()) {
        return err;
    }
    
    for (int i = 0; i < kMax_Bytecodes; i++) {
        bytecode_handler_entries_[i] = DCHECK_NOTNULL(metadata_space_->bytecode_handlers()[i])->entry();
    }
    trampoline_suspend_point_ = metadata_space_->trampoline_suspend_point();

    initialized_ = true;
    return Error::OK();
}

Error Isolate::LoadBaseLibraries() {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    Local<Closure> fun(FunctionTemplate::New(Runtime::lang_println));
    if (auto rs = AddExternalLinkingFunction("lang.println", fun); !rs) {
        return rs;
    }
    
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
    
    ErrorFeedback feedback;
    base::StandaloneArena arena(env_->GetLowLevelAllocator());

    return Compiler::CompileInterpretion(this, dir, &feedback, &arena);
}

void Isolate::Run() {
    DCHECK(initialized_);
    scheduler_->Start(); // Start worker threads
    
    // Run main thread
    scheduler_->machine0()->Entry();
    
    // Main thread finished
    // Should shutdown others worker threads
    scheduler_->Shutdown();
}

Isolate::Isolate(const Options &opts)
    : new_space_initial_size_(opts.new_space_initial_size)
    , base_pkg_dir_(opts.base_pkg_dir)
    , env_(opts.env)
    , heap_(new Heap(env_->GetLowLevelAllocator()))
    , metadata_space_(new MetadataSpace(env_->GetLowLevelAllocator()))
    , scheduler_(new Scheduler(opts.concurrency <= 0 ?
                               env_->GetNumberOfCPUCores() : opts.concurrency,
                               opts.env->GetLowLevelAllocator()))
    , persistent_dummy_(new GlobalHandleNode{})
    , bytecode_handler_entries_(new Address[kMax_Bytecodes])
    , trampoline_suspend_point_(reinterpret_cast<Address>(BadSuspendPointDummy))
    , factory_(new Factory()) {
}

Isolate::~Isolate() {
    delete factory_;
    delete [] bytecode_handler_entries_;
    
    while (!QUEUE_EMPTY(persistent_dummy_)) {
        auto x = persistent_dummy_->next_;
        QUEUE_REMOVE(x);
        delete x;
    }
    delete persistent_dummy_;

    delete metadata_space_;
    delete scheduler_;
    delete heap_;
}

void Isolate::Enter() {
    {
        std::lock_guard<std::mutex> lock(init_mutex);
        DCHECK(__isolate == nullptr) << "Reinitialize isolate";
        __isolate = this;
        DCHECK_EQ(this, __isolate);
    }
    InitializeSyntaxLibrary();

    // Init machine0
    scheduler_->machine0()->Enter();

    // Initialize factory
    factory_->Initialize();
}

void Isolate::Exit() {
    // Exit machine0
    scheduler_->machine0()->Exit();

    FreeSyntaxLibrary();
    {
        std::lock_guard<std::mutex> lock(init_mutex);
        DCHECK_EQ(this, __isolate);
        __isolate = nullptr;
    }
}

} // namespace lang

} // namespace mai
