#include "nyaa/heap.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/spaces.h"
#include "mai-lang/nyaa.h"
#include "glog/logging.h"
#include <set>

namespace mai {
    
namespace nyaa {
    
Heap::Heap(NyaaCore *N)
    : owns_(DCHECK_NOTNULL(N))
    , os_page_size_(N->isolate()->env()->GetLowLevelAllocator()->granularity())
    , new_space_(new NewSpace())
    , old_space_(new OldSpace(kOldSpace, false, /*executable*/ N->isolate()))
    , code_space_(new OldSpace(kCodeSpace, true, /*executable*/ N->isolate()))
    , large_space_(new LargeSpace(N->stub()->minor_area_max_size(), N->isolate())) {
    ::memset(&final_records_, 0, sizeof(final_records_));
}

/*virtual*/ Heap::~Heap() {
    while (final_records_) {
        FinalizerRecord *p = final_records_;
        final_records_ = p->next;
        p->fp(p->host, owns_);
        delete p;
    }
    for (const auto &pair : remember_set_) {
        delete pair.second;
    }
    
    delete large_space_;
    delete code_space_;
    delete old_space_;
    delete new_space_;
}

Error Heap::Init() {
    Nyaa *stub = owns_->stub();
    auto rs = new_space_->Init(stub->major_area_initial_size(), owns_->isolate());
    if (!rs) {
        return rs;
    }
    rs = old_space_->Init(stub->minor_area_initial_size(), stub->minor_area_max_size());
    if (!rs) {
        return rs;
    }
    Address probe = old_space_->AllocateRaw(16);
    old_space_->SetAddressColor(probe, initial_color_);
    old_space_->Free(probe, 16);

    rs = code_space_->Init(stub->code_area_initial_size(), stub->code_area_max_size());
    if (!rs) {
        return rs;
    }
    probe = code_space_->AllocateRaw(16);
    code_space_->SetAddressColor(probe, initial_color_);
    code_space_->Free(probe, 16);

    probe = large_space_->AllocateRaw(1);
    large_space_->SetAddressColor(probe, initial_color_);
    large_space_->Free(probe);
    return Error::OK();
}
    
void Heap::GetInfo(Info *info) const {
    info->major_usage     = new_space_->UsageMemory(&info->major_size);
    info->major_rss_size  = new_space_->pages_total_size();

    info->old_usage    = old_space_->MemoryUsage(&info->old_size);
    info->code_usage   = code_space_->MemoryUsage(&info->code_size);
    info->large_usage  = large_space_->MemoryUsage(&info->large_size);
    info->minor_size   = info->old_size;
    info->minor_size  += info->code_size;
    info->minor_size  += info->large_size;
    info->minor_usage  = info->old_usage;
    info->minor_usage += info->code_usage;
    info->minor_usage += info->large_usage;
    
    info->minor_rss_size  = large_space_->pages_total_size();
    info->minor_rss_size += old_space_->pages_total_size();
    info->minor_rss_size += code_space_->pages_total_size();
}

NyObject *Heap::Allocate(size_t request_size, HeapSpace space) {
    // TODO: add a argument for executable
    bool executable = space == kCodeSpace;
    if (request_size > kPageSize / 4) {
        space = kLargeSpace;
        DCHECK_GT(request_size, os_page_size_);
    }

    void *chunk = nullptr;
    switch (space) {
        case kNewSpace:
            chunk = new_space_->AllocateRaw(request_size);
            break;
        case kOldSpace:
            chunk = old_space_->AllocateRaw(request_size);
            break;
        case kCodeSpace:
            chunk = code_space_->AllocateRaw(request_size);
            break;
        case kLargeSpace:
            chunk = large_space_->AllocateRaw(request_size, executable);
            break;
        default:
            break;
    }
    if (!chunk) {
        return nullptr;
    }
    //printf("alloc: %p(%lu)\n", chunk, request_size);
    //DCHECK_NE(432, request_size);

    DCHECK_EQ(reinterpret_cast<uintptr_t>(chunk) % kAligmentSize, 0);
    DbgFillInitZag(chunk, request_size);
    return static_cast<NyObject *>(chunk);
}
    
HeapSpace Heap::Contains(NyObject *ob) {
    if (new_space_->Contains(reinterpret_cast<Address>(ob))) {
        return kNewSpace;
    }
    auto page = Page::FromHeapObject(ob);
    return page->owns_space();
}
    
bool Heap::InNewArea(NyObject *ob) {
    return new_space_->Contains(reinterpret_cast<Address>(ob));
}
    
bool Heap::InFromSemiArea(NyObject *ob) {
    return new_space_->from_area()->Contains(reinterpret_cast<Address>(ob));
}

bool Heap::InToSemiArea(NyObject *ob) {
    return new_space_->to_area()->Contains(reinterpret_cast<Address>(ob));
}
    
void Heap::BarrierWr(NyObject *host, Object **pzwr, Object *val, bool ismt) {
    if (owns_->stub()->nogc()) {
        return;
    }
    auto addr = reinterpret_cast<Address>(DCHECK_NOTNULL(pzwr));

    if (val == Object::kNil || !val->IsObject()) {
        auto iter = remember_set_.find(addr);
        if (iter != remember_set_.end()) {
            delete iter->second;
            remember_set_.erase(iter);
        }
        return;
    }
    // new -> old
    if (InNewArea(host)) {
        return;
    }
    auto iter = remember_set_.find(addr);
    
    DCHECK(InOldArea(host));
    if (iter == remember_set_.end()) {
        // old -> new
        if (InNewArea(val->ToHeapObject())) {
            RememberRecord *wr = new RememberRecord;
            //wr->next = nullptr;
            wr->host = host;
            wr->pzwr = pzwr;
            wr->ismt = ismt;
            
            remember_set_.insert({addr, wr});
        }
    } else {
        DCHECK_EQ(host, iter->second->host);
        if (InOldArea(val->ToHeapObject())) {
            if (iter != remember_set_.end()) {
                delete iter->second;
                remember_set_.erase(iter);
            }
        }
    }
}
    
void Heap::AddFinalizer(NyUDO *host, UDOFinalizer fp) {
    if (!fp) {
        FinalizerRecord dummy {.next = final_records_};
        FinalizerRecord *prev = &dummy, *p = final_records_;
        while (p) {
            if (p->host == host) {
                prev->next = p->next;
                delete p;
                break;
            }
            prev = p;
            p = p->next;
        }
        final_records_ = dummy.next;
    } else {
        FinalizerRecord *fr = new FinalizerRecord;
        fr->next = final_records_;
        fr->host = DCHECK_NOTNULL(host);
        fr->fp   = fp;
        final_records_ = fr;
    }
}
    
Object *Heap::MoveNewObject(Object *addr, size_t size, bool upgrade) {
    SemiSpace *to_area = new_space_->to_area();
    DCHECK(to_area->Contains(reinterpret_cast<Address>(addr)));
    
    Address dst = nullptr;
    if (upgrade) {
        dst = old_space_->AllocateRaw(size);
        ::memcpy(DCHECK_NOTNULL(dst), addr, size);
    } else {
        SemiSpace *from_area = new_space_->from_area();
        dst = from_area->AllocateRaw(size);
        ::memcpy(DCHECK_NOTNULL(dst), addr, size);
    }

    // Set foward address:
#if defined(NYAA_USE_POINTER_COLOR) || defined(NYAA_USE_POINTER_TYPE)
    uintptr_t word = *reinterpret_cast<uintptr_t *>(addr);
    uintptr_t data_bits = word & NyObject::kDataMask;
    word = reinterpret_cast<uintptr_t>(dst) | data_bits | 0x1;
    *reinterpret_cast<uintptr_t *>(addr) = word;
#else // !defined(NYAA_USE_POINTER_COLOR) && !defined(NYAA_USE_POINTER_TYPE)
    uintptr_t word = *reinterpret_cast<uintptr_t *>(addr);
    word = reinterpret_cast<uintptr_t>(dst) | 0x1;
    *reinterpret_cast<uintptr_t *>(addr) = word;
#endif // defined(NYAA_USE_POINTER_COLOR) || defined(NYAA_USE_POINTER_TYPE)
    
    return reinterpret_cast<Object *>(dst);
}
    
void Heap::IterateRememberSet(ObjectVisitor *visitor, bool for_host) {
    if (for_host) {
        std::set<Address> for_clean;
        for (const auto &pair : remember_set_) {
            Heap::RememberRecord *wr = pair.second;
            visitor->VisitPointer(wr->host, reinterpret_cast<Object **>(&wr->host));
            if (!wr->host) {
                delete wr;
                for_clean.insert(pair.first);
            }
        }
        for (Address key : for_clean) {
            remember_set_.erase(key);
        }
    } else {
        for (const auto &pair : remember_set_) {
            Heap::RememberRecord *wr = pair.second;
            if (wr->ismt) {
                visitor->VisitMetatablePointer(wr->host, wr->mtwr);
            } else {
                visitor->VisitPointer(wr->host, wr->pzwr);
            }
        }
    }
}
    
void Heap::IterateFinalizerRecords(ObjectVisitor *visitor) {
    FinalizerRecord dummy {.next = final_records_};
    FinalizerRecord *prev = &dummy, *p = dummy.next;
    while (p) {
        NyUDO *host = DCHECK_NOTNULL(p->host);
        visitor->VisitPointer(nullptr, reinterpret_cast<Object **>(&p->host));
        if (!p->host) {
            prev->next = p->next;
            p->fp(host, owns_);
            delete p;
            p = prev->next;
        } else {
            prev = p;
            p = p->next;
        }
    }
    final_records_ = dummy.next;
}
    
std::vector<Heap::RememberHost> Heap::GetRememberHosts(NyObject *host) const {
    std::vector<RememberHost> result;
    for (const auto &pair : remember_set_) {
        if (pair.second->host == host) {
            RememberRecord *wr = pair.second;
            result.push_back({
                static_cast<bool>(wr->ismt),
                reinterpret_cast<Address>(wr->pzwr)});
        }
    }
    return result;
}
    
} // namespace nyaa
    
} // namespace mai
