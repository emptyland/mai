#ifndef MAI_NYAA_ISOLATE_H_
#define MAI_NYAA_ISOLATE_H_

#include "mai/env.h"

namespace mai {
    
namespace nyaa {
    
class Nyaa;

class Isolate final {
public:
    void Enter();
    void Exit();
    void Dispose();
    
    Nyaa *GetNyaa() const { return static_cast<Nyaa *>(ctx_tls_->Get()); }
    
    Env *env() const { return env_; }
    
    static Isolate *Current();
    static Isolate *New(Env *env = Env::Default());
    
    friend class Nyaa;

    Isolate(const Isolate &) = delete;
    void operator = (const Isolate &) = delete;
private:
    Isolate(Env *env);
    ~Isolate();
    
    bool Init();
    
    void SetNyaa(Nyaa *N) { ctx_tls_->Set(N); }
    
    Env *const env_;
    
    // tls for store Nyaa object.
    std::unique_ptr<ThreadLocalSlot> ctx_tls_;
    
    Isolate *prev_ = nullptr;
}; // class Isolate
    
} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_ISOLATE_H_
