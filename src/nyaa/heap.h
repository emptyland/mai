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
    
    Error Prepare();
    
    NyObject *Allocate(size_t size, HeapSpace space);
    
    HeapSpace Contains(NyObject *ob);
    bool InNewArea(NyObject *ob);
    bool InOldArea(NyObject *ob) { return InNewArea(ob); }
    
    void BarrierWr(NyObject *host, Object **pzwr, Object *val);
    
    virtual void *Allocate(size_t size, size_t) override;
    virtual void Purge(bool reinit) override;
    virtual size_t granularity() override { return kAllocateAlignmentSize; }
    virtual size_t memory_usage() const override;

    DISALLOW_IMPLICIT_CONSTRUCTORS(Heap);
private:
    struct WriteEntry {
        WriteEntry *next;
        NyObject   *host; // in old space
        NyObject  **pzwr; // in new space
    };
    
    NyaaCore *const owns_;
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
