#include "lang/scavenger.h"
#include "lang/isolate-inl.h"
#include "lang/heap.h"
#include "lang/object-visitor.h"
#include "base/slice.h"

namespace mai {

namespace lang {

class Scavenger::RootVisitorImpl : public RootVisitor {
public:
    RootVisitorImpl(Scavenger *owns): owns_(owns) {}
    ~RootVisitorImpl() override = default;
    
    void VisitRootPointers(Any **begin, Any **end) override {
        for (Any **i = begin; i < end; i++) {
            if (!*i) {
                continue;
            }
            if (Any *forward = (*i)->forward()) {
                *i = forward;
                //DCHECK_NE(*i, reinterpret_cast<Any *>(0xfeedfeedfeedfeec));
                continue; // Adjust pointer
            }
            
            if (owns_->heap_->InNewArea(*i)) {
                bool should_promote = reinterpret_cast<Address>(*i) < owns_->promote_level_;
                *i = owns_->heap_->MoveNewSpaceObject(*i, should_promote);
                //DCHECK_NE(*i, reinterpret_cast<Any *>(0xfeedfeedfeedfeec));
                if (should_promote) {
                    owns_->promoted_obs_.push_back(*i);
                }
            }
        }
    }
private:
    Scavenger *owns_;
}; // class Scavenger::RootVisitorImpl

class Scavenger::ObjectVisitorImpl : public ObjectVisitor {
public:
    ObjectVisitorImpl(Scavenger *owns): owns_(owns) {}
    ~ObjectVisitorImpl() override = default;
    
    void VisitPointers(Any *host, Any **begin, Any **end) override {
        for (Any **i = begin; i < end; ++i) {
            Any *obj = *i;
            if (!obj) {
                continue;
            }
            
            if (Any *forward = obj->forward()) {
                *i = forward;
                continue;
            }

            if (owns_->heap_->InNewArea(obj)) {
                bool should_promote = reinterpret_cast<Address>(obj) < owns_->promote_level_;
                *i = owns_->heap_->MoveNewSpaceObject(obj, should_promote);
                DCHECK_NE(*i, reinterpret_cast<Any *>(0xfeedfeedfeedfeec));
                if (should_promote) {
                    owns_->promoted_obs_.push_back(*i);
                }
            }
        }
    }
    
private:
    Scavenger *owns_;
}; // class Scavenger::ObjectVisitorImpl

Scavenger::~Scavenger() /*override*/ {}

void Scavenger::Reset() /*override*/ {
    ::memset(&histogram_, 0, sizeof(histogram_));
    promoted_obs_.clear();
    promote_level_ = nullptr;
}

void Scavenger::Run(base::AbstractPrinter *logger) /*override*/ {
    Env *env = isolate_->env();
    uint64_t jiffy = env->CurrentTimeMicros();
    size_t available = heap_->new_space()->Available();
  
    SemiSpace *survive_area = heap_->new_space()->survive_area();
    float latest_remaining_rate =
        static_cast<float>(isolate_->gc()->latest_minor_remaining_size()) /
        static_cast<float>(survive_area->size());
    
    promote_level_ = survive_area->chunk();
    if (force_promote_) {
        promote_level_ = survive_area->surive_level();
    } else if (latest_remaining_rate > 0.25 /*1/4*/) {
        promote_level_ = survive_area->chunk() + isolate_->gc()->latest_minor_remaining_size();
    }
    logger->Println("[Minor] Promote Level: %p [%p, %p)", promote_level_, survive_area->chunk(),
                    survive_area->limit());
    
    promoted_obs_.clear();
    RootVisitorImpl root_visitor(this);
    isolate_->VisitRoot(&root_visitor);

    ObjectVisitorImpl object_visitor(this);
    RememberMap rset = isolate_->gc()->MergeRememberSet();
    logger->Println("[Minor] RSet size: %zd", rset.size());
    for (const auto &pair : rset) {
        DCHECK(heap_->InOldArea(pair.second.host));
        Any **addr = pair.second.address;
        Any *obj = DCHECK_NOTNULL(*addr);
        if (Any *forward = obj->forward()) {
            *addr = forward;
        } else {
            DCHECK(heap_->InNewArea(obj));
            *addr = heap_->MoveNewSpaceObject(obj, true/*promote*/);
            // printf("[%p] host: %s promote: %p -> %p\n", pair.second.host,
            // pair.second.host->clazz()->name(), obj, *addr);
            promoted_obs_.push_back(*addr);
        }
    }
    isolate_->gc()->PurgeRememberSet();
    
    //SemiSpace *survive_area = heap_->new_space()->survive_area();
    SemiSpaceIterator iter(survive_area);
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        IterateObject(iter.object(), &object_visitor);
    }

    logger->Println("[Minor] Should promoted objects: %zd", promoted_obs_.size());
    while (!promoted_obs_.empty()) {
        Any *object = promoted_obs_.front();
        promoted_obs_.pop_front();
        IterateObject(DCHECK_NOTNULL(object), &object_visitor);
    }

    size_t remaining = heap_->new_space()->Flip(false/*reinit*/);
    // After new space flip, should update all coroutine's heap guards
    SemiSpace *original_area = heap_->new_space()->original_area();
    isolate_->gc()->InvalidateHeapGuards(original_area->chunk(), original_area->limit());
    isolate_->gc()->set_latest_minor_remaining_size(remaining);

    histogram_.micro_time_cost = env->CurrentTimeMicros() - jiffy;
    histogram_.collected_bytes = heap_->new_space()->Available() - available;

    logger->Println("[Minor] Scavenger done, collected: %zd, cost: %" PRIi64 ,
                    histogram_.collected_bytes,
                    histogram_.micro_time_cost);
}

} // namespace lang

} // namespace mai
