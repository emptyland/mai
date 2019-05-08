#include "nyaa/scavenger.h"
#include "nyaa/string-pool.h"
#include "nyaa/heap.h"
#include "nyaa/spaces.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/visitors.h"
#include "nyaa/nyaa-core.h"
#include "mai-lang/nyaa.h"

namespace mai {
    
namespace nyaa {
    
class Scavenger::RootVisitorImpl final : public RootVisitor {
public:
    RootVisitorImpl(Scavenger *owns)
        : owns_(DCHECK_NOTNULL(owns)) {}
    virtual ~RootVisitorImpl() override {}
    
    virtual void VisitRootPointers(Object **begin, Object **end) override {
        for (Object **i = begin; i < end; ++i) {
            Object *o = *i;
            if (o == Object::kNil || !o->IsObject()) {
                continue;
            }
            
            NyObject *ob = o->ToHeapObject();
            if (NyObject *forward = ob->Foward()) {
                *i = forward;
                continue;
            }
            
            if (owns_->heap_->InNewArea(ob)) {
                bool should_upgrade = reinterpret_cast<Address>(ob) < owns_->upgrade_level_;
                *i = owns_->heap_->MoveNewObject(ob, ob->PlacedSize(), should_upgrade);
            }
        }
    }
    
private:
    Scavenger *const owns_;
}; // class Scavenger::RootVisitorImpl
    
class Scavenger::HeapVisitorImpl final : public HeapVisitor {
public:
    HeapVisitorImpl(ObjectVisitor *ob_visitor)
        : ob_visitor_(DCHECK_NOTNULL(ob_visitor)) {}
    virtual ~HeapVisitorImpl() override {}
    virtual void VisitObject(Space *owns, NyObject *ob, size_t placed_size) override {
        ob->Iterate(ob_visitor_);
    }
private:
    ObjectVisitor *ob_visitor_;
}; // class Scavenger::HeapVisitorImpl
    
class Scavenger::ObjectVisitorImpl final : public ObjectVisitor {
public:
    ObjectVisitorImpl(Scavenger *owns) : owns_(DCHECK_NOTNULL(owns)) {}
    virtual ~ObjectVisitorImpl() override {}
    
    virtual void VisitPointers(Object *host, Object **begin, Object **end) override {
        for (Object **i = begin; i < end; ++i) {
            Object *o = *i;
            if (o == Object::kNil || !o->IsObject()) {
                continue;
            }
            
            NyObject *ob = o->ToHeapObject();
            if (NyObject *forward = ob->Foward()) {
                *i = forward;
                continue;
            }
            if (owns_->heap_->InToSemiArea(ob)) {
                bool should_upgrade = reinterpret_cast<Address>(ob) < owns_->upgrade_level_;
                *i = owns_->heap_->MoveNewObject(ob, ob->PlacedSize(), should_upgrade);
            }
        }
    }

    virtual void VisitMetatablePointer(Object *host, uintptr_t *word) override {
#if defined(NYAA_USE_POINTER_COLOR) || defined(NYAA_USE_POINTER_TYPE)
        DCHECK_NE(0, *word);
        DCHECK(!(*word & 0x1));

        uintptr_t data_bits = *word & NyObject::kDataMask;

        Object *mt = reinterpret_cast<Object *>(*word & ~NyObject::kDataMask);
        VisitPointer(host, &mt);

        *word = reinterpret_cast<uintptr_t>(mt) | data_bits;
#else // !defined(NYAA_USE_POINTER_COLOR) && defined(NYAA_USE_POINTER_TYPE)
        // TODO:
#endif // defined(NYAA_USE_POINTER_COLOR) || defined(NYAA_USE_POINTER_TYPE)
    }
private:
    Scavenger *const owns_;
}; // class Scavenger::ObjectVisitorImpl
    
class Scavenger::WeakVisitorImpl final : public ObjectVisitor {
public:
    WeakVisitorImpl(Scavenger *owns) : owns_(DCHECK_NOTNULL(owns)) {}
    virtual ~WeakVisitorImpl() override {}
    
    virtual void VisitPointer(Object *host, Object **p) override {
        NyObject *ob = static_cast<NyObject *>(*p);
        DCHECK(ob->IsObject());
        if (NyObject *foward = ob->Foward()) {
            *p = foward;
            return;
        } else if (owns_->heap_->InOldArea(ob)) {
            return; // Scavenger do not sweep old space, ignore it.
        } else if (owns_->heap_->InToSemiArea(ob)) {
            *p = nullptr; // No one reference this object, should be sweep.
            return;
        }
        // Has some one reference this object, ignore it.
        DCHECK(owns_->heap_->InFromSemiArea(ob));
    }
    virtual void VisitPointers(Object *host, Object **begin, Object **end) override {
        DLOG(FATAL) << "noreached!";
    }
    virtual void VisitMetatablePointer(Object *host, uintptr_t *word) override {
        DLOG(FATAL) << "noreached!";
    }

private:
    Scavenger *const owns_;
}; // class Scavenger::KzPoolVisitorImpl
    
Scavenger::Scavenger(NyaaCore *core, Heap *heap)
    : GarbageCollectionPolicy(core, heap) {
}
    
/*virtual*/ void Scavenger::Run() {
    Env *env = core_->isolate()->env();
    uint64_t jiffy = env->CurrentTimeMicros();
    size_t available = heap_->new_space_->Available();
    
    SemiSpace *to_area = heap_->new_space_->to_area();
    upgrade_level_ = to_area->page()->area_base();
    if (force_upgrade_) {
        upgrade_level_ = to_area->free();
    } else if (heap_->new_space_->GetLatestRemainingRate() > 0.25) {
        upgrade_level_ = heap_->new_space_->GetLatestRemainingLevel();
    }
    
    RootVisitorImpl root_visitor(this);
    core_->IterateRoot(&root_visitor);
    
    ObjectVisitorImpl obj_visitor(this);
    heap_->IterateRememberSet(&obj_visitor, false /*for_host*/ , true /*after_clean*/);

    HeapVisitorImpl heap_visitor(&obj_visitor);
    SemiSpace *from_area = heap_->new_space_->from_area();
    Address begin = RoundUp(from_area->page()->area_base(), kAligmentSize);
    Address end   = from_area->free();
    while (begin < end) {
        from_area->Iterate(begin, end, &heap_visitor);
        begin = end;
        end = from_area->free();
    }
    
    // Process for weak tables
    WeakVisitorImpl weak_visitor(this);
    if (core_->kz_pool()) {
        // kz_pool is a special weak table.
        core_->kz_pool()->Iterate(&weak_visitor);
    }
    heap_->IterateFinalizerRecords(&weak_visitor);
    // TODO:

    size_t total_size = from_area->page()->usable_size();
    heap_->from_semi_area_remain_rate_ = static_cast<float>(from_area->UsageMemory())
                                       / static_cast<float>(total_size);
    heap_->new_space_->Flip(false);
    time_cost_ = (env->CurrentTimeMicros() - jiffy) / 1000.0;
    collected_bytes_ = heap_->new_space_->Available() - available;
    // TODO:
}

} // namespace nyaa
    
} // namespace mai
