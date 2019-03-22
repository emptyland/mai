#include "nyaa/nyaa-core.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/thread.h"
#include "nyaa/heap.h"
#include "nyaa/object-factory-impl.h"
#include "nyaa/builtin.h"
#include "base/slice.h"
#include "mai-lang/nyaa.h"
#include "mai/allocator.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {

NyaaCore::NyaaCore(Nyaa *stub)
    : stub_(DCHECK_NOTNULL(stub))
    , page_alloc_(stub->isolate()->env()->GetLowLevelAllocator())
    , heap_(new Heap(this))
    , factory_(new ObjectFactoryImpl(this, heap_))
    , bkz_pool_(new BuiltinStrPool())
    , kmt_pool_(new BuiltinMetatablePool()) {

    top_slot_ = new HandleScopeSlot;
    top_slot_->scope = nullptr;
    top_slot_->prev  = nullptr;
    top_slot_->base  = nullptr;
    top_slot_->end   = top_slot_->base;
    top_slot_->limit = top_slot_->end;
        
    Error rs = Boot();
    if (!rs) {
        DLOG(ERROR) << rs.ToString();
    }
}

NyaaCore::~NyaaCore() {
    delete factory_;
    delete heap_;
}
    
Error NyaaCore::Boot() {
    Error rs = heap_->Prepare();
    if (!rs) {
        return rs;
    }
    
    NyString **pool_a = &bkz_pool_->kInnerInit;
    for (size_t i = 0; i < kRawBuiltinKzsSize; ++i) {
        pool_a[i] = factory_->NewString(kRawBuiltinKzs[i]);
    }
    
    rs = kmt_pool_->Boot(this);
    if (!rs) {
        return rs;
    }
    
    // Set builtin global variables:
    g_ = factory_->NewMap(32, rand(), false);
    NyString *name = factory_->NewString("thread");
    SetGlobal(name, kmt_pool_->kThread);
    
    for (auto e = &kBuiltinFnEntries[0]; e->name; e++) {
        SetGlobal(factory_->NewString(e->name),
                  factory_->NewDelegated(e->nafn));
    }
    
    // Setup main_thread
    main_thd_ = factory_->NewThread(true /* old */);
    rs = main_thd_->Init();
    if (!rs) {
        return rs;
    }
    return rs;
}
    
Object *NyaaCore::Get(int i) {
    if (i < 0) {
        if (main_thd_->stack_tp_ + i < main_thd_->stack_bp()) {
            Raisef("stack index: %d out of range.", i);
            return nullptr;
        }
    } else {
        if (main_thd_->stack_bp() + i >= main_thd_->stack_tp_) {
            Raisef("stack index: %d out of range.", i);
            return nullptr;
        }
    }
    return main_thd_->Get(i);
}

void NyaaCore::Pop(int n) {
    if (main_thd_->stack_tp_ - n < main_thd_->stack_bp()) {
        Raisef("stack pop: %d out of range.", n);
    } else {
        main_thd_->Pop(n);
    }
}
    
void NyaaCore::SetGlobal(NyString *name, Object *value) { g_->Put(name, value, this); }
    
void NyaaCore::Raisef(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::string msg(base::Vsprintf(fmt, ap));
    va_end(ap);
    //has_raised_ = true;
    
    // TODO:
    DLOG(FATAL) << msg;
}
    
bool NyaaCore::Raised() const {
    return main_thd_->has_raised_;
    // TODO:
}
    
void NyaaCore::Unraise() {
    main_thd_->has_raised_ = false;
    // TODO:
}
    
Isolate *NyaaCore::isolate() const { return stub_->isolate_; }
    
/*static*/ NyaaCore *NyaaCore::Current() {
    return Isolate::Current()->GetNyaa()->core();
}

void NyaaCore::EnterHandleScope(HandleScope *handle_scope) {
    auto prev_slot = top_slot_;
    auto slot = new HandleScopeSlot{DCHECK_NOTNULL(handle_scope), prev_slot};
    slot->base  = prev_slot->end;
    slot->end   = slot->base;
    slot->limit = prev_slot->limit;
    top_slot_ = slot;
}

void NyaaCore::ExitHandleScope() {
    auto slot = top_slot_;
    top_slot_ = slot->prev;
    if (slot->prev->limit != slot->limit &&
        (slot->limit - slot->base) % page_alloc_->granularity() == 0) {
        page_alloc_->Free(slot->base, slot->limit - slot->base);
    }
    delete slot;
}

Address NyaaCore::AdvanceHandleSlots(int n_slots) {
    auto slot = top_slot_;
    DCHECK_GE(n_slots, 0);
    if (!n_slots) {
        return DCHECK_NOTNULL(slot)->end;
    }
    
    auto size = n_slots * sizeof(NyObject **);
    if (slot->end + size >= slot->limit) {
        auto backup = slot->base;
        auto growed_size = RoundUp((slot->limit - slot->base), page_alloc_->granularity())
                + page_alloc_->granularity();
        slot->base = static_cast<Address>(page_alloc_->Allocate(growed_size,
                                                                page_alloc_->granularity()));
        slot->limit = slot->base + growed_size;
        ::memcpy(slot->base, backup, slot->end - backup);
        slot->end = slot->base + (slot->end - backup);
    }
    
    DCHECK_LT(slot->end, slot->limit);
    auto slot_addr = slot->end;
    slot->end += size;
    return slot_addr;
}
    
} // namespace nyaa
    
} // namespace mai
