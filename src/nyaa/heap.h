#ifndef MAI_NYAA_HEAP_H_
#define MAI_NYAA_HEAP_H_

#include "nyaa/memory.h"
#include "base/arena.h"
#include "mai/error.h"

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
    
    std::tuple<NyObject *, Error> Allocate(size_t size, HeapArea kind);
    
    void BarrierWr(NyObject *owns, Object *arg1, Object *arg2 = nullptr);
    
    virtual void *Allocate(size_t size, size_t) override;
    virtual void Purge(bool reinit) override;
    virtual size_t granularity() override { return kAllocateAlignmentSize; }
    virtual size_t memory_usage() const override;

    DISALLOW_IMPLICIT_CONSTRUCTORS(Heap);
private:
    NyaaCore *const owns_;
    NewSpace *new_space_ = nullptr;
    OldSpace *old_space_ = nullptr;
    OldSpace *code_space_ = nullptr;
    LargeSpace *large_space_ = nullptr;
}; // class Heap
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_HEAP_H_
