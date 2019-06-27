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
    
class Heap final {
public:
    Heap(NyaaCore *N);
    virtual ~Heap();
    
    Error Init();
    
    DEF_VAL_GETTER(HeapColor, initial_color);
    DEF_VAL_GETTER(HeapColor, finalize_color);
    DEF_VAL_GETTER(float, from_semi_area_remain_rate);
    DEF_VAL_GETTER(double, major_gc_cost);
    DEF_VAL_GETTER(double, minor_gc_cost);
    
    struct Info {
        size_t major_size;     // total memory can used
        size_t major_usage;    // used memory
        size_t major_rss_size; // os allocated memory
        
        size_t minor_size;     // total memory can used
        size_t minor_usage;    // used memory
        size_t minor_rss_size; // os allocated memory
        
        size_t old_usage;
        size_t old_size;
        size_t code_usage;
        size_t code_size;
        size_t large_usage;
        size_t large_size;
    };
    
    void GetInfo(Info *info) const;
    
    NyObject *Allocate(size_t size, HeapSpace space);

    HeapSpace Contains(NyObject *ob);
    bool InNewArea(NyObject *ob);
    bool InOldArea(NyObject *ob) { return !InNewArea(ob); }
    bool InFromSemiArea(NyObject *ob);
    bool InToSemiArea(NyObject *ob);
    
    void BarrierWr(NyObject *host, Object **pzwr, Object *val, bool ismt = false);
    
    struct RememberHost {
        bool    ismt;
        Address addr_wr;
    };
    std::vector<RememberHost> GetRememberHosts(NyObject *host) const;
    
    void AddFinalizer(NyUDO *host, UDOFinalizer fp);

    friend class Scavenger;
    friend class MarkingSweep;
    FRIEND_UNITTEST_CASE(NyaaScavengerTest, Sanity);
    DISALLOW_IMPLICIT_CONSTRUCTORS(Heap);
private:
    Object *MoveNewObject(Object *addr, size_t size, bool upgrade);
    
    struct RememberRecord {
        NyObject   *host; // [weak ref] in old space
        uint32_t    ismt; // is pzwr metatable address?
        union {
            Object   **pzwr; // use by ismt == false, in new space
            uintptr_t *mtwr; // use by ismt == true
        };
    };
    
    struct FinalizerRecord {
        FinalizerRecord *next;
        NyUDO *host;     // [weak ref]
        UDOFinalizer fp; // The finalizer function pointer
    };
    
    void IterateRememberSet(ObjectVisitor *visitor, bool for_sweep, bool after_clean);
    void IterateFinalizerRecords(ObjectVisitor *visitor);

    NyaaCore *const owns_;
    size_t const os_page_size_;
    NewSpace *new_space_ = nullptr;
    OldSpace *old_space_ = nullptr;
    OldSpace *code_space_ = nullptr;
    LargeSpace *large_space_ = nullptr;
    
    HeapColor initial_color_  = kColorWhite;
    HeapColor finalize_color_ = kColorBlack;
    
    std::unordered_map<Address, RememberRecord *> remember_set_; // elements [weak ref]
    FinalizerRecord *final_records_ = nullptr; // elements [weak ref]
    
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
