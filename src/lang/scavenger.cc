#include "lang/scavenger.h"
#include "lang/isolate-inl.h"
#include "lang/heap.h"
#include "lang/object-visitor.h"

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
                continue; // Adjust pointer
            }
            
            if (owns_->heap_->InNewArea(*i)) {
                bool should_promote = reinterpret_cast<Address>(*i) < owns_->promote_level_;
                *i = owns_->heap_->PromoteObject(*i, should_promote);
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
            if (!*i) {
                continue;
            }
            
            if (Any *forward = (*i)->forward()) {
                *i = forward;
                continue;
            }
            
            if (owns_->heap_->new_space()->InSuriveArea(reinterpret_cast<Address>(*i))) {
                bool should_promote = reinterpret_cast<Address>(*i) < owns_->promote_level_;
                *i = owns_->heap_->PromoteObject(*i, should_promote);
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

void Scavenger::Run() /*override*/ {
    Env *env = isolate_->env();
    uint64_t jiffy = env->CurrentTimeMicros();
    size_t available = heap_->new_space()->Available();
  
    SemiSpace *survie_area = heap_->new_space()->survive_area();
    float latest_remaining_rate =
        static_cast<float>(isolate_->gc()->latest_minor_remaining_size()) /
        static_cast<float>(survie_area->size());
    
    promote_level_ = survie_area->chunk();
    if (force_promote_) {
        promote_level_ = survie_area->surive_level();
    } else if (latest_remaining_rate > 0.25 /*1/4*/) {
        promote_level_ = survie_area->chunk() + isolate_->gc()->latest_minor_remaining_size();
    }
    
    promoted_obs_.clear();
    RootVisitorImpl root_visitor(this);
    isolate_->VisitRoot(&root_visitor);

    ObjectVisitorImpl object_visitor(this);
    RememberSet rset = isolate_->gc()->MergeRememberSet(true/*keep_after*/);
    for (const auto &pair : rset) {
        object_visitor.VisitPointer(pair.second.host, pair.second.address);
    }
    
    SemiSpace *original_area = heap_->new_space()->original_area();
    SemiSpaceIterator iter(original_area);
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        IterateObject(iter.object(), &object_visitor);
    }

    while (!promoted_obs_.empty()) {
        Any *object = promoted_obs_.front();
        promoted_obs_.pop_front();
        if (object) {
            IterateObject(object, &object_visitor);
        }
    }

    size_t remaining = heap_->new_space()->Flip(false/*reinit*/);
    // After new space flip, should update all coroutine's heap guards
    original_area = heap_->new_space()->original_area();
    isolate_->gc()->InvalidateHeapGuards(original_area->chunk(), original_area->limit());
    isolate_->gc()->set_latest_minor_remaining_size(remaining);
    
    histogram_.micro_time_cost = env->CurrentTimeMicros() - jiffy;
    histogram_.collected_bytes = heap_->new_space()->Available() - available;
}

} // namespace lang

} // namespace mai
