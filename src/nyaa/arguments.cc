#include "mai-lang/arguments.h"
#include "nyaa/nyaa-core.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {
    
Arguments::Arguments(Value **address, size_t length)
    : length_(length)
    , address_(address) {
}

Arguments::Arguments(size_t length)
    : length_(length) {
    auto addr = NyaaCore::Current()->AdvanceHandleSlots(static_cast<int>(length));
    address_ = reinterpret_cast<Value **>(addr);
    int64_t i = length;
    while (i-- > 0) {
        address_[i] = nullptr;
    }
}

void Arguments::SetCallee(Local<Value> callee) {
    if (!callee_) {
        callee_ = reinterpret_cast<Value **>(HandleScope::Current()->NewHandle(nullptr));
    }
    *callee_ = *callee;
}
    
} // namespace nyaa
    
} // namespace mai
