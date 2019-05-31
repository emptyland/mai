#ifndef MAI_NYAA_NYAA_H_
#define MAI_NYAA_NYAA_H_

#include "mai-lang/isolate.h"
#include "mai-lang/handles.h"
#include "mai-lang/values.h"
#include "mai-lang/call-info.h"
#include <memory>
#include <set>


namespace mai {

namespace nyaa {

class NyaaCore;
class Isolate;
struct NyaaOptions;
    
class Nyaa final {
public:
    enum Execution {
        kInterpreter, // Interpreter only.
        kAOT, // AOT compiling.
    };

    using Core = nyaa::NyaaCore;
    using Options = NyaaOptions;
    
    Nyaa(const Options &options, Isolate *is);
    ~Nyaa();
    
    int init_thread_stack_size() const { return init_thread_stack_size_; }
    int major_area_initial_size() const { return major_area_initial_size_; }
    int minor_area_initial_size() const { return minor_area_initial_size_; }
    int minor_area_max_size() const { return minor_area_max_size_; }
    int code_area_initial_size() const { return code_area_initial_size_; }
    int code_area_max_size() const { return code_area_max_size_; }
    Execution exec() const { return exec_; }
    const std::set<std::string> &find_paths() const { return find_paths_; }
    bool nogc() const { return nogc_; }
    Core *core() const { return core_.get(); }
    Isolate *isolate() const { return isolate_; }
    
    void AddFindPath(const std::string &path) { find_paths_.insert(path); }
    
    void SetGlobal(Handle<String> name, Handle<Value> value);
    Handle<Value> GetGlobal(Handle<String> name);
    
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
    
    // The heap code area space initial size.
    int code_area_initial_size_;
    
    // The heap code area space limit size.
    int code_area_max_size_;
    
    // Do not use GC memory management.
    bool nogc_;
    
    // Use the short string pool.
    bool use_string_pool_;
    
    // The nyaa execution way.
    Execution exec_;
    
    // Require command find paths.
    std::set<std::string> find_paths_;
    
    std::unique_ptr<Core> core_;
    
    Nyaa *prev_ = nullptr;
}; // class Nyaa

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
    
    // The heap code area space initial size.
    int code_area_initial_size = 10 * 1024 * 1024;
    
    // The heap code area space limit size.
    int code_area_max_size = 32 * 1024 * 1024;
    
    // The init stack size.
    int init_thread_stack_size = 1024;
    
    // Do not use GC memory management.
    bool nogc = false;
    
    // Use the short string pool.
    bool use_string_pool = true;
    
    // The nyaa execution way.
    Nyaa::Execution exec = Nyaa::kInterpreter;
};

} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_NYAA_H_
