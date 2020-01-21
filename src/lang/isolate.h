#ifndef MAI_LANG_ISOLATE_H_
#define MAI_LANG_ISOLATE_H_

#include "mai/env.h"
#include "mai/error.h"

namespace mai {

namespace lang {

class Heap;

struct Options {
    Env *env = Env::Default(); // The base api env pointer
};


// The virtual machine isolate object:
class Isolate final {
public:
    static Isolate *New(const Options &);
    void Dispose();

    Error Initialize();
    
    static Isolate *Get();
    
    friend class IsolateScope;
    
    Isolate(const Isolate&) = delete;
    void operator = (const Isolate &) = delete;
private:
    Isolate(const Options &opts);
    
    void Enter();
    void Leave();
    
    Env *env_;
}; // class Isolate


class IsolateScope final {
public:
    IsolateScope(Isolate *owns)
        : owns_(owns) { owns->Enter(); }

    ~IsolateScope() {
        owns_->Leave();
        owns_->Dispose();
    }
    
    IsolateScope(const IsolateScope &) = delete;
    void operator = (const IsolateScope &) = delete;
private:
    Isolate *owns_;
}; // class IsolateScope

} // namespace lang

} // namespace mai

#endif // MAI_LANG_ISOLATE_H_
