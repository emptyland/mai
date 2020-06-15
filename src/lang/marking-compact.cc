#include "lang/marking-compact.h"
#include "lang/isolate-inl.h"
#include "lang/heap.h"
#include "lang/object-visitor.h"
#include "base/slice.h"

namespace mai {

namespace lang {

class MarkingCompact::RootVisitorImpl : public RootVisitor {
public:
    RootVisitorImpl(MarkingCompact *owns): owns_(owns) {}
    ~RootVisitorImpl() override = default;

    void VisitRootPointers(Any **begin, Any **end) override;
    
private:
    MarkingCompact *owns_;
}; // class MarkingCompact::RootVisitorImpl

void MarkingCompact::RootVisitorImpl::VisitRootPointers(Any **begin, Any **end) /*override*/ {
    for (Any **i = begin; i < end; i++) {
        Any *obj = *i;
        if (!obj) {
            continue;
        }

        if (Any *forward = obj->forward()) {
            *i = forward;
            continue;
        }
        
        owns_->CompactObject(i);
        DCHECK_NE(obj->color(), owns_->heap_->finalize_color());
        (*i)->set_color(KColorGray);
        owns_->gray_.push(*i);
    }
}

class MarkingCompact::ObjectVisitorImpl : public ObjectVisitor {
public:
    ObjectVisitorImpl(MarkingCompact *owns): owns_(owns) {}
    ~ObjectVisitorImpl() override = default;
    
    void VisitPointers(Any *host, Any **begin, Any **end) override {
        for (Any **i = begin; i < end; i++) {
            VisitPointer(host, i);
        }
    }

    void VisitPointer(Any *host, Any **p) override;

private:
    MarkingCompact *owns_;
}; // class MarkingCompact::ObjectVisitorImpl

void MarkingCompact::ObjectVisitorImpl::VisitPointer(Any *host, Any **p) /*override*/ {
    Any *obj = *p;
    if (!obj) {
        return;
    }
    if (Any *forward = obj->forward()) {
        *p = forward;
        return;
    }

    if ((*p)->color() == owns_->heap_->initialize_color()) {
        owns_->CompactObject(p);
        (*p)->set_color(KColorGray);
        owns_->gray_.push(*p);
    }
}

void MarkingCompact::Run(base::AbstractPrinter *logger) {
    Env *env = isolate_->env();
    uint64_t jiffy = env->CurrentTimeMicros();

    // Marking phase:
    logger->Println("[Major] Marking phase start, full gc: %d", full_);
    RootVisitorImpl root_visitor(this);
    ObjectVisitorImpl object_visitor(this);
    int count = UnbreakableMark(&root_visitor, &object_visitor);
    logger->Println("[Major] Marked %d gray objects", count);

    // Compacting phase:
    histogram_.collected_bytes += (heap_->old_space()->used_size() - old_used_);
    heap_->old_space()->Compact(old_survivors_, old_used_);
    
    histogram_.collected_bytes += (heap_->code_space()->used_size() - code_used_);
    heap_->code_space()->Compact(code_survivors_, code_used_);
    
    // Sweep large objects
    count = SweepLargeSpace();
    logger->Println("[Major] Collected %d large objects", count);
    // Sweep new objects
    if (full_) {
        count = UnbreakableSweepNewSpace();
        logger->Println("[Major] Collected %d new objects", count);
    }

    // Weak references sweeping:
    count = PurgeWeakObjects();
    logger->Println("[Major] Purge %d weak objects", count);

    heap_->SwapColors();
    histogram_.micro_time_cost = env->CurrentTimeMicros() - jiffy;
    logger->Println("[Major] Marking-compact done, collected: %zd, cost: %" PRIi64 ,
                    histogram_.collected_bytes, histogram_.micro_time_cost);
}

void MarkingCompact::CompactObject(Any **address) {
    Any *obj = DCHECK_NOTNULL(*address);
    if (heap_->InNewArea(obj) && full_) {
        // TODO:
        *address = heap_->MoveNewSpaceObject(obj, false/*promote*/);
        return;
    }
    DCHECK(heap_->InOldArea(obj));
    PageHeader *header = PageHeader::FromAddress(reinterpret_cast<Address>(obj));
    switch (header->owner_space()) {
        case kOldSpace: {
            OldSpace *old_space = heap_->old_space();
            Page *original = Page::Cast(header);
            Page *survivor = old_survivors_.empty() ? nullptr : old_survivors_.back();
            size_t object_size = original->AllocatedSize(reinterpret_cast<Address>(obj));
            DCHECK_GT(object_size, kPointerSize);
            if (survivor == nullptr || object_size > survivor->available()) {
                survivor = old_space->GetOrNewFreePage();
                old_survivors_.push_back(survivor);
            }
            *address = heap_->MoveOldSpaceObject(original, obj, survivor);
            old_used_ += object_size;
        } break;
            
        case kCodeSpace: {
            OldSpace *code_space = heap_->code_space();
            Page *original = Page::Cast(header);
            Page *survivor = code_survivors_.empty() ? nullptr : code_survivors_.back();
            size_t object_size = original->AllocatedSize(reinterpret_cast<Address>(obj));
            DCHECK_GT(object_size, kPointerSize);
            if (survivor == nullptr || object_size > survivor->available()) {
                survivor = code_space->GetOrNewFreePage();
                code_survivors_.push_back(survivor);
            }
            *address = heap_->MoveOldSpaceObject(original, obj, survivor);
            code_used_ += object_size;
        } break;

        case kLargeSpace:
            // Just mark it
            break;

        default:
            NOREACHED();
            break;
    }
}

} // namespace lang

} // namespace mai
