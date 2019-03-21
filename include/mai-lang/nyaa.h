#ifndef MAI_NYAA_NYAA_H_
#define MAI_NYAA_NYAA_H_

#include "mai-lang/isolate.h"
#include "mai-lang/handles.h"
#include "mai-lang/values.h"
#include "mai-lang/call-info.h"
#include <memory>


namespace mai {

namespace nyaa {

class NyaaCore;
class Isolate;
    
class Nyaa final {
public:
    using Core = nyaa::NyaaCore;
    
    Nyaa(Isolate *is);
    ~Nyaa();
    
    Core *core() const { return core_.get(); }
    
    Isolate *isolate() const { return isolate_; }
    
    friend class nyaa::NyaaCore;
    friend class Isolate;
    
    Nyaa(const Nyaa &) = delete;
    void operator = (const Nyaa &) = delete;
private:
    Isolate *const isolate_;
    std::unique_ptr<Core> core_;
    Nyaa *prev_ = nullptr;
}; // class Nyaa


class TryCatch final {
public:
    TryCatch(Isolate *isolate);
    ~TryCatch();

    Handle<Value> Exception() const;
    
private:
    Isolate *const isolate_;
}; // class TryCatch

} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_NYAA_H_
