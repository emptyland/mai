#include "nyaa/heap.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/spaces.h"
#include "mai-lang/nyaa.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {
    
Heap::Heap(NyaaCore *N)
    : owns_(DCHECK_NOTNULL(N))
    , new_space_(new NewSpace())
    , old_space_(new OldSpace(kOldSpace, false, N->isolate())) {
        // TODO:
}

/*virtual*/ Heap::~Heap() {
    delete new_space_;
}

Error Heap::Prepare() {
    Nyaa *stub = owns_->stub();
    auto rs = new_space_->Init(stub->major_area_initial_size(), owns_->isolate());
    if (!rs) {
        return rs;
    }
    rs = old_space_->Init(stub->minor_area_initial_size(), stub->minor_area_max_size());
    if (!rs) {
        return rs;
    }
    return Error::OK();
}

NyObject *Heap::Allocate(size_t size, HeapSpace space) {
    if (size > kLargePageThreshold) {
        space = kLargeSpace;
    }
    
    void *chunk = nullptr;
    switch (space) {
        case kNewSpace:
            chunk = new_space_->AllocateRaw(size);
            break;
        case kOldSpace:
            chunk = old_space_->AllocateRaw(size);
            break;
        case kCodeSpace:
            // TODO:
            break;
        case kLargeSpace:
            // TODO:
            break;
        default:
            break;
    }
    if (!chunk) {
        return nullptr;
    }
    DCHECK_EQ(reinterpret_cast<uintptr_t>(chunk) % kAligmentSize, 0);
    DbgFillInitZag(chunk, size);
    return static_cast<NyObject *>(chunk);
}
    
HeapSpace Heap::Contains(NyObject *ob) {
//    if (owns_->initialized() && ob->PlacedSize() > kLargePageThreshold) {
//        return kLargeSpace;
//    }
    // TODO:
    if (new_space_->Contains(reinterpret_cast<Address>(ob))) {
        return kNewSpace;
    }
    auto page = Page::FromHeapObject(ob);
    return page->owns_space();
}
    
bool Heap::InNewArea(NyObject *ob) {
    return new_space_->Contains(reinterpret_cast<Address>(ob));
}
    
void Heap::BarrierWr(NyObject *host, Object **pzwr, Object *val) {
    //printf("%p(%p) = %p\n", host, pzwr, val);
    if (owns_->stub()->nogc()) {
        return;
    }
//#if 0
    DCHECK_NOTNULL(pzwr);
    auto addr = reinterpret_cast<Address>(pzwr);
    auto iter = old_to_new_.find(addr);

    if (val == Object::kNil || !val->IsObject()) {
        if (iter != old_to_new_.end()) {
            delete iter->second;
            old_to_new_.erase(iter);
        }
        return;
    }
    // new -> old
    if (InNewArea(host)) {
        return;
    }
    
    DCHECK(InOldArea(host));
    if (iter == old_to_new_.end()) {
        // old -> new
        if (InNewArea(val->ToHeapObject())) {
            WriteEntry *wr = new WriteEntry;
            wr->next = nullptr;
            wr->host = host;
            wr->pzwr = reinterpret_cast<NyObject **>(pzwr);
            
            old_to_new_.insert({addr, wr});
        }
    } else {
        DCHECK_EQ(host, iter->second->host);
        if (InOldArea(val->ToHeapObject())) {
            if (iter != old_to_new_.end()) {
                delete iter->second;
                old_to_new_.erase(iter);
            }
        }
    }
//#endif
}

/*virtual*/ void *Heap::Allocate(size_t size, size_t /*alignment*/) {
    return Allocate(size, kNewSpace);
}

/*virtual*/ void Heap::Purge(bool reinit) {
    // TODO:
}

/*virtual*/ size_t Heap::memory_usage() const {
    // TODO:
    return 0;
}
    
} // namespace nyaa
    
} // namespace mai
