#include "lang/marking-sweep.h"
#include "lang/isolate-inl.h"
#include "lang/heap.h"
#include "lang/object-visitor.h"
#include "base/slice.h"

namespace mai {

namespace lang {

class MarkingSweep::RootVisitorImpl final : public RootVisitor {
public:
    RootVisitorImpl(MarkingSweep *owns) : owns_(owns) {}
    ~RootVisitorImpl() override = default;
    
    void VisitRootPointers(Any **begin, Any **end) override {
        for (Any **i = begin; i < end; i++) {
            Any *obj = *i;
            if (!obj) {
                continue;
            }
            
            DCHECK(obj->is_directly());
            DCHECK_NE(owns_->heap_->finalize_color(), obj->color());
            obj->set_color(KColorGray);
            owns_->gray_.push(obj);
        }
    }
private:
    MarkingSweep *const owns_;
}; // class MarkingSweeper::RootVisitorImpl


class MarkingSweep::ObjectVisitorImpl final : public ObjectVisitor {
public:
    ObjectVisitorImpl(MarkingSweep *owns): owns_(owns) {}
    ~ObjectVisitorImpl() override = default;

    void VisitPointers(Any *host, Any **begin, Any **end) override {
        for (Any **i = begin; i < end; i++) {
            Any *obj = *i;
            if (!obj) {
                continue;
            }

            DCHECK(obj->is_directly());
            // Transform white to gray
            if (obj->color() == owns_->heap_->initialize_color()) {
                obj->set_color(KColorGray);
                owns_->gray_.push(obj);
            }
        }
    }
private:
    MarkingSweep *const owns_;
}; // class MarkingSweep::ObjectVisitorImpl


class MarkingSweep::WeakVisitorImpl final : public ObjectVisitor {
public:
    WeakVisitorImpl(MarkingSweep *owns): owns_(owns) {}
    ~WeakVisitorImpl() override = default;
    
    void VisitPointers(Any *host, Any **begin, Any **end) override {
        for (Any **i = begin; i < end; i++) {
            VisitPointer(host, i);
        }
    }

    void VisitPointer(Any *host, Any **p) override {
        Any *obj = *p;
        DCHECK(!owns_->full_ || obj->is_directly());
        if (Any *forward = obj->forward()) {
            DCHECK_EQ(owns_->heap_->finalize_color(), forward->color());
            *p = forward;
        } else if (obj->color() != owns_->heap_->finalize_color()) {
            DCHECK_NE(KColorGray, obj->color());
            *p = nullptr;
        }
    }
    
private:
    MarkingSweep *const owns_;
}; // class MarkingSweep::WeakVisitorImpl

void MarkingSweep::Run(base::AbstractPrinter *logger) /*override*/ {
    Env *env = isolate_->env();
    uint64_t jiffy = env->CurrentTimeMicros();
    
    // Marking phase:
    RootVisitorImpl root_visitor(this);
    isolate_->VisitRoot(&root_visitor);

    ObjectVisitorImpl object_visitor(this);
    while (!gray_.empty()) {
        Any *obj = gray_.top();
        gray_.pop();

        if (obj->color() == KColorGray) {
            obj->set_color(heap_->finalize_color());
            IterateObject(obj, &object_visitor);
        } else {
            DCHECK_EQ(obj->color(), heap_->finalize_color());
        }
    }
    
    // Sweeping phase:
    if (OldSpace *old_space = heap_->old_space()) {
        OldSpace::Iterator iter(old_space);
        for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
            Any *obj = iter.object();
            if (obj->color() == heap_->initialize_color()) {
                histogram_.collected_bytes += iter.object_size();
                histogram_.collected_objs++;
                old_space->Free(reinterpret_cast<Address>(obj), true/*should_merge*/);
            }
        }
        // TODO: merge chunks
    }

    if (LargeSpace *large_space = heap_->large_space()) {
        LargeSpace::Iterator iter(large_space);
        for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
            Any *obj = iter.object();
            if (obj->color() == heap_->initialize_color()) {
                histogram_.collected_bytes += iter.object_size();
                histogram_.collected_objs++;
                large_space->Free(reinterpret_cast<Address>(obj));
            }
        }
    }

    if (full_) {
        SemiSpace::Iterator iter(heap_->new_space()->original_area());
        for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
            Any *obj = iter.object();
            if (obj->color() == heap_->initialize_color()) {
                histogram_.collected_bytes += iter.object_size();
                histogram_.collected_objs++;
            } else {
                // Keep the lived object
                heap_->MoveNewSpaceObject(obj, false/*promote*/);
            }
        }
        size_t remaining = heap_->new_space()->Flip(false/*reinit*/);
        // After new space flip, should update all coroutine's heap guards
        SemiSpace *original_area = heap_->new_space()->original_area();
        isolate_->gc()->InvalidateHeapGuards(original_area->chunk(), original_area->limit());
        isolate_->gc()->set_latest_minor_remaining_size(remaining);
    }
    
    // Weak references sweeping:
    WeakVisitorImpl weak_visitor(this);
    RememberSet rset = isolate_->gc()->MergeRememberSet(false/*keep_after*/);
    for (const auto &rd : rset) {
        weak_visitor.VisitPointer(rd.second.host, rd.second.address);
    }

    heap_->SwapColors();
    histogram_.micro_time_cost = env->CurrentTimeMicros() - jiffy;
}

} // namespace lang

} // namespace mai
