#include "nyaa/marking-sweep.h"
#include "nyaa/string-pool.h"
#include "nyaa/heap.h"
#include "nyaa/spaces.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/visitors.h"
#include "nyaa/nyaa-core.h"
#include "mai-lang/nyaa.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {
    
class MarkingSweep::RootVisitorImpl final : public RootVisitor {
public:
    RootVisitorImpl(MarkingSweep *owns) : owns_(DCHECK_NOTNULL(owns)) {}
    virtual ~RootVisitorImpl() override {}
    
    virtual void VisitRootPointers(Object **begin, Object **end) override {
        for (Object **i = begin; i < end; ++i) {
            Object *o = *i;
            if (o == Object::kNil || !o->IsObject()) {
                continue;
            }
            
            NyObject *ob = o->ToHeapObject();
            DCHECK(ob->is_direct());
            DCHECK_NE(owns_->heap_->finalize_color(), ob->GetColor());
            ob->SetColor(KColorGray);
            owns_->gray_.push(ob);
        }
    }
    
private:
    MarkingSweep *const owns_;
}; // class MarkingSweeper::RootVisitorImpl
    
class MarkingSweep::ObjectVisitorImpl final : public ObjectVisitor {
public:
    ObjectVisitorImpl(MarkingSweep *owns) : owns_(DCHECK_NOTNULL(owns)) {}
    virtual ~ObjectVisitorImpl() override {}
    
    virtual void VisitPointers(Object *host, Object **begin, Object **end) override {
        for (Object **i = begin; i < end; ++i) {
            Object *o = *i;
            if (o == Object::kNil || !o->IsObject()) {
                continue;
            }

            NyObject *ob = o->ToHeapObject();
            DCHECK(ob->is_direct());
            if (ob->GetColor() == owns_->heap_->initial_color()) {
                ob->SetColor(KColorGray);
                owns_->gray_.push(ob);
            }
        }
    }

    virtual void VisitMetatablePointer(Object *host, uintptr_t *word) override {
#if defined(NYAA_USE_POINTER_COLOR)
        DCHECK_NE(0, *word);
        DCHECK(!(*word & 0x1));

        Object *mt = reinterpret_cast<Object *>(*word & ~NyObject::kColorMask);
        VisitPointer(host, &mt);
#else // !defined(NYAA_USE_POINTER_COLOR)
        // TODO:
#endif // defined(NYAA_USE_POINTER_COLOR)
    }
private:
    MarkingSweep *const owns_;
}; // class MarkingSweeper::ObjectVisitorImpl
    
class MarkingSweep::HeapVisitorImpl final : public HeapVisitor {
public:
    HeapVisitorImpl(MarkingSweep *owns) : owns_(DCHECK_NOTNULL(owns)) {}
    virtual ~HeapVisitorImpl() override {}
    
    virtual void VisitObject(Space *owns, NyObject *ob, size_t placed_size) override {
        HeapSpace space = owns_->heap_->Contains(ob);
        DCHECK_EQ(owns->kind(), space);
        if (ob->GetColor() == owns_->heap_->finalize_color()) { // Has marked!
            return;
        }
        
        Address addr = reinterpret_cast<Address>(ob);
        switch (space) {
            case kOldSpace:
                static_cast<OldSpace *>(owns)->Free(addr, placed_size, false);
                owns_->collected_bytes_ += placed_size;
                owns_->collected_objs_++;
                break;
            case kLargeSpace:
                static_cast<LargeSpace *>(owns)->Free(addr);
                owns_->collected_bytes_ += placed_size;
                owns_->collected_objs_++;
                break;
            default:
                DLOG(FATAL) << "noreached!" << space;
                break;
        }
    }

private:
    MarkingSweep *const owns_;
}; // class MarkingSweeper::HeapVisitorImpl
    
class MarkingSweep::KzPoolVisitorImpl final : public ObjectVisitor {
public:
    KzPoolVisitorImpl(MarkingSweep *owns) : owns_(DCHECK_NOTNULL(owns)) {}
    virtual ~KzPoolVisitorImpl() override {}
    
    virtual void VisitPointer(Object *host, Object **p) override {
        DCHECK((*p)->IsObject());
        
        NyObject *ob = (*p)->ToHeapObject();
        DCHECK(ob->is_direct());
        
        if (ob->GetColor() != owns_->heap_->finalize_color()) {
            DCHECK_NE(KColorGray, ob->GetColor());
            *p = nullptr;
        }
    }
    virtual void VisitPointers(Object *host, Object **begin, Object **end) override {
        DLOG(FATAL) << "noreached!";
    }
    virtual void VisitMetatablePointer(Object *host, uintptr_t *word) override {
        DLOG(FATAL) << "noreached!";
    }
    
private:
    MarkingSweep *const owns_;
}; // class MarkingSweeper::KzPoolVisitorImpl
    
MarkingSweep::MarkingSweep(NyaaCore *core, Heap *heap)
    : GarbageCollectionPolicy(core, heap) {
}

MarkingSweep::~MarkingSweep() {
}

void MarkingSweep::Run() {
    Env *env = core_->isolate()->env();
    uint64_t jiffy = env->CurrentTimeMicros();
    
    // Marking phase:
    RootVisitorImpl root_visitor(this);
    core_->IterateRoot(&root_visitor);
    
    ObjectVisitorImpl obj_visitor(this);
    while (!gray_.empty()) {
        NyObject *ob = gray_.top();
        gray_.pop();

        if (ob->GetColor() == KColorGray) {
            //printf("mark: %p\n", ob);
            ob->SetColor(heap_->finalize_color());
            ob->Iterate(&obj_visitor);
        } else {
            DCHECK_EQ(ob->GetColor(), heap_->finalize_color());
        }
    }
    
    // Sweeping phase:
    HeapVisitorImpl heap_visitor(this);
    heap_->old_space_->Iterate(&heap_visitor);
    heap_->code_space_->Iterate(&heap_visitor);
    heap_->large_space_->Iterate(&heap_visitor);

    // Weak table sweeping:
    KzPoolVisitorImpl kzpool_visitor(this);
    heap_->IterateRememberSet(&kzpool_visitor, true/*for_host*/, false/*after_clean*/);
    core_->kz_pool()->IterateForSweep(&kzpool_visitor);
    
    // Finalize:
    std::swap(heap_->initial_color_, heap_->finalize_color_);
    time_cost_ = (env->CurrentTimeMicros() - jiffy) / 1000.0;
}
    
} // namespace nyaa
    
} // namespace mai