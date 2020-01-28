#include "lang/handle.h"
#include "lang/isolate-inl.h"
#include "lang/machine.h"

namespace mai {

namespace lang {

// Into this scope
HandleScope::HandleScope(Initializer/*avoid c++ compiler optimize this object*/) {
    DCHECK_NE(this, Machine::Get()->top_handle_scope());
    Machine::Get()->EnterHandleScope(this);
    close_ = false;
}

// Out this scope
HandleScope::~HandleScope() {
    if (!close_) {
        Close();
    }
}

// Get number of handles
int HandleScope::GetNumberOfHanldes() const {
    auto slot = Machine::Get()->top_slot();
    return static_cast<int>((slot->end - slot->base) >> kPointerShift);
}

void HandleScope::Close() {
    DCHECK(!close_);
    DCHECK_EQ(this, Machine::Get()->top_handle_scope());
    Machine::Get()->ExitHandleScope();
    close_ = true;
}

// Get the prev(scope) HandleScope
HandleScope *HandleScope::GetPrev() { return Machine::Get()->top_slot()->prev->scope; }

// Get this scope's HandleScope
/*static*/ HandleScope *HandleScope::Current() { return Machine::Get()->top_handle_scope(); }

// New handle address and set heap object pointer
void **HandleScope::NewHandle(const void *value) {
    DCHECK_EQ(this, Machine::Get()->top_handle_scope()) << "Not any HandleScope in current scope.";
    DCHECK(!close_) << "HandleScope is close!";

    void **location = reinterpret_cast<void **>(Machine::Get()->AdvanceHandleSlots(1));
    *location = const_cast<void *>(value);
    return location;
}

} // namespace lang

} // namespace mai
