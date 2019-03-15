#ifndef MAI_NYAA_ISOLATE_H_
#define MAI_NYAA_ISOLATE_H_

#include "mai/env.h"

namespace mai {
    
namespace nyaa {
    
class Nyaa;
    
struct IsolateOptions {
    IsolateOptions() = default;

    Env *env = Env::Default();
    
    // The heap young generation space initial size.
    int major_area_initial_size = 10 * 1024 * 1024;
    
    // The heap young generation space limit size.
    int major_area_max_size = 20 * 1024 * 1024;
};

class Isolate final {
public:
    using Options = IsolateOptions;
    
    void Enter();
    void Exit();
    void Dispose();
    
    static Isolate *current();
    static Isolate *New(const Options &opts = Options());
    
    Isolate(const Isolate &) = delete;
    void operator = (const Isolate &) = delete;
private:
    Isolate(const Options &opts);
    ~Isolate();
    
    bool Init();
    
    Env *const env_;

    // The heap young generation space initial size.
    int major_area_initial_size_;
    
    // The heap young generation space limit size.
    int major_area_max_size_;
    
    Isolate *prev_ = nullptr;
}; // class Isolate
    
} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_ISOLATE_H_
