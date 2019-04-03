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
class ObjectVisitor;
    
class Heap final : public base::Arena {
public:
    Heap(NyaaCore *N);
    virtual ~Heap();
    
    Error Init();
    
    DEF_VAL_GETTER(HeapColor, initial_color);
    DEF_VAL_GETTER(HeapColor, finalize_color);
    DEF_VAL_GETTER(float, from_semi_area_remain_rate);
    DEF_VAL_GETTER(double, major_gc_cost);
    DEF_VAL_GETTER(double, minor_gc_cost);
    
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

    friend class Scavenger;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Heap);
private:
    struct WriteEntry {
        WriteEntry *next;
        NyObject   *host; // in old space
        uint32_t    ismt; // is pzwr metatable address?
        union {
            Object  **pzwr; // use by ismt == false, in new space
            uintptr_t *mtwr; // use by ismt == true
        };
    };
    
    void IterateOldToNewWr(ObjectVisitor *visitor, bool after_clean);

    NyaaCore *const owns_;
    size_t const os_page_size_;
    NewSpace *new_space_ = nullptr;
    OldSpace *old_space_ = nullptr;
    OldSpace *code_space_ = nullptr;
    LargeSpace *large_space_ = nullptr;
    
    HeapColor initial_color_  = kColorWhite;
    HeapColor finalize_color_ = kColorBlack;
    
    std::unordered_map<Address, WriteEntry *> old_to_new_;
    
    float from_semi_area_remain_rate_ = 0;
    double major_gc_cost_ = 0;
    double minor_gc_cost_ = 0;
    
#if defined(DEBUG) || defined(_DEBUG)
    std::vector<View<Byte>> debug_track_blocks_;
#endif // defined(DEBUG) || defined(_DEBUG)
    //WriteEntry *wrbuf_ = nullptr;
}; // class Heap
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_HEAP_H_
