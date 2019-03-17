#ifndef MAI_NYAA_NYAA_CORE_H_
#define MAI_NYAA_NYAA_CORE_H_

#include "base/base.h"

namespace mai {
class Allocator;
namespace nyaa {
    
class Nyaa;
class HandleScope;
    
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
    
    void EnterHandleScope(HandleScope *handle_scope);
    void ExitHandleScope();
    
    Address AdvanceHandleSlots(int n_slot);
    
    HandleScope *current_handle_scope() const { return top_slot_->scope; }
    
    static NyaaCore *Current();
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyaaCore);
private:
    Nyaa *const stub_;
    Allocator *const page_alloc_;
    HandleScopeSlot *top_slot_;
};
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_NYAA_CORE_H_
