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
                *i = owns_->MoveObject(ob, ob->PlacedSize());
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
                *i = owns_->MoveObject(ob, ob->PlacedSize());
            }
        }
    }

    virtual void VisitMetatablePointer(Object *host, uintptr_t *word) override {
#if defined(NYAA_USE_POINTER_COLOR)
        DCHECK_NE(0, *word);
        DCHECK(!(*word & 0x1));

        uintptr_t color_bits = *word & NyObject::kColorMask;

        Object *mt = reinterpret_cast<Object *>(*word & ~NyObject::kColorMask);
        VisitPointer(host, &mt);

        *word = reinterpret_cast<uintptr_t>(mt) | color_bits;
#else // !defined(NYAA_USE_POINTER_COLOR)
        // TODO:
#endif // defined(NYAA_USE_POINTER_COLOR)
    }
private:
    Scavenger *const owns_;
}; // class Scavenger::ObjectVisitorImpl
    
class Scavenger::KzPoolVisitorImpl final : public ObjectVisitor {
public:
    KzPoolVisitorImpl(Scavenger *owns) : owns_(DCHECK_NOTNULL(owns)) {}
    virtual ~KzPoolVisitorImpl() override {}
    
    virtual void VisitPointer(Object *host, Object **p) override {
        NyString *s = static_cast<NyString *>(*p);
        DCHECK(s->IsObject());
        if (NyObject *foward = s->Foward()) {
            *p = foward;
            return;
        } else if (owns_->heap_->InOldArea(s)) {
            DCHECK(s->IsString());
            return; // Scavenger do not sweep old space, ignore it.
        } else if (owns_->heap_->InToSemiArea(s)) {
            DCHECK(s->IsString());
            *p = nullptr; // No one reference this string, should be sweep.
            return;
        }
        // Has some one reference this string, ignore it.
        DCHECK(owns_->heap_->InFromSemiArea(s));
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
    : core_(core)
    , heap_(heap) {
    if (heap_->from_semi_area_remain_rate_ > 0.25) { // 1/4
        should_upgrade_ = true;
    }
}
    
void Scavenger::Run() {
    uint64_t jiffy = core_->isolate()->env()->CurrentTimeMicros();
    RootVisitorImpl visitor(this);
    core_->IterateRoot(&visitor);
    
    ObjectVisitorImpl obv(this);
    HeapVisitorImpl hpv(&obv);

    heap_->IterateRememberSet(&obv, false /*for_host*/ , true /*after_clean*/);

    SemiSpace *from_area = heap_->new_space_->from_area();
    Address begin = RoundUp(from_area->page()->area_base(), kAligmentSize);
    Address end   = from_area->free();
    while (begin < end) {
        from_area->Iterate(begin, end, &hpv);
        begin = end;
        end = from_area->free();
    }
    
    // Process for weak tables
    if (core_->kz_pool()) {
        // kz_pool is a special weak table.
        KzPoolVisitorImpl wov(this);
        core_->kz_pool()->IterateForSweep(&wov);
    }
    // TODO:
    
    size_t total_size = from_area->page()->usable_size();
    heap_->from_semi_area_remain_rate_ = static_cast<float>(from_area->UsageMemory())
                                       / static_cast<float>(total_size);
    heap_->new_space_->Purge(false);
    heap_->major_gc_cost_ = (core_->isolate()->env()->CurrentTimeMicros() - jiffy) / 1000.0;
    // TODO:
}
    
Object *Scavenger::MoveObject(Object *addr, size_t size) {
    SemiSpace *to_area = heap_->new_space_->to_area();
    DCHECK(to_area->Contains(reinterpret_cast<Address>(addr)));

    Address dst = nullptr;
    if (should_upgrade_) {
        dst = heap_->old_space_->AllocateRaw(size);
        DCHECK_NOTNULL(dst);
        ::memcpy(dst, addr, size);
    } else {
        SemiSpace *from_area = heap_->new_space_->from_area();
        dst = from_area->AllocateRaw(size);
        DCHECK_NOTNULL(dst);
        //printf("move: %p(%lu) <- %p\n", dst, size, addr);
        ::memcpy(dst, addr, size);
    }

    // Set foward address:
#if defined(NYAA_USE_POINTER_COLOR)
    uintptr_t word = *reinterpret_cast<uintptr_t *>(addr);
    uintptr_t color_bits = word & NyObject::kColorMask;
    word = reinterpret_cast<uintptr_t>(dst) | color_bits | 0x1;
    *reinterpret_cast<uintptr_t *>(addr) = word;
#else // !defined(NYAA_USE_POINTER_COLOR)
    uintptr_t word = *reinterpret_cast<uintptr_t *>(addr);
    word = reinterpret_cast<uintptr_t>(dst) | 0x1;
    *reinterpret_cast<uintptr_t *>(addr) = word;
#endif // defined(NYAA_USE_POINTER_COLOR)

    return reinterpret_cast<Object *>(dst);
}

} // namespace nyaa
    
} // namespace mai
