#include "nyaa/scavenger.h"
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
                Address from = reinterpret_cast<Address>(ob);
                size_t  from_size = ob->PlacedSize();
                
                *i = owns_->MoveNewSpace(from, from_size);
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
    virtual void VisitObject(NyObject *ob, size_t placed_size) override {
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
                Address from = reinterpret_cast<Address>(ob);
                size_t  from_size = ob->PlacedSize();
                
                *i = owns_->MoveNewSpace(from, from_size);
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
    
void Scavenger::Run() {
    RootVisitorImpl visitor(this);
    core_->IterateRoot(&visitor);
    
    ObjectVisitorImpl obv(this);
    HeapVisitorImpl hpv(&obv);
    
    for (const auto &pair : heap_->old_to_new_) {
        Heap::WriteEntry *wr = pair.second;
        if (wr->ismt) {
            obv.VisitMetatablePointer(wr->host, wr->mtwr);
        } else {
            obv.VisitPointer(wr->host, wr->pzwr);
        }
    }
    heap_->old_to_new_.clear();

    SemiSpace *from_area = heap_->new_space_->from_area();
    Address begin = from_area->page()->area_base(), end = from_area->free();
    while (begin < end) {
        from_area->Iterate(begin, end, &hpv);
        begin = end;
        end = from_area->free();
    }
    
    heap_->new_space_->Purge(false);
    // TODO:
}
    
Object *Scavenger::MoveNewSpace(Address addr, size_t size) {
    //heap_->new_space_->from_area_->page()
    SemiSpace *from_area = heap_->new_space_->from_area();
    SemiSpace *to_area   = heap_->new_space_->to_area();
    
    DCHECK(to_area->Contains(addr));
    Address dst = from_area->free();
    ::memcpy(dst, addr, size);
    from_area->free_ += RoundUp(size, kAllocateAlignmentSize);
    
    *reinterpret_cast<uintptr_t *>(addr) |= 0x1;
//    uintptr_t foward = *reinterpret_cast<uintptr_t *>(addr) | 0x1;
//    *reinterpret_cast<uintptr_t *>(addr) = foward;
    return reinterpret_cast<Object *>(dst);
}

} // namespace nyaa
    
} // namespace mai
