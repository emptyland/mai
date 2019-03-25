#ifndef MAI_NYAA_NYAA_CORE_H_
#define MAI_NYAA_NYAA_CORE_H_

#include "base/base.h"
#include "mai/error.h"

namespace mai {
class Allocator;
namespace nyaa {
    
class Nyaa;
class Isolate;
class HandleScope;
class Heap;
class Object;
class NyString;
class NyMap;
class NyThread;
class ObjectFactory;
    
struct BuiltinStrPool;
struct BuiltinMetatablePool;
    
struct HandleScopeSlot {
    HandleScope     *scope;
    HandleScopeSlot *prev;
    Address          base;
    Address          end;
    Address          limit;
};
    
class NyaaCore final {
public:
    NyaaCore(Nyaa *stub);
    ~NyaaCore();
    
    Error Boot();
    
    // Stack operations:
    Object *Get(int i);
    void Pop(int n = 1);
    
    // Global variable operations:
    void SetGlobal(NyString *name, Object *value);
    
    void Raisef(const char *fmt, ...);
    void Vraisef(const char *fmt, va_list ap);
    void Unraise();
    bool Raised() const;
    
    void InsertThread(NyThread *thd);
    void RemoveThread(NyThread *thd);
    
    void EnterHandleScope(HandleScope *handle_scope);
    void ExitHandleScope();
    
    Address AdvanceHandleSlots(int n_slot);
    
    HandleScope *current_handle_scope() const { return top_slot_->scope; }
    
    DEF_PTR_GETTER(Nyaa, stub);
    DEF_PTR_GETTER(Heap, heap);
    DEF_PTR_GETTER(ObjectFactory, factory);
    DEF_PTR_GETTER(NyMap, g);
    DEF_PTR_GETTER(NyThread, main_thd);
    DEF_PTR_PROP_RW(NyThread, curr_thd);
    BuiltinMetatablePool *kmt_pool() const { return kmt_pool_.get(); }
    BuiltinStrPool *bkz_pool() const { return bkz_pool_.get(); }
    Isolate *isolate() const;
    
    static NyaaCore *Current();
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyaaCore);
private:
    Nyaa *const stub_;
    Allocator *const page_alloc_;
    Heap *const heap_;
    ObjectFactory *const factory_;
    HandleScopeSlot *top_slot_;
    
    std::unique_ptr<BuiltinStrPool> bkz_pool_;
    std::unique_ptr<BuiltinMetatablePool> kmt_pool_;

    NyMap *g_; // [strong ref]
    NyThread *main_thd_; // [strong ref]
    NyThread *curr_thd_; // [strong ref]
};
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_NYAA_CORE_H_
