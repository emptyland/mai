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
class NyTable;
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
    
    void SetGlobal(NyString *name, Object *value);
    
    void Raisef(const char *fmt, ...);
    
    void EnterHandleScope(HandleScope *handle_scope);
    void ExitHandleScope();
    
    Address AdvanceHandleSlots(int n_slot);
    
    HandleScope *current_handle_scope() const { return top_slot_->scope; }
    
    DEF_PTR_GETTER(Nyaa, stub);
    DEF_PTR_GETTER(Heap, heap);
    DEF_PTR_GETTER(ObjectFactory, factory);
    DEF_PTR_GETTER(NyTable, g);
    DEF_VAL_GETTER(bool, has_raised)
    
    Isolate *isolate() const;
    
    static NyaaCore *Current();
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyaaCore);
    
    BuiltinMetatablePool *kmt_pool() const { return kmt_pool_.get(); }
    BuiltinStrPool *bkz_pool() const { return bkz_pool_.get(); }
private:
    Nyaa *const stub_;
    Allocator *const page_alloc_;
    Heap *const heap_;
    ObjectFactory *const factory_;
    HandleScopeSlot *top_slot_;
    bool has_raised_ = false;
    
    std::unique_ptr<BuiltinStrPool> bkz_pool_;
    std::unique_ptr<BuiltinMetatablePool> kmt_pool_;

    NyTable *g_;
    NyThread *main_thd_;
};
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_NYAA_CORE_H_
