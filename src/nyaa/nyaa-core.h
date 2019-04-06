#ifndef MAI_NYAA_NYAA_CORE_H_
#define MAI_NYAA_NYAA_CORE_H_

#include "base/base.h"
#include "mai/error.h"

namespace mai {
class Allocator;
class RandomGenerator;
namespace nyaa {
    
class Nyaa;
class Isolate;
class HandleScope;
class Heap;
class NyObject;
class Object;
class NyString;
class NyMap;
class NyThread;
class ObjectFactory;
class RootVisitor;
class StringPool;

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
    
    template<class T, class V>
    inline V *BarrierWr(NyObject *host, T **pzwr, V *val) {
        T *test = val;
        InternalBarrierWr(host, reinterpret_cast<Object **>(pzwr), test);
        return val;
    }

    void IterateRoot(RootVisitor *visitor);
    
    HandleScope *current_handle_scope() const { return top_slot_->scope; }
    
    DEF_PTR_GETTER(Nyaa, stub);
    Heap *heap() const { return heap_.get(); }
    ObjectFactory *factory() const { return factory_.get(); }
    DEF_VAL_GETTER(bool, initialized);
    DEF_PTR_GETTER(NyMap, g);
    DEF_PTR_GETTER(NyThread, main_thd);
    DEF_PTR_PROP_RW(NyThread, curr_thd);
    BuiltinMetatablePool *kmt_pool() const { return kmt_pool_.get(); }
    StringPool *kz_pool() const { return kz_pool_.get(); }
    BuiltinStrPool *bkz_pool() const { return bkz_pool_.get(); }
    Isolate *isolate() const;
    
    static NyaaCore *Current();
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyaaCore);
private:
    void InternalBarrierWr(NyObject *host, Object **pzwr, Object *val);
    
    Nyaa *const stub_;
    Allocator *const page_alloc_;
    std::unique_ptr<Heap> heap_;
    std::unique_ptr<ObjectFactory> factory_;
    HandleScopeSlot *top_slot_;
    bool initialized_ = false;
    std::unique_ptr<RandomGenerator> random_;
    std::unique_ptr<StringPool> kz_pool_; // elements [weak ref]
    std::unique_ptr<BuiltinStrPool> bkz_pool_; // elements [strong ref]
    std::unique_ptr<BuiltinMetatablePool> kmt_pool_; // elements [strong ref]

    NyMap *g_ = nullptr; // [strong ref]
    NyThread *main_thd_ = nullptr; // [strong ref]
    NyThread *curr_thd_ = nullptr; // [strong ref]
};
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_NYAA_CORE_H_
