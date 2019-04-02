#ifndef MAI_NYAA_HEAP_H_
#define MAI_NYAA_HEAP_H_

#include "nyaa/memory.h"
#include "base/arena.h"
#include "mai/error.h"
#include <unordered_map>

namespace mai {
    
namespace nyaa {
    
class NyaaCore;
class Object;
class NyObject;
class NewSpace;
class OldSpace;
class LargeSpace;
    
class Heap final : public base::Arena {
public:
    Heap(NyaaCore *N);
    virtual ~Heap();
    
    Error Init();
    
    NyObject *Allocate(size_t size, HeapSpace space);
    
    HeapSpace Contains(NyObject *ob);
    bool InNewArea(NyObject *ob);
    bool InOldArea(NyObject *ob) { return !InNewArea(ob); }
    bool InFromSemiArea(NyObject *ob);
    bool InToSemiArea(NyObject *ob);
    
    void BarrierWr(NyObject *host, Object **pzwr, Object *val, bool ismt = false);
    
    virtual void *Allocate(size_t size, size_t) override;
    virtual void Purge(bool reinit) override;
    virtual size_t granularity() override { return kAllocateAlignmentSize; }
    virtual size_t memory_usage() const override;

    struct WriteEntry {
        WriteEntry *next;
        NyObject   *host; // in old space
        uint32_t    ismt; // is pzwr metatable address?
        union {
            Object  **pzwr; // use by ismt == false, in new space
            uintptr_t *mtwr; // use by ismt == true
        };
    };

    friend class Scavenger;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Heap);
private:
    NyaaCore *const owns_;
    size_t const os_page_size_;
    NewSpace *new_space_ = nullptr;
    OldSpace *old_space_ = nullptr;
    OldSpace *code_space_ = nullptr;
    LargeSpace *large_space_ = nullptr;
    
    std::unordered_map<Address, WriteEntry *> old_to_new_;
    //WriteEntry *wrbuf_ = nullptr;
}; // class Heap
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_HEAP_H_
