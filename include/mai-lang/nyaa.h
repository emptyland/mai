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
    
struct NyaaOptions {
    NyaaOptions() = default;
    
    Env *env = Env::Default();
    
    // The heap young generation space initial size.
    int major_area_initial_size = 10 * 1024 * 1024;
    
    // The heap young generation space limit size.
    int major_area_max_size = 20 * 1024 * 1024;
    
    // The heap old generation space initial size.
    int minor_area_initial_size = 10 * 1024 * 1024;
    
    // The heap old generation space limit size.
    int minor_area_max_size = 512 * 1024 * 1024;
    
    // The init stack size.
    int init_thread_stack_size = 1024;
    
    // Do not use GC memory management.
    bool nogc = false;
};
    
class Nyaa final {
public:
    using Core = nyaa::NyaaCore;
    using Options = NyaaOptions;
    
    Nyaa(const NyaaOptions &options, Isolate *is);
    ~Nyaa();
    
    int init_thread_stack_size() const { return init_thread_stack_size_; }
    int major_area_initial_size() const { return major_area_initial_size_; }
    int minor_area_initial_size() const { return minor_area_initial_size_; }
    int minor_area_max_size() const { return minor_area_max_size_; }
    bool nogc() const { return nogc_; }
    
    Core *core() const { return core_.get(); }
    
    Isolate *isolate() const { return isolate_; }
    
    friend class nyaa::NyaaCore;
    friend class Isolate;
    
    Nyaa(const Nyaa &) = delete;
    void operator = (const Nyaa &) = delete;
private:
    Isolate *const isolate_;

    // The heap young generation space initial size.
    int major_area_initial_size_;
    
    // The heap young generation space limit size.
    int major_area_max_size_;
    
    // The init stack size.
    int init_thread_stack_size_;
    
    // The heap old generation space initial size.
    int minor_area_initial_size_;
    
    // The heap old generation space limit size.
    int minor_area_max_size_;
    
    // Do not use GC memory management.
    bool nogc_;
    
    std::unique_ptr<Core> core_;
    
    Nyaa *prev_ = nullptr;
}; // class Nyaa


} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_NYAA_H_
