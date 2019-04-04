#include "nyaa/nyaa-core.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/thread.h"
#include "nyaa/heap.h"
#include "nyaa/object-factory-impl.h"
#include "nyaa/malloc-object-factory.h"
#include "nyaa/builtin.h"
#include "nyaa/visitors.h"
#include "nyaa/string-pool.h"
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
    , bkz_pool_(new BuiltinStrPool())
    , kmt_pool_(new BuiltinMetatablePool()) {
    if (stub_->nogc()) {
        factory_.reset(new MallocObjectFactory(this));
    } else {
        factory_.reset(new ObjectFactoryImpl(this, heap_.get()));
    }
        
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

}
    
Error NyaaCore::Boot() {
    Error rs = heap_->Init();
    if (!rs) {
        return rs;
    }
    
    NyString **pool_a = reinterpret_cast<NyString **>(bkz_pool_.get());
    //NyString **pool_a = &bkz_pool_->kInnerInit;
    for (size_t i = 0; i < kRawBuiltinKzsSize; ++i) {
        pool_a[i] = factory_->NewString(kRawBuiltinKzs[i], true /*old*/);
    }

    rs = kmt_pool_->Boot(this);
    if (!rs) {
        return rs;
    }
    
    // Set builtin global variables:
    g_ = factory_->NewMap(32, /*capacity*/ rand(), /*seed*/ 0, /*kid*/ false, /*linear*/
                          true /*old*/);
    
    for (auto e = &kBuiltinFnEntries[0]; e->name; e++) {
        SetGlobal(factory_->NewString(e->name),
                  factory_->NewDelegated(e->nafn));
    }
    
    // Setup main_thread
    main_thd_ = factory_->NewThread(true /* old */);
    main_thd_->next_ = main_thd_;
    main_thd_->prev_ = main_thd_;
    rs = main_thd_->Init();
    if (!rs) {
        return rs;
    }
    curr_thd_ = main_thd_;

    // Setup builtin classes.
    SetGlobal(factory_->NewString("coroutine"), kmt_pool_->kThread);
    
    DCHECK(!initialized_);
    initialized_ = true;
    return rs;
}
    
Object *NyaaCore::Get(int i) {
    if (i < 0) {
        if (main_thd_->stack_tp_ + i < main_thd_->frame_bp()) {
            Raisef("stack index: %d out of range.", i);
            return nullptr;
        }
    } else {
        if (main_thd_->frame_bp() + i >= main_thd_->stack_tp_) {
            Raisef("stack index: %d out of range.", i);
            return nullptr;
        }
    }
    return main_thd_->Get(i);
}

void NyaaCore::Pop(int n) {
    if (main_thd_->stack_tp_ - n < main_thd_->frame_bp()) {
        Raisef("stack pop: %d out of range.", n);
    } else {
        main_thd_->Pop(n);
    }
}
    
void NyaaCore::SetGlobal(NyString *name, Object *value) { g_->RawPut(name, value, this); }
    
void NyaaCore::Raisef(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    Vraisef(fmt, ap);
    va_end(ap);
}
    
void NyaaCore::Vraisef(const char *fmt, va_list ap) {
    main_thd_->has_raised_ = true;
    std::string msg(base::Vsprintf(fmt, ap));
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
    
void NyaaCore::IterateRoot(RootVisitor *visitor) {
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&g_));
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&main_thd_));
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&curr_thd_));
    
    Object **pool_a = reinterpret_cast<Object **>(bkz_pool_.get());
    visitor->VisitRootPointers(pool_a, pool_a + kRawBuiltinKzsSize);

    pool_a = reinterpret_cast<Object **>(kmt_pool_.get());
    visitor->VisitRootPointers(pool_a, pool_a + kRawBuiltinkmtSize);

    for (auto thd = main_thd_->next_; thd != main_thd_; thd = thd->next_) {
        thd->IterateRoot(visitor);
    }
    for (auto slot = top_slot_; slot != nullptr; slot = slot->prev) {
        visitor->VisitRootPointers(reinterpret_cast<Object **>(slot->base),
                                   reinterpret_cast<Object **>(slot->end));
    }
}
    
void NyaaCore::InsertThread(NyThread *thd) {
    thd->next_ = main_thd_;
    auto *prev = main_thd_->prev_;
    thd->prev_ = prev;
    prev->next_ = thd;
    main_thd_->prev_ = thd;
}

void NyaaCore::RemoveThread(NyThread *thd) {
    thd->prev_->next_ = thd->next_;
    thd->next_->prev_ = thd->prev_;
#if defined(DEBUG) || defined(_DEBUG)
    thd->next_ = nullptr;
    thd->prev_ = nullptr;
#endif
}
    
void NyaaCore::InternalBarrierWr(NyObject *host, Object **pzwr, Object *val) {
    heap_->BarrierWr(host, pzwr, val);
}
    
} // namespace nyaa
    
} // namespace mai
