#include "nyaa/nyaa-core.h"
#include "nyaa/thread.h"
#include "nyaa/function.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/heap.h"
#include "nyaa/object-factory.h"
#include "nyaa/builtin.h"
#include "nyaa/visitors.h"
#include "nyaa/string-pool.h"
#include "nyaa/scavenger.h"
#include "nyaa/marking-sweep.h"
#include "nyaa/marking-compaction.h"
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
    , kmt_pool_(new BuiltinMetatablePool())
    , code_pool_(new BuiltinCodePool())
    , next_udo_kid_(kUdoKidBegin + 1)
    , logger_(stdout) {
    if (stub_->nogc()) {
        factory_.reset(ObjectFactory::NewMallocFactory(this));
    } else {
        factory_.reset(ObjectFactory::NewHeapFactory(this, heap_.get()));
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
    if (Error rs = isolate()->env()->NewRealRandomGenerator(&random_); !rs) {
        return rs;
    }
    if (Error rs = heap_->Init(); !rs) {
        return rs;
    }
    
    if (stub_->use_string_pool_) {
        Allocator *lla = isolate()->env()->GetLowLevelAllocator();
        kz_pool_.reset(new StringPool(lla, random_.get()));
    }
    
    NyString **pool_a = reinterpret_cast<NyString **>(bkz_pool_.get());
    for (size_t i = 0; i < kRawBuiltinKzsSize; ++i) {
        pool_a[i] = factory_->NewString(kRawBuiltinKzs[i], true /*old*/);
    }
    if (Error rs = kmt_pool_->Boot(this); !rs) {
        return rs;
    }
// TODO: if (Error rs = code_pool_->Boot(this); !rs) {
//        return rs;
//    }

    // Set builtin global variables:
    g_ = NewEnv(nullptr);
    
    // Files has be loaded:
    loads_ = factory_->NewMap(32/*capacity*/, random_->NextU32()/*seed*/, 0/*kid*/, false/*linear*/,
                              true/*old*/);

    // Setup main_thread
    current_thread_ = factory_->NewThread(stub_->init_thread_stack_size());

    // Setup builtin classes.
    SetGlobal(factory_->NewString("coroutine"), kmt_pool_->kThread);
    
    DCHECK(!initialized_);
    initialized_ = true;
    return Error::OK();
}

Object *NyaaCore::Get(int i) {
    // TODO:
    TODO();
    return Object::kNil;
}

void NyaaCore::Pop(int n) {
    // TODO:
    TODO();
}
    
void NyaaCore::SetGlobal(NyString *name, Object *value) { g_->RawPut(name, value, this); }
    
NyMap *NyaaCore::NewEnv(NyMap *base) {
    NyMap *env = base;
    if (!env) {
        env = factory_->NewMap(32/*capacity*/, random_->NextU32()/*seed*/, 0/*kid*/,
                               false/*linear*/, true/*old*/);
    }

    for (auto e = &kBuiltinFnEntries[0]; e->name; e++) {
        NyString *name = factory_->NewString(e->name);
        if (!env->RawGet(name, this)) {
            env->RawPut(factory_->NewString(e->name, true/*old*/),
                        factory_->NewDelegated(e->nafn, 0/*n_upvals*/, true/*old*/), this);
        }
    }
    env->RawPut(bkz_pool()->kInnerG, env, this);
    return env;
}
    
void NyaaCore::Raisef(const char *fmt, ...) {
// TODO: va_list ap;
//    va_start(ap, fmt);
//    curr_thd_->Vraisef(fmt, ap);
//    va_end(ap);
    TODO();
}

void NyaaCore::Vraisef(const char *fmt, va_list ap) {
// TODO: curr_thd_->Vraisef(fmt, ap);
    TODO();
}
    
Isolate *NyaaCore::isolate() const { return stub_->isolate_; }
    
/*static*/ NyaaCore *NyaaCore::Current() {
    return Isolate::Current()->GetNyaa()->core();
}
    
Address NyaaCore::GetRecoverPointAddress() {
    return code_pool_->kEntryTrampoline->entry_address() + recover_point_pc_;
}

Address NyaaCore::GetSuspendPointAddress() {
    return code_pool_->kRecoverIfNeed->entry_address() + suspend_point_pc_;
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
    DCHECK(slot->prev || slot->prev->end == slot->base);
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
    
void NyaaCore::GarbageCollect(GarbageCollectionMethod method,
                              GarbageCollectionHistogram *histogram) {
    switch (method) {
        case kMajorGC: {
            Scavenger gc(this, heap());
            gc.Run();
            
            histogram->collected_bytes = gc.collected_bytes();
            histogram->collected_objs  = gc.collected_objs();
            histogram->time_cost       = gc.time_cost();
        } break;
        case kMinorGC: {
            MarkingSweep gc(this, heap());
            gc.Run();

            histogram->collected_bytes = gc.collected_bytes();
            histogram->collected_objs  = gc.collected_objs();
            histogram->time_cost       = gc.time_cost();
        } break;
        case kFullGC: {
            Scavenger major_gc(this, heap());
            major_gc.Run();
            
            histogram->collected_bytes = major_gc.collected_bytes();
            histogram->collected_objs  = major_gc.collected_objs();
            histogram->time_cost       = major_gc.time_cost();
            MarkingSweep minor_gc(this, heap());
            minor_gc.Run();
            histogram->collected_bytes += minor_gc.collected_bytes();
            histogram->collected_objs  += minor_gc.collected_objs();
            histogram->time_cost       += minor_gc.time_cost();
        } break;
        default:
            break;
    }
}
    
void NyaaCore::GarbageCollectionSafepoint(const char *file, int line) {
    GarbageCollectionHistogram histogram;
    if (gc_force_count_ > 0) {
        GarbageCollect(kMajorGC, &histogram);
        --gc_force_count_;
        return;
    }
    
    Heap::Info info;
    heap_->GetInfo(&info);
    
    bool hit = false;
    if (info.major_usage > (info.major_size >> 1)) {
        hit = true;
        GarbageCollect(kMajorGC, &histogram);
    }
    if (info.minor_usage > (info.minor_size >> 1)) {
        hit = true;
        GarbageCollect(kMinorGC, &histogram);
    }
    
//    if (hit) {
//        printf("[%d]freed: %f MB, %f ms\n", gc_tick_,
//               histogram.collected_bytes/static_cast<float>(base::kMB),
//               histogram.time_cost);
//    }
    gc_ticks_++;
}
    
void NyaaCore::IterateRoot(RootVisitor *visitor) {
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&g_));
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&loads_));
    visitor->VisitRootPointer(reinterpret_cast<Object **>(&current_thread_));
    
    Object **pool_a = reinterpret_cast<Object **>(bkz_pool_.get());
    visitor->VisitRootPointers(pool_a, pool_a + kRawBuiltinKzsSize);

    pool_a = reinterpret_cast<Object **>(kmt_pool_.get());
    visitor->VisitRootPointers(pool_a, pool_a + kRawBuiltinkmtSize);
    
    pool_a = reinterpret_cast<Object **>(code_pool_.get());
    visitor->VisitRootPointers(pool_a, pool_a + kRawBuiltinCodeSize);

    for (auto thread = current_thread_; thread != nullptr; thread = thread->prev()) {
        thread->IterateRoot(visitor);
    }

    for (auto slot = top_slot_; slot != nullptr; slot = slot->prev) {
        visitor->VisitRootPointers(reinterpret_cast<Object **>(slot->base),
                                   reinterpret_cast<Object **>(slot->end));
    }
}
    
void NyaaCore::InternalBarrierWr(NyObject *host, Object **pzwr, Object *val) {
    heap_->BarrierWr(host, pzwr, val);
}
    
} // namespace nyaa
    
} // namespace mai
